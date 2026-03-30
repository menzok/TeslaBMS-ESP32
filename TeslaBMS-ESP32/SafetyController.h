#pragma once

#include "config.h"

// Forward declaration – avoids circular includes with BMSModuleManager.h
class BMSModuleManager;

// =====================================================================
// SafetyController
//
// Single source of truth for all safety-related decisions in the BMS.
// Monitors the pack state every second and determines whether the pack
// is safe to charge, discharge, or whether any alarm/trip condition
// exists.
//
// Rules:
//   - Instantiated once in the main .ino.
//   - Called exclusively from BMSModuleManager::update() (never directly
//     from loop()).
//   - Reads data only through BMSModuleManager getters; never accesses
//     hardware itself.
//   - Out of scope: contactor/relay driving, balancing decisions,
//     any hardware I/O.
// =====================================================================
class SafetyController
{
public:
    SafetyController(BMSModuleManager& bms, EEPROMSettings& settings);

    // Called once per second from BMSModuleManager::update().
    void update();

    // --- Outputs ---
    bool    isChargingAllowed()    const;
    bool    isDischargingAllowed() const;
    bool    isFaulted()            const;
    uint8_t getAlarmBitmask()      const;

    // Bitmask bit positions (same format used for MQTT)
    static const uint8_t ALARM_OVER_V    = 0x01;
    static const uint8_t ALARM_UNDER_V   = 0x02;
    static const uint8_t ALARM_OVER_T    = 0x04;
    static const uint8_t ALARM_UNDER_T   = 0x08;
    static const uint8_t ALARM_CELL_GAP  = 0x10;

private:
    BMSModuleManager& _bms;
    EEPROMSettings&   _settings;

    // Alarm state – held across calls for hysteresis (no rapid chatter)
    bool _alarmOverV   = false;
    bool _alarmUnderV  = false;
    bool _alarmOverT   = false;
    bool _alarmUnderT  = false;
    bool _alarmCellGap = false;
};
