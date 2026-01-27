#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include "pins.hpp"

// Waveshare 2.9" e-Paper Module (SKU:12563), black & white, 296x128
// Try the generic 2.9" b/w driver first; if the screen仍无任何反应，可改为 GxEPD2_290_T5 再测试
// using EPaperDriver = GxEPD2_290;
// using EPaperDriver = GxEPD2_290_T5;
using EPaperDriver = GxEPD2_290_T94_V2;
// using EPaperDriver = GxEPD2_290_BS;

extern GxEPD2_BW<EPaperDriver, EPaperDriver::HEIGHT> display;

class EPaper
{
public:
    static void init();
    static void showHello();
    static void showBattery(uint8_t percent);
    static void showAccumulatingTest(uint8_t steps = 10, uint16_t delayMs = 500);
    static void showBlackWhiteTest();

private:
    static void drawBasePage();
};
