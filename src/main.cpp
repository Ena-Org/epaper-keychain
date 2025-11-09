// Core & project headers
#include <Arduino.h>
#include "pins.hpp"
#include <led.hpp>
// #include <Arduino_GFX_Library.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// 固定到 Round Display 的引脚
// #define TFT_CS   D1
// #define TFT_DC   D3
// #define TFT_BL   D6
// #define TFT_SCK  D8
// #define TFT_MOSI D10
// // 没有用到 MISO，传 -1

// // ESP32 专用 SPI 总线 + GC9A01 圆屏
// Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, -1);
// Arduino_GFX *gfx    = new Arduino_GC9A01(bus, -1 /*RST 没接就 -1*/, 0 /*rotation*/, true /*IPS*/);


#define TFT_CS   D1
#define TFT_DC   D3
#define TFT_BL   D6
#define TFT_SCK  D8
#define TFT_MOSI D10
#define TFT_RST  -1   // 没接复位脚就 -1

SPIClass spi = SPI;
Adafruit_GC9A01A tft(&spi, TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // 背光脚（D6），必须拉高
  // 背光必须拉高，不然你永远觉得“没点亮”
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 指定 SPI 针脚，然后初始化屏
  spi.begin(TFT_SCK, /*MISO*/ -1, TFT_MOSI);
  tft.begin();           // 分辨率默认 240x240
  tft.setRotation(0);

  tft.fillScreen(GC9A01A_BLACK);
  tft.fillRect(0, 0, 240, 240, GC9A01A_GREEN);
  tft.setCursor(30, 110);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);
  tft.print("GC9A01 OK");
}

void loop() {
}
