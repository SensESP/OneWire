# OneWire Temperature Sensor Add-on Library for SensESP

This repository implements a OneWire temperature sensor library for [SensESP](https://signalk.org/SignalK/SensESP/).
It is based on the [OneWireNg](https://github.com/pstolarz/OneWireNg) Arduino library and adds support for SensESP producer-consumer communication and web configuration UI.

OneWire temperature sensors such as the DS18B20 are simple and very inexpensive temperature sensors ubiquitous in the hobbyist scene. Many OneWire sensors can be connected to a single 1-Wire bus and can be used to measure temperature, humidity, or other data.

To use the library in your own projects, you have to include it in your `platformio.ini` `lib_deps` section:

    lib_deps =
        SignalK/SensESP@^2.0.0
        SensESP/OneWire@^2.0.0

See also the [example main file](blob/main/examples/onewire_temperature_example.cpp).

For more information on using SensESP and external add-on libraries, see the [SensESP documentation](https://signalk.org/SensESP/docs/).
