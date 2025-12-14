#include <Arduino.h>
#include "led.hpp"
#include "touch.hpp"
#include "epaper.hpp"

static Led led;
static Touch touch;

void setup()
{
    // Serial.begin(115200);
    // delay(2000);
    // led.init();
    // touch.init();
    // EPaper::init();
    // // Run a simple black/white diagnostic first; if this fails, SPI wiring/pins are wrong
    // EPaper::showBlackWhiteTest();
    // // Then try the accumulating test once we know refresh works
    // EPaper::showAccumulatingTest(10, 500);

    Serial.println(">>> setup: before led/touch init");
    led.init();
    touch.init();
    Serial.println(">>> setup: before EPaper::init");
    EPaper::init();
    Serial.println(">>> setup: after EPaper::init, before showBlackWhiteTest");
    EPaper::showBlackWhiteTest();
    Serial.println(">>> setup: after showBlackWhiteTest");
}

void loop()
{
    led.loop();
}
