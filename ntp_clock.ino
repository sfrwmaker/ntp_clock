#include <TinyGPS++.h>
#include <TinyGPSPlus.h>
#include <Time.h>
#include <TimeLib.h>
#include "dspl.h"
#include "modem.h"
#include "jsonrpc.h"
#include "ntp.h"
#include "web.h"
#include "press.h"
#include "hb.h"

/*
 * Arduino IDE setup:
 * Board: Generic ESP8266 Module
 * Upload Speed: 115200
 * CPU Frequency: 80 MHz
 * Crystal Frequency: 26 MHz (Or Serial port speed would be incorrent)
 * Flash Size: 4M (FS:3MB OTA:~512KB)
 * Flash Mode: DIO
 * Flash Frequency: 40 MHz
 * Reset Method: dtr (aka nodemcu)
 * Debug port: Disabled
 * Debug Level: Nothing
 * IwIP Variant: v2 Lower memory
 * VTables: Flash
 * Exceptions: Disabled
 * Stack Protection: Disabled
 * Erase Flash: Only Sketch
 * NONOS SDK version nonos-sdk 2.2.1+100
 * SSL Support: All ciphers (most compatible)
 * MMU: 32KB cache + 32 IRAM (balanced)
 * Non-32 Bit Access: Use pgm_read macros for IRAM/PROGMEM
 * Built Led: 2 
 */

 #define FW_VERSION   ("1.01")

// max7219 interface. Esp8266 pin numbers gpioXX
const uint8_t M_CS          = 16;                   // Max7219 chip select pin
const uint8_t HB_PIN        = 12;                   // External NE555 timer reset pin
const uint8_t LED_PIN       = 2;                    // Built-in led

DSPL            dspl(M_CS);
MODEM           modem;
WEB             server;
JSON_RPC        rpc;
ntpClock        ntp;
PRESS           pSensor;
HB              hb(HB_PIN);
TinyGPSPlus     gps;
static uint32_t emp_data;
static bool     pressure_sensor = false;            // The pressure sensor BME280 initialized successful flag
static time_t   web_off_tm      = 0;                // Time to switch-off the web server
static bool     gps_ok          = false;            // GPS module synched to Satelite sucesfully
static bool     just_started    = true;
bool            gps_exists      = true;             // GPS module exists in the clock

String      main_cfg    = "/cfg.json";

static int16_t empAverage(uint8_t emp_k, int32_t v) {
    uint8_t round_v = emp_k >> 1;
    emp_data += v - (emp_data + round_v) / emp_k;
    return (emp_data + round_v) / emp_k;
}

uint32_t SeaPressure(float raw_pressure, int16_t altitude) {
    float pSeaCoeff = 1.0;
    if (altitude > 1) {
        pSeaCoeff = 1.0 + 0.000125 * (float)altitude;
    }
    float p = raw_pressure * pSeaCoeff;
    return round(p);
}

static void show(void) {
    static const uint32_t  one_year = 31536000;
    static uint32_t mode_change     = 0;
    static uint32_t last_mode       = 0;
    static uint8_t  mode            = 0;
    static bool     show_time       = false;
    static uint8_t  dot             = 0;
     
    uint32_t now_ms = millis();
    if ((now_ms - last_mode) >= mode_change) {              // Change the display mode
        last_mode = now_ms;
        if (pressure_sensor) {
            ++mode;
            if (mode > 4) mode = 0;                         // 1 means no temperature or pressure
            mode_change = 2000;
        } else {
            mode = 0;
        }
        switch (mode) {
            case 1:                                         // Show the month and the day
                if (now() > one_year) {
                    dspl.showTime(day(), month(), 4);
                    show_time = false;
                } else {
                    mode_change = 0;                        // Skip this mode
                }
                break;
            case 2:                                         // Show ambient temperature
                {
                    int16_t amb_temp = pSensor.temperature();
                    dspl.showTemperature(amb_temp);
                    show_time = false;
                }
                break;
            case 3:                                         // Show Pressure
                {
                    uint32_t pressure = pSensor.seaPressure(); // Pa
                    pressure = pSensor.mmHg(pressure);      // mmHg
                    dspl.showPressure(pressure);
                    show_time = false;
                }
                break;
            case 4:                                         // Show period to resync NTP
                if (!gps_exists && millis() > 300000 && !ntp.timeSynced()) {
                    uint16_t time_to_sync = modem.timeToSync();
                    dspl.showSyncRemains(time_to_sync);
                    show_time = false;
                } else {
                    mode_change = 0;                        // Skip this mode
                }
                break;
            case 0:                                         // Show the time
                show_time = true;
                mode_change = random(40, 60) * 1000;
                break;
            default:
                break;
        }
    }
    if (show_time) {
        time_t n = now(); n += ntp.TZ(n);                   // Local time
        uint8_t dot_mask = 0b1111;
        if (gps_ok || ntp.timeSynced()) {
            dot_mask    = 0b100;
        }
        tmElements_t tm;
        breakTime(n, tm);
        dspl.showTime(tm.Hour, tm.Minute, dot);
        dot ^= dot_mask;
        if ((dot & 0b100) == 0) dot = 0;
    }
}

