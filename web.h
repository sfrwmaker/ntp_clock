#ifndef _ESP_WM_WEB
#define _ESP_WM_EWB

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

typedef enum e_web_state {
    WEB_OFF = 0, WEB_STARTED, WEB_AP
} t_WEB_state;

class WEB : public ESP8266WebServer {
    public:
        WEB(uint16_t port = 80) : ESP8266WebServer(port) { }
        void        stopServer(void)        { ESP8266WebServer::stop(); status = WEB_OFF; }
        bool        setupAP(void);
        void        setupWEBserver(void);
        t_WEB_state status  = WEB_OFF;
};

#endif
