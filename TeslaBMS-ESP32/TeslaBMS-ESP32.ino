#include "Logger.h"
#include "SerialConsole.h"
#include "BMSModuleManager.h"
#include "SafetyController.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>

WiFiClient espClient;
PubSubClient mqtt(espClient);
Preferences prefs;

// ==================== CHANGE THESE ====================
const char* ssid = "Zoom";
const char* password = "gdr543l7";
const char* mqtt_server = "192.168.1.213";   // ← your Pi IP
// =======================================================

BMSModuleManager bms;
EEPROMSettings settings;
SafetyController safety(bms, settings);
SerialConsole console;
uint32_t lastUpdate = 0;

void saveSettings() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUChar("version",       settings.version);
    prefs.putUInt ("canSpeed",      settings.canSpeed);
    prefs.putUChar("batteryID",     settings.batteryID);
    prefs.putUChar("logLevel",      settings.logLevel);
    prefs.putFloat("OverVSP",       settings.OverVSetpoint);
    prefs.putFloat("UnderVSP",      settings.UnderVSetpoint);
    prefs.putFloat("ChargeVSP",     settings.ChargeVsetpoint);
    prefs.putFloat("DischVSP",      settings.DischVsetpoint);
    prefs.putFloat("ChargeHys",     settings.ChargeHys);
    prefs.putFloat("DischHys",      settings.DischHys);
    prefs.putFloat("WarnOff",       settings.WarnOff);
    prefs.putFloat("CellGap",       settings.CellGap);
    prefs.putFloat("IgnoreVolt",    settings.IgnoreVolt);
    prefs.putUChar("IgnoreTemp",    settings.IgnoreTemp);
    prefs.putFloat("OverTSP",       settings.OverTSetpoint);
    prefs.putFloat("UnderTSP",      settings.UnderTSetpoint);
    prefs.putFloat("balanceV",      settings.balanceVoltage);
    prefs.putFloat("balanceHyst",   settings.balanceHyst);
    prefs.putInt  ("Scells",        settings.Scells);
    prefs.putInt  ("Pstrings",      settings.Pstrings);
    prefs.putUShort("triptime",     settings.triptime);
    prefs.putInt("modulesInSeries", settings.modulesInSeries);
    prefs.putInt("parallelStrings", settings.parallelStrings);
    prefs.putFloat("capacityPerStringAh", settings.capacityPerStringAh);
    prefs.end();
    Logger::console("Settings saved to NVS");
}

void loadSettings() {
    prefs.begin(PREFS_NAMESPACE, true); // read-only
    uint8_t storedVersion = prefs.getUChar("version", 0);
    prefs.end();

    if (storedVersion != EEPROM_VERSION) {
        // No valid settings stored – apply factory defaults
        Logger::console("No saved settings found (or version mismatch). Loading factory defaults.");
        settings.version          = EEPROM_VERSION;
        settings.checksum         = 0;
        settings.canSpeed         = 500000;
        settings.batteryID        = 1;
        settings.logLevel         = 2;
        settings.OverVSetpoint    = 4.20f;
        settings.UnderVSetpoint   = 3.00f;
        settings.ChargeVsetpoint  = 4.15f;
        settings.DischVsetpoint   = 3.10f;
        settings.ChargeHys        = 0.05f;
        settings.DischHys         = 0.05f;
        settings.WarnOff          = 0.10f;
        settings.CellGap          = 0.15f;
        settings.IgnoreVolt       = 0.50f;
        settings.IgnoreTemp       = 0;     // 0 = use both sensors
        settings.OverTSetpoint    = 50.0f;
        settings.UnderTSetpoint   = 0.0f;
        settings.balanceVoltage   = 4.10f;
        settings.balanceHyst      = 0.04f;
        settings.Scells           = 6;
        settings.Pstrings         = 1;
        settings.triptime         = 1000;  // 1 second fault persistence before trip
        settings.modulesInSeries = 2;      // your emulator default
        settings.parallelStrings = 2;
        settings.capacityPerStringAh = 232.0f;
        saveSettings();
    } else {
        Logger::console("Loading settings from NVS.");
        prefs.begin(PREFS_NAMESPACE, true);
        settings.version          = storedVersion;
        settings.checksum         = 0;
        settings.canSpeed         = prefs.getUInt ("canSpeed",   500000);
        settings.batteryID        = prefs.getUChar("batteryID",  1);
        settings.logLevel         = prefs.getUChar("logLevel",   2);
        settings.OverVSetpoint    = prefs.getFloat("OverVSP",    4.20f);
        settings.UnderVSetpoint   = prefs.getFloat("UnderVSP",   3.00f);
        settings.ChargeVsetpoint  = prefs.getFloat("ChargeVSP",  4.15f);
        settings.DischVsetpoint   = prefs.getFloat("DischVSP",   3.10f);
        settings.ChargeHys        = prefs.getFloat("ChargeHys",  0.05f);
        settings.DischHys         = prefs.getFloat("DischHys",   0.05f);
        settings.WarnOff          = prefs.getFloat("WarnOff",    0.10f);
        settings.CellGap          = prefs.getFloat("CellGap",    0.15f);
        settings.IgnoreVolt       = prefs.getFloat("IgnoreVolt", 0.50f);
        settings.IgnoreTemp       = prefs.getUChar("IgnoreTemp", 0);
        settings.OverTSetpoint    = prefs.getFloat("OverTSP",    50.0f);
        settings.UnderTSetpoint   = prefs.getFloat("UnderTSP",   0.0f);
        settings.balanceVoltage   = prefs.getFloat("balanceV",   4.10f);
        settings.balanceHyst      = prefs.getFloat("balanceHyst",0.04f);
        settings.Scells           = prefs.getInt  ("Scells",     6);
        settings.Pstrings         = prefs.getInt  ("Pstrings",   1);
        settings.triptime         = prefs.getUShort("triptime",  1000);
        settings.modulesInSeries = prefs.getInt("modulesInSeries", 2);
        settings.parallelStrings = prefs.getInt("parallelStrings", 2);
        settings.capacityPerStringAh = prefs.getFloat("capacityPerStringAh", 232.0f);
        prefs.end();
    }

    Logger::setLoglevel((Logger::LogLevel)settings.logLevel);
}

