#include "onewire_temperature.h"

#include <algorithm>

#include "OneWireNg_CurrentPlatform.h"
#include "utils/Placeholder.h"

#define DEVICE_DISCONNECTED_C -127

namespace sensesp::onewire {

const OWDevAddr null_ow_addr = {0, 0, 0, 0, 0, 0, 0, 0};

void owda_to_string(char* str, const OWDevAddr& addr) {
  // brute force it
  sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1],
          addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
}

bool string_to_owda(OWDevAddr* addr, const char* str) {
  // brute force it
  uint vals[8];
  int num_items =
      sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", &vals[0], &vals[1],
             &vals[2], &vals[3], &vals[4], &vals[5], &vals[6], &vals[7]);
  for (int i = 0; i < 8; i++) {
    (*addr)[i] = vals[i];
  }
  return num_items == 8;
}

DallasTemperatureSensors::DallasTemperatureSensors(int pin, String config_path, DSTherm::Resolution res)
    : Sensor(config_path) {
  onewire_ = new OneWireNg_CurrentPlatform(pin,
                                           false  // disable internal pull-up
  );
  resolution_ = res;

  DSTherm drv{*onewire_};

#if (CONFIG_MAX_SEARCH_FILTERS > 0)
  static_assert(CONFIG_MAX_SEARCH_FILTERS >= DSTherm::SUPPORTED_SLAVES_NUM,
                "OneWireNg config: CONFIG_MAX_SEARCH_FILTERS too small");
#else
#error "OneWireNg config: CONFIG_MAX_SEARCH_FILTERS not defined"
#endif

  // in the process of 1-wire bus scan
  // filter out supported Dallas sensors
  drv.filterSupportedSlaves();

  OWDevAddr owda;

  for (const auto& addr : *onewire_) {
    std::copy(std::begin(addr), std::end(addr), std::begin(owda));
    known_addresses_.insert(owda);
#ifndef DEBUG_DISABLED
    char addrstr[24];
    owda_to_string(addrstr, owda);
    debugI("Found OneWire sensor %s", addrstr);
#endif
    // Set common max. resolution (12-bits) for all handled sensors.
    // The configuration will be valid until subsequent power cut-off.
    drv.writeScratchpad(addr, 0, 0,  // disable alarm notifications
                        resolution_);
  }
}

bool DallasTemperatureSensors::register_address(const OWDevAddr& addr) {
  auto search_known = known_addresses_.find(addr);
  if (search_known == known_addresses_.end()) {
    // address is not known
    return false;
  }
  auto search_reg = registered_addresses_.find(addr);
  if (search_reg != registered_addresses_.end()) {
    // address is already registered
    return false;
  }

  registered_addresses_.insert(addr);
  return true;
}

bool DallasTemperatureSensors::get_next_address(OWDevAddr* addr) {
  // find the next address from known_addresses which is
  // not present in registered_addresses
  for (auto known : known_addresses_) {
    auto reg_it = registered_addresses_.find(known);
    if (reg_it == registered_addresses_.end()) {
      *addr = known;
      return true;
    }
  }
  return false;
}

OneWireTemperature::OneWireTemperature(DallasTemperatureSensors* dts,
                                       uint read_delay, String config_path)
    : sensesp::FloatSensor(config_path), dts_{dts}, read_delay_{read_delay} {
  load();
  if (address_ == null_ow_addr) {
    // previously unconfigured sensor
    bool success = dts_->get_next_address(&address_);
    if (!success) {
      debugE(
          "FATAL: Unable to allocate a OneWire sensor for %s. "
          "All sensors have already been configured. "
          "Check the physical wiring of your sensors.",
          config_path.c_str());
      found_ = false;
    } else {
      debugD("Registered a new OneWire sensor");
      dts_->register_address(address_);
    }
  } else {
    bool success = dts_->register_address(address_);
    if (!success) {
      char addrstr[24];
      owda_to_string(addrstr, address_);
      debugE(
          "FATAL: OneWire sensor %s at %s is missing. "
          "Check the physical wiring of your sensors.",
          config_path.c_str(), addrstr);
      found_ = false;
    }
  }

  conversion_delay_ = dts_->getConversionTime();
  if (found_) {
    // read_delay must be at least a little longer than conversion_delay
    if (read_delay_ < conversion_delay_ + 50) {
      read_delay_ = conversion_delay_ + 50;
    }
    SensESPBaseApp::get_event_loop()->onRepeat(read_delay_,
                                               [this]() { this->update(); });
  }
}

void OneWireTemperature::update() {
  dts_->get_dallas_driver().convertTemp(
      *reinterpret_cast<OneWireNg::Id*>(address_.data()),
      0  // don't wait for conversion
  );

  // temp converstion can take up to 750 ms, so wait before reading
  SensESPBaseApp::get_event_loop()->onDelay(conversion_delay_,
                                            [this]() { this->read_value(); });
}

void OneWireTemperature::read_value() {
  Placeholder<DSTherm::Scratchpad> scrpd;
  float tempC = DEVICE_DISCONNECTED_C;

  if (dts_->get_dallas_driver().readScratchpad(
          *reinterpret_cast<OneWireNg::Id*>(address_.data()), &scrpd) ==
      OneWireNg::EC_SUCCESS) {
    tempC = ((DSTherm::Scratchpad&)scrpd).getTemp() / 1000.0;
  }

  // we're on purpose ignoring the "conversion not ready" value (+85°C)
  // because the update method always waits for 750 ms before reading the value
  if (tempC == DEVICE_DISCONNECTED_C) {
    char ow_addr_str[24];
    owda_to_string(ow_addr_str, address_);
    debugW("Failed to read 1-Wire device %s", ow_addr_str);
    return;
  }
  // getTempC returns degrees Celsius but Signal K expects Kelvin
  this->emit(tempC + 273.15);
}

bool OneWireTemperature::to_json(JsonObject& root) {
  char addr_str[24];
  owda_to_string(addr_str, address_);
  root["address"] = addr_str;
  root["found"] = found_;
  return true;
}

bool OneWireTemperature::from_json(const JsonObject& config) {
  if (!config["address"].is<String>()) {
    return false;
  }
  String address = config["address"];
  string_to_owda(&address_, address.c_str());
  return true;
}

}  // namespace sensesp::onewire
