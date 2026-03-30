#pragma once
#include "config.h"
#include "BMSModule.h"


class BMSModuleManager
{
public:
    BMSModuleManager();
    int seriescells();
    void clearmodules();
    void StopBalancing();
    void balanceCells();
    void setupBoards();
    void findBoards();
    void renumberBoardIDs();
    void clearFaults();
    void sleepBoards();
    void wakeBoards();
    void getAllVoltTemp();
    void setBatteryID(int id);
    void setPstrings(int Pstrings);
    void setSensors(int sensor, float Ignore);
    float getPackVoltage();
    float getAvgTemperature();
    float getHighTemperature();
    float getLowTemperature();
    float getAvgCellVolt();
    float getLowCellVolt();
    float getHighCellVolt();
    float getHighVoltage();
    float getLowVoltage();
    int getBalancing();
    int getNumModules();
    void printPackSummary();
    void printPackDetails();
    void setModuleTopology(int series, int parallel);   // NEW
    void setCapacityPerStringAh(float ahPerString);
    float getSOC();                                     // NEW
    void updateSOC();                                   // NEW
    void loadSOCFromEEPROM();                           // NEW
    void saveSOCToEEPROM();                             // NEW
    void setCurrentAmps(float amps);                    // NEW - shunt stub
    float getCurrentAmps() { return currentAmps; }      // NEW - getter

private:
    float packVolt;                         // All modules added together
    int Pstring;
    float LowCellVolt;
    float HighCellVolt;
    float lowestPackVolt;
    float highestPackVolt;
    float lowestPackTemp;
    float highestPackTemp;
    float highTemp;
    float lowTemp;
    BMSModule modules[MAX_MODULE_ADDR + 1]; // store data for as many modules as we've configured for.
    int batteryID;
    int numFoundModules;                    // The number of modules that seem to exist
    bool isFaulted;
    int spack;
    int CellsBalancing;
    // === NEW: module topology & capacity (loaded from settings in .ino) ===
    int modulesInSeries = 0;           // will be set by loadSettings()
    int parallelStrings = 0;
    float capacityPerStringAh = 0.0f;
    float packCapacityAh = 0.0f;       // calculated automatically
    float remainingAh = 0.0f;
    float socPercent = 50.0f;
    float currentAmps = 0.0f;          // SHUNT STUB
    unsigned long lastSOCUpdate = 0;
    unsigned long lastSaveMillis = 0;
    unsigned long restStartTime = 0;
};
