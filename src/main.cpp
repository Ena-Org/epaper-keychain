#include <Arduino.h>
#include "led.hpp"
#include "touch.hpp"
#include "epaper.hpp"
#include "wifi.hpp"
#include "wifi_config.hpp"
#include "logger_config.hpp"
#include "http.hpp"
#include "logger.hpp"
#include "text_sync/text_sync.hpp"
#include "render_epaper/render_epaper.hpp"

#include "uploader/uploader.hpp"
#include "cmd_router/cmd_router.hpp"

static Led led;
static Touch touch;
static Wifi wifi;

static Uploader::UploadSession uploadSession;
static CmdRouter cmdRouter;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;

void setup()
{
    Serial.begin(115200);
    // delay(2000);

    Logger::LoggerConfig logCfg;
    logCfg.level = ProjectLog::kLevel;
    Logger::init(logCfg);

    // usbTransport.begin(115200)
    // usbTransport.writeLine("OK READY");

    // );
    // cmdRouter.begin();

    

    // LOGI("MAIN", "setup: before led/touch init");
    // led.init();
    // touch.init();
    // LOGI("MAIN", "setup: before EPaper::init");
    // EPaper::init();
    // LOGI("MAIN", "setup: after EPaper::init, before showBlackWhiteTest");
    // EPaper::showBlackWhiteTest();
    // LOGI("MAIN", "setup: after showBlackWhiteTest");

    // Wifi::Config wifiCfg;
    // wifiCfg.ssid = WIFI_SSID;
    // wifiCfg.password = WIFI_PASSWORD;
    // LOGI("MAIN", "wifi target ssid=%s", wifiCfg.ssid.empty() ? "<empty>" : wifiCfg.ssid.c_str());
    // wifi.init(wifiCfg);
    // if (!wifi.ensureConnected(WIFI_CONNECT_TIMEOUT_MS))
    //     LOGE("MAIN", "wifi connect failed, reason=%d", wifi.lastReason());
    // else
    //     LOGI("MAIN", "wifi connected, ip=%s, rssi=%d", wifi.ipString().c_str(), wifi.rssi());



    
}

void loop()
{
    // cmdRouter.loop(usbTransport, uploadSession);
    delay(10);
}
