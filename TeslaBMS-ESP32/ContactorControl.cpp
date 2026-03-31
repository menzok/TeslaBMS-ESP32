// ============================================================================
// ContactorControl.cpp  —  Pack contactor state machine
// ============================================================================
// See ContactorControl.h for state-machine description and pin assignments.
// ============================================================================

#include "ContactorControl.h"
#include "Logger.h"

// ---------------------------------------------------------------------------
// Internal helpers — thin wrappers around SystemIO
// ---------------------------------------------------------------------------

void ContactorControl::_setMainPos(bool on)
{
    systemIO.setOutput(CONTACTOR_MAIN_POS, on ? HIGH_12V : GND);
}

void ContactorControl::_setMainNeg(bool on)
{
    systemIO.setOutput(CONTACTOR_MAIN_NEG, on ? HIGH_12V : GND);
}

void ContactorControl::_setPreCharge(bool on)
{
    systemIO.setOutput(CONTACTOR_PRECHARGE, on ? HIGH_12V : GND);
}

void ContactorControl::_openAll()
{
    _setMainPos(false);
    _setPreCharge(false);
    _setMainNeg(false);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ContactorControl::begin()
{
    _openAll();
    _state = ContactorState::OPEN;
    Logger::info("ContactorControl: initialized — contactors OPEN");
}

void ContactorControl::update(bool hasAlarm)
{
    // Any active alarm trips us into FAULT from any non-FAULT state.
    if (hasAlarm && _state != ContactorState::FAULT)
    {
        _openAll();
        _state = ContactorState::FAULT;
        Logger::warn("ContactorControl: FAULT — alarm active, contactors OPEN");
        return;
    }

    // Pre-charge timeout: advance PRE_CHARGE → CLOSED once timer expires.
    if (_state == ContactorState::PRE_CHARGE)
    {
        if (millis() - _stateEntryMs >= PRECHARGE_TIME_MS)
        {
            _setMainPos(true);
            _setPreCharge(false);
            _state = ContactorState::CLOSED;
            Logger::info("ContactorControl: pre-charge complete — contactors CLOSED");
        }
    }
}

void ContactorControl::requestClose()
{
    if (_state == ContactorState::FAULT)
    {
        Logger::warn("ContactorControl: cannot close — in FAULT state, call resetFault() first");
        return;
    }
    if (_state == ContactorState::CLOSED || _state == ContactorState::PRE_CHARGE)
        return; // already closing or closed

    // Begin pre-charge sequence.
    _setMainPos(false);
    _setPreCharge(true);
    _setMainNeg(true);
    _state        = ContactorState::PRE_CHARGE;
    _stateEntryMs = millis();
    Logger::info("ContactorControl: pre-charge started");
}

void ContactorControl::requestOpen(bool hasAlarm)
{
    _openAll();
    _state = hasAlarm ? ContactorState::FAULT : ContactorState::OPEN;
    Logger::info("ContactorControl: contactors OPEN%s",
                 hasAlarm ? " (FAULT)" : "");
}

void ContactorControl::resetFault()
{
    if (_state != ContactorState::FAULT) return;
    _state = ContactorState::OPEN;
    Logger::info("ContactorControl: FAULT cleared — call requestClose() to reconnect");
}

ContactorControl contactorControl;
