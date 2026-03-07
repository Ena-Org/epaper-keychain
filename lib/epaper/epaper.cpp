#include "epaper.hpp"

GxEPD2_BW<EPaperDriver, EPaperDriver::HEIGHT> display(
    EPaperDriver(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

/**
 * @brief 初始化电子纸屏幕
 * 
 * 该函数执行电子纸屏幕的初始化流程，包括:
 * 1. 配置电源引脚为输出模式并拉高电源
 * 2. 延迟200ms以确保电源稳定
 * 3. 打印调试信息，显示所有相关GPIO引脚配置
 * 4. 初始化SPI通信接口
 * 5. 初始化显示屏驱动程序
 * 6. 设置屏幕旋转角度为90度
 * 7. 设置为全窗口更新模式
 * 
 * @note 该函数应在程序启动时调用，用于准备电子纸屏幕的所有硬件和软件配置
 * @see EPD_PWR_PIN, EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN, EPD_SCK_PIN, EPD_MOSI_PIN
 */
void EPaper::init()
{
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    delay(200); 

    Serial.printf("EPD pins -> PWR:%d CS:%d DC:%d RST:%d BUSY:%d SCK:%d MOSI:%d\n",
                  EPD_PWR_PIN, EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN, EPD_SCK_PIN, EPD_MOSI_PIN);

    SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN);
    display.init(115200, true, 2, false);
    Serial.println(">>> EPaper init done (power on + SPI begin + display.init)");
    display.setRotation(1);
    display.setFullWindow();
}

/**
 * @brief 绘制基础页面
 * 
 * 该函数在电子墨水屏上绘制基础页面内容，包括：
 * - 在坐标(10, 40)处显示欢迎信息"Hello, Ena 小猫!"，字体大小为2
 * - 在坐标(10, 80)处显示屏幕型号信息"2.9\" e-Paper (SKU:12563)"，字体大小为1
 * 
 * 所有文本都使用黑色(GxEPD_BLACK)进行显示。
 * 
 * @return void
 * 
 * @note 此函数假设display对象已初始化
 */
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

/**
 * @brief 显示欢迎页面
 * 
 * @details 该函数用于在电子墨水屏上显示欢迎页面。
 *          函数首先设置全屏窗口模式，然后进入分页显示循环。
 *          在第一页中，先填充白色背景，再绘制基础页面内容。
 *          最后再绘制一次基础页面以确保内容正确显示。
 * 
 * @return void
 * 
 * @note 该函数会完全刷新屏幕显示，可能耗时较长。
 */
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

/**
 * @brief 显示电池电量指示器
 * 
 * 在电子纸屏幕上绘制一个电池图形，并显示当前电量百分比。
 * 电池图形包含一个矩形电池主体和一个小的正极标记。
 * 电池内部用黑色填充表示当前电量水平。
 * 
 * @param percent 电池电量百分比，范围为 0-100
 *                如果传入值大于 100，则自动限制为 100
 * 
 * @note 函数会完整刷新屏幕显示，包括：
 *       - 位置 (10, 20) 处绘制 80x30 像素的电池框
 *       - 在电池右侧绘制正极标记
 *       - 根据百分比填充电池内部
 *       - 在屏幕下方 (10, 70) 显示百分比数值
 * 
 * @return void
 */
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

/**
 * @brief 显示累积测试动画
 * 
 * 该函数通过逐步增加黑色矩形块的数量来演示电子纸屏幕的累积效果。
 * 每一步会在屏幕上绘制越来越多的方形块，用于测试屏幕的显示性能。
 * 
 * @param steps 显示的步骤总数，每个步骤会增加一个新的矩形块
 * @param delayMs 每个步骤之间的延迟时间，单位为毫秒
 * 
 * @details
 * - 使用全屏窗口模式进行显示
 * - 第一步显示1个矩形块，最后一步显示(steps)个矩形块
 * - 矩形块从左到右排列，基础位置为(10, 110)
 * - 每个矩形块尺寸为8x8像素，块之间间隔为4像素
 * - 每个步骤下方会显示当前步骤数
 * 
 * @return void
 * 
 * @note 该函数会阻塞执行，总耗时为 steps * delayMs 毫秒
 */
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

/**
 * @brief 显示黑白测试画面
 * 
 * 在电子纸屏幕上显示黑白测试内容。此函数将屏幕填充为白色背景，
 * 并在屏幕中央显示黑色的"E-paper OK"文本，用于验证电子纸显示
 * 功能是否正常工作。
 * 
 * @details
 * - 设置全屏刷新区域
 * - 填充屏幕为白色背景
 * - 设置文本颜色为黑色
 * - 设置文本大小为2倍
 * - 在坐标(20, 70)处显示"E-paper OK"文本
 * 
 * @return void
 * 
 * @note 此函数为阻塞操作，会等待屏幕刷新完成
 */
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


