#include "epaper_render.hpp"
#include <Arduino.h>
#include "browser_connection_state.hpp"
#include "epaper.hpp"
#include "log_streaming.hpp"
#include "system_view.hpp"

namespace
{
	/**
	 * @brief 全局初始化状态标志
	 * @details 用于记录电子纸显示模块是否已初始化。
	 *          - true: 模块已初始化，可以正常使用
	 *          - false: 模块未初始化，需要调用初始化函数
	 * @note 线程操作此变量时应考虑同步问题
	 */
	bool g_initialized = false;

	/**
	 * @brief 全局显示模式变量
	 * @details 用于存储电子纸屏幕的当前显示模式，默认设置为系统视图模式。
	 *          该变量控制电子纸渲染器如何呈现内容和界面布局。
	 */
	static EpaperRender::DisplayMode g_mode = EpaperRender::DisplayMode::SystemView;

	/**
	 * @brief 全局日志流对象
	 *
	 * 用于处理应用程序的日志流输出。该全局对象可以在整个程序中使用，
	 * 用来记录、管理和输出各种日志信息。
	 *
	 * @note 作为全局变量，请确保在多线程环境下的线程安全性
	 */
	LogStreaming g_log_streaming;

	/**
	 * @brief 系统视图全局实例
	 *
	 * 这是一个全局的SystemView对象，用于管理和控制系统的显示视图。
	 * 该实例在整个应用程序的生命周期内维护系统的视觉表示状态。
	 *
	 * @note 该对象具有全局作用域，需谨慎使用以避免状态不一致
	 */
	SystemView g_system_view;
}

/**
 * @brief 初始化电子纸渲染模块
 *
 * 该函数负责初始化电子纸显示系统的所有组件。执行以下操作：
 * 1. 初始化EPaper底层驱动
 * 2. 初始化日志流模块
 * 3. 初始化系统视图模块
 * 4. 根据浏览器连接状态确定初始显示模式
 *    - 若已连接则使用日志流模式
 *    - 未连接则使用系统视图模式
 * 5. 切换到相应的显示模式
 * 6. 标记模块为已初始化状态
 *
 * @note 此函数应该在系统启动时调用一次
 * @see DisplayMode, BrowserConnectionState::is_connected(), switch_mode_()
 */
void EpaperRender::init()
{
	EPaper::init();
	g_log_streaming.init();
	g_system_view.init();

	const bool connected = BrowserConnectionState::is_connected();
	g_mode = connected ? DisplayMode::LogStreaming : DisplayMode::SystemView;
	switch_mode_(g_mode);
	g_initialized = true;
}

/**
 * @brief 电子纸显示渲染主循环
 * 
 * 该函数是渲染系统的核心循环函数，负责：
 * 1. 检查初始化状态，未初始化则直接返回
 * 2. 根据浏览器连接状态切换显示模式
 *    - 已连接：切换到日志流模式
 *    - 未连接：切换到系统视图模式
 * 3. 根据当前模式执行对应的更新和渲染操作
 *    - 日志流模式：更新日志流状态，按需渲染日志内容
 *    - 系统视图模式：按需渲染系统信息视图
 * 
 * @note 该函数应该在主程序循环中持续调用以保持显示内容的实时更新
 * @note 仅在g_initialized为true时才执行实际的渲染逻辑
 * 
 * @see switch_mode_() 模式切换函数
 * @see BrowserConnectionState::is_connected() 浏览器连接状态检查
 */
void EpaperRender::loop()
{
	if (!g_initialized)
		return;

	const bool connected = BrowserConnectionState::is_connected();
	const DisplayMode target_mode = connected ? DisplayMode::LogStreaming : DisplayMode::SystemView;
	if (target_mode != g_mode)
		switch_mode_(target_mode);

	const uint32_t now_ms = millis();
	if (g_mode == DisplayMode::LogStreaming)
	{
		g_log_streaming.tick(now_ms);
		if (g_log_streaming.needs_render())
			g_log_streaming.render();

		return;
	}

	if (g_system_view.needs_render())
		g_system_view.render();
}

/**
 * @brief 切换显示模式
 * @details 将全局显示模式切换为指定的模式。如果切换到日志流模式，则强制刷新日志流视图；
 *          否则标记系统视图为需要重绘状态。
 * @param next_mode 要切换到的目标显示模式
 * @see DisplayMode
 */
void EpaperRender::switch_mode_(DisplayMode next_mode)
{
	g_mode = next_mode;
	if (g_mode == DisplayMode::LogStreaming)
	{
		g_log_streaming.force_refresh();
		return;
	}

	g_system_view.mark_dirty();
}