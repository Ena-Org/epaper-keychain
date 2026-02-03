#include <Arduino.h>
#include "led.hpp"
#include "touch.hpp"
#include "epaper.hpp"
#include "wifi.hpp"
#include "http.hpp"
#include "text_sync/text_sync.hpp"

static Led led;
static Touch touch;

#ifndef API_TEXT_URL
#define API_TEXT_URL "http://192.168.1.10:8080/api/text/latest"
#endif

WifiManager wifi;
SimpleHttp http;
TextSync textSync(http);

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
