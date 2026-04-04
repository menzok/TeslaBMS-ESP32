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


    // Dummy hook -  replace this later with real SOC handler
    float getPackCurrentAmps() const;

private:
    ContactorState currentState = OPEN;
    unsigned long prechargeStartTime = 0;
    unsigned long postCloseDelayStart = 0;
};