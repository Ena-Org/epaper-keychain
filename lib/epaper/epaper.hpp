#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <vector>
#include "pins.hpp"

/**
 * @typedef EPaperDriver
 * @brief 电子纸显示驱动器类型定义
 *
 * 将 GxEPD2_290_T94_V2 类型别名为 EPaperDriver，用于简化代码中对该电子纸显示器驱动的引用。
 *
 * @details
 * - GxEPD2: GxEPD2 库提供的电子墨水屏驱动框架
 * - 290: 2.9 英寸显示屏尺寸
 * - T94: 芯片型号为 T94
 * - V2: 版本 2
 *
 * 该驱动支持电子纸显示的刷新、部分更新等功能，适用于低功耗显示应用场景。
 */
using EPaperDriver = GxEPD2_290_T94_V2;

/**
 * @brief 电子墨水屏显示对象
 *
 * 全局电子墨水屏显示驱动实例，支持黑白两色显示模式。
 * 使用 GxEPD2 库提供的黑白显示类模板，配合自定义的电子墨水屏驱动程序。
 *
 * @details
 * - 显示类型：GxEPD2_BW（黑白显示）
 * - 驱动器：EPaperDriver
 * - 显示高度：由 EPaperDriver::HEIGHT 定义
 *
 * @note 该对象需在使用前进行初始化设置，如调用 init() 方法。
 *
 * @see EPaperDriver
 * @see GxEPD2_BW
 */
extern GxEPD2_BW<EPaperDriver, EPaperDriver::HEIGHT> display;

class EPaper
{
public:
    /**
     * @brief 初始化电子墨水屏
     *
     * 该函数用于初始化电子墨水屏的硬件配置和相关参数，
     * 包括通信接口、显示缓冲区、刷新频率等设置。
     * 必须在使用电子墨水屏的其他功能之前调用此函数。
     *
     * @note 此函数应该只调用一次，通常在程序启动时执行
     *
     * @see deinit()
     */
    static void init();

    /**
     * @brief 显示欢迎信息到电子墨水屏
     *
     * 这是一个静态成员函数，用于在电子墨水屏上显示"Hello"欢迎信息。
     * 该函数处理所有必要的屏幕初始化和刷新操作。
     *
     * @note 调用此函数前，请确保电子墨水屏已正确初始化
     *
     * @return void
     */
    static void showHello();

    /**
     * @brief 显示电池电量百分比
     * @details 在电子纸屏幕上渲染并显示当前电池电量百分比
     * @param percent 电池电量百分比，范围为 0-100
     * @return void
     */
    static void showBattery(uint8_t percent);


    /**
     * @brief 显示累积测试
     * 
     * 该函数用于测试电子墨水屏的累积显示效果。通过多次刷新屏幕来验证显示
     * 的稳定性和图像的累积叠加效果。
     * 
     * @param steps 测试步数，表示执行测试的次数。默认值为10次。
     * @param delayMs 每个步骤之间的延迟时间，单位为毫秒。默认值为500毫秒。
     * 
     * @return 无返回值
     * 
     * @note 这是一个静态成员函数，可以直接通过类名调用。
     * @note 该函数主要用于调试和测试目的，不建议在生产环境中使用。
     * 
     */
    static void showAccumulatingTest(uint8_t steps = 10, uint16_t delayMs = 500);

    /**
     * @brief 显示黑白测试画面
     * 
     * 此函数用于显示电子墨水屏的黑白测试模式，通常用于测试屏幕的显示效果
     * 和颜色渲染能力。测试画面会显示纯黑色和纯白色的图案。
     * 
     * @note 这是一个静态函数，可以直接通过类名调用
     * 
     * @see updateDisplay()
     */
    static void showBlackWhiteTest();

    /**
     * @brief 显示日志行信息
     * 
     * 在电子纸屏幕上显示指定标题和日志行内容。
     * 
     * @param title 日志显示窗口的标题，C风格字符串指针
     * @param lines 包含日志行内容的字符串向量，每个元素代表一行日志
     * @param frame_id 帧ID，用于标识当前显示的帧或渲染批次
     * 
     * @note 这是一个静态方法，可直接通过类名调用
     * @note 日志行内容将根据屏幕尺寸自动调整显示格式
     */
    static void showLogLines(const char *title, const std::vector<String> &lines, uint32_t frame_id);


    /**
     * @brief 显示系统占位符
     * 
     * 在电子纸屏幕上显示系统级别的占位符内容，通常用于显示系统信息或临时提示。
     * 
     * @param title 占位符的标题文本，用于显示主要信息头
     * @param body 占位符的正文内容，用于显示详细信息或说明
     * 
     * @note 这是一个静态方法，可以直接通过类名调用，无需实例化对象
     * 
     * @see showSystemPlaceholder(const char*, const char*)
     */
    static void showSystemPlaceholder(const char *title, const char *body);

private:
    /**
     * @brief 绘制基础页面
     * 
     * 该函数用于绘制电子纸显示屏的基础页面布局。
     * 它初始化和设置显示屏上的基本UI元素，为后续
     * 的内容显示提供基础框架。
     * 
     * @note 这是一个静态函数，可以直接通过类名调用
     * @warning 调用此函数前，请确保显示屏已正确初始化
     * 
     * @see 相关功能请参考其他绘制函数
     */
    static void drawBasePage();
};