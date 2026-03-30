# TeslaBMS-ESP32 — Change Log

This document describes the improvements made to bring the firmware closer to the
upstream [Tom-evnut/TeslaBMSV2](https://github.com/Tom-evnut/TeslaBMSV2) codebase,
add active safety monitoring, and make settings persist across reboots.

---

## Summary of Changes

| Area | What changed |
|------|-------------|
| **Settings persistence** | All settings now saved to ESP32 NVS flash via `Preferences`; survive reboots |
| **BMS communication** | Better voltage scaling, stop-balance before read, dead-cell filtering |
| **Safety alarms** | Active overvoltage / undervoltage / temperature / cell-gap monitoring |
| **New settings** | Charge/discharge setpoints, cell gap, sensor selection, parallel strings |
| **Serial console** | All new settings accessible via USB serial; `E` key dumps all values |

---

## 1. Settings Persistence (ESP32 NVS)

**Previously:** `EEPROM.write()` was commented out. Every reboot reset all settings to
hardcoded defaults. Any changes made via the serial console were lost on power-off.

**Now:**
- `loadSettings()` reads all settings from the ESP32 Non-Volatile Storage (NVS) using
  the Arduino `Preferences` library on every boot.
- `saveSettings()` is called automatically after every serial console configuration
  command, immediately persisting the change to flash.
- On the very first boot (or after a firmware version change), factory defaults are
  written to NVS automatically so subsequent boots use those stored values.

`EEPROM_VERSION` has been updated to `0x11`. Any future change to the `EEPROMSettings`
struct **must** increment this constant so the firmware detects the mismatch and resets
to safe defaults.

---

## 2. BMS Communication Protocol Improvements

These align with the upstream TeslaBMSV2 protocol implementation.

### BMSModule (per-module driver)

| Change | Detail |
|--------|--------|
| Improved voltage constant | `0.0020346293922562f` (was `0.002034609f`) — more accurate 16-bit scaling |
| Module voltage from cells | `moduleVolt` is now the arithmetic sum of all 6 cell voltages rather than the raw GPAI register, giving more accurate module-level readings |
| Dead-cell filtering | A new `IgnoreVolt` threshold skips cells at or below that voltage in low/high/average calculations. Prevents phantom low readings from missing or disconnected cells |
| Cell count debouncing | Active cell count (`scells`) only changes after 3 consecutive consistent reads, preventing transient single-poll dropouts from being treated as cell removals |
| Temperature sensor selection | `settempsensor(n)` — `0` = use both sensors (return min/max), `1` = TS1 only, `2` = TS2 only |
| New methods | `clearmodule()`, `stopBalance()`, `getscells()`, `settempsensor()`, `setIgnoreCell()` |

### BMSModuleManager (pack-level manager)

| Change | Detail |
|--------|--------|
| Stop balance before read | All modules have their balancing resistors stopped before measurements, with a brief settle delay (200 ms for small packs, 50 ms for larger). Prevents balancing current from distorting voltage readings |
| Pack-wide cell extremes | `getLowCellVolt()` and `getHighCellVolt()` now track the single lowest and highest cell voltage across the entire pack, updated every scan cycle |
| Temperature filtering | Modules with disconnected temperature sensors (reading below `TEMP_SENSOR_DISCONNECTED` = −70 °C) are excluded from average/high/low temperature calculations |
| Parallel string support | `setPstrings(n)` divides the summed module voltages by the number of parallel strings to give true string voltage |
| Improved `printPackDetails()` | Now displays series cell count, parallel string count, and the mV delta between the highest and lowest cell in the pack |
| New methods | `clearmodules()`, `StopBalancing()`, `seriescells()`, `setSensors()`, `setPstrings()`, `getHighTemperature()`, `getLowTemperature()`, `getLowCellVolt()`, `getHighCellVolt()`, `getLowVoltage()`, `getHighVoltage()`, `getBalancing()`, `getNumModules()` |

### Named Constants (config.h)

Magic numbers that previously appeared inline have been replaced with named constants:

| Constant | Value | Meaning |
|----------|-------|---------|
| `CELL_MAX_VALID_VOLT` | 4.5 V | Upper bound for a valid cell reading |
| `CELL_OPEN_VOLT` | 60.0 V | Threshold above which a reading is treated as open-circuit |
| `TEMP_SENSOR_DISCONNECTED` | −70.0 °C | Temperature below which a sensor is treated as disconnected |

---

## 3. Safety Alarms

A new `checkSafetyAlarms()` function is called every second after `getAllVoltTemp()`.
It checks five conditions and logs a message **only when the state changes** (set or
clear), preventing console spam.

| Alarm | Condition | MQTT bit |
|-------|-----------|----------|
| Overvoltage | Highest cell > `VOLTLIMHI` | `0x01` |
| Undervoltage | Lowest cell < `VOLTLIMLO` (cells > 0.5 V only) | `0x02` |
| Over-temperature | Highest temp > `TEMPLIMHI` (sensor connected) | `0x04` |
| Under-temperature | Lowest temp < `TEMPLIMLO` (sensor connected) | `0x08` |
| Cell gap | `HighCell − LowCell` > `CELLGAP` | `0x10` |

The MQTT JSON payload now includes extra fields alongside the original `Dc`/`Soc` fields:

```json
{
  "Dc":      {"Voltage": 48.50, "Current": 0.00, "Power": 0.00},
  "Soc":     75,
  "Alarms":  0,
  "LowCell": 3.982,
  "HighCell":3.994,
  "CellGap": 12
}
```

`Alarms` is a bitmask (see table above). `CellGap` is in millivolts.

---

## 4. New and Extended Settings

### Added fields to `EEPROMSettings`

| Field | Command | Default | Description |
|-------|---------|---------|-------------|
| `ChargeVsetpoint` | `CHARGEVSP` | 4.15 V | Per-cell target voltage while charging |
| `DischVsetpoint` | `DISCHVSP` | 3.10 V | Per-cell minimum voltage while discharging |
| `ChargeHys` | `CHARGEHYS` | 0.05 V | Hysteresis below charge setpoint before re-enabling charge |
| `DischHys` | `DISCHHYS` | 0.05 V | Hysteresis above discharge cutoff before re-enabling discharge |
| `WarnOff` | `WARNOFF` | 0.10 V | Voltage offset from trip thresholds at which to warn |
| `CellGap` | `CELLGAP` | 0.20 V | Maximum permitted spread between highest and lowest cell |
| `IgnoreVolt` | `IGNOREVOLT` | 0.50 V | Cells at or below this voltage are excluded from calculations |
| `IgnoreTemp` | `IGNORETEMP` | 0 | Temperature sensor selection (0=both, 1=TS1 only, 2=TS2 only) |
| `Scells` | `SCELLS` | 6 | Number of series cells per module string |
| `Pstrings` | `PSTRINGS` | 1 | Number of parallel battery strings |
| `triptime` | `TRIPTIME` | 1000 ms | How long a fault must persist before it is acted on |

---

## 5. Serial Console Reference

Connect a serial terminal to the USB port at **115200 baud** with line endings enabled
(LF, CR, or CRLF). Type a command and press Enter.

### Single-key Commands

| Key | Action |
|-----|--------|
| `h` / `H` / `?` | Show this help menu |
| `E` | **Dump all current settings** with their current values |
| `S` | Put all BMS boards into sleep mode |
| `W` | Wake up all BMS boards |
| `C` | Clear all board faults |
| `F` | Scan for and list all connected boards |
| `R` | Renumber all boards in daisy-chain order |
| `B` | Trigger one balancing pass immediately |
| `p` | Toggle pack summary printout every 3 s |
| `d` | Toggle pack details printout every 3 s |

### Configuration Commands  (`COMMAND=value` + Enter)

All configuration commands save the new value to NVS immediately.

#### General

| Command | Range | Default | Description |
|---------|-------|---------|-------------|
| `LOGLEVEL=n` | 0–4 | 2 | 0=debug, 1=info, 2=warn, 3=error, 4=off |
| `CANSPEED=n` | 33000–1000000 | 500000 | CAN bus speed in baud (reserved for future use) |
| `BATTERYID=n` | 1–14 | 1 | Battery identifier on CAN bus |
| `PSTRINGS=n` | 1–16 | 1 | Number of parallel battery strings |
| `SCELLS=n` | 1–6 | 6 | Series cells per module string |

#### Voltage

| Command | Range | Default | Description |
|---------|-------|---------|-------------|
| `VOLTLIMHI=v` | 0.0–6.0 V | 4.20 V | Overvoltage alarm threshold per cell |
| `VOLTLIMLO=v` | 0.0–6.0 V | 3.00 V | Undervoltage alarm threshold per cell |
| `CHARGEVSP=v` | 0.0–6.0 V | 4.15 V | Charge voltage setpoint per cell |
| `DISCHVSP=v` | 0.0–6.0 V | 3.10 V | Discharge cutoff voltage per cell |
| `CHARGEHYS=v` | 0.0–1.0 V | 0.05 V | Charge hysteresis |
| `DISCHHYS=v` | 0.0–1.0 V | 0.05 V | Discharge hysteresis |
| `WARNOFF=v` | 0.0–1.0 V | 0.10 V | Warning offset from trip thresholds |
| `CELLGAP=v` | 0.0–1.0 V | 0.20 V | Maximum allowed cell-to-cell spread |
| `IGNOREVOLT=v` | 0.0–3.0 V | 0.50 V | Ignore cells at or below this voltage |

#### Balancing

| Command | Range | Default | Description |
|---------|-------|---------|-------------|
| `BALVOLT=v` | 0.0–6.0 V | 4.10 V | Cell voltage at which balancing activates |
| `BALHYST=v` | 0.0–1.0 V | 0.04 V | How far below BALVOLT before balancing stops |

#### Temperature

| Command | Range | Default | Description |
|---------|-------|---------|-------------|
| `TEMPLIMHI=t` | 0.0–100.0 °C | 50.0 °C | Over-temperature alarm threshold |
| `TEMPLIMLO=t` | −20.0–120.0 °C | −20.0 °C | Under-temperature alarm threshold |
| `IGNORETEMP=n` | 0–2 | 0 | Temperature sensor selection (0=both, 1=TS1, 2=TS2) |

#### Timing

| Command | Range | Default | Description |
|---------|-------|---------|-------------|
| `TRIPTIME=ms` | 0–60000 | 1000 | Fault must persist this long (ms) before alarm fires |

---

## 6. Hardware Configuration

Edit the top of `TeslaBMS-ESP32.ino` to set your WiFi and MQTT broker details:

```cpp
const char* ssid        = "YourSSID";
const char* password    = "YourPassword";
const char* mqtt_server = "192.168.1.x";   // IP address of your MQTT broker
```

BMS modules are connected to **Serial2** (pins 16 RX / 17 TX) at 612500 baud.
The USB serial console runs at **115200 baud** on `Serial` (the USB port).

---

## 7. MQTT Topics

| Topic | Direction | Description |
|-------|-----------|-------------|
| `device/teslabms/Status` | Publish (once at boot) | Device registration for dbus-mqtt-devices |
| `teslabms/battery` | Publish (every second) | Battery state JSON (see Section 3) |
