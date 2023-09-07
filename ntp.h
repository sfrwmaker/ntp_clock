#ifndef _WESP_NTP_H
#define _WESP_NTP_H

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <Time.h>
#include <TimeLib.h>

//------------------------------------------ NTP clock ---------------------------------------------------------

typedef struct s_DST_rule {
    uint8_t month;
    uint8_t day;
    uint8_t w_day;
    uint8_t w_num;
    uint8_t quoter_hour;
    int8_t  utc_shift;
} t_DST_rule;

const int NTP_PACKET_SIZE = 48;                     // NTP time stamp is in the first 48 bytes of the message
class ntpClock {
    public:
        ntpClock()                                  { }
        void        init(String sn);
        bool        syncTime(bool force=false);     // Synchronize the local time with NTP source
        bool        timeSynced(void);
        time_t      utcTime(void);
        time_t      ntpTime(void);
        String      ntpStr(time_t ts = 0);
        String      ntpServerName(void)             { return ntp_server_name;                   }
        time_t      nextSync(void)                  { return ntp_sync_time + TZ(ntp_sync_time); } 
        void        srvSet(String ntp_server);
        void        setDst(bool winter, uint8_t month, uint8_t day, uint8_t week_day, uint8_t week_number, uint16_t when, int16_t shift_minute);
        int32_t     TZ(time_t n);
        uint8_t     dstMonth(bool winter);
        uint8_t     dstDay(bool winter);
        uint8_t     dstWDay(bool winter);
        uint8_t     dstWNum(bool winter);
        uint16_t    dstWhen(bool winter);
        int16_t     dstShift(bool winter);
    private:
        void        sendNTPpacket(IPAddress& address);
        time_t      next(uint8_t hour, uint8_t month, uint8_t day, uint8_t week_day, uint8_t week_number, int32_t utc_shift);
        time_t      calculateNextSync(time_t n);
        uint32_t    tmDiff(time_t x, time_t y);
        uint8_t     packetBuffer[NTP_PACKET_SIZE];
        String      ntp_server_name;
        time_t      ntp_sync_time   = 0;            // The time when sync the clock
        t_DST_rule  tz[2]           = {0};          // 0-th - summer, 1-th  - winter 
        WiFiUDP     udp;                            // A UDP instance to let us send and receive packets over UDP
        time_t      next_w = 0;                     // Time to switch to winter zone, cached value
        time_t      next_s = 0;                     // Time to switch to summer zone, cached value
        time_t      last_sync_time  = 0;            // Last Time value received from NTP source
        uint32_t    sync_to         = 90*60;        // Time sync timeout: 1 minute to 1 day
        bool        time_synched    = false;        // Flag indicating the time was synched with NTP server
        bool        sync_nightly    = true;         // Sync at night period, 3:00 am
        const uint16_t ntp_port = 2390;             // Local port to listen for UDP packets
        const uint8_t days_per_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        const uint32_t def_sync_to = 6*60*60;
};

#endif
