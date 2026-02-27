#include <Arduino.h>
#include "led.hpp"
#include "touch.hpp"
#include "epaper.hpp"
#include "wifi.hpp"
#include "http.hpp"
#include "text_sync/text_sync.hpp"
#include "render_epaper/render_epaper.hpp"

static Led led;
static Touch touch;

#ifndef API_TEXT_URL
#define API_TEXT_URL "http://185.106.176.15:3000/v1/epaper/test"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "CMCC-202-5G"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "13942929840000"
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

    wifi.init(WIFI_SSID, WIFI_PASSWORD);

    http.setTimeout(8000);
    textSync.begin(API_TEXT_URL);
    // 服务器要求 POST 调用，默认为空 body 和 JSON Content-Type，如需自定义可修改
    textSync.setPostMode(true, "", "application/json");

    String initialText;
    if (wifi.isConnected() && textSync.fetchNow(initialText))
        RenderEPaper::render(initialText.c_str());
}

void loop()
{
    wifi.loop();

    if (!wifi.isConnected())
    {
        delay(50);
        return;
    }

    String text;
    if (textSync.poll(text))
    {
        RenderEPaper::render(text.c_str());
    }

    // led.loop();
    delay(10);
}
