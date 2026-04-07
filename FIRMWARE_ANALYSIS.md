# Firmware Analysis — TeslaBMS-ESP32

**Date:** 2026-04-07  
**Scope:** `BMSOverlord`, `BMSModuleManager`, `BMSModule`, `SOCCalculator`, `ContactorController`, `EEPROMSettings`, `Logger`, `BMSUtil`, `config.h`, `TeslaBMS-ESP32.ino`  
**SerialConsoleMenu excluded** per instructions.

---

## 1. Overall Completeness & Quality Assessment

The firmware is well-structured for an embedded BMS: clean class separation, a proper state machine in `BMSOverlord`, an OCV-SOC lookup table with bilinear temperature interpolation, coulomb-counting with OCV blending, per-cell debounced fault detection, and a hardware watchdog. Naming and comments are clear throughout. The SOC table is based on real HPPC data with proper attribution.

**Positive highlights:**
- Hardware watchdog configured and tightened after successful boot (good practice).
- Per-cell fault debouncing prevents noise-triggered shutdowns.
- Precharge sequence is non-blocking and has a safety timeout fallback.
- OCV-SOC table uses bilinear interpolation across both voltage and temperature axes.
- Coulomb-counting with OCV blend correction and hard endpoint resets is solid.
- Fault log with ring-buffer shift (oldest dropped) is appropriate for the EEPROM size.

**Critical pre-existing bugs** (present before the second task arrives, affecting normal operation today):

1. EEPROM is never actually initialised (`EEPROM.begin()` missing) — settings are never persisted.
2. The cell safety-check loop is 0-indexed but module storage is 1-indexed — causes false undervoltage faults on every cycle from boot.
3. Debounce counters use `==` instead of `>=` — active faults are cleared after exactly one cycle, causing continuous fault/normal oscillation.

These three bugs alone mean the BMS as shipped does not function correctly. All have been fixed in this PR.

---

## 2. Bugs / Issues / Edge Cases — Full List

### 🔴 Critical

#### Bug 1 — `runSafetyChecks()`: Wrong module indexing (false faults + missed real modules)
**File:** `BMSOverlord.cpp`  
**Original:**
```cpp
for (uint8_t m = 0; m < bms.getNumberOfModules(); m++) {
    CellDetails cell = bms.getCellDetails(m, c);
```
Modules are stored 1-indexed (`modules[1]`..`modules[N]`). `getCellDetails(0, c)` accesses `modules[0]`, whose `cellVolt[]` are all `0.0f`. Since `0.0f < UnderVSetpoint (2.90 V)`, the BMS fires a false undervoltage fault on **every boot** for every cell. The last real module (address `numFoundModules`) is never checked.  
**Fix:** Changed to `for (int m = 1; m <= MAX_MODULE_ADDR; m++) { if (!bms.moduleExists(m)) continue; ... }` — matching the pattern used everywhere else in the codebase. Added `moduleExists()` accessor to `BMSModuleManager`.

---

#### Bug 2 — Debounce `==` instead of `>=` → fault/normal oscillation every cycle
**File:** `BMSOverlord.cpp`  
**Original:**
```cpp
if (++ovDebounce[m][c] == eepromdata.CELL_FAULT_DEBOUNCE) {
    anyFault = true;
}
```
With `CELL_FAULT_DEBOUNCE = 3`: cycle 3 sets `anyFault = true` → Fault state. Cycle 4: counter is 4, `4 == 3` is false → `anyFault` stays false → immediately back to Normal, `clearLastFaultIfResolved()` is called. The BMS oscillates every single cycle for the entire duration of any real fault. After 253 more cycles the `uint8_t` wraps to 3 again, re-logging the fault and re-saving to flash.  
Same bug in `uvDebounce`, `otDebounce`, `utDebounce`, and `commsDebounce`.  
**Fix:** Capped counter at 255, changed log trigger to `== threshold`, changed `anyFault` assertion to `>= threshold`.

---

