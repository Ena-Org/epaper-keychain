#include "logger.hpp"
#include <stdarg.h>

namespace Logger
{
	static Level s_level = INFO;
	static Print *s_out = &Serial;

	void setOutput(Print *out)
	{
		if (out)
			s_out = out;
	}

	void setLevel(Level level)
	{
		s_level = level;
	}

	Level level()
	{
		return s_level;
	}

	static char levelChar(Level lvl)
	{
		switch (lvl)
		{
		case TRACE:
			return 'T';
		case DEBUG:
			return 'D';
		case INFO:
			return 'I';
		case WARN:
			return 'W';
		case ERROR:
			return 'E';
		default:
			return '?';
		}
	}

	void log(Level lvl, const char *tag, const String &msg)
	{
		if (lvl < s_level || !s_out)
			return;

		const unsigned long ms = millis();
		s_out->printf("[%c][%s][%lums] %s\n", levelChar(lvl), tag ? tag : "-", ms, msg.c_str());
	}

	void logf(Level lvl, const char *tag, const char *fmt, ...)
	{
		if (lvl < s_level || !s_out || !fmt)
			return;

		char buf[256];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		log(lvl, tag, String(buf));
	}
}
