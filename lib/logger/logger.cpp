#include "logger.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace
{
	constexpr size_t kLogMsgBufferSize = 256;

	uint32_t default_now_ms()
	{
		return millis();
	}

	Logger::LoggerConfig g_config;
	uint32_t (*g_now_ms)() = default_now_ms;

	const char *level_text(Logger::LogLevel level)
	{
		switch (level)
		{
		case Logger::LogLevel::Error:
			return "ERROR";
		case Logger::LogLevel::Warn:
			return "WARN";
		case Logger::LogLevel::Info:
			return "INFO";
		case Logger::LogLevel::Debug:
			return "DEBUG";
		case Logger::LogLevel::Verbose:
			return "VERBOSE";
		case Logger::LogLevel::Off:
			return "OFF";
		default:
			return "UNKNOWN";
		}
	}

	const char *level_color(Logger::LogLevel level)
	{
		switch (level)
		{
		case Logger::LogLevel::Error:
			return "\x1b[31m";
		case Logger::LogLevel::Warn:
			return "\x1b[33m";
		case Logger::LogLevel::Info:
			return "\x1b[32m";
		case Logger::LogLevel::Debug:
			return "\x1b[36m";
		case Logger::LogLevel::Verbose:
			return "\x1b[37m";
		default:
			return "\x1b[0m";
		}
	}

	bool should_output(Logger::LogLevel level)
	{
		if (level == Logger::LogLevel::Off || g_config.level == Logger::LogLevel::Off)
		{
			return false;
		}

		return static_cast<uint8_t>(level) <= static_cast<uint8_t>(g_config.level);
	}

	void print_prefix(Logger::LogLevel level, const char *tag)
	{
		Stream *out = g_config.out;
		if (out == nullptr)
		{
			return;
		}

		bool has_prefix = false;

		if (g_config.show_time)
		{
			const uint32_t now = (g_now_ms != nullptr) ? g_now_ms() : millis();
			out->print('[');
			out->print(now);
			out->print(']');
			has_prefix = true;
		}

		if (g_config.show_level)
		{
			if (has_prefix)
			{
				out->print(' ');
			}
			out->print('[');
			out->print(level_text(level));
			out->print(']');
			has_prefix = true;
		}

		if (g_config.show_tag && tag != nullptr && tag[0] != '\0')
		{
			if (has_prefix)
			{
				out->print(' ');
			}
			out->print('[');
			out->print(tag);
			out->print(']');
			has_prefix = true;
		}

		if (has_prefix)
		{
			out->print(' ');
		}
	}
}

void Logger::init(const LoggerConfig &cfg)
{
	g_config = cfg;
	if (g_config.out == nullptr)
	{
		g_config.out = &Serial;
	}
	if (g_now_ms == nullptr)
	{
		g_now_ms = default_now_ms;
	}
}

void Logger::set_level(LogLevel level)
{
	g_config.level = level;
}

Logger::LogLevel Logger::get_level()
{
	return g_config.level;
}

void Logger::set_time_provider(uint32_t (*now_ms)())
{
	g_now_ms = (now_ms != nullptr) ? now_ms : default_now_ms;
}

void Logger::log(LogLevel level, const char *tag, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vlog(level, tag, fmt, ap);
	va_end(ap);
}

void Logger::vlog(LogLevel level, const char *tag, const char *fmt, va_list ap)
{
	Stream *out = g_config.out;
	if (out == nullptr || fmt == nullptr || !should_output(level))
	{
		return;
	}

	if (g_config.color)
	{
		out->print(level_color(level));
	}

	print_prefix(level, tag);

	char message[kLogMsgBufferSize];
	const int n = vsnprintf(message, sizeof(message), fmt, ap);

	if (n < 0)
	{
		out->print("<format-error>");
	}
	else
	{
		out->print(message);
		if (static_cast<size_t>(n) >= sizeof(message))
		{
			out->print("...");
		}
	}

	if (g_config.color)
	{
		out->print("\x1b[0m");
	}

	out->print("\r\n");
}

void Logger::hexdump(LogLevel level, const char *tag, const void *data, size_t len, size_t bytes_per_line)
{
	if (!should_output(level))
	{
		return;
	}

	if (data == nullptr)
	{
		log(level, tag, "<null>");
		return;
	}

	if (len == 0)
	{
		log(level, tag, "<empty>");
		return;
	}

	if (bytes_per_line == 0)
	{
		bytes_per_line = 16;
	}

	const uint8_t *bytes = static_cast<const uint8_t *>(data);

	for (size_t offset = 0; offset < len; offset += bytes_per_line)
	{
		char line[220];
		size_t written = 0;

		int ret = snprintf(line + written, sizeof(line) - written, "%04X: ", static_cast<unsigned int>(offset));
		if (ret < 0)
		{
			continue;
		}
		written += static_cast<size_t>(ret);

		const size_t row_len = ((offset + bytes_per_line) <= len) ? bytes_per_line : (len - offset);

		for (size_t i = 0; i < bytes_per_line; ++i)
		{
			if (written + 4 >= sizeof(line))
			{
				break;
			}

			if (i < row_len)
			{
				ret = snprintf(line + written, sizeof(line) - written, "%02X ", bytes[offset + i]);
			}
			else
			{
				ret = snprintf(line + written, sizeof(line) - written, "   ");
			}

			if (ret < 0)
			{
				break;
			}
			written += static_cast<size_t>(ret);
		}

		if (written + 4 < sizeof(line))
		{
			ret = snprintf(line + written, sizeof(line) - written, "|");
			if (ret > 0)
			{
				written += static_cast<size_t>(ret);
			}
		}

		for (size_t i = 0; i < row_len; ++i)
		{
			if (written + 2 >= sizeof(line))
			{
				break;
			}

			const unsigned char c = bytes[offset + i];
			line[written++] = static_cast<char>(std::isprint(c) ? c : '.');
			line[written] = '\0';
		}

		if (written + 2 < sizeof(line))
		{
			line[written++] = '|';
			line[written] = '\0';
		}

		log(level, tag, "%s", line);
	}
}

void Logger::flush()
{
	if (g_config.out != nullptr)
	{
		g_config.out->flush();
	}
}
