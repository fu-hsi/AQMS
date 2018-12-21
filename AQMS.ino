/**
 * Air Quality Monitoring Station by Fu-Hsi
 */

// ESP
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>

// File system (Log)
#include <FS.h>

// Sensors
#include <PMS.h>
#include <Adafruit_SHT31.h>
//#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>

// Don't touch this ;-)
#define FU_PROFILE

#ifdef FU_PROFILE
#include "Settings.h"
#else
// Debug (D4)
#define DBG Serial1

// Webservice
static const char* WS_UA = "AQMS/2.0";
static const char* WS_TOKEN = "YOUR TOKEN";
static const char* WS_URL = "YOUR SERVER SCRIPT";

// Wireless credentials
static const char* WL_SSID = "YOUR SSID";
static const char* WL_PASS = "YOUR PASSWORD";
static const uint8_t WL_CHANNEL = 0;

// OTA credentials
static const char* BE_USERNAME = "admin";
static const char* BE_PASSWORD = "admin";

// PMS configuration (INTERVAL & DELAY can't be equal!!!)
static const uint32_t PMS_READ_INTERVAL = 2.5 * 60 * 1000;
static const uint32_t PMS_READ_DELAY = 30000;

// Takes N samples and counts the average
static const uint8_t PMS_READ_SAMPLES = 2;
#endif // FU_PROFILE

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

PMS pms(Serial);
Adafruit_SHT31 sht;
//Adafruit_BMP280 bm;
Adafruit_BME280 bm;

uint32_t timerInterval = PMS_READ_DELAY;

// ESP8266 CheckFlashConfig by Markus Sattler
void flashInfo() {
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();

    DBG.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
    DBG.printf("Flash real size: %u bytes\n\n", realSize);

    DBG.printf("Flash ide  size: %u bytes\n", ideSize);
    DBG.printf("Flash ide speed: %u Hz\n", ESP.getFlashChipSpeed());
    DBG.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

    if (ideSize != realSize) {
        DBG.println("Flash Chip configuration wrong!\n");
    } else {
        DBG.println("Flash Chip configuration ok.\n");
    }
}

void setup(void) {
    DBG.begin(9600);
    Wire.begin(5, 4); // D1, D2
    Serial.begin(PMS::BAUD_RATE);

    DBG.println();
    DBG.println(F("Booting Sketch..."));

    // DBG.setDebugOutput(true);
    // flashInfo();

    if (!sht.begin(0x44)) { // 0x45 for the alternative address
        DBG.println(F("SHT31 error."));
    }

    if (!bm.begin(0x76)) {
        DBG.println(F("BME/P280 error."));
    }

    if (!SPIFFS.begin()) {
        DBG.println(F("Failed to mount file system."));
    }

    pms.wakeUp();
    pms.passiveMode();

    DBG.print(F("Connecting"));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WL_SSID, WL_PASS, WL_CHANNEL);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        DBG.print(".");
    }

    DBG.println();
    DBG.print(F("Connected, IP address: "));
    DBG.println(WiFi.localIP());

    // WiFi.printDiag(DBG);

    httpServer.on("/", []() {
        httpServer.send(200, "text/html; charset=utf-8", F("<!DOCTYPE html>\
<html>\
  <head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  	<meta charset=\"utf-8\">\
    <title>Air Quality Monitoring Station</title>\
  </head>\
<body>\
<h1>Air Quality Monitoring Station</h1>\
<ul>\
<li><a href=\"/update\">Firmware update</a></li>\
<li><a href=\"/status\">Status</a></li>\
<li><a href=\"/log\">Log</a></li>\
<li><a href=\"/clear\">Clear Log</a></li>\
<li><a href=\"/stop\">Server stop</a></li>\
<li><a href=\"/restart\">Restart</a></li>\
</ul>\
</body>\
</html>"));
    });

    httpServer.on("/status", []() {
        httpServer.send(200, "text/html", "Connected to: " + WiFi.SSID());
    });

    httpServer.on("/log", []() {
        if (File fp = SPIFFS.open("/data.txt", "r"))
        {
            httpServer.streamFile(fp, "text/plain");
            fp.close();
        }
        else
        {
            httpServer.send(200, "text/html", "Failed to open file for reading.");
        }
    });

    httpServer.on("/clear", []() {
        SPIFFS.remove("/data.txt");
        httpServer.send(200, "text/html", "Log cleared.");
    });

    httpServer.on("/stop", []() {
        httpServer.send(200, "text/html", "Server stopped.");
        httpServer.stop();
    });

    httpServer.on("/restart", []() {
        httpServer.sendHeader("Refresh", "15; url=/");
        httpServer.send(302, "text/html", "Rebooting...");
        delay(1000);
        ESP.restart();
    });

    httpUpdater.setup(&httpServer, BE_USERNAME, BE_PASSWORD);
    httpServer.begin();
    DBG.printf("HTTPUpdateServer ready! Open http://%s in your browser\n", WiFi.localIP().toString().c_str());
}

