#include "logger.hpp"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
	/**
	 * @brief 日志消息缓冲区的大小常量
	 * 
	 * 用于定义日志系统中单条日志消息的最大缓冲区大小。
	 * 该值决定了日志消息的最大长度限制为256字节。
	 * 
	 * @note 这是一个编译时常量,在程序运行时无法修改。
	 *       如果日志消息超过此大小,可能会被截断或导致缓冲区溢出。
	 */
	constexpr size_t kLogMsgBufferSize = 256;

	/**
	 * @brief 日志行缓冲区的大小
	 * 
	 * 定义了单条日志消息的最大缓冲区大小，单位为字节。
	 * 此常量用于限制日志输出的最大长度，防止缓冲区溢出。
	 * 
	 * @note 该值为编译期常量，在程序运行时不可修改。
	 */
	constexpr size_t kLogLineBufferSize = 384;

	/**
	 * @brief 获取当前系统运行时间（毫秒）
	 * @return uint32_t 系统启动以来经过的毫秒数
	 * @details 该函数是对 millis() 的封装，用于获取设备从启动到当前的运行时间。
	 *          主要用作日志记录的时间戳函数的默认实现。
	 */
	uint32_t default_now_ms()
	{
		return millis();
	}

	/**
	 * @brief 全局日志配置对象
	 * 
	 * 这是一个全局的Logger配置对象，用于存储和管理整个应用程序的日志系统配置。
	 * 该对象包含日志级别、输出格式、输出目标等配置参数。
	 * 
	 * @details
	 * - 在应用程序启动时初始化
	 * - 可被日志子系统的各个模块访问和使用
	 * - 提供统一的日志配置管理
	 * 
	 * @see Logger::LoggerConfig
	 */
	Logger::LoggerConfig g_config;
	
	uint32_t (*g_now_ms)() = default_now_ms;

	/**
	 * @brief 全局历史记录缓冲区
	 * @details 存储日志或事件的历史数据，使用uint8_t类型以支持二进制数据存储
	 * @note 该向量会随着新数据的添加而动态增长
	 */
	std::vector<uint8_t> g_history{};

	/**
	 * @brief 日志历史记录的头指针
	 * @details 用于跟踪循环缓冲区中日志历史记录的当前写入位置。
	 *          当新的新日志条目被添加时,此指针会更新指向下一个要写入的位置。
	 * @note 这是一个全局变量,用于管理日志缓冲区的循环写入。
	 */
	size_t g_history_head = 0;

	/**
	 * @brief 全局历史记录大小变量
	 * @details 用于记录当前历史记录缓冲区的大小，以字节为单位
	 * @note 该变量在日志系统初始化时被设置为0
	 */
	size_t g_history_size = 0;

	/**
	 * @brief 历史记录的容量
	 * 
	 * 全局变量，用于存储日志历史记录的最大容量。
	 * 该值决定了可以保存多少条历史日志记录。
	 * 当容量为0时，表示不保存任何历史记录。
	 */
	size_t g_history_capacity = 0;

	/**
	 * @brief 将日志级别枚举转换为对应的文本表示
	 * 
	 * @param level 日志级别枚举值
	 * @return const char* 返回日志级别的字符串表示，包括：
	 *         - "ERROR"   : 错误级别
	 *         - "WARN"    : 警告级别
	 *         - "INFO"    : 信息级别
	 *         - "DEBUG"   : 调试级别
	 *         - "VERBOSE" : 详细级别
	 *         - "OFF"     : 关闭级别
	 *         - "UNKNOWN" : 未知级别（默认情况）
	 * 
	 * @note 返回的字符串指针为常量，不应被修改
	 */
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

	/**
	 * @brief 根据日志级别返回对应的ANSI颜色代码
	 * 
	 * 该函数根据传入的日志级别返回相应的ANSI转义序列，
	 * 用于在终端输出中为不同的日志级别应用不同的文本颜色。
	 * 
	 * @param level 日志级别，可选值为：
	 *              - Logger::LogLevel::Error   (错误 - 红色)
	 *              - Logger::LogLevel::Warn    (警告 - 黄色)
	 *              - Logger::LogLevel::Info    (信息 - 绿色)
	 *              - Logger::LogLevel::Debug   (调试 - 青色)
	 *              - Logger::LogLevel::Verbose (详细 - 白色)
	 * 
	 * @return const char* ANSI颜色代码字符串指针，默认返回重置代码 "\x1b[0m"
	 * 
	 * @note 返回的颜色代码仅在支持ANSI的终端中有效
	 */
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

	/**
	 * @brief 判断是否应该输出日志
	 * 
	 * 根据传入的日志级别和全局配置的日志级别，判断是否应该输出该条日志。
	 * 
	 * @param level 要检查的日志级别
	 * 
	 * @return true 如果应该输出该级别的日志，返回 true
	 * @return false 如果不应该输出该级别的日志，或日志输出已关闭，返回 false
	 * 
	 * @note 当传入级别或全局配置级别为 Off 时，函数返回 false
	 * @note 日志级别通过数值比较，级别值越小越严格（优先级越高）
	 */
	bool should_output(Logger::LogLevel level)
	{
		if (level == Logger::LogLevel::Off || g_config.level == Logger::LogLevel::Off)
		{
			return false;
		}

		return static_cast<uint8_t>(level) <= static_cast<uint8_t>(g_config.level);
	}

	/**
	 * @brief 格式化日志前缀字符串
	 * 
	 * 根据配置生成日志前缀，包括时间戳、日志级别和标签等信息。
	 * 前缀各部分之间用空格分隔。
	 * 
	 * @param out 输出缓冲区指针，用于存储格式化后的前缀字符串
	 * @param cap 输出缓冲区的容量（字节数）
	 * @param level 日志级别
	 * @param tag 日志标签字符串指针，可为 nullptr
	 * @param now 时间戳（毫秒或其他单位）
	 * 
	 * @return 写入缓冲区的字符数（不包括终止符 '\0'），如果缓冲区无效则返回 0
	 * 
	 * @note
	 * - 如果 out 为 nullptr 或 cap 为 0，函数直接返回 0
	 * - 前缀的各个部分由全局配置 g_config 控制是否显示
	 * - 如果写入的内容超过缓冲区容量，会被截断到 cap-1 个字符
	 * - 输出字符串始终以 '\0' 结尾
	 * - tag 参数在为 nullptr 或空字符串时不会被添加到前缀中
	 */
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

	/**
	 * @brief 重置日志历史记录
	 * 
	 * 清空现有的日志历史记录，并根据指定的容量重新初始化历史记录缓冲区。
	 * 
	 * @param capacity 日志历史记录的新容量大小。如果容量为0，则不分配缓冲区。
	 * 
	 * @details
	 * 此函数会执行以下操作：
	 * - 设置历史记录的最大容量
	 * - 重置头指针到0
	 * - 重置历史记录大小为0
	 * - 清空现有的历史记录容器
	 * - 根据新容量调整容器大小
	 * 
	 * @note 容量为0时，历史记录功能将被禁用。
	 */
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

	/**
	 * @brief 将数据追加到历史记录缓冲区
	 * 
	 * @details 该函数实现了一个循环缓冲区的追加操作。当缓冲区未满时，
	 *          数据被添加到缓冲区末尾；当缓冲区满时，新数据会覆盖最旧的数据。
	 *          如果输入数据长度超过缓冲区容量，则只保留最后的 g_history_capacity 字节。
	 * 
	 * @param[in] data 指向要追加数据的指针
	 * @param[in] len  要追加数据的长度（字节数）
	 * 
	 * @return void
	 * 
	 * @note 如果 g_history_capacity 为 0、data 为空指针或 len 为 0，函数将直接返回
	 * @note 该函数不进行边界检查，调用者需确保缓冲区已正确初始化
	 */
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

