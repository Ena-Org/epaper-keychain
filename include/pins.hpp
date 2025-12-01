
#pragma once
#include <Arduino.h>

#define USER_LED_PIN 21

#define TOUCH_PIN D4

#define RGB_R_PIN D1
#define RGB_G_PIN D2
#define RGB_B_PIN D3


#define EPD_SCK_PIN   D8    // 时钟
#define EPD_MOSI_PIN  D10   // 数据
#define EPD_CS_PIN    D5     // 片选 CS
#define EPD_DC_PIN    D6     // 数据/命令
#define EPD_RST_PIN   D7     // 复位
#define EPD_BUSY_PIN  D0     // 忙信号
#define EPD_PWR_PIN   D3     // 电源