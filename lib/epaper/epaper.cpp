#include "epaper.hpp"

GxEPD2_BW<EPaperDriver, EPaperDriver::HEIGHT> display(
    EPaperDriver(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

void EPaper::init()
{
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    delay(10);

    SPI.begin();

    display.init(115200, true, 2, false);
    display.setRotation(1);
    display.setFullWindow();

    // display.firstPage();
    // do
    // {
    //     display.fillScreen(GxEPD_WHITE);
    // } while (display.nextPage());
}

// void EPaper::showHello()
// {
//     display.setFullWindow();
//     display.setTextColor(GxEPD_BLACK);

//     display.firstPage();
//     do
//     {
//         display.fillScreen(GxEPD_WHITE);

//         display.setCursor(10, 40);
//         display.setTextSize(2);
//         display.print("Hello, Ena 小猫!");

//         display.setCursor(10, 80);
//         display.setTextSize(1);
//         display.print("2.9\" e-Paper (SKU:12563)");
//     } while (display.nextPage());
// }

// void EPaper::showBattery(uint8_t percent)
// {
//     if (percent > 100)
//         percent = 100;

//     display.setFullWindow();
//     display.firstPage();
//     do
//     {
//         display.fillScreen(GxEPD_WHITE);

//         int x = 10, y = 20, w = 80, h = 30;

//         display.drawRect(x, y, w, h, GxEPD_BLACK);
//         display.fillRect(x + w, y + h / 4, 5, h / 2, GxEPD_BLACK);

//         int innerW = (w - 4) * percent / 100;
//         display.fillRect(x + 2, y + 2, innerW, h - 4, GxEPD_BLACK);

//         display.setCursor(10, 70);
//         display.setTextSize(2);
//         display.setTextColor(GxEPD_BLACK);
//         display.printf("%3d%%", percent);
//     } while (display.nextPage());
// }

// void EPaper::showPowerAnimation(uint8_t loops, uint16_t delayMs)
// {
//     display.setFullWindow();
//     display.setTextColor(GxEPD_BLACK);

//     // 外层循环：整体动画重复几轮
//     for (uint8_t round = 0; round < loops; ++round)
//     {
//         // 内层三帧：1 -> 2 -> 3
//         for (uint8_t frame = 1; frame <= 3; ++frame)
//         {
//             display.firstPage();
//             do
//             {
//                 display.fillScreen(GxEPD_WHITE);

//                 // 顶部提示文字
//                 display.setTextSize(1);
//                 display.setCursor(10, 20);
//                 display.print("EPD Power Test");

//                 display.setCursor(10, 32);
//                 display.print("Frame: ");
//                 display.print(frame);

//                 // 中间一个超大的数字，方便离远了也能看
//                 display.setTextSize(5);
//                 display.setCursor(40, 100);
//                 display.print(frame);

//             } while (display.nextPage());

//             delay(delayMs);
//         }
//     }
// }

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
    // do
    // {
    //     display.fillScreen(GxEPD_WHITE);
    //     drawBasePage();
    // } while (display.nextPage());
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

    // 从 0 到 steps-1，一步步往原有页面上叠东西
    for (uint8_t step = 0; step < steps; ++step)
    {
        display.firstPage();
        do
        {
            // 1. 画底色
            display.fillScreen(GxEPD_WHITE);

            // 2. 画原有页面（Hello 那一套）
            drawBasePage();

            // 3. 画“累加内容”：比如一排往右长的小方块
            //    每一帧多一个，看起来就是在原画面上一直增加
            int baseX = 10;
            int baseY = 110;
            int size = 8;
            int gap = 4;

            for (uint8_t i = 0; i <= step; ++i)
            {
                int x = baseX + i * (size + gap);
                display.fillRect(x, baseY, size, size, GxEPD_BLACK);
            }

            // 你也可以在下方加一个小数字显示当前 step
            display.setTextSize(1);
            display.setCursor(10, 140);
            display.print("Step: ");
            display.print(step);

        } while (display.nextPage());

        delay(delayMs);
    }
}