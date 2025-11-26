#include <Arduino.h>
#include "led.hpp"
#include "rgb.hpp"
#include "touch.hpp"
#include "touch_rgb_feature/touch_rgb_feature.hpp"

static Led led;
static Rgb rgb;
static Touch touch;
static TouchRGBFeature touchRgbFeature;
void setup()
{
    led.init();
    rgb.init();
    touch.init();
    touchRgbFeature.init();
}

void loop()
{
    // led.on();
    // rgb.tick();
    // touch.begin();
    touchRgbFeature.begin();
}