/**
 * @brief 初始化日志记录器
 * 
 * @details 配置日志记录器的全局参数，包括输出流和时间获取函数。
 *          如果未指定输出流，则默认使用Serial；如果未指定时间函数，
 *          则使用默认的时间获取函数。同时根据配置设置历史记录缓冲区大小。
 * 
 * @param cfg 日志记录器配置参数，包含输出流、历史缓冲区大小等设置
 * 
 * @note 此函数应在使用日志记录器前调用一次
 * @note 如果cfg.out为nullptr，将自动设置为&Serial
 * @note 如果g_now_ms为nullptr，将自动设置为default_now_ms函数
 * 
 * @return void
 */
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

/**
 * @brief 设置日志级别
 * @param level 要设置的日志级别
 * @details 此函数用于动态修改全局日志配置中的日志级别，
 *          影响后续所有日志输出的过滤行为。
 */
void Logger::set_level(LogLevel level)
{
	g_config.level = level;
}

/**
 * @brief 获取当前日志级别
 * 
 * @return Logger::LogLevel 返回全局配置中设置的日志级别
 * 
 * @details 该函数用于查询系统当前的日志输出级别，
 *          日志级别用于控制哪些日志消息会被输出
 */
Logger::LogLevel Logger::get_level()
{
	return g_config.level;
}

