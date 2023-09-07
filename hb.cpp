#include "hb.h"

HB::HB(uint8_t hb_pin) {
    this->hb_pin = hb_pin;
    pinMode(hb_pin, INPUT);
    period = 0;                                             // auto reset is disabled
    next_hb = 0;
}

void HB::sendHeartbeat(void) {
    pinMode(hb_pin, OUTPUT);
    digitalWrite(hb_pin, LOW);                              // reset ne555 timer
    delay(200);
    pinMode(hb_pin, INPUT);
}

void HB::loop(void) {
    if (period == 0) return;                                // Automatic reset is disabled, do nothing
    if (millis() > next_hb) {                               // It is time to reset NE555
        if (reset_done == 0) {                              // It is time to start
            reset_done = millis() + 200;                    // finish sending reset in reset_done
            pinMode(hb_pin, OUTPUT);
            digitalWrite(hb_pin, LOW);
        } else if (millis() > reset_done) {                 // It is time to stop
            pinMode(hb_pin, INPUT);
            reset_done = 0;
            next_hb = millis() + (uint32_t)period;
        } 
    }
}
