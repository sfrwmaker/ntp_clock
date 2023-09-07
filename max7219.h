#ifndef _MAX7219_H
#define _MAX7219_H

#include <Arduino.h>

class MAX7219 {
    public:
        MAX7219(uint8_t cs_pin, uint8_t number = 1, uint32_t spi_freq = 8000000);   //SPI@8MHZ
        void        init(void);
        void        shutdown(bool on);
        uint8_t     setIntensity(uint8_t module, uint8_t intensity);
        void        clearDisplay(uint8_t module);
        void        setRow(uint8_t module, uint8_t row, uint8_t value);
        void        setDigit(uint8_t module, uint8_t pos, uint8_t hex, bool dp);
        void        setChar(uint8_t module, uint8_t  pos, char  value, bool dp);
    private:
        void        writeRegister(uint8_t reg, uint8_t value, uint8_t module);
        uint32_t    spi_freq; 
        uint8_t     cs_pin      = 15;
        uint8_t     bright      = 8;
        uint8_t     number;
};

#endif
