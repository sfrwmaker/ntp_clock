/*
 * modem.cpp
 *
 *  Created on: 14 jul 2023
 *      Author: Alex
 */

#include "modem.h"
#include "ntp.h"
#include "web.h"

extern ntpClock	ntp;

bool MODEM::isReadyToConnect(void) {
    if (retry_tm == 0) return true;                                 // Retry time not setup, ready to connect to AP
    if (now() + max_retry_to < retry_tm) {                          // Make sure the time of the next attempt was setup correctly
        retry_tm = now() + max_retry_to;
    }
    return (now() >= retry_tm);
}

bool MODEM::isConnected(void) {
    if (status != W_READY) return false;
    if (WiFi.status() == WL_CONNECTED) return true;
    disconnect();
    return false;
}

uint16_t MODEM::timeToSync(void) {
    time_t n = now();
    if (retry_tm < n) return 0;
    return retry_tm - n;
}

void MODEM::connect(void) {
	status = W_UNCONFIG;
	if ((hostname.length() > 0) && (ssid.length() > 0) && (password.length() > 0)) {
        WiFi.disconnect();
		WiFi.hostname(hostname.c_str());
		WiFi.begin((char *)ssid.c_str(), (char *)password.c_str());
		status      = W_TRY;
        start_ms    = millis();
	}
}
void MODEM::disconnect(void) {
	WiFi.disconnect(true);
	status = W_OFF;
}

void MODEM::loop(void) {
    if (status == W_TRY) {
	    if (WiFi.status() == WL_CONNECTED) {
	    	MDNS.begin(hostname.c_str());
	    	status = W_READY;
            retry_count = max_retries;                              // The AP is accesible, set the retry count to its maximum value
            retry_to    = 0;
            retry_tm    = 0;
	    } else if (millis() > start_ms + conn_to) {                 // Failed to connect to AP
            WiFi.disconnect(true);
            if (retry_count == 0) {                                 // No more retry attempts!
                status = W_UNCONFIG;
                return;
            }
            status = W_OFF;
            if (retry_to == 0) {                                    // First retry attempt
                retry_to = 60;
            } else {
                retry_to += 60;                                     // Increment by 1 minute
                if (retry_to > max_retry_to)                        // Limit the retry timeout
                    retry_to = max_retry_to;
                --retry_count;
            }
            retry_tm = now() + retry_to;
	    }
	}
}
