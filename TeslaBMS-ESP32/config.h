#pragma once

#include <Arduino.h>
#include "pins.h"

extern HardwareSerial Serial2;

// --- Serial ports ---
// USB / debug console
#define SERIALCONSOLE   Serial
// Tesla BMS module bus (Serial2 on ESP32)
#define SERIAL          Serial2

// --- Communication ---
#define BMS_BAUD        612500

// --- NVS / Preferences ---
#define PREFS_NAMESPACE "teslabms"

#define REG_DEV_STATUS      0
#define REG_GPAI            1
#define REG_VCELL1          3
#define REG_VCELL2          5
#define REG_VCELL3          7
#define REG_VCELL4          9
#define REG_VCELL5          0xB
#define REG_VCELL6          0xD
#define REG_TEMPERATURE1    0xF
#define REG_TEMPERATURE2    0x11
#define REG_ALERT_STATUS    0x20
#define REG_FAULT_STATUS    0x21
#define REG_COV_FAULT       0x22
#define REG_CUV_FAULT       0x23
#define REG_ADC_CTRL        0x30
#define REG_IO_CTRL         0x31
#define REG_BAL_CTRL        0x32
#define REG_BAL_TIME        0x33
#define REG_ADC_CONV        0x34
#define REG_ADDR_CTRL       0x3B

#define MAX_MODULE_ADDR     0x3E

// --- Sensor validity thresholds ---
// Voltage above which a cell reading is considered invalid (hardware maximum)
#define CELL_MAX_VALID_VOLT 4.5f
// Voltage above which a cell reading is treated as an open-circuit/invalid reading
#define CELL_OPEN_VOLT      60.0f
// Temperature below which a sensor is considered disconnected
#define TEMP_SENSOR_DISCONNECTED -70.0f

// --- NVS version tag ---
// Increment this whenever EEPROMSettings changes or safe-threshold defaults change,
// so that existing devices reload factory defaults on next boot.
#define EEPROM_VERSION 0x16

// =====================================================================
// EEPROMSettings – all user-configurable parameters persisted to NVS.
// =====================================================================
typedef struct {
    uint8_t  version;
    uint8_t  checksum;    // UNUSED: always written as 0, never verified — candidate for removal
    uint32_t canSpeed;    // UNUSED: no CAN bus is initialised in this firmware — candidate for removal
    uint8_t  batteryID;
    uint8_t  logLevel;
    // --- Voltage thresholds ---
    float OverVSetpoint;   // 4.20 V – hard over-voltage trip
    float UnderVSetpoint;  // 3.00 V – hard under-voltage trip
    float ChargeVsetpoint; // 4.15 V – safe full-charge target
    float DischVsetpoint;  // 3.10 V – soft discharge warning floor
    float ChargeHys;       // 0.05 V – charge contactor hysteresis
    float DischHys;        // 0.05 V – discharge contactor hysteresis
    float WarnOff;         // 0.10 V – early-warning buffer
    float CellGap;         // 0.15 V – max allowed cell imbalance before alarm
    float IgnoreVolt;      // 0.50 V – ignore cells at or below this voltage
    uint8_t IgnoreTemp;    // 0 = both sensors, 1 = TS1 only, 2 = TS2 only
    // --- Temperature thresholds ---
    float OverTSetpoint;   //  50.0 °C – high temperature trip
    float UnderTSetpoint;  //   0.0 °C – no charging below freezing
    // --- Balancing ---
    float balanceVoltage;  // 4.10 V – start passive balancing near full
    float balanceHyst;     // 0.04 V – balancing hysteresis
    // --- Legacy topology (kept for SerialConsole compatibility) ---
    int   Scells;
    int   Pstrings;
    uint16_t triptime;     // UNUSED: stored but not yet applied in safety logic — candidate for removal
    // --- Module topology ---
    int   modulesInSeries;      // e.g. 1 or 2 or 4
    int   parallelStrings;      // e.g. 2
    float capacityPerStringAh;  // e.g. 232.0 for one Tesla module/string
} EEPROMSettings;