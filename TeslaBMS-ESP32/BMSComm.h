#pragma once

#include <Arduino.h>
#include "Logger.h"

// ============================================================================
// BMSComm.h  —  BQ76PL536A low-level communication driver
// ============================================================================
// Single source of truth for all SPI-over-UART framing used to talk to the
// BQ76PL536A battery-management ICs.  All register addresses, CRC helpers,
// and framing helpers live here so that BMSModule and BMSModuleManager never
// need to know the wire-level details.
//
// Wire protocol (UART, half-duplex, 612500 baud, 8N1):
//   Write frame  : [ADDR|1] [REG] [DATA ...] [CRC]
//   Read request : [ADDR|0] [REG] [LEN]
//   Read reply   : [ADDR|0] [REG] [LEN] [DATA ...] [CRC]
//   ADDR uses the module's 7-bit address shifted left by 1; the LSB is the
//   R/W flag (1 = write, 0 = read).
//   BROADCAST_ADDR (0x7F) reaches all modules simultaneously.
//
// SERIAL / SERIALCONSOLE are defined in config.h.
// ============================================================================

// ============================================================================
// BQ76PL536A Register Map
// ============================================================================

// --- Group 0: Read-only measurement & status registers (0x00–0x24) ----------

/** Device status byte. Mirrors alert/fault summary bits. */
#define REG_DEV_STATUS      0x00

/** General-purpose analog input (pack/module voltage), high byte.
 *  Read 2 bytes (0x01-0x02). Value = raw16 * 0.002034629 V.
 *  Also the start of the 18-byte bulk-read block:
 *    GPAI(2) + VCELL1-6(12) + TEMP1(2) + TEMP2(2) = 18 bytes. */
#define REG_GPAI            0x01

/** Cell 1 voltage, high byte.  Read 2 bytes. Value = raw16 * 0.000381493 V. */
#define REG_VCELL1          0x03
/** Cell 2 voltage, high byte. */
#define REG_VCELL2          0x05
/** Cell 3 voltage, high byte. */
#define REG_VCELL3          0x07
/** Cell 4 voltage, high byte. */
#define REG_VCELL4          0x09
/** Cell 5 voltage, high byte. */
#define REG_VCELL5          0x0B
/** Cell 6 voltage, high byte. */
#define REG_VCELL6          0x0D

/** Temperature sensor 1, high byte. Read 2 bytes. NTC thermistor raw ADC. */
#define REG_TEMPERATURE1    0x0F
/** Temperature sensor 2, high byte. */
#define REG_TEMPERATURE2    0x11

/** Alert status register.  Bit 2 = SLEEP flag.
 *  Write 0xFF then 0x00 to clear all latched alerts. */
#define REG_ALERT_STATUS    0x20

/** Fault status register (overvoltage / undervoltage faults).
 *  Bit 0 = COV, Bit 1 = CUV, Bit 2 = CRC, Bit 3 = POR, Bit 4 = TEST, Bit 5 = REGS. */
#define REG_FAULT_STATUS    0x21

/** Cell overvoltage fault mask — bit N set means cell N+1 tripped COV. */
#define REG_COV_FAULT       0x22

/** Cell undervoltage fault mask — bit N set means cell N+1 tripped CUV. */
#define REG_CUV_FAULT       0x23

// --- Group 3: Read/write configuration registers (0x30–0x3F) ----------------

/** ADC configuration register.
 *  Bit pattern 0b00111101 = Auto mode, read all inputs (pack + 6 cells + 2 temps).
 *  Must be set before starting a conversion with REG_ADC_CONV. */
#define REG_ADC_CTRL        0x30

/** IO control register.
 *  Bit 0-1 = temperature sensor VSS enable (0b11 = both on).
 *  Bit 2   = SLEEP mode when written via broadcast. */
#define REG_IO_CTRL         0x31

/** Cell balance control register.  Bits 0–5 correspond to cells 1–6.
 *  Write 0x00 first to reset the timer, then write the desired bitmask.
 *  Must also set REG_BAL_TIME before enabling cells. */
#define REG_BAL_CTRL        0x32

