# Dimmable Light for Arduino

![Compile Library Examples](https://github.com/bcelary/dimmable-light/actions/workflows/LibraryBuild.yml/badge.svg)

A library to manage thyristors (aka dimmer or triac) and phase-fired control (aka phase-cutting control) in Arduino environment.

> **Note:** Fork of the [original library](https://github.com/fabianoriccardi/dimmable-light) to continue development with new features and fixes, as the original is no longer actively maintained.

## Motivations

This library started as an experiment with ESP8266 hardware timers to control incandescent lights. It evolved into a multi-platform solution when porting to ESP32 revealed the need for a flexible architecture that adapts to different hardware. At the time, no multi-platform thyristor control libraries existed, leading to this project's publication and ongoing maintenance.

### About the timers

Timer peripherals vary significantly across platforms:

- **ESP8266**: 2 timers, but only 1 available (other reserved for Wi-Fi). 23-bit, no advanced features (input capture, multiple compare channels, bidirectional counting).
- **ESP32**: 4 64-bit timers with up/down counters, but only 1 output compare channel per timer, no input capture.
- **AVR ATmega**: Multiple 8/16-bit timers at lower clock frequencies, requiring complex ISRs for multiple rollovers. Well-documented register specifications.

This library provides an abstraction layer exposing two primitives needed for thyristor control: one-shot timer activation and stop counting.

## Features

1. Control multiple thyristors using a single hardware timer
2. Compatible with multiple platforms (ESP8266/ESP32/AVR/SAMD/RP2040)
3. Interrupt optimization (triggers only when necessary, no periodic interrupts)
4. Control by 2 measurement units: gate activation time or linearized relative power
5. Tunable parameters for hardware-specific optimization
6. Real-time AC frequency monitoring

Comparison with similar libraries:

|                                          | Dimmable Light for Arduino                  | RobotDynOfficial/ RDBDimmer                           | circuitar/ Dimmer                           | AJMansfield/ TriacDimmer                           |
| ---------------------------------------- | ------------------------------------------- | ----------------------------------------------------- | ------------------------------------------- | -------------------------------------------------- |
|                                          |                                             | [Link](https://github.com/RobotDynOfficial/RBDDimmer) | [Link](https://github.com/circuitar/Dimmer) | [Link](https://github.com/AJMansfield/TriacDimmer) |
| Multiple dimmers                         | yes                                         | yes                                                   | yes                                         | 2                                                  |
| Supported frequencies                    | 50/60Hz                                     | 50Hz                                                  | 50/60Hz                                     | 50/60Hz                                            |
| Supported architectures                  | AVR, SAMD, ESP8266, ESP32, RP2040           | AVR, SAMD, ESP8266, ESP32, STM32F1, STM32F4, SAM      | AVR                                         | AVR                                                |
| Control _effective_ delivered power      | yes, dynamic calculation                    | no                                                    | yes, static lookup table                    | no                                                 |
| Fade gradually to new value              | no                                          | no                                                    | yes, configurable speed                     | no                                                 |
| Full-wave mode                           | no                                          | no                                                    | yes (count mode)                            | no                                                 |
| Time resolution                          | 1μs                                         | 1/100 of semi-period length (83μs@60Hz)               | 1/100 of semi-period energy (83μs@60Hz)     | 0.5μs                                              |
| Smart interrupt management               | yes, automatically activated only if needed | no                                                    | no                                          | no                                                 |
| Number of interrupts per semi-period (1) | number of instantiated dimmers + 1          | 100                                                   | 100                                         | 3                                                  |
| Frequency monitor                        | yes                                         | no                                                    | no                                          | no                                                 |

(1) In the worst case, with default settings

## Installation

Install directly from GitHub.

**PlatformIO** - add to `platformio.ini`:

```ini
[env:your_board]
lib_deps =
    https://github.com/bcelary/dimmable-light.git#v1.7.0
```

**AVR boards** (Arduino/Genuino Uno): Also requires [ArduinoSTL](https://github.com/mike-matera/ArduinoSTL).

**Example 6**: Requires [ArduinoSerialCommand](https://github.com/kroimon/Arduino-SerialCommand).

**AVR core**: Use v1.8.2 or lower due to ArduinoSTL incompatibility with newer versions.

**PlatformIO AVR**: Add `lib_compat_mode = strict` in `platformio.ini` `env` section to avoid STL conflicts between ArduinoSTL and default environment STL (not needed in Arduino-AVR core).

## Usage

The main APIs are accessible through DimmableLight class. Instantiate one or more DimmableLight, specifying the corresponding activation pin.

    DimmableLight dimmer(3);

Set the Zero Cross pin, calling the static method `setSyncPin`:

    DimmableLight::setSyncPin(2);

Then call the static method `begin`:

    DimmableLight::begin();

it enables the interrupt on Zero Cross Detection that checks if any thyristor must be activated. To set the activation time, call the method `setBrightness`:

    dimmer.setBrightness(150);

the given value is the brightness level. The method accepts values in range [0; 200], where 0 is off and 200 is maximum brightness.

If you encounter flickering due to electrical network noise, enable `#define FILTER_INT_PERIOD` at the beginning of `thyristor.cpp`.

If you have strict memory constraints, use `dimmable_light.h` or `dimmable_light_linearized.h` directly instead of `dimmable_light_manager.h` to avoid STL container overhead (and ArduinoSTL dependency on AVR).

For ready-to-use code look in `examples` folder. For more details check the header files and the [Wiki](https://github.com/bcelary/dimmable-light/wiki).

## Examples

8 examples included. Start with example 1 if you're a beginner.

- **Examples 3 & 5**: ESP8266/ESP32 only (require Ticker library)
- **Example 7**: Linear power control instead of gate activation time control
- **Example 6**: 8-dimmer luminous effects. [Video](https://youtu.be/DRJcCIZw_Mw) shows effects 9 & 11. Uses [this board](https://www.ebay.it/itm/124269741187) or equivalent.

Hardware setup for example 6:

!["Lamps"](https://i.ibb.co/zVBRB9k/IMG-4045.jpg "Lamps")
8 incandescent bulbs.

!["Boards"](https://i.ibb.co/YN2Fktn/IMG-4041.jpg "Boards")
Wemos D1 mini (v2.3.0) and 8-channel dimmer board.
