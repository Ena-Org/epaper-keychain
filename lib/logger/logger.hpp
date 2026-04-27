#pragma once
#include <Arduino.h>
#include <cstdarg>
#include <cstddef>
#include <cstdint>

class Logger
{
public:
  /**
   * @brief 日志级别枚举。
   *
   * 用于控制日志输出的详细程度，数值越小通常表示级别越高、越重要。
   *
   * 级别说明：
   * - Error（0）：错误信息，仅记录导致功能失败或异常的问题。
   * - Warn（1）：警告信息，表示潜在问题或非致命异常。
   * - Info（2）：常规信息，用于记录系统运行中的关键状态变化。
   * - Debug（3）：调试信息，提供用于定位问题的详细上下文。
   * - Verbose（4）：冗长信息，输出最细粒度的运行细节。
   * - Off（255）：关闭日志输出。
   */
  enum class LogLevel : uint8_t
  {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
    Verbose = 4,
    Off = 255,
  };

  /**
   * @brief 日志系统配置项
   *
   * 用于控制日志输出目标、日志级别以及日志格式展示行为。
   *
   * @var out
   * 日志输出流指针，默认输出到串口（Serial）。
   *
   * @var level
   * 日志过滤级别，仅输出不低于该级别的日志。
   *
   * @var show_time
   * 是否在日志前缀中显示时间信息。
   *
   * @var show_level
   * 是否在日志前缀中显示日志级别。
   *
   * @var show_tag
   * 是否在日志前缀中显示标签（Tag）。
   *
   * @var color
   * 是否启用彩色日志输出（通常依赖终端/串口工具支持）。
   */
  struct LoggerConfig
  {
    Stream *out = &Serial;
    LogLevel level = LogLevel::Info;
    bool show_time = true;
    bool show_level = true;
    bool show_tag = true;
    bool color = false;
    size_t history_bytes = 0;
  };

  /**
   * @brief 初始化日志系统配置。
   *
   * 根据传入的配置参数完成日志模块的初始化（如日志级别、输出目标、格式等）。
   * 建议在程序启动早期调用，且通常只初始化一次；重复初始化的行为取决于具体实现。
   *
   * @param cfg 日志配置对象，包含日志系统初始化所需的全部参数。
   */
  static void init(const LoggerConfig &cfg);

  /**
   * @brief 设置日志输出级别。
   *
   * 调用此函数可更新当前日志系统的最小输出级别；低于该级别的日志将被忽略，
   * 高于或等于该级别的日志会被正常记录。
   *
   * @param level 要设置的日志级别（例如：调试、信息、警告、错误等）。
   */
  static void set_level(LogLevel level);

  /**
   * @brief 获取当前日志系统的全局日志等级。
   *
   * 该函数返回当前生效的日志过滤级别，通常用于判断某条日志是否应被输出。
   *
   * @return LogLevel 当前日志等级。
   */
  static LogLevel get_level();

  /**
   * @brief 设置日志模块的时间提供函数。
   *
   * 该接口用于注入一个返回“当前毫秒时间戳”的函数指针，日志系统后续会通过此回调获取时间，
   * 以支持不同平台或自定义时钟来源（如系统 tick、RTC、模拟时间等）。
   *
   * @param now_ms 指向时间函数的指针；函数需无参数并返回当前时间（单位：毫秒，uint32_t）。
   *
   * @note 传入的函数应可长期有效，且在日志调用期间可安全执行。
   * @warning 若未设置或传入无效回调，日志时间戳行为可能不符合预期。
   */
  static void set_time_provider(uint32_t (*now_ms)()); 

  /**
   * @brief 记录一条日志消息（支持可变参数格式化）。
   *
   * 该函数根据指定的日志级别与标签输出日志内容，`fmt` 为格式化字符串，
   * 后续参数需与格式化占位符一一对应。
   *
   * @param level 日志级别，用于控制日志的重要性与输出策略。
   * @param tag 日志标签，通常用于标识模块或子系统来源。
   * @param fmt C 风格格式化字符串（如 `printf`）。
   * @param ... 与 `fmt` 中占位符对应的可变参数列表。
   */
  static void log(LogLevel level, const char *tag, const char *fmt, ...);

