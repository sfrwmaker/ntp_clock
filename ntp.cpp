#include <Time.h>
#include <TimeLib.h>
#include "ntp.h"

//------------------------------------------ NTP clock ---------------------------------------------------------
void ntpClock::init(String sn) {
    udp.begin(ntp_port);
    ntp_server_name = sn;
    ntp_sync_time   = 0;                        // Force to sync time
    time_synched    = false;
    sync_to         = def_sync_to;
    last_sync_time  = 0;
    sync_nightly    = true;
}

bool ntpClock::syncTime(bool force) {
    if (!force) {                                   // Sync time periodically
        if (ntp_sync_time == 0 || now() >= ntp_sync_time)
            force = true;
    }
    if (force) {                                    // Make 3 iterations, compare the results before setting the time
        time_t t = utcTime();
        time_t n = now();
        if (sync_nightly && n > ntp_sync_time + 3600) { // tried to connect more than hour
            sync_nightly = false;                   // Perhaps, the WiFi AP was turned-off nightly
//Serial.println("Failed to sync at night");
        }
        ntp_sync_time   = calculateNextSync(n);
        time_synched    = true;
        if (t > last_sync_time) {
//Serial.print("Greater, ");
            if (tmDiff(t, n) <= 60) {               // Time synched OK
                sync_to += 3600;                    // Increase the time sync timeout
                setTime(t);
                last_sync_time = t;
//Serial.println("< 1m ");
                return true;
            } else if (tmDiff(t, n) < 300) {
                sync_to -= 3600;                     // Short the sync period
                setTime(t);
                last_sync_time = t;
//Serial.println("< 5m");
                return true;
            } else if (n < 31536000) {              // Less than one year, clock not set-up yet
                setTime(t);
                sync_to = def_sync_to;
                time_synched   = false;
                ntp_sync_time = t + 10;
//Serial.println("first setup");
                return true;
            }
        }
        // Failed to sync time, resync in 10 seconds
        time_synched    = false;
        sync_to -= 600;
        ntp_sync_time   = n + 30;
//Serial.print(" quick retry: "); Serial.println(tmDiff(t, n));
        return false;
    }
    return false;
}

bool ntpClock::timeSynced(void) {
    return time_synched && ntp_sync_time > 0 && now() < ntp_sync_time;
}

time_t ntpClock::utcTime(void) {
    IPAddress timeServerIP;
    const char *c = ntp_server_name.c_str();
    WiFi.hostByName(c, timeServerIP);
    sendNTPpacket(timeServerIP);
    yield();
    delay(1000);
    yield();
    delay(1000);
    int cb = udp.parsePacket();
    if (cb) {
        udp.read(packetBuffer, NTP_PACKET_SIZE);    // read the packet into the buffer
        // the timestamp starts at byte 40 of the received packet and is four bytes, or two words, long. First, esxtract the two words:
        uint32_t highWord = word(packetBuffer[40], packetBuffer[41]);
        uint32_t lowWord  = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer this is NTP time (seconds since Jan 1 1900):
        uint32_t secsSince1900 = highWord << 16 | lowWord;
        // now convert NTP time into everyday time: Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        time_t epoch = secsSince1900 - 2208988800UL;
        return epoch; 
    }
    return 0;
}

time_t ntpClock::ntpTime(void) {
    time_t tm = utcTime();
    if (tm < 1536000) return tm;                        // Failed to get NTP time
    setTime(tm);
    return tm + TZ(tm);
}

String ntpClock::ntpStr(time_t ts) {
    if (ts == 0) ts = ntpTime();
    char buff[25];
    sprintf(buff, "%02d:%02d, %02d-%02d-%4d", hour(ts), minute(ts), day(ts), month(ts), year(ts));
    return String(buff);
}

void ntpClock::setDst(bool winter, uint8_t month, uint8_t day, uint8_t week_day, uint8_t week_number, uint16_t when, int16_t shift_minute) {
    uint8_t indx            = (winter)?1:0;
    if (day > 0) {
        week_day = week_number = 0;
    }
    tz[indx].month          = month;
    tz[indx].day            = day;
    tz[indx].w_day          = week_day;
    tz[indx].w_num          = week_number;
    tz[indx].quoter_hour    = when / 15;
    tz[indx].utc_shift      = shift_minute / 15;
    next_w                  = 0;
    next_s                  = 0;
}

int32_t ntpClock::TZ(time_t n) {
    int32_t result = (int32_t)tz[0].utc_shift * 15*60;  // In seconds (0-th is a summer time) 
    if (tz[0].utc_shift == tz[1].utc_shift) {           // Constant time
        tz[0].month         = 0;    tz[1].month         = 0;
        tz[0].day           = 0;    tz[1].day           = 0;
        tz[0].w_day         = 0;    tz[1].w_day         = 0;
        tz[0].w_num         = 0;    tz[1].w_num         = 0;
        tz[0].quoter_hour   = 0;    tz[1].quoter_hour   = 0;
        return result;
    }
    if (tz[0].month == 0 || tz[1].month == 0 || tz[0].month > 12 || tz[1].month > 12) // Wrong month format
        return result;
    // update the switching time cache if current time is later
    if (n >= next_w) {
        next_w  = next(tz[1].quoter_hour, tz[1].month, tz[1].day, tz[1].w_day, tz[1].w_num, tz[0].utc_shift * 15*60);
    }
    if (n > next_s) {
        next_s  = next(tz[0].quoter_hour, tz[0].month, tz[0].day, tz[0].w_day, tz[0].w_num, tz[1].utc_shift * 15*60);
    }
    if (next_w > next_s) {                              // Winter time
        result = (int32_t)tz[1].utc_shift * 15*60;
    }
    return result;
}

