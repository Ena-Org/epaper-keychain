#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include "pins.hpp"

using EPaperDriver = GxEPD2_290_T94_V2;

extern GxEPD2_BW<EPaperDriver, EPaperDriver::HEIGHT> display;

class EPaper
{
public:
    static void init();
    static void showHello();
    static void showBattery(uint8_t percent);
    static void showAccumulatingTest(uint8_t steps = 10, uint16_t delayMs = 500);

private:
    static void drawBasePage();
};