static void showAP(void) {
    static uint8_t msg[] = "   ESP8266 192.168.4.1  ";
    static uint8_t msg_indx = 0;                            // Current index of the message string

    uint8_t symbols[4];
    uint8_t in = msg_indx;
    for (uint8_t i = 0; i < 4; ++i) {
        symbols[i] = msg[in++];
        if (msg[in] == '.') {
            symbols[i] |= 0x80;                             // This bit indicating that dot should be displayed
            ++in;
        }
    }
    if (in > sizeof(msg)) {
        msg_indx = 0;
    } else {
        ++msg_indx;
        if (msg[msg_indx] == '.')
            ++msg_indx;
    }
    dspl.show4Symbols(symbols);
}

time_t timeGPS(void) {
    time_t gmt = 0;
    tmElements_t tm;
    if (gps.time.isValid() && gps.date.isValid()) {         // Use GPS if the data is available
        tm.Day      = gps.date.day();
        tm.Month    = gps.date.month();
        tm.Year     = CalendarYrToTm(gps.date.year());
        tm.Hour     = gps.time.hour();
        tm.Minute   = gps.time.minute();
        tm.Second   = gps.time.second();
        gmt         = makeTime(tm);
        gps_ok      = true;
        if (just_started) {
            web_off_tm  = gmt;
            web_off_tm += (just_started)?30*60:60;
        }
    }
    return gmt;
}

void ts(time_t n) {
    tmElements_t tm;
    breakTime(n, tm);
    char ts[40];
    sprintf(ts, "%02d-%02d-%4d %02d:%02d:%02d", tm.Day, tm.Month, tm.Year + 1970, tm.Hour, tm.Minute, tm.Second);
    Serial.print(ts);
}

void log(const char *msg) {
    time_t n = now(); n += ntp.TZ(n);
    ts(n); Serial.print(": "); Serial.println(msg);
}

// Workflow:
// Init hardware components
// Load config from LittleFS, perhaps, clear the gps_exists flag depending on config
// If the GPS module is not installed in the board, you can disable gps module checking at startup
void setup() {
    hb.setPeriod(4000);                                     // reset NE555 timer every 4 seconds
    pinMode(A0, INPUT);
    uint16_t bright = analogRead(A0);
    bright = empAverage(10, bright);
    randomSeed(bright);
    Serial.begin(74880);
    Serial.begin(9600, SERIAL_8N1, SERIAL_FULL, 1, false);
    Serial.println("\n\n\n");
    Serial.print("Clock v."); Serial.print(FW_VERSION); Serial.print(", "); Serial.println(__DATE__);
    if (!LittleFS.begin()) {
        Serial.println("format LittleFS");
        LittleFS.format();
    }
    hb.sendHeartbeat();
    dspl.init();
    dspl.setLimits(10, 800, 0, 15, 7);                      // Will be setup by config
    rpc.init();
    ntp.init("");
    rpc.readConfig(main_cfg);                               // Load parameters, initialize all instances
    pressure_sensor = pSensor.init();

    if (gps_exists) {                                       // gps_exists flag can be changed by config, see jsonrpc.cpp
        setSyncProvider(timeGPS);
        setSyncInterval(60);
        server.setupAP();                                   // Start access point to configure settings
        web_off_tm = now() + 30*60;                         // Run the web server for 30 minutes
    } else {
        Serial.println("no GPS");
    }
}

