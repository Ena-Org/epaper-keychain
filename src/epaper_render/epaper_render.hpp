#pragma once

class EpaperRender
{
public:
	enum class DisplayMode : unsigned char
	{
		LogStreaming,
		SystemView,
	};

public:
	/**
	 * @brief 初始化电子纸显示模块
	 * 
	 * 该函数用于初始化电子纸显示屏的相关配置和资源。
	 * 包括初始化硬件接口、显示缓冲区、通信协议等。
	 * 必须在使用其他电子纸相关函数之前调用。
	 * 
	 * @note 这是一个静态方法，可以直接通过类名调用
	 * @warning 重复调用此函数可能导致资源重复初始化
	 * 
	 * @return void
	 * 
	 * @see epaper_render 类的其他方法
	 */
	static void init();

	/**
	 * @brief 主循环函数
	 * @details 此函数处理电子纸显示屏的主循环逻辑，持续处理
	 *          事件、更新显示内容和管理系统状态
	 * @note 此函数应在主程序的主循环中调用
	 * @see loop()
	 */
	static void loop();

private:
	///
	/// @brief 切换显示模式
	/// @details 根据传入的下一个模式参数，将设备切换到指定的显示模式。
	///          此函数为私有静态成员函数，仅供类内部使用。
	/// @param next_mode 要切换到的目标显示模式
	/// @return 无返回值
	///
	/// @note 此函数为内部实现细节，不应由外部代码直接调用
	///
	static void switch_mode_(DisplayMode next_mode);
};