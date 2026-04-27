#pragma once
#include <Arduino.h>
#include <cstddef>
#include <cstdint>
#include <vector>

class LogStreaming
{
public:
  /**
   * @brief 初始化日志流模块
   * 
   * 设置和初始化日志流系统的必要配置和资源。
   * 必须在使用任何日志流功能之前调用此函数。
   * 
   * @note 该函数应该在程序启动时调用一次
   */
  void init();

  /**
   * @brief 处理日志流的定时更新
   * @param now_ms 当前时间戳，单位为毫秒
   * @details 该函数应该定期调用以处理日志流的相关任务，
   *          如缓冲区刷新、超时处理等。通过传入当前时间戳，
   *          可以实现基于时间的事件调度和超时检测。
   * @note 建议在主循环或定时器中周期性调用此函数
   */
  void tick(uint32_t now_ms);

  /**
   * @brief 检查是否需要重新渲染
   * 
   * 判断当前状态是否需要进行渲染操作。通常用于优化渲染性能，
   * 避免不必要的重绘。
   * 
   * @return true 如果需要进行渲染，false 否则
   */
  bool needs_render() const;

  /**
   * @brief 渲染日志流内容到显示设备
   * 
   * 该函数负责将当前积累的日志数据渲染到电子墨水屏或其他显示设备上。
   * 它会处理日志格式化、布局和绘制操作。
   * 
   * @note 该函数应在日志数据更新后调用以刷新显示
   * @see log_streaming 类的其他相关函数
   */
  void render();

  /**
   * @brief 强制刷新日志流输出
   * 
   * 此函数用于立即刷新日志流缓冲区，确保所有待输出的日志内容
   * 被立即写入到目标输出设备（如文件、控制台等）。
   * 
   * 通常在以下情况中使用：
   * - 需要确保日志立即可见
   * - 在程序退出或关键点时确保日志不丢失
   * - 性能敏感的情况下手动控制刷新时机
   */
  void force_refresh();

private:

  /**
   * @brief 刷新历史记录
   * 
   * 该函数用于更新或重新加载日志流的历史记录。
   * 当需要重新初始化历史数据或同步最新的历史记录状态时调用此方法。
   * 
   * @private 这是一个私有成员函数，仅供类内部使用
   * 
   * @note 此操作可能涉及I/O或内存操作，请谨慎使用
   */
  void refresh_history_();

  /**
   * @brief 解析历史字节数据中的日志行
   * 
   * 该函数将原始的历史字节数据解析为单个日志行。
   * 
   * @param history_bytes 包含日志历史数据的字节向量，数据来自日志缓冲区
   * 
   * @note 这是一个私有方法，仅供类内部使用
   * @note 解析结果会被存储到适当的内部数据结构中
   * 
   * @see log_streaming 类的其他相关方法
   */
  void parse_lines_(const std::vector<uint8_t> &history_bytes);

private:
  /**
   * @brief 存储最后一次历史记录的字节数据
   * 
   * 该向量用于保存上一次日志或状态的历史记录。
   * 以字节数组形式存储，便于进行二进制数据操作和持久化。
   * 
   * @note 初始状态为空向量
   */
  std::vector<uint8_t> last_history_{};

  /**
   * @brief 存储解析后的日志行
   * @details 用于保存从日志流中解析出来的每一行文本。
   *          每个元素都是一个String对象，代表一行日志内容。
   */
  std::vector<String> parsed_lines_{};
  
  /**
   * @brief 标记日志流是否已修改
   * @details 用于指示日志流的内容是否发生了变化，以便决定是否需要重新渲染。
   */
  bool dirty_ = true;

  /**
   * @brief 表示当前显示窗口顶部所对应的日志行索引
   * 
   * 用于在日志流中跟踪滚动位置。当日志内容超过显示区域时，
   * 此成员变量记录当前显示的第一行日志在完整日志缓冲区中的索引位置。
   * 初始值为 0，表示从日志开头开始显示。
   */
  size_t top_line_index_ = 0;
  
  /// @brief 帧ID计数器
  /// 
  /// 用于追踪和标识每个处理的数据帧，从0开始递增。
  /// 每次发送或接收新帧时应递增此计数器。
  uint32_t frame_id_ = 0;
  
  /// @brief 上次轮询的时间戳（毫秒）
  /// @details 用于记录最后一次调用轮询操作的时间，以便判断轮询间隔
  uint32_t last_poll_ms_ = 0;

  /// @brief 上次滚动的时间戳（毫秒）
  /// 用于记录最后一次执行滚动操作的时间，以毫秒为单位
  uint32_t last_scroll_ms_ = 0;
};
