#include <memory>

#include "sensesp/signalk/signalk_output.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/ui/config_item.h"
#include "sensesp_app_builder.h"
#include "sensesp_onewire/onewire_temperature.h"

using namespace reactesp;
using namespace sensesp;
using namespace sensesp::onewire;

void setup() {
  SetupLogging();

  // Create the global SensESPApp() object.
  SensESPAppBuilder builder;
  sensesp_app = builder.get_app();

  /*
     Find all the sensors and their unique addresses. Then, each new instance
     of OneWireTemperature will use one of those addresses. You can't specify
     which address will initially be assigned to a particular sensor, so if you
     have more than one sensor, you may have to swap the addresses around on
     the configuration page for the device. (You get to the configuration page
     by entering the IP address of the device into a browser.)
  */

  /*
     Tell SensESP where the sensor is connected to the board
     ESP32 pins are specified as just the X in GPIOX
  */
  uint8_t pin = 4;

  DallasTemperatureSensors* dts = new DallasTemperatureSensors(pin);

  // Define how often SensESP should read the sensor(s) in milliseconds
  uint read_delay = 500;

  // Below are temperatures sampled and sent to Signal K server
  // To find valid Signal K Paths that fits your need you look at this link:
  // https://signalk.org/specification/1.4.0/doc/vesselsBranch.html

  // Measure coolant temperature
  auto coolant_temp =
      new OneWireTemperature(dts, read_delay, "/coolantTemperature/oneWire");

  ConfigItem(coolant_temp)
      ->set_title("Coolant Temperature")
      ->set_description("Temperature of the engine coolant")
      ->set_sort_order(100);

  auto coolant_temp_calibration =
      new Linear(1.0, 0.0, "/coolantTemperature/linear");

  ConfigItem(coolant_temp_calibration)
      ->set_title("Coolant Temperature Calibration")
      ->set_description("Calibration for the coolant temperature sensor")
      ->set_sort_order(200);

  auto coolant_temp_sk_output = new SKOutputFloat(
      "propulsion.mainEngine.coolantTemperature", "/coolantTemperature/skPath");

  ConfigItem(coolant_temp_sk_output)
      ->set_title("Coolant Temperature Signal K Path")
      ->set_description("Signal K path for the coolant temperature")
      ->set_sort_order(300);

  coolant_temp->connect_to(coolant_temp_calibration)
      ->connect_to(coolant_temp_sk_output);

  // Measure exhaust temperature
  auto* exhaust_temp =
      new OneWireTemperature(dts, read_delay, "/exhaustTemperature/oneWire");
  auto* exhaust_temp_calibration =
      new Linear(1.0, 0.0, "/exhaustTemperature/linear");
  auto* exhaust_temp_sk_output = new SKOutputFloat(
      "propulsion.mainEngine.exhaustTemperature", "/exhaustTemperature/skPath");

  exhaust_temp->connect_to(exhaust_temp_calibration)
      ->connect_to(exhaust_temp_sk_output);

  // Measure temperature of 24v alternator
  auto* alt_24v_temp =
      new OneWireTemperature(dts, read_delay, "/24vAltTemperature/oneWire");
  auto* alt_24v_temp_calibration =
      new Linear(1.0, 0.0, "/24vAltTemperature/linear");
  auto* alt_24v_temp_sk_output = new SKOutputFloat(
      "electrical.alternators.24V.temperature", "/24vAltTemperature/skPath");

  alt_24v_temp->connect_to(alt_24v_temp_calibration)
      ->connect_to(alt_24v_temp_sk_output);

  // Measure temperature of 12v alternator
  auto* alt_12v_temp =
      new OneWireTemperature(dts, read_delay, "/12vAltTemperature/oneWire");
  auto* alt_12v_temp_calibration =
      new Linear(1.0, 0.0, "/12vAltTemperature/linear");
  auto* alt_12v_temp_sk_output = new SKOutputFloat(
      "electrical.alternators.12V.temperature", "/12vAltTemperature/skPath");

  alt_12v_temp->connect_to(alt_12v_temp_calibration)
      ->connect_to(alt_12v_temp_sk_output);
}

// main program loop
void loop() {
  static auto* event_loop = sensesp_app->get_event_loop();
  event_loop->tick();
}
