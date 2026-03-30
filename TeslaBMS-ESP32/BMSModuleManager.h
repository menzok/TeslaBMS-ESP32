#pragma once
#include "config.h"
#include "BMSModule.h"

// Forward declaration – SafetyController.h includes BMSModuleManager.h,
// so we use a pointer + forward declaration to break the cycle.
class SafetyController;

class BMSModuleManager
{
public:
    BMSModuleManager();
    int seriescells();
    void clearmodules();    // UNUSED: never called externally — candidate for removal
    void StopBalancing();   // UNUSED: never called externally — candidate for removal
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
    float getHighVoltage();  // UNUSED: never called — candidate for removal
    float getLowVoltage();   // UNUSED: never called — candidate for removal
    int getBalancing();
    int getNumModules();
    void printPackSummary();
    void printPackDetails();
    void setModuleTopology(int series, int parallel);
    void setCapacityPerStringAh(float ahPerString);
    float getSOC();
    void updateSOC();
    void loadSOCFromEEPROM();
    void saveSOCToEEPROM();
    void setCurrentAmps(float amps);
    float getCurrentAmps() { return currentAmps; }

    // --- Master updater ---
    // Call once per second from loop(). Executes in order:
    //   balanceCells → getAllVoltTemp → getAvgTemperature → updateSOC → SafetyController::update()
    void update();

    // Provide the SafetyController instance before the first update() call.
    void setSafetyController(SafetyController* sc);

private:
    float packVolt;
    int Pstring;
    float LowCellVolt;
    float HighCellVolt;
    // UNUSED: tracked session extremes but no getter is ever called — candidate for removal
    float lowestPackVolt;
    float highestPackVolt;
    // UNUSED: tracked session extremes but no getter is ever called — candidate for removal
    float lowestPackTemp;
    float highestPackTemp;
    float highTemp;
    float lowTemp;
    BMSModule modules[MAX_MODULE_ADDR + 1];
    int batteryID;
    int numFoundModules;
    bool isFaulted;
    int spack;
    int CellsBalancing;
    int modulesInSeries = 0;
    int parallelStrings = 0;
    float capacityPerStringAh = 0.0f;
    float packCapacityAh = 0.0f;
    float remainingAh = 0.0f;
    float socPercent = 50.0f;
    float currentAmps = 0.0f;
    unsigned long lastSOCUpdate = 0;
    unsigned long lastSaveMillis = 0;
    unsigned long restStartTime = 0;

    SafetyController* safetyController = nullptr;
};
