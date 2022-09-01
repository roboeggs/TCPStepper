#define numPins 8
/* указываем пины на которых работаем */
// если плата сбрасывается после прошивки, то указан неверный пин

#if defined(ESP32)
    const uint8_t stepPin[numPins] = {22, 19, 5, 17, 4, 16, 18, 23}; //esp32
    const uint8_t sleepPin[4] = {21, 33, 15, 3}; //esp32
#elif defined(ESP8266)
    const uint8_t stepPin[numPins] = {0, 2, 4, 5, 13, 12, 15, 16}; //esp8266
    uint8_t sleepPin[4] = {13, 12, 15, 16}; //esp8266
#endif