/**
 * @brief 设置时间提供者函数
 * @details 配置日志系统使用的时间获取函数。如果传入的函数指针为空，
 *          则使用默认的时间提供者函数。
 * @param now_ms 函数指针，指向返回当前时间（单位：毫秒）的函数。
 *               如果为 nullptr，则使用默认的 default_now_ms 函数。
 * @return 无返回值
 * @note 此函数用于自定义日志系统的时间源，允许用户提供自定义的时间获取方式。
 */
void Logger::set_time_provider(uint32_t (*now_ms)())
{
	g_now_ms = (now_ms != nullptr) ? now_ms : default_now_ms;
}

/**
 * @brief 记录日志消息
 * 
 * @param level 日志级别，用于指定日志的严重程度
 * @param tag 日志标签，用于标识日志来源或分类
 * @param fmt 格式化字符串，支持类似printf的格式说明符
 * @param ... 变长参数列表，对应格式化字符串中的占位符
 * 
 * @details 该函数使用可变参数列表实现格式化日志输出。
 *          内部通过va_list将参数传递给vlog()函数进行实际的日志处理。
 * 
 * @example
 *          Logger::log(LogLevel::INFO, "APP", "User login: %s", username);
 */
void Logger::log(LogLevel level, const char *tag, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vlog(level, tag, fmt, ap);
	va_end(ap);
}

/**
 * @brief 记录日志消息的核心实现函数
 * 
 * 该函数处理日志的格式化、输出和历史记录。根据配置的日志级别、
 * 颜色设置等参数，将格式化后的日志消息输出到指定的流中，
 * 同时保存到日志历史记录中。
 * 
 * @param level 日志级别，用于判断是否应该输出该日志
 * @param tag 日志标签，通常用于标识日志来源或模块名称
 * @param fmt 日志格式字符串，遵循printf格式
 * @param ap 可变参数列表，包含格式字符串中的参数值
 * 
 * @details
 * - 若输出流为空、格式字符串为空或不应输出该级别日志，函数将直接返回
 * - 若启用了彩色输出，将添加对应日志级别的颜色前缀
 * - 若消息被截断，将在末尾添加省略号("...")表示
 * - 日志历史记录中将保存完整的前缀、消息和换行符
 * - 使用两个缓冲区分别存储前缀和消息，防止缓冲区溢出
 * 
 * @note
 * - 此函数使用 vsnprintf 进行格式化，若格式字符串有误会显示"<format-error>"
 * - 日志会通过 g_config.out 流输出，同时通过 history_append 保存到历史记录
 * - 时间戳通过 g_now_ms() 或 millis() 函数获取
 */
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

