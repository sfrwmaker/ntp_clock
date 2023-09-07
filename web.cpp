#include <Time.h>
#include <TimeLib.h>
#include <LittleFS.h>
#include "web.h"
#include "ntp.h"
#include "modem.h"
#include "press.h"
#include "dspl.h"

extern  DSPL        dspl;
extern  ntpClock    ntp;
extern  WEB         server;
extern  MODEM       modem;
extern  PRESS       pSensor;
extern  String      main_cfg;
extern  bool        gps_exists;

static String dst_name[2]   = {"Summer rule:", "Winter rule:"};
static String months[13]    = {",-", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
static String wdays[8]      = {"-", "Sunday", "Monday", "Tuesday", "Wednesday", "Thurshday", "Friday", "Saturday"};
static String wnums[8]      = {"-", "1", "2", "3", "4", "5", "b. last", "last"};
static String unit_list[3]   = {"metric", "standard", "imperial"};

static String   selectMenu(const String& menu_id, const String& menu_label, int8_t first, int8_t last, String list_items[], int8_t active, bool add_zero);
static String   selectMenu(const String& menu_id, const String& menu_label, int8_t first, int8_t last, uint8_t multiply, int8_t active, bool add_zero);
static uint16_t readTime(String hhmm);

// WEB handlers
static void handleRoot(void);
static void handleNotFound(void);

// web page styles
const char style[] PROGMEM = R"=====(
<style>
body  {background-color:powderblue;}
input {background-color:powderblue;}
select {background-color:powderblue;}
t1    {color: black; align-text: center; font-size: 200%;}
s1    {color: black; align-text: right; font-size: 70%;}
r1    {color: black; align-text: right; font-size: 150%; margin-right:10px}
h1    {color: blue;  padding: 5px 0; align-text: center; font-size: 150%; font-weight: bold;}
h2    {color: black; padding: 0 0; align-text: center; font-size: 120%; font-weight: bold;}
table { border: 0px; border-collapse: collapse;}
th    {vertical-align: middle; text-align: left; font-weight: normal;;}
td    {vertical-align: middle; text-align: center;}
ul    {list-style-type: none; margin: 0; padding: 0; overflow: hidden; background-color: #333;}
li    {float: left; border-right:1px solid #bbb;}
li:last-child {border-right: none;}
li a  {display: block; color: white; text-align: center; padding: 14px 16px; text-decoration: none;}
li a:hover:not(.active) {background-color: #111;}
.active {background-color: #6aa7e3;}
form {padding: 20px;}
form .field {padding: 4px; margin: 1px;}
form .field label {display: inline-block; width:240px; margin-left:5px; text-align: left;}
form .field input {display: inline-block; size=20;}
form .field select {display: inline-block; size=20;}
.myframe {width:1100px; -moz-border-radius:5px; border-radius:5px; -webkit-border-radius:5px;}
.myheader {margin-left:10px; margin-top:10px;}
</style>
</head>
)=====";

//------------------------------------------ WEB server --------------------------------------------------------
bool WEB::setupAP(void) {
    const char *ssid = "esp8266";
    bool stat = WiFi.softAP(ssid);                          // use default IP address, 192.168.4.1
    if (stat) {
        ESP8266WebServer::on("/",       handleRoot);
        ESP8266WebServer::onNotFound(handleNotFound);
        ESP8266WebServer::begin();
        status = WEB_AP;
    }
    return stat;
}

void WEB::setupWEBserver(void) {
      ESP8266WebServer::on("/",         handleRoot);
      ESP8266WebServer::onNotFound(handleNotFound);
      ESP8266WebServer::begin();
      status = WEB_STARTED;
}

//==============================================================================================================
String main_menu[2][2] = {
      {"Weather", "/"},
      {"Setup", "/setup"}
};

void header(const String title, bool refresh = false) {
      String hdr = "<!DOCTYPE HTML>\n<html><head>\n";
      if (refresh) hdr += "<meta http-equiv='refresh' content='40'/>";
      hdr += "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n<title>";
      hdr += title;
      hdr += "</title>\n";
      server.sendContent(hdr);
      server.sendContent_P(style);
}

String mainMenu(uint8_t active) {
    String menu = "<ul>";
    for (uint8_t i = 0; i < 2; ++i) {                       // Main menu
        menu += "<li><a ";
        if (i == active)                                    // This is active menu item
            menu += "class='active' ";
        menu += "href='" + main_menu[i][1] + "'>" + main_menu[i][0] + "</a></li>\n";
    }
    menu += "<li style='float:right'><a href='/'>";
    menu += ntp.ntpStr();
    menu += "</a></li>\n</ul>\n";
    return menu;
}

//------------------------------------------ URL handlers ------------------------------------------------------

static void handleRoot(void) {
    if (server.args() > 0) {                                // Setup new NTP and WiFi values
        String hn       = server.arg("myname");
        String ssid     = server.arg("ssid");
        String pass     = server.arg("passwd");
        String tmsn     = server.arg("ntp_server");
        String alt      = server.arg("alt");
        String gps      = server.arg("gps");
        String br_min   = server.arg("br_min");
        String br_max   = server.arg("br_max");
        String tz_month = server.arg("s_month");
        String tz_day   = server.arg("s_day");
        String tz_wnum  = server.arg("s_wnum");
        String tz_wday  = server.arg("s_wday");
        String tz_hour  = server.arg("s_hour");
        String tz_shift = server.arg("s_shift");
        String tz_shiftm = server.arg("s_shiftm");
        modem.setName(hn);
        modem.setSSID(ssid);
        modem.setPasswd(pass);
        ntp.srvSet(tmsn);
        pSensor.setAltitude(alt.toInt());
        dspl.setLimits(br_min.toInt(), br_max.toInt(), 0, 15, 7);
        uint16_t loc_minute = readTime(tz_hour);
        int16_t shift = tz_shift.toInt() * 60;
        if (shift >= 0) shift += tz_shiftm.toInt(); else shift -= tz_shiftm.toInt();
        ntp.setDst(false, tz_month.toInt(), tz_day.toInt(), tz_wday.toInt(), tz_wnum.toInt(), loc_minute, shift);
        File f = LittleFS.open(main_cfg.c_str(), "w");
        if (f) {
            // {"hostname": "<hostname>", "ssid": "<ssid>", "password": "<password>", "ntp_server": "<ntp server>",
            // "alt": "<altitude>", "gps": "<check/no>",
            f.print("{\"hostname\": \"");
            f.print(hn);
            f.print("\", \"ssid\": \"");
            f.print(ssid);
            f.print("\", \"password\": \"");
            f.print(pass);
            f.print("\",\n\"ntp_server\": \"");
            f.print(tmsn);
            f.print("\", \"alt\": \"");
            f.print(alt);
            f.print("\", \"gps\": \"");
            if (gps.equals("check")) {
                f.print(gps);
            } else {
                f.print("no");
            }
            f.print("\",\n");
            // "brightness": {"min": "<min_brightness>", "max": "<max_brightness>"},
            f.print("\"brightness\": {\"min\": \"");
            f.print(br_min);
            f.print("\", \"max\": \"");
            f.print(br_max);
            f.print("\"},\n");
            //      "tz": {"period": "summer", "month": "<month>", "day": "<day>", "week_num": "<week num>", "week_day":, "<week day>",
            //      "loc_minute": "<minute>", "shift_m": "<shift minute>"}
            f.print("\"tz\": {\"period\": \"summer\", \"month\": \"");
            f.print(tz_month);
            f.print("\", \"day\": \"");
            f.print(tz_day);
            f.print("\", \"week_num\": \"");
            f.print(tz_wnum);
            f.print("\", \"week_day\": \"");
            f.print(tz_wday);
            f.print("\", \"loc_minute\": \"");
            f.print(String(loc_minute));
            f.print("\", \"shift_m\": \"");
            f.print(String(shift));
            f.print("\"},\n");
            tz_month = server.arg("w_month");
            tz_day   = server.arg("w_day");
            tz_wnum  = server.arg("w_wnum");
            tz_wday  = server.arg("w_wday");
            tz_hour  = server.arg("w_hour");
            tz_shift = server.arg("w_shift");
            tz_shiftm = server.arg("w_shiftm");
            //      "tz": {"period": "winter", "month": "<month>", "day": "<day>", "week_num": "<week num>", "week_day":, "<week day>",
            //      "loc_minute": "<minute>", "shift_m": "<shift minute>"}}
            f.print("\"tz\": {\"period\": \"winter\", \"month\": \"");
            f.print(tz_month);
            f.print("\", \"day\": \"");
            f.print(tz_day);
            f.print("\", \"week_num\": \"");
            f.print(tz_wnum);
            f.print("\", \"week_day\": \"");
            f.print(tz_wday);
            f.print("\", \"loc_minute\": \"");
            loc_minute = readTime(tz_hour);
            f.print(String(loc_minute));
            f.print("\", \"shift_m\": \"");
            shift = tz_shift.toInt() * 60;
            if (shift >= 0) shift += tz_shiftm.toInt(); else shift -= tz_shiftm.toInt();
            ntp.setDst(true, tz_month.toInt(), tz_day.toInt(), tz_wday.toInt(), tz_wnum.toInt(), loc_minute, shift);
            f.print(String(shift));
            f.print("\"}\n}");
            f.close();
        }
    }
    
    header("setup");
    String body = "<body>";
    // Network setup frame
    body += "<div align='center'><t1>Setup</t1></div><br>\n";
    body += "<form action='/'>\n";
    body += "<div align='center'><fieldset class='myframe'>\n";
    body += "<legend>Network setup</legend>\n";
    body += "<div class='field'><label for='myname'>My hostname:</label>";
    body += "<input type='text' name='myname' pattern='[a-z.-]+' value='";
    body += modem.getName();
    body += "'></div>\n<div class='field'><label for='ssid'>Wifi SSID:</label>";
    body += "<input type='text' name='ssid' value='";
    body += modem.getSSID();
    body += "'></div>\n<div class='field'><label for='passwd'>Wifi Password:</label>";
    body += "<input type='text' name='passwd' value='";
    body += modem.getPasswd();
    body += "'></div>\n<div class='field'><label for='ntp_server'>NTP Server Name:</label>";
    body += "<input type='text' name='ntp_server' pattern='[a-z0-9.-]+' value='";
    body += ntp.ntpServerName();
    body += "'></div>\n<div class='field'><label for='alt'>Altitude:</label>";
    body += "<input type='text' name='alt' pattern='[0-9]+' value='";
    body += pSensor.getAltitudeS();
    body += "'></div>\n<div class='field'>";
    body += "<input type='checkbox' name='gps' value='check'";
    if (gps_exists) body += " checked";
    body += "><label for='gps'>Check GPS receiver</label></div></fieldset></div><br>\n";

    body += "<div align='center'><fieldset class='myframe'>\n<legend>Display Brightness</legend>";
    body += "<div class='slidecontainer'>";
    body += "<label for='br_min'>Minimal Brightness:</label>\n";
    body += "<input type='range' min='0' max='200' class='slider' step='1' name='br_min' value='";
    body += String(dspl.lightMin());
    body += "' list='minlabel'><datalist id='minlabel'>\n";
    body += "<option>40</option><option>80</option><option>120</option><option>160</option></datalist>\n</div>";
    body += "<div class='slidecontainer'>";
    body += "<label for='br_max'>Maximal Brightness:</label>\n";
    body += "<input type='range' min='250' max='1000' class='slider' step='10' name='br_max' value='";
    body += String(dspl.lightMax());
    body += "' list='maxlabel'><datalist id='maxlabel'>\n";
    body += "<option>300</option><option>400</option><option>500</option><option>600</option>";
    body += "<option>700</option><option>800</option><option>900</option></datalist></div></fieldset></div><br>\n";
    
    body += "<div align='center'><fieldset class='myframe'>\n<legend>Time zone rules</legend>\n<table><tr>";
    for (uint8_t rule = 0; rule < 2; ++rule) {
        body += "<th><div align='right'><r1>";
        body += dst_name[rule];
        body += "</r1></div></th>\n<th>";
        String id = (rule == 0)?"s_month":"w_month";        // MONTH
        uint8_t start_month = rule * 6 + 2;
        body += selectMenu(id, String("month:"), start_month, start_month + 3, months, ntp.dstMonth(rule == 1), true);
        id = (rule == 0)?"s_day":"w_day";                   // DAY
        body += selectMenu(id, String("day:"), 1, 31, 1, ntp.dstDay(rule == 1), true);
        body += "</th>\n<th>";
        id = (rule == 0)?"s_wnum":"w_wnum";                 // WEEK DAY (week number)
        body += selectMenu(id, String("week day:"), 1, 7, wnums, ntp.dstWNum(rule == 1), true);
        body += "\n";
        id = (rule == 0)?"s_wday":"w_wday";                 // WEEK DAY
        body += selectMenu(id, String(""), 1, 7, wdays, ntp.dstWDay(rule == 1), true);
        body += "</th>\n<th><label for='";
        id = (rule == 0)?"s_hour":"w_hour";
        body += id;                                         // HOUR
        body += "'>hour:</label>\n";
        body += "<input type='time' id='";
        body += id;
        body += "' name='";
        body += id;
        body += "' value='";
        char b[10];
        uint16_t when_minutes = ntp.dstWhen(rule == 1);
        sprintf(b, "%02d:%02d", when_minutes / 60, when_minutes % 60);
        body += String(b); body += "'></th>\n<th>";
        id = (rule == 0)?"s_shift":"w_shift";               // TIME SHIFT (hours)
        int16_t shift_minutes = ntp.dstShift(rule == 1);
        body += selectMenu(id, String("time shift:"), -12, 12, 1, shift_minutes / 60, false);
        body += " h\n";
        id = (rule == 0)?"s_shiftm":"w_shiftm";             // TIME SHIFT (minutes)
        body += selectMenu(id, String(""), 0, 4, 15, shift_minutes % 60, false);
        body += " m</th></tr>";
    }
    body += "</table>\n</fieldset></div><br>\n<div align='center'><input type='submit' value='Apply'></div>\n";
    body += "</form>\n</body>\n</html>\n";
    server.sendContent(body);
}

static void handleNotFound(void) {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); ++i) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

//==============================================================================================================
static String selectMenu(const String& menu_id, const String& menu_label, int8_t first, int8_t last, String list_items[], int8_t active, bool add_zero) {
    String body = "";
    if (first > last) return body;
    if (menu_label.length() > 0) {
        body += "<label for='"; body += menu_id; body += "'>"; body += menu_label; body += "</label>\n";
    }
    body += "<select name='"; body += menu_id; body += "' id='"; body += menu_id; body += "'>\n";
    if (add_zero) {
        body += "<option ";
        if (0 == active) body += "selected ";
        body += "value='0'>-</option>\n";
    }
    for (int8_t i = first; i <= last; ++i) {
        body += "<option ";
        if (i == active) body += "selected ";
        body += "value='"; body += String(i); body += "'>"; body += list_items[i]; body += "</option>\n";
    }
    body += "</select>";
    return body;
}

static String selectMenu(const String& menu_id, const String& menu_label, int8_t first, int8_t last, uint8_t multiply, int8_t active, bool add_zero) {
    String body = "";
    if (first > last) return body;
    if (menu_label.length() > 0) {
        body += "<label for='"; body += menu_id; body += "'>"; body += menu_label; body += "</label>\n";
    }
    body += "<select name='"; body += menu_id; body += "' id='"; body += menu_id; body += "'>\n";
    if (add_zero) {
        body += "<option ";
        if (0 == active) body += "selected ";
        body += "value='0'>-</option>\n";
    }
    for (int8_t i = first; i <= last; ++i) {
        body += "<option ";
        if (i == active) body += "selected ";
        body += "value='"; body += String(i); body += "'>"; body += String(i*multiply); body += "</option>\n";
    }
    body += "</select>";
    return body;
}

static uint16_t readTime(String hhmm) {
    uint16_t loc_minute = hhmm.charAt(0) - (int)'0';
    loc_minute *= 10;
    loc_minute += hhmm.charAt(1) - (int)'0';
    loc_minute *= 60;
    loc_minute += (hhmm.charAt(3) - (int)'0') * 10;
    loc_minute += hhmm.charAt(4) - (int)'0';
    if (loc_minute >= 24*60) loc_minute = 24*60-1;
    return loc_minute;
}

time_t strDate(String tm) {
    tmElements_t tms;
    uint16_t    ye;
    uint8_t     stat = 0;
    char c;
    for (uint8_t i = 0; i < tm.length(); ++i) {
        while (i < tm.length()) {                           // Skip preceded invalid symbols
            c = tm.charAt(i);
            if ((c >= '0') || (c <= '9')) break;
            ++i;
        }
        uint8_t j = 0;
        while (i+j < tm.length()) {
            c = tm.charAt(i+j);
            if ((c < '0') || (c > '9')) break;
            ++j;
        }
        switch (stat) {
            case 0:                                         // Day
                if (c == '-') {
                    tms.Day = tm.substring(i, i+j).toInt();
                    if (tms.Day > 31) tms.Day = 15;
                    ++stat;
                }
                break;
            case 1:                                         // Month
                if (c == '-') {
                  tms.Month = tm.substring(i, i+j).toInt();
                  if (tms.Month > 12) tms.Month = 12;
                  ++stat;
                }
                break;
            case 2:                                         // Year
                ye = tm.substring(i, i+j).toInt();
                if (ye > 1970) ye -= 1970;
                if (ye < 30) ye = 30;
                tms.Year = ye;
                ++stat;
                break;
            default:                                        // Invalid status
                break;
        }
        i += j;
    }
    tms.Second = 0;
    tms.Minute = 0;
    tms.Hour   = 12;
    time_t t = makeTime(tms);
    return t;
}
