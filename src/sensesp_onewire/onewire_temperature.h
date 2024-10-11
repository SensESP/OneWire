#ifndef _SENSESP_SENSORS_ONEWIRE_H_
#define _SENSESP_SENSORS_ONEWIRE_H_

#include <set>

#include "OneWireNg.h"
#include "drivers/DSTherm.h"
#include "sensesp.h"
#include "sensesp/sensors/sensor.h"

namespace sensesp::onewire {

typedef std::array<uint8_t, 8> OWDevAddr;

/**
 * @brief Finds all 1-Wire temperature sensors that are connected
 * to the ESP and makes the address(es) and temperature data available
 * to the OneWireTemperature class.
 *
 * This class has no pre-defined limit on the number of 1-Wire sensors
 * it can work with. From a practical standpoint, it should handle all
 * the sensors you're likely to connect to a single ESP.
 *
 * @param pin The GPIO pin to which you have the data wire of the
 * 1-Wire sensor(s) connected.
 *
 * @param config_path Currently not used for this class, don't provide
 * it - it defaults to a blank String.
 **/
class DallasTemperatureSensors : public sensesp::Sensor<float> {
 public:
  DallasTemperatureSensors(int pin, String config_path = "",
                           DSTherm::Resolution resolution = DSTherm::RES_12_BIT,
                           uint32_t conversion_delay = 750);
  bool register_address(const OWDevAddr& addr);
  bool get_next_address(OWDevAddr* addr);
  DSTherm get_dallas_driver() { return DSTherm{*onewire_}; }

  uint32_t get_conversion_delay() { return conversion_delay_; }

 private:
  OneWireNg* onewire_;
  std::set<OWDevAddr> known_addresses_;
  std::set<OWDevAddr> registered_addresses_;
  DSTherm::Resolution resolution_;
  uint32_t conversion_delay_;
};

/**
 * @brief Used to read the temperature from a single 1-Wire temperature
 * sensor. If you have X sensors connected to your ESP, you need to create
 * X instances of this class in main.cpp.
 *
 * All instances of this class have
 * a pointer to the same instance of DallasTemperatureSensors.
 *
 * Temperature is read in Celsius, then converted to Kelvin before sending
 * to Signal K.
 *
 * It will use the next available sensor address from all those found by the
 * DallasTemperatureSensors class. Once a sensor is "registered" by this class,
 * it will keep reading that sensor (by its unique hardware address) unless you
 * change the address in the Config UI.
 *
 * @see
 *https://github.com/SignalK/SensESP/tree/master/examples/thermocouple_temperature_sensor
 *
 * @param dts Pointer to an instance of a DallasTemperatureSensors class.
 *
 * @param read_delay How often to read the temperature. It takes up to 750 ms
 *for the data to be read by the chip, so this parameter can't be less than 800.
 *If you make it less than 800, the program will force it to be 800. You should
 * probably make it 1000 (the default) or more, to be safe.
 *
 * @param config_path The path to configure the sensor address in the Config UI.
 **/
class OneWireTemperature : public sensesp::FloatSensor {
 public:
  OneWireTemperature(DallasTemperatureSensors* dts, uint32_t read_delay = 1000,
                     String config_path = "");
  virtual bool to_json(JsonObject& doc) override final;
  virtual bool from_json(const JsonObject& config) override final;

 private:
  DallasTemperatureSensors* dts_;
  uint32_t conversion_delay_ = DSTherm::MAX_CONV_TIME;
  uint32_t read_delay_;
  bool found_ = true;
  OWDevAddr address_ = {};
  void update();
  void read_value();
};

inline const String ConfigSchema(const OneWireTemperature& obj) {
  return R"({"type":"object","properties":{"address":{"title":"OneWire address","type":"string"},"found":{"title":"Device found","type":"boolean","readOnly":true}}})";
}

inline bool ConfigRequiresRestart(const OneWireTemperature& obj) {
  return true;
}

}  // namespace sensesp::onewire

#endif
