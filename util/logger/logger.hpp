 #pragma once
#include <Arduino.h>

namespace Logger
{
	enum Level
	{
		TRACE = 0,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		NONE
	};

	void setOutput(Print *out);   // 默认 Serial
	void setLevel(Level level);    // 设置全局日志级别
	Level level();                 // 获取当前级别

	void logf(Level level, const char *tag, const char *fmt, ...);
	void log(Level level, const char *tag, const String &msg);
}

// 便捷宏
#define LOGT(tag, fmt, ...) Logger::logf(Logger::TRACE, tag, fmt, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...) Logger::logf(Logger::DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOGI(tag, fmt, ...) Logger::logf(Logger::INFO, tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) Logger::logf(Logger::WARN, tag, fmt, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...) Logger::logf(Logger::ERROR, tag, fmt, ##__VA_ARGS__)