// Workflow:
// 1. If gps_exists flag is active check the GPS module input stream on the hardware serial for first minute
//    If there are symbols from the serial, assume the GPS module present on the board, use it to sync time
//    When time synched with GPS module, set gps_ok flag; Stop count the bytes received on the serial
//    If no symbols from the serial in first minute, assume the GPS module does not exist on the board, set gps_exists flag to false
// 2. If the GPS module is not detected, use NTP server to sync time
//    If the time is not synced for a long time (sync period is 6 hours) start synching procedure:
//      2a. Sync procedure
//          Try to connect to the Wifi AP, if succeded, send the NTP request
//          Start WEB server allowing changing configuration data for 30 minute when the clock just started
//          If cannot sync time with NTP server or connect to the Wifi AP, start own Wifi AP for configuration
void loop() {
    static uint32_t next_loop       = 0;
    static uint32_t gps_symbols     = 0;
    static bool     led_status      = false;

    bool lf = false;
    hb.loop();                                              // Periodically reset NE555
    uint32_t n = millis();
    if (n > next_loop) {
        next_loop = n + 1000;
        if (!gps_exists && !ntp.timeSynced() && modem.isReadyToConnect()) { // It is time to sync local clock
            if (modem.status == W_UNCONFIG) {               // Failed to connect to wifi
                if (server.status == WEB_OFF) {
                    server.setupAP();                       // Start access point to configure settings
                    log("AP started");
                } else if (server.status != WEB_AP) {
                    server.stopServer();
                    log("WEB stopped");
                }
            } else if (modem.status == W_OFF) {             // Ready to connect to access point
                if (server.status == WEB_AP) {
                    server.stopServer();
                    log("WEB stopped");
                    web_off_tm  = 0;
                } else if (server.status == WEB_STARTED) {
                    server.stopServer();
                }
                modem.connect();
                log("connecting");
            } else if (modem.isConnected()) {               // Already connected to the access point
                if (web_off_tm == 0) {                      // Make sure the modem will be switched-off
                    if (just_started) {
                        server.setupWEBserver();
                        log("WEB started");
                    } else {
                        log("Connected");
                    }
                }
                if (ntp.syncTime()) {                       // Reset web server timeout
                    log("NTP synched");
                    Serial.print("Next sync ");
                    ts(ntp.nextSync()); Serial.println("");
                }
                // syncTime() changed the clock in any case, calculate the time to switch modem off
                web_off_tm = now();
                web_off_tm += (just_started)?30*60:70;
            }
        }
        if (web_off_tm > 0 && now() > web_off_tm) {
            just_started = false;
            if (server.status == WEB_STARTED) {
                server.stopServer();
                digitalWrite(LED_PIN, LOW);
                if (!gps_exists) log("stop server");
            }
            if (modem.status == W_READY) {
                modem.disconnect();
                if (!gps_exists) log("stop modem");
            }
        }
        if (!gps_exists && server.status == WEB_AP) {
            showAP();
        } else {
            show();
        }
        if (server.status != WEB_OFF) {                     // Blink by built-in LED
            led_status = !led_status;
            digitalWrite(LED_PIN, led_status);
        }
    }

    if (gps_exists) {
        if (!gps_ok && millis() > 60000) {         
            if (gps_symbols < 50) {                         // No GPS module working!
                gps_exists = false;
                setSyncProvider(0);                         // Delete GPS time provider
            }
        }
        if (Serial.available() > 0) {
            uint8_t c = Serial.read();
            if (!gps_ok) ++gps_symbols;
            gps.encode(c);
        } else {
            delay(50);
        }
    } else {
        delay(50);
    }
    uint16_t bright = analogRead(A0);
    bright = empAverage(10, bright);
    dspl.adjustBright(bright);
    yield();
    if (server.status != WEB_OFF)
        server.handleClient();
    modem.loop();
}
