#ifndef _PRESS_H
#define _PRESS_H

#include <BlueDot_BME280.h>

class PRESS : public BlueDot_BME280 {
    public:
        PRESS(void) :   BlueDot_BME280()                    { }
        bool        init(void);
        int16_t     temperature(void);                      // 1/10 Celsius degree
        uint32_t    pressure(void);                         // Pa
        uint32_t    seaPressure(void);                      // Pa
        uint16_t    mmHg(uint32_t p);
        void        setAltitude(int16_t a)                  { altitude = a; }
        String      getAltitudeS(void);
    private:
        int16_t     altitude    = 1;
};

#endif