void loop(void) {
    static uint32_t timerLast = 0;
    httpServer.handleClient();

    uint32_t timerNow = millis();
    if (timerNow - timerLast >= timerInterval) {
        timerLast = timerNow;
        timerCallback();
        timerInterval = timerInterval == PMS_READ_DELAY ? PMS_READ_INTERVAL : PMS_READ_DELAY;
    }
}

void timerCallback() {
    if (timerInterval == PMS_READ_DELAY)
    {
        DBG.println("Going to sleep");
        collectAndSendData();
    }
    else
    {
        pms.wakeUp();
        DBG.println("Waking up");
    }
}

void collectAndSendData()
{
    static int id = 0;
    
    String postData = "id=";
    postData.concat(id++);
    
    PMS::DATA currData;
    PMS::DATA avgData;

    memset(&currData, 0, sizeof(currData));
    memset(&avgData, 0, sizeof(avgData));

    int rawVoltage = analogRead(A0);

#ifdef FU_PROFILE
    float voltage = rawVoltage * (4.2 / 1024);
#else
    // 180k, max 5.0V
    // float voltage = rawVoltage * (5.0 / 1024);

    // 100k, max 4.2V
    float voltage = rawVoltage * (4.2 / 1024);
#endif // FU_PROFILE

    postData.concat("&rv=");
    postData.concat(rawVoltage);

    postData.concat("&v=");
    postData.concat(voltage);

    byte samplesTaken = 0;
    for (byte sampleIdx = 0; sampleIdx < PMS_READ_SAMPLES; sampleIdx++)
    {
        pms.requestRead();
        if (pms.readUntil(currData, 2000))
        {
            samplesTaken++;

            avgData.PM_AE_UG_1_0 += currData.PM_AE_UG_1_0;
            avgData.PM_AE_UG_2_5 += currData.PM_AE_UG_2_5;
            avgData.PM_AE_UG_10_0 += currData.PM_AE_UG_10_0;
        }
        delay(1000);
    }

    if (samplesTaken > 0)
    {
        avgData.PM_AE_UG_1_0 /= samplesTaken;
        avgData.PM_AE_UG_2_5 /= samplesTaken;
        avgData.PM_AE_UG_10_0 /= samplesTaken;

        postData.concat("&pm1=");
        postData.concat(avgData.PM_AE_UG_1_0);
        postData.concat("&pm25=");
        postData.concat(avgData.PM_AE_UG_2_5);
        postData.concat("&pm10=");
        postData.concat(avgData.PM_AE_UG_10_0);
    }

    pms.sleep();

    float temperature = sht.readTemperature();
    if (!isnan(temperature))
    {
        postData.concat("&temperature=");
        postData.concat(temperature);
    }

    float humidity = sht.readHumidity();
    if (!isnan(humidity))
    {
        postData.concat("&humidity=");
        postData.concat(humidity);
    }

    float pressure = bm.readPressure();
    if (!isnan(pressure))
    {
        postData.concat("&pressure=");
        postData.concat(pressure / 100.0);
    }

    float bm_temp = bm.readTemperature();
    if (!isnan(bm_temp))
    {
        postData.concat("&bm280_temp=");
        postData.concat(bm_temp);
    }

    float bm_hum = bm.readHumidity();
    if (!isnan(bm_hum))
    {
        postData.concat("&bm280_hum=");
        postData.concat(bm_hum);
    }

    if (postData.length())
    {
        saveToFile(postData);
        if (WiFi.isConnected()) {
            HTTPClient http;

            http.begin(WS_URL);
            http.setUserAgent(WS_UA);
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            http.addHeader("X-TOKEN", WS_TOKEN);

            int httpCode = http.POST(postData);
            http.end();
        }
        else
        {
            DBG.println("Error in WiFi connection");
        }
    }
}

bool saveToFile(const String& data)
{
    DBG.println(data);
    if (File fp = SPIFFS.open("/data.txt", "a"))
    {
        fp.println(data);
        fp.close();
        return true;
    }
    else
    {
        DBG.println("Failed to open file for writing.");
        return false;
    }
}
