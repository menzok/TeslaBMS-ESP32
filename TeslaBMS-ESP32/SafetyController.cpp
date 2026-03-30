#include "SafetyController.h"
#include "BMSModuleManager.h"
#include "Logger.h"

SafetyController::SafetyController(BMSModuleManager& bms, EEPROMSettings& settings)
    : _bms(bms), _settings(settings)
{
}

void SafetyController::update()
{
    float highCell = _bms.getHighCellVolt();
    float lowCell  = _bms.getLowCellVolt();
    // getAvgTemperature() is called by BMSModuleManager::update() before this
    // method, ensuring getHighTemperature()/getLowTemperature() are current.
    float highTemp = _bms.getHighTemperature();
    float lowTemp  = _bms.getLowTemperature();

    float cellGap = highCell - lowCell;

    // --- Evaluate new alarm states ---
    bool newAlarmOV = (highCell > _settings.OverVSetpoint);
    bool newAlarmUV = (lowCell > 0.5f && lowCell < _settings.UnderVSetpoint);  // ignore unpopulated/disconnected cells
    bool newAlarmOT = (highTemp > TEMP_SENSOR_DISCONNECTED && highTemp > _settings.OverTSetpoint);
    bool newAlarmUT = (highTemp > TEMP_SENSOR_DISCONNECTED && lowTemp  < _settings.UnderTSetpoint);
    bool newAlarmCG = (cellGap > _settings.CellGap);

    // --- Apply hysteresis: log only on state changes ---
    if (newAlarmOV != _alarmOverV) {
        if (newAlarmOV) Logger::warn("ALARM SET: Cell overvoltage: %.3fV (limit %.3fV)",   highCell, _settings.OverVSetpoint);
        else            Logger::info("ALARM CLEAR: Cell overvoltage resolved");
        _alarmOverV = newAlarmOV;
    }
    if (newAlarmUV != _alarmUnderV) {
        if (newAlarmUV) Logger::warn("ALARM SET: Cell undervoltage: %.3fV (limit %.3fV)",  lowCell,  _settings.UnderVSetpoint);
        else            Logger::info("ALARM CLEAR: Cell undervoltage resolved");
        _alarmUnderV = newAlarmUV;
    }
    if (newAlarmOT != _alarmOverT) {
        if (newAlarmOT) Logger::warn("ALARM SET: Over temperature: %.1fC (limit %.1fC)",   highTemp, _settings.OverTSetpoint);
        else            Logger::info("ALARM CLEAR: Over temperature resolved");
        _alarmOverT = newAlarmOT;
    }
    if (newAlarmUT != _alarmUnderT) {
        if (newAlarmUT) Logger::warn("ALARM SET: Under temperature: %.1fC (limit %.1fC)",  lowTemp,  _settings.UnderTSetpoint);
        else            Logger::info("ALARM CLEAR: Under temperature resolved");
        _alarmUnderT = newAlarmUT;
    }
    if (newAlarmCG != _alarmCellGap) {
        if (newAlarmCG) Logger::warn("ALARM SET: Cell gap too large: %.0fmV (limit %.0fmV)", cellGap * 1000.0f, _settings.CellGap * 1000.0f);
        else            Logger::info("ALARM CLEAR: Cell gap within limits");
        _alarmCellGap = newAlarmCG;
    }
}

bool SafetyController::isChargingAllowed() const
{
    // Disallow charging if any voltage or temperature alarm that can damage cells is active
    return !_alarmOverV && !_alarmOverT && !_alarmUnderT;
}

bool SafetyController::isDischargingAllowed() const
{
    // Disallow discharging below the hard under-voltage floor
    return !_alarmUnderV;
}

bool SafetyController::isFaulted() const
{
    return _alarmOverV || _alarmUnderV || _alarmOverT || _alarmUnderT || _alarmCellGap;
}

uint8_t SafetyController::getAlarmBitmask() const
{
    uint8_t mask = 0;
    if (_alarmOverV)   mask |= ALARM_OVER_V;
    if (_alarmUnderV)  mask |= ALARM_UNDER_V;
    if (_alarmOverT)   mask |= ALARM_OVER_T;
    if (_alarmUnderT)  mask |= ALARM_UNDER_T;
    if (_alarmCellGap) mask |= ALARM_CELL_GAP;
    return mask;
}
