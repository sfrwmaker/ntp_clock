/*
 * jsonrpc.cpp
 *
 *  Created on: 23 apr 2022
 *      Author: Alex
 */

#include "Arduino.h"
#include "jsonrpc.h"
#include "modem.h"
#include "ntp.h"
#include "web.h"
#include "press.h"
#include "dspl.h"

extern  DSPL        dspl;
extern  ntpClock    ntp;
extern  WEB         server;
extern  MODEM       modem;
extern  PRESS       pSensor;
extern  bool        gps_exists;

void writeAnswer(String ret) {
	Serial.print("{ ");
	Serial.print(ret.c_str());
	Serial.print(" }\n");
}

//--------------------------------------------------- Serial port command parser ------------------------------
void SERIAL_PARSER::startDocument() {
	while (!s_key.empty() ) {
		s_key.pop();
	}
	d_key.clear();
}

void SERIAL_PARSER::startObject() {
	s_key.push(d_key);
}

void SERIAL_PARSER::endObject() {
    String p_key = "";
    if (!s_key.empty()) {
        p_key = s_key.top();
    }
    if (p_key.equals("tz") && tz_type != TZ_NONE) {  // Time Zone setup done
        ntp.setDst(tz_type == TZ_WINTER, dst_month, dst_day, dst_w_day, dst_w_num, dst_when, dst_shift);
        tz_type     = TZ_NONE;
        dst_month   = 0;
        dst_day     = 0;
        dst_w_day   = 0;
        dst_w_num   = 0;
        dst_when    = 0;
        dst_shift   = 0;
    } else if (p_key.equals("brightness")) {
        dspl.setLimits(br_min, br_max, 0, 15, 7);
    }
	s_key.pop();
	d_key.clear();
}

void SERIAL_PARSER::startArray() {
	s_array.push(d_key);
}

void SERIAL_PARSER::endArray() {
	s_array.pop();
	d_key.clear();
}

bool SERIAL_PARSER::parse(const char c) {
	if (!is_body && (c == '{' || c == '[')) {
		is_body = true;
	    parser.reset();
	}
	if (is_body) {
		parser.parse(c);
		return !is_body;									// The command has been finished!
	}
	return false;
}

//--------------------------------------------------- Command parser ------------------------------------------
void JSON_RPC::value(String value) {
	String p_key = "";
	if (!s_key.empty()) {
		p_key = s_key.top();
	}
	if (d_key.equals("hostname")) {
		modem.setName(value);
	} else if (d_key.equals("ssid")) {
		modem.setSSID(value);
	} else if (d_key.equals("password")) {
		modem.setPasswd(value);
	} else if (d_key.equals("alt")) {
        pSensor.setAltitude(value.toInt());
    } else if (d_key.equals("gps")) {
        gps_exists = value.equals("check");
    } else if (d_key.equals("ntp_server")) {
		ntp.srvSet(value);
	} else if (p_key.equals("brightness")) {
         if (d_key.equals("min")) {
            br_min = value.toInt();
         } else if (d_key.equals("max")) {
            br_max = value.toInt();
         }
	} else if (p_key.equals("tz")) {
        if (d_key.equals("period")) {
            if (value.equals("summer")) {
                tz_type = TZ_SUMMER;
            } else if (value.equals("winter")) {
                tz_type = TZ_WINTER;
            } else {
                tz_type = TZ_NONE;
            }
        } else if (tz_type != TZ_NONE && d_key.equals("month")) {
            dst_month = value.toInt();
        } else if (tz_type != TZ_NONE && d_key.equals("day")) {
            dst_day = value.toInt();
        } else if (tz_type != TZ_NONE && d_key.equals("week_num")) {
            dst_w_num = value.toInt();
        } else if (tz_type != TZ_NONE && d_key.equals("week_day")) {
            dst_w_day = value.toInt();
        } else if (tz_type != TZ_NONE && d_key.equals("loc_minute")) {
            dst_when = value.toInt();
        } else if (tz_type != TZ_NONE && d_key.equals("shift_m")) {
            dst_shift = value.toInt();
        }
	}
}

void JSON_RPC::readConfig(String& name) {
    File f = LittleFS.open(name.c_str(), "r");
    if (f) {
        while (f.available()) {
            char c = f.read();
            parse(c);
        }
        f.close();
    }
}
