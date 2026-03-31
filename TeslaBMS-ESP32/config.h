#pragma once

#include <Arduino.h>

// Serial port used for BMS module communication (BQ76PL536A daisy-chain bus).
// Pins are set in setup(): Serial2.begin(BMS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN).
extern HardwareSerial Serial2;

// USB/UART console — used for debug output and SerialConsole commands.
#define SERIALCONSOLE   Serial

// BMS module bus (BQ76PL536A daisy-chain, half-duplex UART at 612500 baud).
#define SERIAL          Serial2

// Current-shunt emulator bus (simple ASCII protocol, 9600 baud).
// NOTE: Serial2 is taken by the BMS; assign the shunt to Serial1.
//       Default ESP32 Serial1 pins are RX=18, TX=19 — change to match hardware.
#define SERIAL_SHUNT        Serial1
#define SERIAL_SHUNT_BAUD   9600
#define SERIAL_SHUNT_RX_PIN 18
#define SERIAL_SHUNT_TX_PIN 19

// BQ76PL536A register addresses, protocol constants, and validity thresholds
// are defined in BMSComm.h.  Include it here so that every file that pulls in
// config.h also gets the register map without a separate include.
#include "BMSComm.h"


#define DIN1                55
#define DIN2                54
#define DIN3                57
#define DIN4                56
#define DOUT4_H             2
#define DOUT4_L             3
#define DOUT3_H             4
#define DOUT3_L             5
#define DOUT2_H             6
#define DOUT2_L             7
#define DOUT1_H             8
#define DOUT1_L             9

typedef struct {
    uint8_t version;
    uint8_t checksum;
    uint32_t canSpeed;
    uint8_t batteryID;
    uint8_t logLevel;
    float OverVSetpoint;
    float UnderVSetpoint;
    float ChargeVsetpoint;
    float DischVsetpoint;
    float ChargeHys;
    float DischHys;
    float WarnOff;
    float CellGap;
    float IgnoreVolt;
    uint8_t IgnoreTemp;
    float OverTSetpoint;
    float UnderTSetpoint;
    float balanceVoltage;
    float balanceHyst;
    int Scells;
    int Pstrings;
    uint16_t triptime;
    // === NEW FIELDS ===
    int modulesInSeries;      // e.g. 1 or 2 or 4
    int parallelStrings;      // e.g. 2
    float capacityPerStringAh; // e.g. 232.0 for one Tesla module/string
} EEPROMSettings;

#define EEPROM_VERSION 0x15