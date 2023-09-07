// Heart-beat manager. Periodically sets to LOW the heartbeat pin to reset the hardware NE555 watchdog timer
#ifndef _HB_H
#define _HB_H

#include <Arduino.h>

class HB {
    public:
        HB(uint8_t hb_pin);
        void        sendHeartbeat(void);
        void        setPeriod(uint16_t to = 0)      { period = to; }
        void        loop(void);
    private:
        uint8_t     hb_pin;                         // NE555 reset pin (heart beat)
        uint16_t    period;                         // The period to send the heartbeat to the ne555 (ms)
        uint32_t    next_hb;                        // Time to reset the NE555 (ms)
        uint32_t    reset_done;                     // Time to stop reseting NE555
};

#endif
