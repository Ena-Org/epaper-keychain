#pragma once
#include <Arduino.h>

class Touch
{
public:
    bool static TOUCH_ACTIVE_HIGH;
    void init();
    void begin();
};