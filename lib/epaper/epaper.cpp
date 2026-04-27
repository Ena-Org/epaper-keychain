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
 * @brief 绘制电子墨水屏的基础页面
 * 
 * 该函数在显示屏上绘制基础页面，包括设置文本颜色、文本大小和光标位置，
 * 然后在屏幕上显示"E-paper Keychain"文本。
 * 
 * @details
 * - 文本颜色：黑色 (GxEPD_BLACK)
 * - 文本大小：2
 * - 光标位置：X=8, Y=32
 * - 显示内容："E-paper Keychain"
 * 
 * @return void
 * 
 * @note 调用此函数前，请确保显示屏已正确初始化
 */
void EPaper::drawBasePage()
{
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(2);
    display.setCursor(8, 32);
    display.print("E-paper Keychain");
}

/**
 * @brief 显示欢迎界面
 * 
 * 该函数用于在电子墨水屏上显示准备就绪的欢迎界面。
 * 首先填充屏幕为白色，绘制基础页面，然后在屏幕上显示"Ready"文本。
 * 
 * @details
 * 函数执行流程：
 * - 设置全屏显示窗口
 * - 进入第一页
 * - 填充屏幕背景为白色
 * - 绘制基础页面内容
 * - 设置文本大小为1
 * - 在坐标(8, 56)处显示"Ready"文本
 * - 循环更新屏幕页面直到完成
 * 
 * @return void
 */
void EPaper::showHello()
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        drawBasePage();
        display.setTextSize(1);
        display.setCursor(8, 56);
        display.print("Ready");
    } while (display.nextPage());
}

/**
 * @brief 显示累积测试动画
 * 
 * 该函数用于测试电子纸显示屏的增量式绘制功能。它会逐步在屏幕上
 * 绘制黑色方块，每一步增加一个新的方块，形成累积效果，用于验证
 * 显示屏的刷新和绘制性能。
 * 
 * @param steps 要显示的步数，即最终会绘制的黑色方块个数（范围：0-255）
 * @param delayMs 每一步之间的延迟时间，单位为毫秒
 * 
 * @details
 * - 函数会清空屏幕并绘制基础页面（drawBasePage）
 * - 在每一步中，从左到右累积绘制8x8像素的黑色方块
 * - 方块之间的间距为10像素，起始位置为x=8，y=90
 * - 使用全窗口模式刷新显示屏
 * 
 * @return void
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
            for (uint8_t i = 0; i <= step; ++i)
            {
                const int x = 8 + static_cast<int>(i) * 10;
                display.fillRect(x, 90, 8, 8, GxEPD_BLACK);
            }
        } while (display.nextPage());
        delay(delayMs);
    }
}

/**
 * @brief 显示黑白测试图案
 * 
 * 该函数在电子纸屏幕上显示黑白对比测试图案。屏幕被分为左右两部分：
 * - 左半部分：黑色背景，显示白色的"BLACK"文本
 * - 右半部分：白色背景，显示黑色的"WHITE"文本
 * 
 * 该测试图案用于验证电子纸显示器的黑白显示效果和文本渲染能力。
 * 
 * @return void
 */
void EPaper::showBlackWhiteTest()
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.fillRect(0, 0, display.width() / 2, display.height(), GxEPD_BLACK);
        display.setTextColor(GxEPD_WHITE);
        display.setTextSize(1);
        display.setCursor(8, 20);
        display.print("BLACK");
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(display.width() / 2 + 8, 20);
        display.print("WHITE");
    } while (display.nextPage());
}

/**
 * @brief 在电子纸显示屏上显示日志行列表
 * 
 * 此函数将日志信息以格式化的方式显示在电子纸显示屏上，包括标题、框架ID和多行日志内容。
 * 函数会自动处理换行和屏幕边界，超出屏幕高度的日志行将被截断。
 * 
 * @param title 显示屏幕顶部的标题文字。若为 nullptr，将使用默认标题 "Logs"
 * @param lines 包含要显示的日志行的字符串向量。每个元素代表一行日志内容
 * @param frame_id 当前帧的ID编号，将在标题右侧显示为 "#<frame_id>" 的格式
 * 
 * @note 
 * - 标题字体大小为 2，日志内容字体大小为 1
 * - 每行日志间距为 14 像素
 * - 超出屏幕高度的日志行将被自动截断，不会显示
 * - 函数会自动刷新电子纸显示屏以显示结果
 * 
 * @return void
 */
void EPaper::showLogLines(const char *title, const std::vector<String> &lines, uint32_t frame_id)
{
    const char *safe_title = (title != nullptr) ? title : "Logs";

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        display.setTextSize(2);
        display.setCursor(8, 20);
        display.print(safe_title);

        display.setTextSize(1);
        display.setCursor(230, 20);
        display.print("#");
        display.print(static_cast<unsigned long>(frame_id));

        display.drawFastHLine(0, 26, display.width(), GxEPD_BLACK);

        int y = 42;
        const int line_height = 14;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            display.setCursor(8, y);
            display.print(lines[i]);
            y += line_height;
            if (y > static_cast<int>(display.height()) - 2)
            {
                break;
            }
        }
    } while (display.nextPage());
}

/**
 * @brief 显示系统占位符界面
 * 
 * 在电子墨水屏上显示一个系统占位符界面，包含标题和正文内容。
 * 如果传入的参数为空，将使用默认值。
 * 
 * @param title 界面标题文本指针。若为 nullptr，则使用默认标题 "System"
 * @param body 界面正文文本指针。若为 nullptr，则使用默认正文 "Placeholder"
 * 
 * @details
 * 界面布局如下：
 * - 清空屏幕并填充为白色背景
 * - 在顶部（Y=32）以 2 倍字体大小显示标题
 * - 在标题下方显示黑色分割线（Y=38）
 * - 在分割线下方（Y=62）以 1 倍字体大小显示正文
 * 
 * @return void
 * 
 * @note 此函数使用全屏更新模式显示内容
 */
void EPaper::showSystemPlaceholder(const char *title, const char *body)
{
    const char *safe_title = (title != nullptr) ? title : "System";
    const char *safe_body = (body != nullptr) ? body : "Placeholder";

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        display.setTextSize(2);
        display.setCursor(8, 32);
        display.print(safe_title);

        display.drawFastHLine(0, 38, display.width(), GxEPD_BLACK);

        display.setTextSize(1);
        display.setCursor(8, 62);
        display.print(safe_body);
    } while (display.nextPage());
}
