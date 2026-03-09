#include <Arduino.h>

#include "logger_config.hpp"

#include "cmd/cmd.hpp"
#include "epaper_render/epaper_render.hpp"
#include "logger.hpp"

namespace
{
    Transport g_transport;
    Codec g_codec;
    Router g_router;
    Cmd g_cmd;
}

void setup()
{
    Serial.begin(115200);
    Logger::LoggerConfig logCfg;
    logCfg.level = ProjectLog::kLevel;
    logCfg.history_bytes = 2048;
    Logger::init(logCfg);
    g_cmd.init(g_transport, g_codec, g_router);
    EpaperRender::init();
}

void loop()
{
    g_cmd.loop();
    EpaperRender::loop();
}
