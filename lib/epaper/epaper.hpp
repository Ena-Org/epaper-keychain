// lib/epaper/epaper.hpp
#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include "pins.hpp"

// ★★ 这里的 GxEPD2_290_T94 是「示例型号」★★
// 你的屏幕是哪一款，要对照 GxEPD2 示例里的列表改成对应类名。
// 先用这个占位跑通结构，后面我们再精确匹配。

// #include <GxEPD2_3C.h>  // 如果是双色/三色屏，看你实际屏幕类型

// 先假设是 2.9" 296x128 黑白屏
// #include <GxEPD2_display_selection.h>
// 如果这个头文件报错，说明需要手动包含具体型号：
// #include <GxEPD2_290_T94.h>

// using EPaperDriver = GxEPD2_290_T94_V2;
using EPaperDriver = GxEPD2_290;

extern GxEPD2_BW<EPaperDriver, EPaperDriver::HEIGHT> display;

class EPaper
{
public:
    static void init();
    static void showHello();
    static void showBattery(uint8_t percent);
};
