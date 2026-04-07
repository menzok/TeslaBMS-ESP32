#pragma once
#include <Arduino.h>
#include "EEPROMSettings.h"
#include "BMSModuleManager.h"
#include "config.h"

extern BMSModuleManager bms;
// ─── Coulomb counter auto-reset thresholds ───────────────────────────────────
#define SOC_CELL_FULL_VOLTAGE       4.18f   // V/cell - declare 100%
#define SOC_CELL_EMPTY_VOLTAGE      3.00f   // V/cell - declare 0%
#define SOC_RESET_CONFIRM_TICKS     5       // consecutive ticks before reset fires

// ─── OCV correction ──────────────────────────────────────────────────────────
#define SOC_OCV_REST_BLEND_RATE     0.05f   // fraction of error corrected per tick at rest
#define SOC_ZERO_CURRENT_THRESHOLD  0.8f    // amps - below this = pack at rest

// ─── ADC ─────────────────────────────────────────────────────────────────────
#define SOC_ADC_OVERSAMPLE          8       // samples averaged per current reading
#define SOC_ZERO_CAL_SAMPLES        32      // blocking samples taken in begin()


/*
 * OCV-SOC Lookup Table for Panasonic NCA Lithium-Ion Cells
 *
 * Data source:
 *   McMaster University battery test data (Tesla/Panasonic 2170 NCA cells, 4.5 Ah nominal)
 *   Specifically Cell 4 ("m1000" – 1000 kg payload + HVAC ON)
 *   Extracted from HPPC test long zero-current rest periods (end-of-rest voltage used)
 *   Temperatures: -10°C, 10°C, 25°C, 40°C
 *   Full dataset available here: https://battery.mcmaster.ca/research/standardized-blind-modeling-tool/
 *
 * Why I chose this dataset for my BMS:
 *   My target pack is a Tesla Model S 85 kWh module using Panasonic NCR18650PF cells.
 *   These 2170 cells are from a Model 3, so they are not an exact match.
 *
 *   However the chemistry is the same Panasonic NCA family, and the OCV curve is extremely close
 *   (typical difference is only 10-25 mV across the full SOC range). For a practical BMS this
 *   is more than good enough. The only real differences are internal resistance and thermal
 *   behavior, which are not used in my model and do not affect this OCV lookup table.
 *
 *   This was the highest-quality, easiest-to-work-with public NCA dataset available with proper
 *   long relaxation periods, so I used it to build a solid, temperature-dependent OCV table.
 *
 */

// ─── OCV-SOC lookup table (m1000 HPPC data) ─────────────────
#define SOC_LUT_POINTS  14
#define SOC_LUT_TEMPS    4

static const float SOC_LUT_SOC[SOC_LUT_POINTS] = {
     0.0f,  5.0f, 10.0f, 15.0f, 20.0f, 30.0f,
    40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f,
    95.0f, 100.0f
};

static const float SOC_LUT_TEMP_POINTS[SOC_LUT_TEMPS] = {
    -10.0f, 10.0f, 25.0f, 40.0f
};

// [SOC_point][temp_col]  ← REAL DATA  HPPC rests
static const float SOC_LUT_OCV[SOC_LUT_POINTS][SOC_LUT_TEMPS] = {
    //   -10°C     10°C      25°C      40°C
    { 3.3274f,  3.1611f,  3.0364f,  2.9789f },  //   0%   (extrapolated data)
    { 3.3070f,  3.1413f,  3.1198f,  3.1182f },  //   5%
    { 3.4013f,  3.2065f,  3.2541f,  3.2423f },  //  10%
    { 3.3713f,  3.3339f,  3.3667f,  3.3539f },  //  15%
    { 3.4473f,  3.4300f,  3.4512f,  3.4457f },  //  20%
    { 3.5422f,  3.5419f,  3.5517f,  3.5396f },  //  30%
    { 3.6314f,  3.6467f,  3.6598f,  3.6529f },  //  40%
    { 3.7212f,  3.7408f,  3.7561f,  3.7537f },  //  50%
    { 3.8208f,  3.8275f,  3.8405f,  3.8403f },  //  60%
    { 3.8936f,  3.9217f,  3.9388f,  3.9395f },  //  70%
    { 3.9779f,  4.0251f,  4.0463f,  4.0451f },  //  80%
    { 4.0603f,  4.0897f,  4.1005f,  4.1032f },  //  90%
    { 4.0898f,  4.1007f,  4.1226f,  4.1264f },  //  95%
    { 4.1624f,  4.1664f,  4.1797f,  4.1891f }   // 100%
};
_Static_assert(sizeof(SOC_LUT_OCV) / sizeof(SOC_LUT_OCV[0]) == SOC_LUT_POINTS, "SOC LUT row mismatch");
_Static_assert(sizeof(SOC_LUT_OCV[0]) / sizeof(float) == SOC_LUT_TEMPS, "SOC LUT col mismatch");
// ─────────────────────────────────────────────────────────────────────────────

class SOCCalculator {
public:


    

    SOCCalculator();

    // Call once in setup() BEFORE contactor closes.
    // Blocks ~160ms for ADC zero calibration - safe, contactor is open.
    // cellsInSeries  : number of series cells (to get per-cell voltage)
    // packCapacityAh : total pack Ah

    void begin();


    // Call every SOC_UPDATE_INTERVAL_MS from main scheduler - never blocks
    void update();

    // Pack current in amps (+ charge, - discharge)
    // Returns 0.0 if currentSensorPresent == false
    // Used by ContactorController for precharge detection
    float getPackCurrentAmps() const;
 

    // SOC as 0-100 byte for BatterySummary.soc
    uint8_t getSOCByte() const;

private:
    // ── Internal ESP32 ADC constants ─────────────
    static constexpr float ADC_VREF_11DB = 3.9f;    // full scale at ADC_11db attenuation
    static constexpr float ADC_MAX_COUNT = 4095.0f;  // 12-bit resolution
	float   currentSensorZeroOffsetV;   // Sensor Drift on ADC measured at boot but needs running calibration.
    // ── Runtime state ───────────────────────────────────────────────────
    float         _packCapacityAh;
    int           _cellsInSeries;
    float         _filteredCurrentA;
    float         _lastCurrentA;
    unsigned long _lastUpdateMs;
    bool          _initialised;
    unsigned long _lastSaveMs;
    float         _currentSensorAdaptiveOffsetV;

    // ── Endpoint reset confirmation ───────────────────────────────────────
    uint8_t       _fullConfirmTicks;
    uint8_t       _emptyConfirmTicks;

    // ── Helpers ───────────────────────────────────────────────────────────
    float   _readCurrentAmps()                          const;
    float   _adcToVoltage(int raw)                      const;
    float   _ocvToSOC(float cellVoltage, float tempC)   const;
    float   _clampSOC(float soc)                        const;
};