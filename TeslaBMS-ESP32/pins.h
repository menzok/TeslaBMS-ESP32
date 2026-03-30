#pragma once

// =====================================================================
// Hardware Pins
// All physical pin assignments for this board are defined here.
// Do NOT scatter pin numbers throughout other source files.
// =====================================================================

// --- BMS module serial bus (Serial2 on ESP32) ---
#define BMS_SERIAL_RX_PIN   16
#define BMS_SERIAL_TX_PIN   17

// --- Digital inputs (fault chain / contactor feedback) ---
#define DIN1                55
#define DIN2                54
#define DIN3                57
#define DIN4                56

// --- Digital outputs (half-bridge drivers) ---
#define DOUT4_H             2
#define DOUT4_L             3
#define DOUT3_H             4
#define DOUT3_L             5
#define DOUT2_H             6
#define DOUT2_L             7
#define DOUT1_H             8
#define DOUT1_L             9