#### Bug 3 — `EEPROM.begin()` never called
**File:** `EEPROMSettings.cpp`  
On ESP32-Arduino, `EEPROM.get()`, `EEPROM.put()`, and `EEPROM.commit()` are all **no-ops** until `EEPROM.begin(size)` is called. Without this line, every boot reads 0xFF bytes, fails the version/checksum check, calls `loadDefaults()`, then `save()` — which also silently does nothing. **Settings are never persisted.**  
**Fix:** Added `EEPROM.begin(sizeof(EEPROMData))` as the first line of `EEPROMSettings::load()`.

---

#### Bug 4 — `millis()` overflow in `loop()`
**File:** `TeslaBMS-ESP32.ino`  
```cpp
if (millis() > (lastUpdate + 1000)) {
```
After ~49 days `millis()` wraps to 0. `0 > (large_value + 1000)` is false. The BMS stops updating until the overflow resolves.  
**Fix:** `if (millis() - lastUpdate >= 1000UL)` — unsigned subtraction is always correct across the rollover.

---

### 🟠 High

#### Bug 5 — Division by zero: `getAvgTemperature()` and `getAvgCellVolt()` when no modules found
**File:** `BMSModuleManager.cpp`  
```cpp
avg = avg / (float)numFoundModules;  // NaN/Inf when 0
```
If no modules respond (first boot, wiring fault), this produces NaN/Inf which propagates into the SOC calculator and safety checks.  
**Fix:** Added `if (numFoundModules == 0) return 0.0f;` guard before the division.

---

#### Bug 6 — Division by zero: `SOCCalculator::begin()` when `parallelStrings == 0` or no modules
**File:** `SOCCalculator.cpp`  
```cpp
_cellsInSeries = (int)(bms.getNumberOfModules() / eepromdata.parallelStrings) * 6;
// ...
float cellVoltage = summary.voltage / (float)_cellsInSeries;  // /0 crash
float deltaSOC = (deltaAh / _packCapacityAh) * 100.0f;        // /0 crash
```
If `parallelStrings` is 0 (corrupted EEPROM before defaults load) or no modules are found, both cause division by zero in `update()` every tick.  
**Fix:** Added guards: `strings = max(parallelStrings, 1)`, `if (_cellsInSeries == 0) _cellsInSeries = 1`.

---

#### Bug 7 — Overcurrent check is unidirectional; misses discharge overcurrent
**File:** `BMSOverlord.cpp`  
```cpp
if (currentA > eepromdata.OVERCURRENT_THRESHOLD_A) {
```
`_filteredCurrentA` is negative during discharge. `-400 A > 350 A` is false. A large discharge event is silently ignored.  
**Fix:** `if (fabsf(currentA) > eepromdata.OVERCURRENT_THRESHOLD_A)`

---

#### Bug 8 — `loadDefaults()` never resets `socPercent` and `coulombCountAh`
**File:** `EEPROMSettings.cpp`  
`loadDefaults()` calls the three partial-reset functions but none of them touch `socPercent` or `coulombCountAh`. The constants `DEFAULT_SOC_PERCENT` and `DEFAULT_COULOMB_COUNT_AH` exist but were never applied by any reset path.  
**Fix:** Added both fields to `resetBatteryConfig()`.

---

### 🟡 Medium

#### Bug 9 — `storageBalanceEndMs` comparison not millis-overflow-safe
**File:** `BMSOverlord.cpp`  
```cpp
storageBalanceEndMs = now + eepromdata.STORAGE_BALANCE_DURATION_MS;
// ...
if (balancingActive && now >= storageBalanceEndMs) {
```
After a millis rollover, `0 >= large_wrapped_number` fires immediately, ending the balance cycle prematurely.  
**Fix:** Renamed to `storageBalanceStartMs`; comparison changed to `(now - storageBalanceStartMs) >= STORAGE_BALANCE_DURATION_MS`.

---

#### Bug 10 — Adaptive current sensor offset `_currentSensorAdaptiveOffsetV` is unbounded
**File:** `SOCCalculator.cpp`  
```cpp
_currentSensorAdaptiveOffsetV += 0.0003f * socError;
```
No `constrain()`. Over a long run with any systematic SOC error the value drifts arbitrarily large, eventually invalidating all current readings.  
**Fix:** `constrain(..., -0.1f, 0.1f)` — ±0.1 V ≈ ±80 A at default sensor range.

---

