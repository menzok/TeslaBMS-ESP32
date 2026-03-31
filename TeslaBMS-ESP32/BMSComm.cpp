// ============================================================================
// BMSComm.cpp  —  BQ76PL536A low-level communication driver implementation
// ============================================================================
// See BMSComm.h for full protocol and register documentation.
// ============================================================================

#include "config.h"
#include "BMSComm.h"
#include "Logger.h"

// ---------------------------------------------------------------------------
// genCRC — CRC-8 (polynomial 0x07)
// ---------------------------------------------------------------------------
uint8_t BMSComm::genCRC(uint8_t *data, int lenInput)
{
    uint8_t generator = 0x07;
    uint8_t crc = 0;

    for (int x = 0; x < lenInput; x++)
    {
        crc ^= data[x];

        for (int i = 0; i < 8; i++)
        {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ generator);
            else
                crc <<= 1;
        }
    }

    return crc;
}

// ---------------------------------------------------------------------------
// sendData — transmit a framed request on the BMS UART bus
// ---------------------------------------------------------------------------
void BMSComm::sendData(uint8_t *data, uint8_t dataLen, bool isWrite)
{
    uint8_t orig     = data[0];
    uint8_t addrByte = data[0];

    if (isWrite) addrByte |= 1;          // set write bit in address byte

    SERIAL.write(addrByte);
    SERIAL.write(&data[1], dataLen - 1); // assumes at least 2 bytes (addr + reg)
    data[0] = addrByte;
    if (isWrite) SERIAL.write(genCRC(data, dataLen));

    if (Logger::isDebug())
    {
        SERIALCONSOLE.print("Sending: ");
        SERIALCONSOLE.print(addrByte, HEX);
        SERIALCONSOLE.print(" ");
        for (int x = 1; x < dataLen; x++) {
            SERIALCONSOLE.print(data[x], HEX);
            SERIALCONSOLE.print(" ");
        }
        if (isWrite) SERIALCONSOLE.print(genCRC(data, dataLen), HEX);
        SERIALCONSOLE.println();
    }

    data[0] = orig; // restore caller's array
}

// ---------------------------------------------------------------------------
// getReply — drain available bytes from the BMS UART into caller's buffer
// ---------------------------------------------------------------------------
int BMSComm::getReply(uint8_t *data, int maxLen)
{
    int numBytes = 0;

    if (Logger::isDebug()) SERIALCONSOLE.print("Reply: ");

    while (SERIAL.available() && numBytes < maxLen)
    {
        data[numBytes] = SERIAL.read();
        if (Logger::isDebug()) {
            SERIALCONSOLE.print(data[numBytes], HEX);
            SERIALCONSOLE.print(" ");
        }
        numBytes++;
    }

    // Discard overflow bytes to keep the RX buffer clean
    if (maxLen == numBytes)
    {
        while (SERIAL.available()) SERIAL.read();
    }

    if (Logger::isDebug()) SERIALCONSOLE.println();

    return numBytes;
}

// ---------------------------------------------------------------------------
// sendDataWithReply — send a request, wait, read reply; retry up to 3 times
// ---------------------------------------------------------------------------
int BMSComm::sendDataWithReply(uint8_t *data, uint8_t dataLen, bool isWrite,
                                uint8_t *retData, int retLen)
{
    int attempts = 1;
    int returnedLength = 0;

    while (attempts < 4)
    {
        sendData(data, dataLen, isWrite);
        delay(2 * ((retLen / 8) + 1)); // proportional wait for reply
        returnedLength = getReply(retData, retLen);
        if (returnedLength == retLen) return returnedLength;
        attempts++;
    }

    return returnedLength; // return best effort on persistent failure
}
