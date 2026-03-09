#include "log_streaming.hpp"
#include <algorithm>
#include "epaper.hpp"
#include "logger.hpp"

namespace
{
  constexpr uint32_t kHistoryPollIntervalMs = 600;
  constexpr uint32_t kScrollIntervalMs = 1400;
  constexpr size_t kMaxHistoryBytes = 2048;
  constexpr size_t kVisibleLines = 6;
  constexpr size_t kMaxLineChars = 42;
}

/**
 * @brief 初始化日志流对象
 * 
 * 清除所有历史记录和解析的日志行，重置滚动位置和时间戳，
 * 并刷新日志历史记录。此方法应在使用LogStreaming对象前调用。
 * 
 * @details
 * - 清空 last_history_ 容器
 * - 清空 parsed_lines_ 容器
 * - 将脏标志设置为 true，标记需要重新渲染
 * - 重置顶部行索引为 0
 * - 重置帧ID为 0
 * - 清空最后轮询时间戳
 * - 清空最后滚动时间戳
 * - 调用 refresh_history_() 刷新日志历史记录
 */
void LogStreaming::init()
{
  last_history_.clear();
  parsed_lines_.clear();
  dirty_ = true;
  top_line_index_ = 0;
  frame_id_ = 0;
  last_poll_ms_ = 0;
  last_scroll_ms_ = 0;
  refresh_history_();
}

/**
 * @brief 更新日志流的显示状态
 * 
 * 此函数应定期调用以处理日志历史记录的刷新和自动滚动。
 * 根据设定的时间间隔，执行以下操作：
 * 1. 定期刷新日志历史记录
 * 2. 当日志行数超过可显示行数时，自动向下滚动显示
 * 
 * @param now_ms 当前时间戳（毫秒），用于判断是否达到触发条件
 * 
 * @note 需要在主循环中定期调用此函数以保持日志显示更新
 * @note 当发生滚动时，会将 dirty_ 标志设置为 true，表示需要重新渲染
 */
void LogStreaming::tick(uint32_t now_ms)
{
  if ((now_ms - last_poll_ms_) >= kHistoryPollIntervalMs)
  {
    last_poll_ms_ = now_ms;
    refresh_history_();
  }

  if (parsed_lines_.size() > kVisibleLines && (now_ms - last_scroll_ms_) >= kScrollIntervalMs)
  {
    last_scroll_ms_ = now_ms;
    const size_t line_count = parsed_lines_.size();
    top_line_index_ = (top_line_index_ + 1) % line_count;
    dirty_ = true;
  }
}

/**
 * @brief 检查日志流是否需要重新渲染
 * 
 * @return true 如果日志流的内容已被修改，需要重新渲染
 * @return false 如果日志流的内容未被修改，无需重新渲染
 */
bool LogStreaming::needs_render() const
{
  return dirty_;
}

/**
 * @brief 渲染日志流显示
 * 
 * 该函数负责准备并显示日志行。如果没有日志记录，则显示"No logs yet"消息。
 * 否则，从缓存的解析日志中获取指定数量的可见行，使用循环索引实现日志滚动显示。
 * 最后调用EPaper模块将日志行显示在电子纸屏幕上，并更新脏标志状态。
 * 
 * @details
 * - 如果parsed_lines_为空，显示提示信息"No logs yet"
 * - 计算需要显示的行数（不超过kVisibleLines的最大值）
 * - 使用top_line_index_和模运算实现日志的循环滚动显示
 * - 调用EPaper::showLogLines()在电子纸屏幕上渲染日志
 * - 将dirty_标志设置为false，表示内容已呈现
 * 
 * @return void
 * 
 * @see EPaper::showLogLines
 */
void LogStreaming::render()
{
  std::vector<String> visible_lines;

  if (parsed_lines_.empty())
  {
    visible_lines.push_back("No logs yet");
  }
  else
  {
    const size_t line_count = parsed_lines_.size();
    const size_t to_show = std::min(kVisibleLines, line_count);
    visible_lines.reserve(to_show);

    for (size_t i = 0; i < to_show; ++i)
    {
      const size_t idx = (top_line_index_ + i) % line_count;
      visible_lines.push_back(parsed_lines_[idx]);
    }
  }

  EPaper::showLogLines("Browser Log", visible_lines, frame_id_);
  dirty_ = false;
}

/**
 * @brief 强制刷新日志显示
 * 
 * 将脏标志位设置为true，标记日志流需要进行刷新操作。
 * 在下一次更新循环中，标记为脏的内容将被重新渲染或刷新。
 */
void LogStreaming::force_refresh()
{
  dirty_ = true;
}

void LogStreaming::refresh_history_()
{
  const size_t history_size = Logger::history_size();
  const size_t copy_size = std::min(history_size, kMaxHistoryBytes);

  std::vector<uint8_t> snapshot(copy_size);
  if (copy_size > 0)
  {
    const size_t copied = Logger::copy_history(snapshot.data(), snapshot.size());
    snapshot.resize(copied);
  }

  if (snapshot == last_history_)
  {
    return;
  }

  last_history_ = snapshot;
  parse_lines_(last_history_);

  if (top_line_index_ >= parsed_lines_.size())
  {
    top_line_index_ = 0;
  }

  ++frame_id_;
  dirty_ = true;
}

void LogStreaming::parse_lines_(const std::vector<uint8_t> &history_bytes)
{
  parsed_lines_.clear();

  if (history_bytes.empty())
  {
    return;
  }

  String current_line;
  current_line.reserve(kMaxLineChars);

  for (size_t i = 0; i < history_bytes.size(); ++i)
  {
    const char ch = static_cast<char>(history_bytes[i]);

    if (ch == '\r')
    {
      continue;
    }

    if (ch == '\n')
    {
      if (current_line.length() > 0)
      {
        parsed_lines_.push_back(current_line);
        current_line = "";
      }
      continue;
    }

    if (current_line.length() >= kMaxLineChars)
    {
      parsed_lines_.push_back(current_line);
      current_line = "";
    }

    current_line += ch;
  }

  if (current_line.length() > 0)
  {
    parsed_lines_.push_back(current_line);
  }

  if (parsed_lines_.size() > 64)
  {
    const size_t remove_count = parsed_lines_.size() - 64;
    parsed_lines_.erase(parsed_lines_.begin(), parsed_lines_.begin() + remove_count);
  }
}
