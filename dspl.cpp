#include "dspl.h"

// Symbol table starting from Ascii 40 code
const static uint8_t char_table[] = {
    0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B10000000, 0B00000001, 0B10000000, 0B00000000, // 40-47
    0B01111110, 0B00110000, 0B01101101, 0B01111001, 0B00110011, 0B01011011, 0B01011111, 0B01110000, // 48-55 ('0' - '7')
    0B01111111, 0B01111011, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, // 56-63
    0B00000000, 0B01110111, 0B00011111, 0B00001101, 0B00111101, 0B01001111, 0B01000111, 0B00000000, // 64-71 ('@' - 'G')
    0B00110111, 0B00000000, 0B00000000, 0B00000000, 0B00001110, 0B00000000, 0B00000000, 0B00000000, // 72-79 ('H' - 'O')
    0B01100111, 0B00000000, 0B00000000, 0B01011011, 0B00000000, 0B00000000, 0B00000000, 0B00000000, // 80-87 ('P' - 'W')
    0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00000000, 0B00001000, // 88-95 ('X' - '_')
};

void DSPL::brightness(uint8_t bright) {
    bright &= 0x0f;
    if (this->bright != bright) {
        setIntensity(0, bright);
        this->bright = bright;
    }
}

void DSPL::adjustBright(uint16_t adc) {
    LEDBR::update(adc);
    uint8_t br = LEDBR::brightness();
    this->brightness(br);
}

void DSPL::showTime(uint8_t hour, uint8_t minute, uint8_t dot_mask, uint8_t display_mask) {
    int8_t  digit = 0;
    bool    clear_first_digit = (hour < 1);
    uint8_t mask = 1;
    for (int8_t d = 3; d >= 0; --d) {                       // Print out digit from right to left
        if (d >= 2) {                                       // Minutes
            digit = minute % 10;
            minute /= 10;
        } else {                                            // Hours
            digit = hour % 10;
            hour /= 10;
        }
        if (display_mask & mask)
            setDigit(0, d, digit, dot_mask & mask);
        else
            setChar(0,  d, ' ',   dot_mask & mask);
        mask <<= 1;
    }
}

void DSPL::showTemperature(int16_t temp) {
    if (temp >= 0) {
        setRow(0, 3, 0b01100011);                           // degree sign
        for (int8_t i = 2; i >= 0; --i) {
            uint8_t dgt = temp % 10;
            temp /= 10;
            setDigit(0, i, dgt, i == 1);
        }
    } else {
        temp *= -1;
        setChar(0, 0, '-', false);
        for (int8_t i = 3; i >= 1; --i) {
            uint8_t dgt = temp % 10;
            temp /= 10;
            setDigit(0, i, dgt, i == 2);
        }
    }
}

void DSPL::showPressure(uint16_t pressure) {
    char sym = 'P';
    if (pressure < 750)
        sym = 'L';
    else if (pressure > 770)
        sym = 'H';
    setChar(0, 0, sym, true);
    for (uint8_t i = 3; i >= 1; --i) {
        uint8_t dgt = pressure % 10;
        pressure /= 10;
        setDigit(0, i, dgt, false);
    }
}

void DSPL::show4Symbols(uint8_t sym[4]) {
    for (uint8_t i = 0; i < 4; ++i) {
        uint8_t s = sym[i];
        uint8_t dot_mask = s & 0x80;
        s &= ~0x80;
        if (s >= 40 && s <= 95) {
            s -= 40;
            setRow(0, i, char_table[s] | dot_mask);
        } else {
            setChar(0, i, ' ', false);
        }
    }
}

void DSPL::showSyncRemains(uint16_t sec) {
    if (sec > 999) sec = 999;                               // Show 3 digits only
    setDigit(0, 0, 5, true);                                // Looks like 'S.'
    for (uint8_t i = 3; i > 0; --i) {
        setDigit(0, i, sec%10, false);
        sec /= 10;
    }
}

LEDBR::LEDBR(void) {
    light   = 0;
    bright  = 0;
    hyst    = 0;
    l_min   = 0;
    l_max   = 4095;
    b_min   = 0;
    b_max   = 255;
}

void LEDBR::init(void) {
    light   = (l_min + l_max) / 2;
    bright  = map(light, l_min, l_max, b_min, b_max);
}

void LEDBR::setLimits(uint16_t light_min, uint16_t light_max, uint8_t br_min, uint8_t br_max, uint8_t hysteresis) {
    l_min   = light_min;
    l_max   = light_max;
    b_min   = br_min;
    b_max   = br_max;
    hyst    = hysteresis;
    init();
}

uint8_t LEDBR::brightness(void) {
    return constrain(bright, b_min, b_max);
}

void LEDBR::update(uint16_t light_now) {
    int16_t  h = 0;

    if (light_now > light) {
        h = hyst * (-1);
        light += ((light_now - light) >> 3) | 1;
    } else if (light_now < light) {
        h = hyst;
        light -= ((light - light_now) >> 3) | 1;
    }
    int16_t l = light + h;
    if (l < 0) l = 0;

    l = constrain(l, l_min, l_max);
    uint8_t br = map(l, l_min, l_max, b_min, b_max);
    if ((light_now > light) && (br > bright)) {
        bright = br;
    } else if ((light_now < light) && (br < bright)) {
        bright = br;
    }
    if (abs(int16_t(bright)-int16_t(br)) >= 2) {
        bright = br;
    }
}
