#include <Arduino.h>
#include "led.hpp"
#include "touch.hpp"
#include "epaper.hpp"

static Led led;
static Touch touch;

void setup()
{
    Serial.begin(115200);
    delay(2000); 

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
