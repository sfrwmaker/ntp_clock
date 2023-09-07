#include "max7219.h"

#ifndef _DSPL_H
#define _DSPL_H

class LEDBR {
    public:
        LEDBR(void);
        void        init(void);
        void        setLimits(uint16_t light_min, uint16_t light_max, uint8_t br_min, uint8_t br_max, uint8_t hysteresis = 0);
        uint8_t     brightness(void);
        void        update(uint16_t adc);
        uint16_t    lightMin(void)                  { return l_min; }
        uint16_t    lightMax(void)                  { return l_max; }
    private:
        int16_t     light;                          // The last measured light
        uint8_t     bright;                         // The last calculated brightness
        uint16_t    l_min, l_max;                   // The light levels (min, max)
        uint8_t     b_min, b_max;                   // The brightness limits
        uint8_t     hyst;                           // hysteresis for brightness change
};

class DSPL : protected MAX7219, public LEDBR {
    public:
        DSPL(uint8_t cs) : MAX7219(cs)                      { }
        void        init(void)                              { MAX7219::init(); LEDBR::init();   }
        void        clear(void)                             { MAX7219::clearDisplay(1);         }
        void        brightness(uint8_t bright);
        void        adjustBright(uint16_t adc);
        void        showTime(uint8_t hour, uint8_t minute, uint8_t dot_mask, uint8_t display_mask = 0b1111);
        void        showTemperature(int16_t temp);          // show the temperature in celsius * 10
        void        showPressure(uint16_t pressure);        // show presure mmHg
        void        show4Symbols(uint8_t sym[4]);
        void        showSyncRemains(uint16_t sec);
    private:
        uint8_t     bright      = 16;
};

#endif
