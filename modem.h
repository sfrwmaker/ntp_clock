/*
 * modem.h
 *
 *  Created on: 23 apr 2022
 *      Author: Alex
 */

#ifndef _MODEM_H_
#define _MODEM_H_

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <Time.h>
#include <TimeLib.h>

typedef enum e_wifi {
	W_UNCONFIG = 0, W_OFF, W_TRY, W_READY
} t_stat;

class MODEM {
	public:
		MODEM(void)											{ }
		void			setName(const String name)			{ hostname = name; 		}
		void			setSSID(const String ssid)			{ this->ssid = ssid;	}
		void			setPasswd(const String pass)		{ password = pass;		}
        String&         getName(void)                       { return hostname;      }
        String&         getSSID(void)                       { return ssid;          }
        String&         getPasswd(void)                     { return password;      }
        bool            isReadyToConnect(void);
        uint16_t        timeToSync(void);
        bool            isConnected(void);
		void			connect(void);
		void			disconnect(void);
		void			loop();
		t_stat 			status = W_OFF;
	private:
        uint32_t        start_ms    = 0;                    // Time when started to connect to access point
		String		    hostname	= "";
		String		    ssid		= "";
		String		    password	= "";
        time_t          retry_tm    = 0;                    // Time when retry to connect to Access Point
        uint16_t        retry_to    = 0;                    // Next reconnect timeout (s)
        uint8_t         retry_count = 0;                    // Count of the recconnect attempts. 0 means last try
        const uint32_t  conn_to     = 60000;                // Conection timeout
        const uint16_t  max_retry_to= 600;                  // The maximum retry timeout value
        const uint8_t   max_retries = 150;
};



#endif
