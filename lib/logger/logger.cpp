#include "logger.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
	constexpr size_t kLogMsgBufferSize = 256;
	constexpr size_t kLogLineBufferSize = 384;

	uint32_t default_now_ms()
	{
		return millis();
	}

	Logger::LoggerConfig g_config;
	uint32_t (*g_now_ms)() = default_now_ms;
	std::vector<uint8_t> g_history{};
	size_t g_history_head = 0;
	size_t g_history_size = 0;
	size_t g_history_capacity = 0;

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

	size_t format_prefix(char *out, size_t cap, Logger::LogLevel level, const char *tag, uint32_t now)
	{
		if (out == nullptr || cap == 0)
		{
			return 0;
		}

		size_t pos = 0;
		bool has_prefix = false;

		if (g_config.show_time)
		{
			const int ret = snprintf(out + pos, cap - pos, "[%lu]", static_cast<unsigned long>(now));
			if (ret > 0)
			{
				pos += static_cast<size_t>(ret);
			}
			has_prefix = true;
		}

		if (g_config.show_level)
		{
			const int ret = snprintf(out + pos, cap - pos, "%s[%s]", has_prefix ? " " : "", level_text(level));
			if (ret > 0)
			{
				pos += static_cast<size_t>(ret);
			}
			has_prefix = true;
		}

		if (g_config.show_tag && tag != nullptr && tag[0] != '\0')
		{
			const int ret = snprintf(out + pos, cap - pos, "%s[%s]", has_prefix ? " " : "", tag);
			if (ret > 0)
			{
				pos += static_cast<size_t>(ret);
			}
			has_prefix = true;
		}

		if (has_prefix)
		{
			const int ret = snprintf(out + pos, cap - pos, " ");
			if (ret > 0)
			{
				pos += static_cast<size_t>(ret);
			}
		}

		if (pos >= cap)
		{
			pos = cap - 1;
		}
		out[pos] = '\0';
		return pos;
	}

	void history_reset(size_t capacity)
	{
		g_history_capacity = capacity;
		g_history_head = 0;
		g_history_size = 0;
		g_history.clear();
		if (g_history_capacity > 0)
		{
			g_history.resize(g_history_capacity);
		}
	}

	void history_append(const uint8_t *data, size_t len)
	{
		if (g_history_capacity == 0 || data == nullptr || len == 0)
		{
			return;
		}

		if (len > g_history_capacity)
		{
			data += (len - g_history_capacity);
			len = g_history_capacity;
		}

		for (size_t i = 0; i < len; ++i)
		{
			if (g_history_size < g_history_capacity)
			{
				const size_t idx = (g_history_head + g_history_size) % g_history_capacity;
				g_history[idx] = data[i];
				++g_history_size;
			}
			else
			{
				g_history[g_history_head] = data[i];
				g_history_head = (g_history_head + 1) % g_history_capacity;
			}
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
	set_history_capacity(g_config.history_bytes);
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

	const uint32_t now = (g_now_ms != nullptr) ? g_now_ms() : millis();

	if (g_config.color)
	{
		out->print(level_color(level));
	}

	char prefix[kLogLineBufferSize];
	const size_t prefix_len = format_prefix(prefix, sizeof(prefix), level, tag, now);
	if (prefix_len > 0)
	{
		out->write(reinterpret_cast<const uint8_t *>(prefix), prefix_len);
	}

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

	char line[kLogLineBufferSize];
	size_t line_pos = 0;
	if (prefix_len > 0)
	{
		const size_t copy = (prefix_len < sizeof(line) - 1) ? prefix_len : (sizeof(line) - 1);
		std::memcpy(line + line_pos, prefix, copy);
		line_pos += copy;
	}
	if (n < 0)
	{
		const char err_text[] = "<format-error>";
		const size_t copy = (sizeof(err_text) - 1 < sizeof(line) - 1 - line_pos)
		                          ? (sizeof(err_text) - 1)
		                          : (sizeof(line) - 1 - line_pos);
		std::memcpy(line + line_pos, err_text, copy);
		line_pos += copy;
	}
	else
	{
		const size_t msg_len = std::strlen(message);
		const size_t copy = (msg_len < sizeof(line) - 1 - line_pos) ? msg_len : (sizeof(line) - 1 - line_pos);
		std::memcpy(line + line_pos, message, copy);
		line_pos += copy;
		if (static_cast<size_t>(n) >= sizeof(message) && line_pos + 3 < sizeof(line))
		{
			line[line_pos++] = '.';
			line[line_pos++] = '.';
			line[line_pos++] = '.';
		}
	}

	if (line_pos >= sizeof(line))
	{
		line_pos = sizeof(line) - 1;
	}
	line[line_pos] = '\0';

	history_append(reinterpret_cast<const uint8_t *>(line), line_pos);
	const uint8_t newline[] = {'\r', '\n'};
	history_append(newline, sizeof(newline));
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

void Logger::set_history_capacity(size_t bytes)
{
	history_reset(bytes);
}

size_t Logger::history_capacity()
{
	return g_history_capacity;
}

size_t Logger::history_size()
{
	return g_history_size;
}

void Logger::clear_history()
{
	history_reset(g_history_capacity);
}

size_t Logger::dump_history(Stream &out)
{
	if (g_history_capacity == 0 || g_history_size == 0)
	{
		return 0;
	}

	size_t written = 0;
	for (size_t i = 0; i < g_history_size; ++i)
	{
		const size_t idx = (g_history_head + i) % g_history_capacity;
		written += out.write(&g_history[idx], 1);
	}
	return written;
}

size_t Logger::copy_history(uint8_t *out, size_t maxLen)
{
	if (out == nullptr || maxLen == 0 || g_history_capacity == 0 || g_history_size == 0)
	{
		return 0;
	}

	const size_t to_copy = (g_history_size < maxLen) ? g_history_size : maxLen;
	for (size_t i = 0; i < to_copy; ++i)
	{
		const size_t idx = (g_history_head + i) % g_history_capacity;
		out[i] = g_history[idx];
	}
	return to_copy;
}
