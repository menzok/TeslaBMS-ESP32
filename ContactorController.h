/*
 * ContactorController.h
 *
 * Non-blocking contactor controller for Tesla BMS
 */

#pragma once
#include <Arduino.h>
    enum ContactorState {
        OPEN,
        PRECHARGING,
        CONNECTED,
        FAULT
    };

class ContactorController {

public:

    void init();
    void open();
    void close();
    void update();

    ContactorState getState() const;




private:
    ContactorState currentState = OPEN;
    unsigned long prechargeStartTime = 0;
    unsigned long postCloseDelayStart = 0;
    unsigned long faultStartTime = 0;     // time fault was entered, for retry cooldown
};