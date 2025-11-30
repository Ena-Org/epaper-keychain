// lib/epaper/epaper.cpp
#include "epaper.hpp"

// 在这里真正定义全局 display 对象
GxEPD2_BW<EPaperDriver, EPaperDriver::HEIGHT> display(
    EPaperDriver(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)
);

// GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> display(
//   GxEPD2_290_T94_V2(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)
// );

void EPaper::init()
{
    // 如果你用的是 XIAO 默认 SPI 引脚，这里直接 SPI.begin() 就行
    SPI.begin();  // 用默认 SCK / MOSI / MISO

    // Waveshare 2.9" V2 官方建议用短一点的 reset 脉冲，
    // GxEPD2 里可以用这种带参数的 init（不写参数也能跑）：
    // display.init(115200, true, 2, false);

    display.init();
    display.setRotation(1);      // 横竖屏自己调
    display.setFullWindow();

    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());
}

void EPaper::showHello()
{
    display.setFullWindow();
    display.setTextColor(GxEPD_BLACK);

    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        display.setCursor(10, 40);
        display.setTextSize(2);
        display.print("Hello, Ena!");

        display.setCursor(10, 80);
        display.setTextSize(1);
        display.print("2.9\" e-Paper (SKU:12563)");
    } while (display.nextPage());
}

void EPaper::showBattery(uint8_t percent)
{
    if (percent > 100) percent = 100;

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        int x = 10, y = 20, w = 80, h = 30;

        display.drawRect(x, y, w, h, GxEPD_BLACK);
        display.fillRect(x + w, y + h / 4, 5, h / 2, GxEPD_BLACK);

        int innerW = (w - 4) * percent / 100;
        display.fillRect(x + 2, y + 2, innerW, h - 4, GxEPD_BLACK);

        display.setCursor(10, 70);
        display.setTextSize(2);
        display.setTextColor(GxEPD_BLACK);
        display.printf("%3d%%", percent);
    } while (display.nextPage());
}
