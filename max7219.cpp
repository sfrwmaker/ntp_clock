#include <SPI.h>
#include "max7219.h"

enum    max7219_register {
    MAX7219_REG_NOOP = 0, MAX7219_REG_DIGIT0 = 1, MAX7219_REG_DECODEMODE = 9,
    MAX7219_REG_INTENSITY, MAX7219_REG_SCANLIMIT, MAX7219_REG_SHUTDOWN, MAX7219_REG_DISPLAYTEST = 0xF
};

const static uint8_t hex_digit[] = {
    0B01111110, 0B00110000, 0B01101101, 0B01111001, 0B00110011, 0B01011011, 0B01011111, 0B01110000,
    0B01111111, 0B01111011, 0B01110111, 0B00011111, 0B00001101, 0B00111101, 0B01001111, 0B01000111
};

// Symbol table starting from Ascii 40 code
const static uint8_t char_table[] = {
    0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B10000000, 0B00000001, 0B10000000, 0B00000000, // 40-47
    0B01111110, 0B00110000, 0B01101101, 0B01111001, 0B00110011, 0B01011011, 0B01011111, 0B01110000, // 48-55 ('0' - '7')
    0B01111111, 0B01111011, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000,
    0B00000000, 0B01110111, 0B00011111, 0B00001101, 0B00111101, 0B01001111, 0B01000111, 0B00000000, // 64-71 ('@' - 'G')
    0B00110111, 0B00000000, 0B00000000, 0B00000000, 0B00001110, 0B00000000, 0B00000000, 0B00000000, // 72-79 ('H' - 'O')
    0B01100111, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, // 80-87 ('P' - 'W')
    0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00001000, // 88-95 ('X' - '_')
};

MAX7219::MAX7219(uint8_t cs_pin, uint8_t number, uint32_t spi_freq) {
    this->cs_pin    = cs_pin;
    this->number    = number;
    this->spi_freq  = spi_freq;
    pinMode(cs_pin, OUTPUT);
    digitalWrite(cs_pin, HIGH);
    SPI.begin();
}

void MAX7219::init(void) {
    writeRegister(MAX7219_REG_SCANLIMIT,    7,      number);        // show all 8 digits
    writeRegister(MAX7219_REG_DECODEMODE,   0,      number);        // using an led matrix (not digits)
    writeRegister(MAX7219_REG_DISPLAYTEST,  0,      number);        // no display test
    writeRegister(MAX7219_REG_INTENSITY,    bright, number);        // character intensity: range: 0 to 15
    writeRegister(MAX7219_REG_SHUTDOWN,     0,      number);        // in shutdown mode
    delay(100);
    shutdown(false);
    clearDisplay(number);
}

void MAX7219::shutdown(bool on) {
    writeRegister(MAX7219_REG_SHUTDOWN, on?0:1, 0);
}

uint8_t MAX7219::setIntensity(uint8_t module, uint8_t intensity) {
    if (intensity < 16) {
        bright = intensity;
        writeRegister(MAX7219_REG_INTENSITY, bright, module);
    }
    return bright;
}

void MAX7219::clearDisplay(volatile uint8_t module) {
    for (uint8_t dgt = 0; dgt < 8; ++dgt) {
        writeRegister(dgt+MAX7219_REG_DIGIT0, 0, module);
    }
}

void MAX7219::setRow(uint8_t module, uint8_t row, uint8_t value) {
    if (row < 8) {
        writeRegister(row+MAX7219_REG_DIGIT0, value, module);
    }
}

void MAX7219::setDigit(uint8_t module, uint8_t pos, uint8_t hex, bool dp) {
    if (pos < 8 && hex < 16) {
        uint8_t v   = hex_digit[hex];
        if (dp) v  |= 0B10000000;
        writeRegister(pos+MAX7219_REG_DIGIT0, v, module);
    }
}

void MAX7219::setChar(uint8_t module, uint8_t  pos, char  value, bool dp) {
    if (value < 40 || value > 'z') value = 40;                      // Draw space in case of wrong character
    if (value >= 'a') value -= 32;                                  // Translate small letter to Big letter
    value -= 40;
    if (pos < 8 && value < sizeof(char_table) ) {
        uint8_t v   = char_table[(uint8_t)value];
        if (dp) v  |= 0B10000000;
        writeRegister(pos+MAX7219_REG_DIGIT0, v, module);
    }
}

void MAX7219::writeRegister(volatile uint8_t reg, volatile uint8_t value, volatile uint8_t module) {
    SPI.beginTransaction(SPISettings(spi_freq, MSBFIRST, SPI_MODE0));
    digitalWrite(cs_pin, LOW);
    for (int8_t i = number-1; i >= 0; --i) {
        if ((module >= number) or (module == i)) {                  // module >= 0 means all modules
            SPI.transfer(reg);
            SPI.transfer(value);
        } else {
            SPI.transfer(0);
            SPI.transfer(0);
        }
    }
    digitalWrite(cs_pin, HIGH);
    SPI.endTransaction();
    delay(10);
}
