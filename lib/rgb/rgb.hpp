#pragma once
#include <Arduino.h>

class Rgb
{
public:
    bool static RGB_COMMON_ANODE;
    void init();
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void loop();
};
