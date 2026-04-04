#pragma once

#include <Arduino.h>
#include <EEPROM.h>

// ====================== EEPROM Version ======================
#define EEPROM_VERSION      0x11

// ====================== Default Settings ======================
// These defaults prioritize safety, longevity, and real-world solar/off-grid usage with Tesla modules

constexpr float DEFAULT_OVERVOLTAGE = 4.25f;   // 50mV safety headroom above the cell's absolute maximum of 4.20V
constexpr float DEFAULT_UNDERVOLTAGE = 2.90f;   // Conservative lower limit for daily cycling; protects long-term battery health
constexpr float DEFAULT_OVERTEMP = 60.0f;   // Maximum safe temperature for Tesla modules in enclosures/solar sheds
constexpr float DEFAULT_UNDERTEMP = -10.0f;  // Standard safe lower limit for discharge on 18650/2170 cells
constexpr float DEFAULT_BALANCE_VOLTAGE = 3.95f;   // Balancing starts near full charge so passive bleed resistors can work effectively
constexpr float DEFAULT_BALANCE_HYST = 0.025f;  // 25mV hysteresis prevents rapid chattering and unnecessary heat
constexpr bool   DEFAULT_PRECHARGE_ENABLED = true; // Pre-charge is generally recommended to protect contactors and reduce arcing, especially in high-voltage setups
constexpr uint32_t DEFAULT_PRECHARGE_TIMEOUT_MS = 8000;     // 8 seconds max for pre-charge
constexpr bool DEFAULT_CURRENT_SENSOR_PRESENT = false; // Hall effect / current detector installed?

// ====================== EEPROM Settings Struct ======================
typedef struct {
    uint8_t version;
    uint8_t checksum;
    uint8_t logLevel;
	// Saftey THresholds
    float OverVSetpoint;
    float UnderVSetpoint;
    float OverTSetpoint;
    float UnderTSetpoint;
    float balanceVoltage;
    float balanceHyst;
    // Contactor settings
    bool    prechargeEnabled;
    bool    currentSensorPresent;     // Hall effect sensor installed?
    uint32_t prechargeTimeoutMs;      // fallback timer when no current sensor
} EEPROMData;

extern EEPROMData eepromdata;

class EEPROMSettings {
public:
    static void load();
    static void save();
    static void loadDefaults();
};