void setup() {
    delay(2000);
    SERIALCONSOLE.begin(115200);
    SERIALCONSOLE.println("\n=== TeslaBMS MQTT for dbus-mqtt-devices ===");

    SERIAL.begin(BMS_BAUD, SERIAL_8N1, BMS_SERIAL_RX_PIN, BMS_SERIAL_TX_PIN);

    pinMode(13, INPUT);
    loadSettings();

    bms.setModuleTopology(settings.modulesInSeries, settings.parallelStrings);
    bms.setCapacityPerStringAh(settings.capacityPerStringAh);
    bms.loadSOCFromEEPROM();
    bms.setSafetyController(&safety);
    SERIALCONSOLE.println("Init BMS board numbers");
    bms.renumberBoardIDs();
    bms.clearFaults();
    bms.findBoards();

    int expectedModules = settings.modulesInSeries * settings.parallelStrings;
    if (bms.getNumModules() != expectedModules) {
        Logger::error("=== MODULE COUNT MISMATCH ===");
        Logger::error("Configured: %d modules in series × %d parallel = %d total",
            settings.modulesInSeries, settings.parallelStrings, expectedModules);
        Logger::error("Detected on bus: %d modules", bms.getNumModules());
        Logger::error("Pack voltage will be INCORRECT until config is fixed!");
    }

    Logger::console("Pack configured: %dS × %dP | %.0f Ah per string | Total capacity %.0f Ah | SOC = %.1f%%",
        settings.modulesInSeries, settings.parallelStrings,
        settings.capacityPerStringAh,
        settings.capacityPerStringAh * settings.parallelStrings,
        bms.getSOC());


    // Apply sensor/cell ignore settings to manager
    bms.setPstrings(settings.Pstrings);
    bms.setSensors(settings.IgnoreTemp, settings.IgnoreVolt);
   


    // ====================  WIFI BLOCK ====================
    SERIALCONSOLE.print("SSID: ");
    SERIALCONSOLE.println(ssid);
    SERIALCONSOLE.print("Password length: ");
    SERIALCONSOLE.println(strlen(password));
    SERIALCONSOLE.print("Password bytes (hex): ");
    for (int i = 0; i < (int)strlen(password); i++) {
        SERIALCONSOLE.printf("%02X ", password[i]);
    }
    SERIALCONSOLE.println("\n");

    SERIALCONSOLE.println("Scanning for networks...");
    int n = WiFi.scanNetworks();
    SERIALCONSOLE.printf("%d networks found\n", n);
    for (int i = 0; i < n; i++) {
        if (String(WiFi.SSID(i)) == ssid) SERIALCONSOLE.print(">>> ");
        SERIALCONSOLE.printf("%s  RSSI:%d\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }

    SERIALCONSOLE.printf("\nConnecting to %s...\n", ssid);
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
        delay(500);
        SERIALCONSOLE.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        SERIALCONSOLE.println("\n\nWiFi CONNECTED!");
        SERIALCONSOLE.print("IP address: ");
        SERIALCONSOLE.println(WiFi.localIP());
    } else {
        SERIALCONSOLE.println("\n\nWiFi FAILED!");
        SERIALCONSOLE.printf("Status code: %d\n", WiFi.status());
        while (1) delay(1000);
    }

    mqtt.setServer(mqtt_server, 1883);
    mqtt.connect("TeslaBMS");

    // Publish device status once (required by dbus-mqtt-devices)
    mqtt.publish("device/teslabms/Status", "{\"clientId\":\"teslabms\",\"connected\":1,\"version\":\"1.0\",\"services\":{\"b1\":\"battery\"}}");
}

void loop() {
    console.loop();

    if (!mqtt.connected()) {
        mqtt.connect("TeslaBMS");
    }
    mqtt.loop();

    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();

        // Single master update: balanceCells → getAllVoltTemp → updateSOC → safety.update()
        bms.update();

        uint8_t soc     = (uint8_t)bms.getSOC();
        float   packV   = bms.getPackVoltage();
        float   current = bms.getCurrentAmps();
        uint8_t alarms  = safety.getAlarmBitmask();

        char json[192];

        // Exact JSON format the driver expects, with added alarm and cell extremes
        snprintf(json, sizeof(json),
            "{\"Dc\":{\"Voltage\":%.2f,\"Current\":%.2f,\"Power\":%.2f},\"Soc\":%d,"
            "\"Alarms\":%d,\"LowCell\":%.3f,\"HighCell\":%.3f,\"CellGap\":%.0f}",
            packV, current, packV * current, soc,
            alarms, bms.getLowCellVolt(), bms.getHighCellVolt(),
            (bms.getHighCellVolt() - bms.getLowCellVolt()) * 1000.0f);

        mqtt.publish("teslabms/battery", json);
    }
}