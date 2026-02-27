#include "render_epaper.hpp"
#include <ArduinoJson.h>

void RenderEPaper::render(const char* text)
{
  Serial.println("[EINK] Render:");
  Serial.println(text);
}