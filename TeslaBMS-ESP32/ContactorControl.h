#pragma once

#include <Arduino.h>
#include "config.h"
#include "SystemIO.h"

// ============================================================================
// ContactorControl.h  —  Pack contactor state machine
// ============================================================================
// Controls three outputs via SystemIO using the DOUT H/L driver pairs defined
// in config.h:
//
//   DOUT1  — Main positive contactor  (normally open, energise to CLOSE)
//   DOUT2  — Main negative contactor  (normally open, energise to CLOSE)
//   DOUT3  — Pre-charge contactor     (normally open, energise to CLOSE)
//
// State machine:
//
//   OPEN ──requestClose()──► PRE_CHARGE ──timeout──► CLOSED
//     ▲                                                  │
//     │◄────────────requestOpen() / alarm ───────────────┘
//     │
//   FAULT ◄─── requestOpen() while in any state + alarm flag set
//              (stays FAULT until resetFault() is called)
//
// Pre-charge timeout: PRECHARGE_TIME_MS (default 2 000 ms).
// During PRE_CHARGE: negative contactor + pre-charge resistor are closed.
// On transition to CLOSED: main positive contactor closes, pre-charge opens.
// ============================================================================

#define CONTACTOR_MAIN_POS  0   // DOUT1 output index (SystemIO 0-based)
#define CONTACTOR_MAIN_NEG  1   // DOUT2 output index
#define CONTACTOR_PRECHARGE 2   // DOUT3 output index

#define PRECHARGE_TIME_MS   2000UL  // time in pre-charge before closing main

enum class ContactorState
{
    OPEN,        // both main contactors open — no power to load
    PRE_CHARGE,  // negative closed + pre-charge resistor closed
    CLOSED,      // both main contactors closed — pack connected to load
    FAULT        // tripped by alarm — will not re-close until resetFault()
};

class ContactorControl
{
public:
    // Call once in setup() after SystemIO::setup().
    void begin();

    // Call every loop() iteration.
    // hasAlarm: true if any BMS safety alarm is active.
    // When hasAlarm transitions to true the contactors open and enter FAULT.
    void update(bool hasAlarm);

    // Request contactors close.  Begins pre-charge sequence.
    // Ignored when in FAULT state — call resetFault() first.
    void requestClose();

    // Open contactors immediately.  If hasAlarm is true, enters FAULT.
    void requestOpen(bool hasAlarm = false);

    // Clear FAULT state.  Does not close contactors — call requestClose() after.
    void resetFault();

    ContactorState getState() const { return _state; }
    bool isEngaged()          const { return _state == ContactorState::CLOSED; }

private:
    ContactorState _state       = ContactorState::OPEN;
    unsigned long  _stateEntryMs = 0;

    void _openAll();
    void _setMainPos(bool on);
    void _setMainNeg(bool on);
    void _setPreCharge(bool on);
};

extern ContactorControl contactorControl;