/** Balance timer register.
 *  Value 0x82 = 2 minutes. Timer restarts each time REG_BAL_CTRL is written. */
#define REG_BAL_TIME        0x33

/** ADC conversion trigger register.  Write 1 to start a full conversion cycle.
 *  Allow ~2 ms for the conversion to complete before reading results. */
#define REG_ADC_CONV        0x34

/** Module address assignment register.
 *  Write (address | 0x80) to set a module's address during enumeration.
 *  Valid addresses: 0x01–0x3E (addresses are 6-bit, 0x3F is broadcast). */
#define REG_ADDR_CTRL       0x3B

/** Soft reset register.  Write 0xA5 to trigger a full module reset.
 *  After reset all modules revert to address 0 (unenumerated). */
#define REG_RESET           0x3C

// ============================================================================
// Protocol helpers
// ============================================================================

/** Maximum valid module address (0x01–MAX_MODULE_ADDR inclusive). */
#define MAX_MODULE_ADDR     0x3E

/** Broadcast address — all modules on the bus respond.
 *  Formed as (0x3F << 1) | write-bit, but used bare as 0x7F in write frames. */
#define BROADCAST_ADDR      0x7F

/** Voltage above which a cell reading is considered out-of-range / invalid. */
#define CELL_MAX_VALID_VOLT 4.5f

/** Voltage above which a cell reading is treated as open-circuit / not connected. */
#define CELL_OPEN_VOLT      60.0f

/** Temperature below which a sensor is considered disconnected (~−273 °C). */
#define TEMP_SENSOR_DISCONNECTED -70.0f

// ============================================================================
// BMSComm class
// ============================================================================
// All methods are static — no instantiation required.
// Include this header anywhere you need to talk to a BQ76PL536A module.
// ============================================================================

class BMSComm
{
public:
    // ------------------------------------------------------------------------
    /** Compute CRC-8 (polynomial 0x07) over the given byte array.
     *
     *  Used to protect every write frame and to validate every read reply.
     *
     *  Input : data[] — byte array to checksum, lenInput — number of bytes.
     *  Output: 8-bit CRC value.
     */
    static uint8_t genCRC(uint8_t *data, int lenInput);

    // ------------------------------------------------------------------------
    /** Transmit a framed request to the BMS bus (SERIAL).
     *
     *  Constructs the wire frame in-place, sends it, then restores data[0].
     *  Also prints debug hex output if Logger::isDebug().
     *
     *  Input : data[]   — byte array: [ADDR, REG, DATA...] (address pre-shifted).
     *          dataLen  — total length of data[] including address byte.
     *          isWrite  — true = write frame (LSB of address set, CRC appended);
     *                     false = read request (no CRC, LSB clear).
     *  Output: (none) — data[0] restored to original value on return.
     */
    static void sendData(uint8_t *data, uint8_t dataLen, bool isWrite);

    // ------------------------------------------------------------------------
    /** Drain available bytes from the BMS bus into a caller-supplied buffer.
     *
     *  Reads up to maxLen bytes.  If maxLen bytes arrive the remainder of any
     *  partial frame is discarded.  Also prints debug hex if Logger::isDebug().
     *
     *  Input : data[]  — buffer to receive bytes into.
     *          maxLen  — capacity of data[].
     *  Output: number of bytes actually placed in data[].
     */
    static int getReply(uint8_t *data, int maxLen);

    // ------------------------------------------------------------------------
    /** Send a request and wait for the expected reply, retrying up to 3 times.
     *
     *  Handles the latency between sending and the module's reply by inserting
     *  a small delay proportional to the expected reply length.  Retries are
     *  needed because the ESP32 UART baud divisor is not an exact match for
     *  612500 baud, so occasional framing glitches occur.
     *
     *  Input : data[]   — request frame (see sendData).
     *          dataLen  — length of request frame.
     *          isWrite  — true = write frame.
     *          retData[]— buffer for reply bytes.
     *          retLen   — expected reply length (exact).
     *  Output: actual number of reply bytes received (== retLen on success).
     */
    static int sendDataWithReply(uint8_t *data, uint8_t dataLen, bool isWrite,
                                 uint8_t *retData, int retLen);
};
