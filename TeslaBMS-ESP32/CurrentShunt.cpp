// ============================================================================
// CurrentShunt.cpp  —  Serial ASCII current-shunt reader
// ============================================================================
// See CurrentShunt.h for protocol description.
// ============================================================================

#include "CurrentShunt.h"

void CurrentShunt::begin()
{
    SERIAL_SHUNT.begin(SERIAL_SHUNT_BAUD, SERIAL_8N1,
                       SERIAL_SHUNT_RX_PIN, SERIAL_SHUNT_TX_PIN);
}

void CurrentShunt::update()
{
    // Use a fixed-size char buffer to avoid String heap allocations.
    // Processes all complete lines available without blocking.
    static char buf[32];
    static uint8_t pos = 0;

    while (SERIAL_SHUNT.available())
    {
        char c = (char)SERIAL_SHUNT.read();
        if (c == '\n')
        {
            buf[pos] = '\0';
            // Trim any trailing CR
            if (pos > 0 && buf[pos - 1] == '\r') buf[--pos] = '\0';

            if (strncmp(buf, "CURRENT:", 8) == 0)
                _currentAmps = atof(buf + 8);

            pos = 0;
        }
        else if (pos < (uint8_t)(sizeof(buf) - 1))
        {
            buf[pos++] = c;
        }
        else
        {
            pos = 0; // overflow — discard this line
        }
    }
}

CurrentShunt currentShunt;
