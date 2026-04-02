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
constexpr uint8_t DEFAULT_ENABLE_FAULT_CHAIN = 0;   // 1 = enabled (recommended), 0 = disabled (use if you don't have a fault chain wired up to BMS Modules)

// ====================== EEPROM Settings Struct ======================
typedef struct {
    uint8_t version;
    uint8_t checksum;
    uint8_t logLevel;

    float OverVSetpoint;
    float UnderVSetpoint;
    float OverTSetpoint;
    float UnderTSetpoint;

    float balanceVoltage;
    float balanceHyst;
    uint8_t enableFaultChain;
} EEPROMData;

extern EEPROMData eepromdata;

class EEPROMSettings {
public:
    static void load();
    static void save();
    static void loadDefaults();
};