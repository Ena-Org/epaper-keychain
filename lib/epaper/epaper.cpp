#include "epaper.hpp"

GxEPD2_BW<EPaperDriver, EPaperDriver::HEIGHT> display(
    EPaperDriver(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

void EPaper::init()
{
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    delay(200); // 给屏供电稳定时间，部分板卡需要更长上电延迟

    Serial.printf("EPD pins -> PWR:%d CS:%d DC:%d RST:%d BUSY:%d SCK:%d MOSI:%d\n",
                  EPD_PWR_PIN, EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN, EPD_SCK_PIN, EPD_MOSI_PIN);

    // Explicitly bind SPI to the wired pins on the XIAonO ESP32S3; the default pins do not match our PCB wiring
    SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN);
    display.init(115200, true, 2, false);
    Serial.println(">>> EPaper init done (power on + SPI begin + display.init)");
    display.setRotation(1);
    display.setFullWindow();
}

void EPaper::drawBasePage()
{
    display.setTextColor(GxEPD_BLACK);

    display.setCursor(10, 40);
    display.setTextSize(2);
    display.print("Hello, Ena 小猫!");

    display.setCursor(10, 80);
    display.setTextSize(1);
    display.print("2.9\" e-Paper (SKU:12563)");
}

void EPaper::showHello()
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        drawBasePage();
    } while (display.nextPage());
    drawBasePage();
}

void EPaper::showBattery(uint8_t percent)
{
    if (percent > 100)
        percent = 100;

    display.setFullWindow();
    display.firstPage();
    do
    {
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

void EPaper::showAccumulatingTest(uint8_t steps, uint16_t delayMs)
{
    display.setFullWindow();
    for (uint8_t step = 0; step < steps; ++step)
    {
        display.firstPage();
        do
        {
            display.fillScreen(GxEPD_WHITE);
            drawBasePage();
            int baseX = 10;
            int baseY = 110;
            int size = 8;
            int gap = 4;
            for (uint8_t i = 0; i <= step; ++i)
            {
                int x = baseX + i * (size + gap);
                display.fillRect(x, baseY, size, size, GxEPD_BLACK);
            }
            display.setTextSize(1);
            display.setCursor(10, 140);
            display.print("Step: ");
            display.print(step);

        } while (display.nextPage());
        delay(delayMs);
    }
}

void EPaper::showBlackWhiteTest()
{
    display.setFullWindow();

    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setTextSize(2);
        display.setCursor(20, 70);
        display.print("E-paper OK");
    } while (display.nextPage());
}


