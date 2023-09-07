#include "press.h"

bool PRESS::init(void) {
    BlueDot_BME280::parameter.communication = 0;            // BME sensor on I2C bus
    BlueDot_BME280::parameter.I2CAddress = 0x76;            // BME sensor I2C Address
    BlueDot_BME280::parameter.sensorMode = 0b11;            // In normal mode the sensor measures continually (default value)
    BlueDot_BME280::parameter.IIRfilter = 0b100;            // factor 16 (default value)
    BlueDot_BME280::parameter.humidOversampling = 0b101;    // factor 16 (default value)
    BlueDot_BME280::parameter.tempOversampling = 0b101;     // factor 16 (default value)
    BlueDot_BME280::parameter.pressOversampling = 0b101;    // factor 16 (default value)
    BlueDot_BME280::parameter.pressureSeaLevel = 1013.25;
    BlueDot_BME280::parameter.tempOutsideCelsius = 15;
    return (BlueDot_BME280::init() == 0x60);
}

int16_t PRESS::temperature(void) {
    float t = BlueDot_BME280::readTempC();
    return round(t * 10);
}

uint32_t PRESS::pressure(void) {
    float p = BlueDot_BME280::readPressure();
    return round(p * 100);
}

uint32_t PRESS::seaPressure(void) {
    float pSeaCoeff = 1.0;
    if (altitude > 1) {
        pSeaCoeff = 1.0 + 0.000125 * (float)altitude;
    }
    float p = BlueDot_BME280::readPressure() * 100 * pSeaCoeff;
    return round(p);
}

uint16_t PRESS::mmHg(uint32_t p) {
    return round((float)p*0.00750062);
}

String PRESS::getAltitudeS(void) {
    return String(altitude);
}
