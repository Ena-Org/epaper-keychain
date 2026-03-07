#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
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
     * @details 逐步显示累积的内容，用于测试电子纸显示效果。每一步之间会有延迟。
     * @param steps 累积测试的步数，默认为10步
     * @param delayMs 每一步之间的延迟时间（毫秒），默认为500毫秒
     * @note 这是一个静态方法，可以直接通过类名调用
     */
    static void showAccumulatingTest(uint8_t steps = 10, uint16_t delayMs = 500);

    /**
     * @brief 显示黑白测试画面
     * 
     * 该函数用于在电子纸屏幕上显示黑白测试图案,可用于验证屏幕
     * 的显示效果、对比度和像素点的正常工作状态。
     * 
     * @note 这是一个静态成员函数,可以直接通过类名调用,无需创建实例对象。
     * 
     * @return void
     * 
     * @see showGrayScaleTest()
     */
    static void showBlackWhiteTest();

private:
    /**
     * @brief 绘制基础页面
     * 
     * 该静态方法用于绘制电子纸屏幕的基础页面。基础页面通常包含
     * 应用程序的默认布局、背景元素或通用的UI框架。
     * 
     * @note 这是一个静态方法，可以直接通过类名调用，无需创建实例。
     * 
     * @see drawBasePage()
     */
    static void drawBasePage();
};
