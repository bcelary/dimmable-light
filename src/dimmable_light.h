/******************************************************************************
 *  This file is part of Dimmable Light for Arduino, a library to control     *
 *  dimmers.                                                                  *
 *                                                                            *
 *  Copyright (C) 2018-2023  Fabiano Riccardi                                 *
 *  Copyright (C) 2025       Bart Celary                                      *
 *                                                                            *
 *  Dimmable Light for Arduino is free software; you can redistribute         *
 *  it and/or modify it under the terms of the GNU Lesser General Public      *
 *  License as published by the Free Software Foundation; either              *
 *  version 2.1 of the License, or (at your option) any later version.        *
 *                                                                            *
 *  This library is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
 *  Lesser General Public License for more details.                           *
 *                                                                            *
 *  You should have received a copy of the GNU Lesser General Public License  *
 *  along with this library; if not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/
#ifndef DIMMABLE_LIGHT_H
#define DIMMABLE_LIGHT_H

#include "thyristor.h"
#include <Arduino.h>

/**
 * This is the user-oriented DimmableLight class, a wrapper on Thyristor class.
 * The measurement unit is relative to the semi-period length, and it assumes values
 * in [0;MAX_BRIGHTNESS] range.
 */
class DimmableLight {
public:
  static constexpr uint8_t MAX_BRIGHTNESS = 200;
  static constexpr uint8_t MAX_MIN_BRIGHTNESS = 55;  // Ensures 200 levels (55-255)

  DimmableLight(int pin, uint8_t minBrightness = 0)
    : thyristor(pin), brightness(0), mMinBrightness(minBrightness > MAX_MIN_BRIGHTNESS ? MAX_MIN_BRIGHTNESS : minBrightness) {
    if (nLights < N) {
      nLights++;
    } else {
      Serial.println("Max lights number reached, the light is not created!");
      // return error or exception
    }
  }

  /**
   * Set the brightness, 0 to turn off the lamp
   * Accepts input range 0-MAX_BRIGHTNESS (values >MAX_BRIGHTNESS clamped to MAX_BRIGHTNESS)
   * Maps to hardware range minBrightness-HW_MAX
   * Input 0 always maps to hardware 0 (off)
   * Input 1-MAX_BRIGHTNESS maps linearly to minBrightness-HW_MAX
   */
  void setBrightness(uint8_t bri) {
    static constexpr uint8_t HW_MAX = 255;

    // Clamp input to valid range
    if (bri > MAX_BRIGHTNESS) {
      bri = MAX_BRIGHTNESS;
    }

    // Store input value
    brightness = bri;

    // Map to hardware brightness range
    uint8_t hwBri;
    if (bri == 0) {
      hwBri = 0;  // Always off
    } else if (mMinBrightness == 0) {
      // Map 1-MAX_BRIGHTNESS to 0-HW_MAX (simple scaling)
      hwBri = ((uint16_t)bri * HW_MAX) / MAX_BRIGHTNESS;
    } else {
      // Map 1-MAX_BRIGHTNESS to minBrightness-HW_MAX
      hwBri = mMinBrightness + ((uint16_t)(bri - 1) * (HW_MAX - mMinBrightness)) / (MAX_BRIGHTNESS - 1);
    }

#ifdef NETWORK_FREQ_FIXED_50HZ
    uint16_t newDelay = 10000 - (uint16_t)(((uint32_t)hwBri * 10000) / 255);
#elif defined(NETWORK_FREQ_FIXED_60HZ)
    uint16_t newDelay = 8333 - (uint16_t)(((uint32_t)hwBri * 8333) / 255);
#elif defined(NETWORK_FREQ_RUNTIME)
    uint16_t newDelay =
      Thyristor::getSemiPeriod() - (uint16_t)(((uint32_t)hwBri * Thyristor::getSemiPeriod()) / 255);
#endif
    thyristor.setDelay(newDelay);
  };

  /**
   * Return the current brightness (input scale 0-MAX_BRIGHTNESS).
   */
  uint8_t getBrightness() const {
    return brightness;
  }

  /**
   * Get minimum brightness threshold (hardware scale).
   */
  uint8_t getMinBrightness() const {
    return mMinBrightness;
  }

  /**
   * Set minimum brightness threshold (hardware scale).
   * Maximum allowed value is MAX_MIN_BRIGHTNESS to preserve precision.
   */
  void setMinBrightness(uint8_t minBrightness) {
    if (minBrightness > MAX_MIN_BRIGHTNESS) {
      minBrightness = MAX_MIN_BRIGHTNESS;
    }
    mMinBrightness = minBrightness;
    // Reapply current brightness with new mapping
    if (brightness > 0) {
      setBrightness(brightness);
    }
  }

  /**
   * Turn on the light at full power.
   */
  void turnOn() {
    setBrightness(MAX_BRIGHTNESS);
  }

  /**
   * Turn off the light.
   */
  void turnOff() {
    setBrightness(0);
  }

  static float getFrequency() {
    return Thyristor::getFrequency();
  }

#ifdef NETWORK_FREQ_RUNTIME
  static void setFrequency(float frequency) {
    Thyristor::setFrequency(frequency);
  }
#endif

#ifdef MONITOR_FREQUENCY
  static float getDetectedFrequency() {
    return Thyristor::getDetectedFrequency();
  }

  static bool isFrequencyMonitorAlwaysOn() {
    return Thyristor::isFrequencyMonitorAlwaysOn();
  }

  static void frequencyMonitorAlwaysOn(bool enable) {
    Thyristor::frequencyMonitorAlwaysOn(enable);
  }
#endif

  ~DimmableLight() {
    nLights--;
  }

  /**
   * Setup the timer and the interrupt routine.
   */
  static void begin() {
    Thyristor::begin();
  }

  /**
   * Set the pin dedicated to receive the AC zero cross signal.
   */
  static void setSyncPin(uint8_t pin) {
    Thyristor::setSyncPin(pin);
  }

  /**
   * Set the pin direction (RISING (default), FALLING, CHANGE).
   */
  static void setSyncDir(decltype(RISING) dir) {
    Thyristor::setSyncDir(dir);
  }

  /**
   * Set the pin pullup (true = INPUT_PULLUP, false = INPUT). The internal pullup resistor is not
   * available for each platform and each pin.
   */
  static void setSyncPullup(bool pullup) {
    Thyristor::setSyncPullup(pullup);
  }

  /**
   * Return the number of instantiated lights.
   */
  static uint8_t getLightNumber() {
    return nLights;
  };

private:
  static const uint8_t N = Thyristor::N;
  static uint8_t nLights;

  Thyristor thyristor;

  /**
   * Store the current brightness (input scale 0-MAX_BRIGHTNESS).
   */
  uint8_t brightness;

  /**
   * Minimum brightness threshold (hardware scale 0-HW_MAX).
   */
  uint8_t mMinBrightness;
};

#endif  // END DIMMABLE_LIGHT_H
