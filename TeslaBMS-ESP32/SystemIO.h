// UNUSED: SystemIO is never included or called from the main sketch.
// systemIO.setup() is not invoked anywhere.
// Candidate for removal: delete SystemIO.h and SystemIO.cpp if hardware I/O
// through this abstraction is not needed.
#pragma once

enum OUTPUTSTATE {
    FLOATING = 0,
    HIGH_12V = 1,
    GND = 2
};

class SystemIO
{
public:
    SystemIO();
    void setup();
    bool readInput(int pin);
    void setOutput(int pin, OUTPUTSTATE state);
    
private:

};

extern SystemIO systemIO;
