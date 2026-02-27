#pragma once
#include <Arduino.h>

class TouchRGBFeature
{
public:
    static int CH_R;
    static int CH_G;
    static int CH_B;
    static int PWM_FREQ;
    static int PWM_RES;
    static bool ledOn;
    static bool lastTouched;

    void init();
    void begin();
};