uint8_t ntpClock::dstMonth(bool winter) {
    uint8_t indx  = (winter)?1:0;
    return tz[indx].month;
}

uint8_t ntpClock::dstDay(bool winter) {
    uint8_t indx  = (winter)?1:0;
    return tz[indx].day;
}

uint8_t ntpClock::dstWDay(bool winter) {
    uint8_t indx  = (winter)?1:0;
    return tz[indx].w_day;    
}

uint8_t ntpClock::dstWNum(bool winter) {
    uint8_t indx  = (winter)?1:0;
    return tz[indx].w_num;
}

uint16_t ntpClock::dstWhen(bool winter) {
    uint8_t indx  = (winter)?1:0;
    return tz[indx].quoter_hour * 15;
}
int16_t ntpClock::dstShift(bool winter) {
    uint8_t indx  = (winter)?1:0;
    return tz[indx].utc_shift * 15;
}

void ntpClock::sendNTPpacket(IPAddress& address) {
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;                       // LI, Version, Mode
    packetBuffer[1] = 0;                                // Stratum, or type of clock
    packetBuffer[2] = 6;                                // Polling Interval
    packetBuffer[3] = 0xEC;                             // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // all NTP fields have been given values, now you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123);                      // NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

void ntpClock::srvSet(String ntp_server) {
    if (ntp_server.length() > 32) {
        ntp_server_name = ntp_server.substring(0, 32);
    } else {
        ntp_server_name = ntp_server;
    }
}

time_t ntpClock::next(uint8_t hour, uint8_t month, uint8_t day, uint8_t week_day, uint8_t week_number, int32_t utc_shift) {
    tmElements_t tm;
    time_t n = now() + utc_shift;                       // local time
    breakTime(n, tm);
    tm.Second   = 0;
    tm.Minute   = (hour & 0x3) * 15;
    tm.Hour     = hour >> 2;
    tm.Day      = day;
    tm.Month    = month;
    time_t t = 0;
        
    if (day != 0) {                                     // Month + day
        t = makeTime(tm);
        if (t < n) {                                    // Next year
            ++tm.Year;
            t = makeTime(tm);
        }
    } else {
        if (week_day == 0) week_day = 1;                // Error, the week day cannot be irrelevant
        if (week_number == 0) week_number = 7;           // Error, the week number cannot be irrelevant
        for (uint8_t i = 0; i < 2; ++i) {               // Iterate through this and next year
            tm.Day      = 1;
            t = makeTime(tm);                           // 1-st day of that month
            uint8_t wday = weekday(t);                  // The week day of the 1-st day in that month. sunday is 1, saturday = 7
            uint8_t days_to_add = (wday <= week_day)?week_day-wday:week_day+7-wday;
            tm.Day     += days_to_add;                  // First week day in that month
            uint8_t days = days_per_month[month-1];
            if (month == 2) {                           // February
                uint16_t Year = year(t);
                if (Year & 0x3)                         // Leap year
                    ++days;
            }
            if (week_number < 6) {
                uint16_t d = tm.Day + 7 * (week_number - 1);
                if (d > days) {
                    week_number = 7;                    // Search for the last week day in the month
                }
                tm.Day = d;
            }
            if (week_number == 7) {                     // Search for the last week day in the month
                while (tm.Day + 7 < days)
                    tm.Day += 7;
            } else if (week_number == 6) {
                while (tm.Day + 7 < days)
                    tm.Day += 7;
                tm.Day -= 7;
            }
            t = makeTime(tm);
            if (t >= n)                                 // This year
                break;
            ++tm.Year;
        }
    }
    return t - utc_shift;
}

time_t ntpClock::calculateNextSync(time_t n) {
    sync_to = constrain(sync_to, 6*60*60, 48*60*60);    // Limit time sync timeoutvalue 6h <= timeout <= 48h
    if (sync_nightly) {                                 // Calculate next sync time at 3:00 am
        int32_t tz = TZ(n);
        n += tz;                                        // Local time
        tmElements_t tm;
        breakTime(n, tm);
        tm.Hour     = 3;
        tm.Minute   = 0;
        time_t n_sync = makeTime(tm);                   // Today, 3:00
        for (uint8_t i = 0; i < 2; ++i) {               // 2 days max period
            if (n_sync > n + sync_to)
                break;
            n_sync += 86400;                            // next day
        }
        return n_sync - tz;
    }
    return n + sync_to;
}

uint32_t ntpClock::tmDiff(time_t x, time_t y) {
    if ((uint32_t)x > (uint32_t)y) return (uint32_t)x - (int32_t)y;
    return (uint32_t)y - (uint32_t)x;
}
