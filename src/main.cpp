#include <Arduino.h>
#include "led.hpp"
// #include "rgb.hpp"
#include "touch.hpp"
// #include "touch_rgb/touch_rgb.hpp"
#include "epaper.hpp"

static Led led;
// static Rgb rgb;
static Touch touch;
// static TouchRGBFeature touchRgbFeature;
void setup()
{
    Serial.begin(115200);
    delay(2000);
    led.init();
    // rgb.init();
    touch.init();
    // touchRgbFeature.init();

    EPaper::init();
    // EPaper::showHello();
    EPaper::showAccumulatingTest();
    // EPaper::showPowerAnimation(5, 800);
}

void loop()
{
    // led.on();
    // rgb.tick();
    // touch.begin();
    // touchRgbFeature.begin();s
}