#### Bug 11 — EEPROM checksum is not a real checksum
**File:** `EEPROMSettings.cpp`  
```cpp
eepromdata.checksum = eepromdata.version + 42;
```
Only validates that the version byte is sane. Any corruption in the other 80+ bytes of the struct goes undetected.  
**Fix:** Replaced with CRC-8 (poly 0x07) over all struct bytes except the checksum field itself. `EEPROM_VERSION` bumped to 0x13 to force a clean reset on devices with the old algorithm.

---

#### Bug 12 — Fault log EEPROM write frequency (flash wear risk)
**File:** `BMSOverlord.cpp`  
Every fault entry triggered an immediate `EEPROM.commit()`. Combined with the old debounce wrap-around bug, this could write to flash every few minutes. Flash is typically rated for ~100k write cycles.  
**Fix:** Rate-limited to at most one fault-log save per 60 seconds. The fault entry is always written to RAM immediately.

---

### 🟢 Low / Cosmetic

#### Bug 13 — Double semicolon
**File:** `ContactorController.cpp`  
```cpp
float current = socCalculator.getPackCurrentAmps();;
```
**Fix:** Removed extra semicolon.

---

#### Bug 14 — Redundant `extern EEPROMData eepromdata` declarations
**Files:** `BMSModule.cpp`, `BMSModuleManager.cpp`  
Both files already `#include "EEPROMSettings.h"` which declares `extern EEPROMData eepromdata`. The second declaration is redundant and misleading.  
**Fix:** Removed both.

---

#### Bug 15 — Precharge completion current check not absolute-value
**File:** `ContactorController.cpp`  
```cpp
if (current < 0.5f) { prechargeDone = true; }
```
ADC noise can make the filtered current slightly negative near zero. `current < 0.5f` is trivially true for e.g. `-2.0 A`, declaring pre-charge done prematurely.  
**Fix:** `if (fabsf(current) < 0.5f)`

---

#### Bug 16 — `int8_t` overflow in temperature encoding
**Files:** `BMSModuleManager.cpp` (`getModuleSummary`, `getCellDetails`)  
Temperature is stored as `raw°C + 40` to fit a signed byte. The old code only guarded negative overflow (`if (temp < 0) temp = 0`); a reading above ~87°C would silently wrap to a large negative number. `getBatterySummary()` already used `constrain(0, 255)` correctly; these two functions were inconsistent.  
**Fix:** Replaced with `constrain(temp, -128, 127)` in both functions.

---

## 3. Mutex Identification

See `MUTEX_IDENTIFICATION_LIST.md` for the complete list of all locations requiring synchronisation when a second FreeRTOS task is added. Summary of highest-risk areas:

- **`eepromdata` global struct** — accessed by every component; any multi-field read or write sequence is a potential torn-access site from a second task.
- **`BMSOverlord::currentState`** — written by `runSafetyChecks()`, read by `getState()` / `isFaulted()` / `handleContactorLogic()`.
- **`BMSModuleManager::modules[]` array** — written wholesale by `getAllVoltTemp()`, read by `getCellDetails()` / `getBatterySummary()` / `printPackSummary()`.
- **`SOCCalculator::_filteredCurrentA`** — written by `update()`, read by `getPackCurrentAmps()`.
- **`BMSUtil::sendData()` / `getReply()`** — share a single UART; concurrent calls interleave bytes and corrupt the BMS protocol.

**No mutex code has been added in this PR** — see `MUTEX_IDENTIFICATION_LIST.md` for the full site-by-site list to use when designing the second task.

---

## 4. Recommended Architectural Follow-ups (not implemented here)

### A — EEPROM accessor layer (before second task)
There are 40+ direct raw-field accesses to `eepromdata` scattered across every `.cpp` file. When the second task arrives, every single one needs a mutex. Strongly recommended: introduce a thin accessor layer (`EEPROMSettings::getThreshold()`, `EEPROMSettings::setSOC()`, etc.) where the mutex is acquired/released internally. This localises all synchronisation to one class.

### B — Watchdog subscription for second task
`esp_task_wdt_add(NULL)` subscribes only the Arduino loop task. When the second FreeRTOS task is created, call `esp_task_wdt_add(NULL)` from within that task's function and call `esp_task_wdt_reset()` on its own schedule. Otherwise the watchdog will not detect a hung second task.
