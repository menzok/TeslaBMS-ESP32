#pragma once

#include <Arduino.h>
#include "config.h"

// ============================================================================
// CurrentShunt.h  —  Serial ASCII current-shunt reader
// ============================================================================
// Reads pack current from a device (or emulator) that sends lines of the form:
//
//   CURRENT:<value>\n
//
// where <value> is a signed float in amps (positive = discharge).
//
// Example messages:
//   CURRENT:12.3
//   CURRENT:-45.7
//   CURRENT:8.2
//
// The shunt device is connected to SERIAL_SHUNT (defined in config.h) at
// SERIAL_SHUNT_BAUD baud.  Serial2 is reserved for the BMS modules; Serial1
// is used for the shunt by default (configure RX/TX pins in config.h).
// ============================================================================

class CurrentShunt
{
public:
    // Call once in setup() to open the serial port.
    void begin();

    // Call every loop() iteration.  Consumes all waiting bytes and updates
    // the stored current value for any complete CURRENT: lines received.
    void update();

    // Returns the most recently received current in amps.
    // Positive = discharging, negative = charging.
    float getCurrent() const { return _currentAmps; }

private:
    float _currentAmps = 0.0f;
};

extern CurrentShunt currentShunt;
