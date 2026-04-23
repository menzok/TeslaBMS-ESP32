/*
 * ContactorController.cpp
 */

#include "ContactorController.h"
#include "config.h"          
#include "EEPROMSettings.h"  
#include "SOCCalculator.h"

extern SOCCalculator socCalculator;


void ContactorController::init() {
    currentState = OPEN;
}

void ContactorController::open() {
    digitalWrite(PRECHARGE_RELAY_PIN, LOW);
    digitalWrite(CONTACTOR_RELAY_PIN, LOW);
    currentState = OPEN;
}

void ContactorController::close() {
    if (currentState == CONNECTED || currentState == FAULT) {
        return;                    // Already closed, or in fault cooldown - do nothing
    }

	prechargeStartTime = millis(); //note time and start pre-charge sequence (doesnt finish here, just starts it)

    if (eepromdata.prechargeEnabled) {
        digitalWrite(PRECHARGE_RELAY_PIN, HIGH);
        currentState = PRECHARGING;
    }
    else {
        digitalWrite(CONTACTOR_RELAY_PIN, HIGH);
        currentState = CONNECTED;
    }
}

void ContactorController::update() {
    unsigned long now = millis();

    switch (currentState) {
    case PRECHARGING: {
        unsigned long elapsed = now - prechargeStartTime;

        if (eepromdata.currentSensorPresent) {
            // Precharge + current sensor:
            // Wait for current to drop AND the timeout to elapse before closing.
            // If the timeout expires while current is still high, fault.
            float current = socCalculator.getPackCurrentAmps();
            bool currentSettled = (fabsf(current) < 0.5f);
            bool timedOut      = (elapsed >= eepromdata.prechargeTimeoutMs);

            if (timedOut && !currentSettled) {
                // Current never dropped — something is wrong, open and fault
                open();
                faultStartTime = now;
                currentState = FAULT;
            }
            else if (currentSettled && timedOut) {
                // Both conditions met — safe to close main contactor
                digitalWrite(CONTACTOR_RELAY_PIN, HIGH);
                postCloseDelayStart = now;
                currentState = CONNECTED;
            }
            // else: still waiting — leave precharge relay on, check again next tick
        }
        else {
            // Precharge but no current sensor: just wait for the timeout, then close
            if (elapsed >= eepromdata.prechargeTimeoutMs) {
                digitalWrite(CONTACTOR_RELAY_PIN, HIGH);
                postCloseDelayStart = now;
                currentState = CONNECTED;
            }
        }
        break;
    }

    case CONNECTED:
        // Turn off precharge relay 500 ms after main contactor closed
        if (now - postCloseDelayStart > 500) {
            digitalWrite(PRECHARGE_RELAY_PIN, LOW);
        }
        break;

    case FAULT:
        // Both relays already off (set by open() when fault was entered).
        // After 60 s cooldown, reset to OPEN so the overlord can retry.
        if (now - faultStartTime >= 60000UL) {
            currentState = OPEN;
        }
        break;

    case OPEN:
        break;
    }
}

ContactorState ContactorController::getState() const {
    return currentState;
}

