#include <Arduino.h>
#include "led.hpp"
#include "rgb.hpp"
#include "touch.hpp"

static Led led;
static Rgb rgb;
static Touch touch;

void setup()
{
    led.init();
    rgb.init();
    touch.init();
}

void loop()
{
    // led.on();
    rgb.loop();
    touch.loop();
}