  /**
   * @brief 使用可变参数列表记录一条日志消息。
   *
   * 该函数是日志输出的底层接口，接收已初始化的 `va_list` 参数，
   * 按指定日志级别与标签对格式化字符串进行处理并输出日志内容。
   * 通常由接受可变参数（`...`）的封装函数在内部调用。
   *
   * @param level 日志级别（如调试、信息、警告、错误等）。
   * @param tag 日志标签，用于标识日志来源模块。
   * @param fmt 格式化字符串（`printf` 风格）。
   * @param ap  已初始化的可变参数列表，需与 `fmt` 中的格式说明符匹配。
   *
   * @note 调用方负责在传入前正确初始化 `va_list`（例如使用 `va_start`），
   *       并在使用后按需结束（例如使用 `va_end`）。
   * @warning `fmt` 与参数类型不匹配会导致未定义行为。
   */
  static void vlog(LogLevel level, const char *tag, const char *fmt, va_list ap);

  /**
   * @brief 以十六进制格式输出内存数据到日志。
   *
   * 该函数会按行格式化二进制数据（默认每行 16 字节），并使用指定日志级别与标签输出，
   * 便于调试通信帧、缓冲区内容或原始字节流。
   *
   * @param level 日志级别（决定输出优先级与过滤行为）。
   * @param tag 日志标签（用于标识日志来源模块）。
   * @param data 待输出的数据起始地址。
   * @param len  待输出的数据长度（字节）。
   * @param bytes_per_line 每行显示的字节数，默认为 16。
   */
  static void hexdump(LogLevel level, const char *tag, const void *data, size_t len, size_t bytes_per_line = 16);

  /**
   * @brief 立即刷新日志缓冲区，将当前已缓存的日志内容写入目标输出。
   *
   * 该函数通常用于确保关键日志在程序退出、异常处理或调试节点处被及时落盘/输出，
   * 以避免因缓冲未提交导致的日志丢失。
   *
   * @note 该函数不接收参数，也不返回值。
   * @warning 若底层输出设备不可用，刷新行为可能失败（具体取决于实现）。
   */
  static void flush();

  /**
   * @brief 配置日志历史缓存容量（字节）
   * @param bytes 历史缓存最大容量，0表示关闭历史
   */
  static void set_history_capacity(size_t bytes);

  /// @brief 获取历史缓存容量（字节）
  static size_t history_capacity();

  /// @brief 获取当前已保存的历史字节数
  static size_t history_size();

  /// @brief 清空历史缓存
  static void clear_history();

  /**
   * @brief 将历史日志输出到指定Stream
   * @return 输出的字节数
   */
  static size_t dump_history(Stream &out);

  /**
   * @brief 复制历史日志到缓冲区
   * @param out 输出缓冲区
   * @param maxLen 输出缓冲区大小
   * @return 实际复制的字节数
   */
  static size_t copy_history(uint8_t *out, size_t maxLen);
};

/**
 * @def LOGE
 * @brief 记录 Error 级别日志。
 * @param tag 日志标签。
 * @param fmt printf 风格格式串。
 * @param ... 格式化参数。
 */
#define LOGE(tag, fmt, ...) Logger::log(Logger::LogLevel::Error, tag, fmt, ##__VA_ARGS__)

/**
 * @def LOGW
 * @brief 记录 Warn 级别日志。
 * @param tag 日志标签。
 * @param fmt printf 风格格式串。
 * @param ... 格式化参数。
 */
#define LOGW(tag, fmt, ...) Logger::log(Logger::LogLevel::Warn, tag, fmt, ##__VA_ARGS__)

/**
 * @def LOGI
 * @brief 记录 Info 级别日志。
 * @param tag 日志标签。
 * @param fmt printf 风格格式串。
 * @param ... 格式化参数。
 */
#define LOGI(tag, fmt, ...) Logger::log(Logger::LogLevel::Info, tag, fmt, ##__VA_ARGS__)

/**
 * @def LOGD
 * @brief 记录 Debug 级别日志。
 * @param tag 日志标签。
 * @param fmt printf 风格格式串。
 * @param ... 格式化参数。
 */
#define LOGD(tag, fmt, ...) Logger::log(Logger::LogLevel::Debug, tag, fmt, ##__VA_ARGS__)

/**
 * @def LOGV
 * @brief 记录 Verbose 级别日志。
 * @param tag 日志标签。
 * @param fmt printf 风格格式串。
 * @param ... 格式化参数。
 */
#define LOGV(tag, fmt, ...) Logger::log(Logger::LogLevel::Verbose, tag, fmt, ##__VA_ARGS__)
