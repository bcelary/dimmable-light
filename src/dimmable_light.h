/******************************************************************************
 *  This file is part of Dimmable Light for Arduino, a library to control     *
 *  dimmers.                                                                  *
 *                                                                            *
 *  Copyright (C) 2018-2023  Fabiano Riccardi                                 *
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
 * in [0;255] range.
 */
class DimmableLight {
public:
  DimmableLight(int pin, uint8_t minBrightness = 0, uint8_t maxBrightness = 255)
    : thyristor(pin), brightness(0), mMinBrightness(minBrightness), mMaxBrightness(maxBrightness) {
    if (nLights < N) {
      nLights++;
    } else {
      Serial.println("Max lights number reached, the light is not created!");
      // return error or exception
    }
  }

  /**
   * Set the brightness, 0 to turn off the lamp
   * Maps input 0-255 to hardware minBrightness-maxBrightness range
   * Input 0 always maps to hardware 0 (off)
   * Input 1-255 maps linearly to minBrightness-maxBrightness
   */
  void setBrightness(uint8_t bri) {
    // Store original input value
    brightness = bri;

    // Map to hardware brightness range
    uint8_t hwBri;
    if (bri == 0) {
      hwBri = 0;  // Always off
    } else if (mMinBrightness == 0 && mMaxBrightness == 255) {
      hwBri = bri;  // No mapping needed
    } else {
      // Map 1-255 to minBrightness-maxBrightness
      hwBri = mMinBrightness + ((uint16_t)(bri - 1) * (mMaxBrightness - mMinBrightness)) / 254;
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
   * Return the current brightness (input scale 0-255).
   */
  uint8_t getBrightness() const {
    return brightness;
  }

  /**
   * Get minimum brightness threshold.
   */
  uint8_t getMinBrightness() const {
    return mMinBrightness;
  }

  /**
   * Get maximum brightness threshold.
   */
  uint8_t getMaxBrightness() const {
    return mMaxBrightness;
  }

  /**
   * Set minimum brightness threshold.
   */
  void setMinBrightness(uint8_t minBrightness) {
    mMinBrightness = minBrightness;
    // Reapply current brightness with new mapping
    if (brightness > 0) {
      setBrightness(brightness);
    }
  }

  /**
   * Set maximum brightness threshold.
   */
  void setMaxBrightness(uint8_t maxBrightness) {
    mMaxBrightness = maxBrightness;
    // Reapply current brightness with new mapping
    if (brightness > 0) {
      setBrightness(brightness);
    }
  }

  /**
   * Turn on the light at full power.
   */
  void turnOn() {
    setBrightness(255);
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
   * Store the current brightness (input scale 0-255).
   */
  uint8_t brightness;

  /**
   * Minimum brightness threshold (hardware scale).
   */
  uint8_t mMinBrightness;

  /**
   * Maximum brightness threshold (hardware scale).
   */
  uint8_t mMaxBrightness;
};

#endif  // END DIMMABLE_LIGHT_H
