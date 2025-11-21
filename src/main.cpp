#include <Arduino.h>
#include "led.hpp"

static Led led;

void setup()
{
    led.init();
}

void loop()
{
    led.loop();
}