/**
 * @brief 以十六进制转储格式输出二进制数据
 * 
 * 将指定的二进制数据按照十六进制格式逐行输出，每行显示指定数量的字节。
 * 输出格式为：偏移地址、十六进制字节值和ASCII可打印字符表示。
 * 
 * @param level 日志级别，用于判断是否需要输出此日志
 * @param tag 日志标签，用于标识日志来源
 * @param data 指向要转储的二进制数据的指针
 * @param len 要转储的数据长度（字节数）
 * @param bytes_per_line 每行显示的字节数，默认为16。若传入0则使用默认值16
 * 
 * @note 
 *   - 如果 data 为 nullptr，将输出 "<null>"
 *   - 如果 len 为 0，将输出 "<empty>"
 *   - 输出格式示例：0001: 48 65 6C 6C 6F 00 00 00 |Hello...|
 *   - 不可打印的字符将用 '.' 代替
 *   - 如果当前日志级别不应输出，函数会直接返回
 * 
 * @return void
 */
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

/**
 * @brief 刷新日志输出流
 * 
 * 将日志缓冲区中的数据写入到输出流中。如果配置的输出流指针不为空，
 * 则调用其 flush() 方法确保所有待处理的日志数据被立即写出。
 * 
 * @note 此函数在需要确保日志立即输出时调用，例如在程序退出前或
 *       需要实时查看日志时。
 * 
 * @return void
 */
void Logger::flush()
{
	if (g_config.out != nullptr)
	{
		g_config.out->flush();
	}
}

/**
 * @brief 设置日志历史记录的容量
 * @param bytes 日志历史记录的最大字节数
 * @details 该函数将重置日志历史记录缓冲区，并设置新的容量限制。
 *          当提供的字节数小于当前使用的字节数时，旧的日志记录将被丢弃。
 */
void Logger::set_history_capacity(size_t bytes)
{
	history_reset(bytes);
}

/**
 * @brief 获取日志历史记录的容量
 * @return size_t 返回日志历史记录能够存储的最大条目数
 */
size_t Logger::history_capacity()
{
	return g_history_capacity;
}

/**
 * @brief 获取日志历史记录的大小
 * @return size_t 返回历史记录的总大小
 */
size_t Logger::history_size()
{
	return g_history_size;
}

/**
 * @brief 清除日志历史记录
 * 
 * 将日志历史记录重置为初始状态，容量恢复到全局历史记录容量值。
 * 此操作会清空所有之前记录的历史日志数据。
 */
void Logger::clear_history()
{
	history_reset(g_history_capacity);
}

/**
 * @brief 将日志历史记录转储到输出流
 * 
 * 将存储在循环缓冲区中的所有日志历史记录按顺序写入到指定的输出流。
 * 缓冲区采用先进先出(FIFO)的方式组织,该函数会从最旧的记录开始输出。
 * 
 * @param out 输出流对象,用于接收日志历史数据
 * 
 * @return 实际写入输出流的字节数。如果历史记录缓冲区为空或容量为0,
 *         则返回0
 * 
 * @note 该函数读取全局的 g_history 循环缓冲区、g_history_head 头指针、
 *       g_history_size 当前大小和 g_history_capacity 总容量
 */
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

/**
 * @brief 将日志历史记录复制到输出缓冲区
 * 
 * @param out 指向输出缓冲区的指针，用于存储复制的日志数据
 * @param maxLen 输出缓冲区的最大长度（字节数）
 * 
 * @return 实际复制的字节数。如果输入参数无效或缓冲区为空，返回0
 * 
 * @note 该函数采用循环缓冲区方式，从最旧的日志记录开始复制到最新的记录
 * @note 如果历史记录大小小于maxLen，则仅复制实际存在的记录数
 * 
 * @warning 调用者需确保out指针有效且maxLen足够大
 */
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
