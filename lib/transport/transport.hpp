#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <vector>

class Transport
{
public:
  /**
   * @enum Event
   * @brief 传输层事件枚举
   * 
   * 定义了传输层可能发生的各种事件类型
   * 
   * @var Event::Connected
   *      连接成功事件
   * 
   * @var Event::Disconnected
   *      连接断开事件
   * 
   * @var Event::Error
   *      传输错误事件
   * 
   * @var Event::HeartbeatTimeout
   *      心跳超时事件
   */
  enum class Event : uint8_t
  {
    Connected,
    Disconnected,
    Error,
    HeartbeatTimeout,
  };

  /**
   * @enum State
   * @brief 传输层连接状态枚举
   * 
   * 定义了传输连接的各种可能状态，用于跟踪和管理连接的生命周期。
   * 
   * @var State::Disconnected 未连接状态，表示传输层当前未建立连接
   * @var State::Connecting 连接中状态，表示传输层正在建立连接
   * @var State::Connected 已连接状态，表示传输层已成功建立连接
   * @var State::Error 错误状态，表示传输层发生错误或连接异常
   */
  enum class State : uint8_t
  {
    Disconnected,
    Connecting,
    Connected,
    Error,
  };

  /**
   * @struct Config
   * @brief 传输层配置结构体
   * 
   * 用于配置传输层的心跳检测和重连相关参数。
   * 
   * @member heartbeatEnabled 是否启用心跳检测，默认为true
   * @member heartbeatIntervalMs 心跳检测间隔，单位为毫秒，默认值为3000ms
   * @member heartbeatTimeoutMs 心跳检测超时时间，单位为毫秒，默认值为10000ms
   * @member autoReconnect 当连接断开时是否自动重连，默认为true
   * @member reconnectIntervalMs 重连间隔，单位为毫秒，默认值为2000ms
   */
  struct Config
  {
    bool heartbeatEnabled = true;
    uint32_t heartbeatIntervalMs = 3000;
    uint32_t heartbeatTimeoutMs = 10000;
    bool autoReconnect = true;
    uint32_t reconnectIntervalMs = 2000;
  };

public:
  /**
   * @brief Transport 类的构造函数
   * 
   * 初始化 Transport 对象，完成必要的成员变量初始化和资源分配。
   */
  Transport();

  /**
   * @brief 初始化传输层模块
   * 
   * @param cfg 传输层配置参数，包含必要的初始化设置
   * 
   * @details 根据提供的配置参数初始化传输层。此函数应在使用其他传输
   *          功能之前调用，用于设置通信协议、波特率、引脚等相关参数。
   * 
   * @note 重复调用此函数可能导致重新初始化，请确保在调用前已释放
   *       之前的资源。
   */
  void init(const Config &cfg);

  /**
   * @brief 设置传输配置
   * 
   * 根据提供的配置参数更新传输层的配置信息。该方法允许
   * 在运行时修改传输行为，如波特率、超时时间等参数。
   * 
   * @param cfg 常引用，包含新的配置参数的Config对象
   * 
   * @note 配置更改可能会影响当前的传输操作，建议在
   *       传输操作完成后调用此方法
   * 
   * @see Config
   */
  void setConfig(const Config &cfg);

  /**
   * @brief 连接到设备或服务
   * 
   * 建立与远程设备或服务的连接。此函数会初始化必要的
   * 通信协议和资源，为后续的数据传输做准备。
   * 
   * @return bool 如果连接成功返回 true，失败返回 false
   * 
   * @note 在调用此函数之前，应确保相关的初始化工作已完成
   * @see disconnect()
   */
  bool connect();

  /**
   * @brief 同步连接到远程服务器
   * 
   * @details 该函数会阻塞当前线程，直到连接建立或超时。
   *          用于初始化传输层的连接状态。
   * 
   * @return bool 如果连接成功返回 true，否则返回 false
   * 
   * @note 调用此函数前，应确保传输层已正确初始化。
   * 
   * @see disconnect()
   */
  bool syncConnect();

  /**
   * @brief 重新连接到服务器或设备
   * 
   * 尝试建立连接或重新建立已断开的连接。如果连接已经存在且活跃，
   * 此函数可能会先断开现有连接，然后重新连接。
   * 
   * @return true 如果重新连接成功，false 如果重新连接失败
   */
  bool reconnect();

  /**
   * @brief 断开连接
   * 
   * 关闭当前的连接，释放相关资源。调用此函数后，
   * 传输对象将不再可用，需要重新连接才能继续通信。
   */
  void disconnect();

  /**
   * @brief 主循环函数
   * 
   * 此函数用于处理传输层的主循环逻辑。通常需要周期性调用此函数
   * 以驱动传输层的事件处理和数据传输。
   * 
   * @return void
   * 
   * @note 此函数应该在主程序循环中持续调用，以确保传输层
   *       能够正常工作。
   */
  void loop();

  /**
   * @brief 发送数据
   * 
   * @param data 指向要发送的数据缓冲区的指针
   * @param len 要发送的数据长度（字节数）
   */
  void send(const uint8_t *data, size_t len);

  /**
   * @brief 注入接收到的原始字节流
   *
   * 由底层传输驱动在收到数据时调用。该函数会：
   * - 记录接收活动时间
   * - 自动识别心跳 ACK（"PONG"）
   * - 将数据追加到内部接收缓冲区，供上层读取
   *
   * @param data 接收数据指针
   * @param len 接收数据长度
   */
  void onBytesReceived(const uint8_t *data, size_t len);

  /**
   * @brief 当前可读的接收缓冲字节数
   * @return size_t 可读取字节数
   */
  size_t available() const;

  /**
   * @brief 从接收缓冲中读取数据
   *
   * @param out 输出缓冲区指针
   * @param maxLen 最多读取字节数
   * @return size_t 实际读取字节数
   */
  size_t receive(uint8_t *out, size_t maxLen);

  /**
   * @brief 通知接收活动
   * 
   * 该函数用于通知系统有数据接收活动发生。常用于更新接收超时计时器、
   * 唤醒设备或触发相关的接收事件处理逻辑。
   * 
   * @note 此函数通常在接收到数据时被调用
   */
  void notifyRxActivity();

  /**
   * @brief 通知心跳确认
   * 
   * 该函数用于通知系统已收到心跳确认应答。
   * 通常在接收到远程设备或服务器的心跳响应时调用此函数,
   * 用于更新心跳状态或重置心跳计时器。
   */
  void notifyHeartbeatAck();

  /// @brief 获取传输层的当前状态
  /// @return 返回当前传输层的状态
  State state() const;

  /**
   * @brief 检查传输连接状态
   * 
   * 检查当前传输层是否已连接到远程设备或服务。
   * 
   * @return true 如果传输层已连接，false 否则未连接
   */
  bool isConnected() const;

private:
  /**
   * @brief 心跳循环函数
   * 
   * 该函数实现了一个持续运行的心跳检测循环，用于定期维护与远程设备或服务的连接状态。
   * 通常在后台线程中执行，以确保传输连接保持活跃状态。
   * 
   * @details
   * - 周期性发送心跳包到对端设备
   * - 监测连接的健康状况
   * - 在连接断开时可触发重连机制
   * 
   * @note 该函数为私有函数，不应被外部直接调用
   * @see heartbeat 相关的公共接口
   */
  void heartbeatLoop_();

  /**
   * @brief 重新连接循环函数
   * 
   * 该函数实现了一个持续的重新连接机制，用于在连接断开时
   * 自动尝试重新建立连接。该函数通常在单独的线程中运行，
   * 不断检查连接状态并在需要时进行重连。
   * 
   * @note 这是一个私有方法（以下划线结尾），不应直接从外部调用
   * @note 该函数可能会进入一个长时间运行的循环，直到连接成功
   *       或收到停止信号
   * 
   * @see reconnect()
   */
  void reconnectLoop_();

  /**
   * @brief 判断是否应该发送心跳包
   * 
   * @param now 当前时间戳（毫秒）
   * @return true 如果应该发送心跳包
   * @return false 如果不应该发送心跳包
   */
  bool shouldSendHeartbeat_(uint32_t now) const;

  /**
   * @brief 检查心跳是否超时
   * 
   * @param now 当前时间戳（单位：毫秒），用于与上次心跳时间进行比较
   * @return true 如果心跳已超时，返回true；否则返回false
   * 
   * @details 该函数用于判断距离上次成功接收心跳信号后是否已经超过了
   *          预设的超时时间限制。通常用于检测通信连接是否仍然活跃。
   */
  bool isHeartbeatTimeout_(uint32_t now) const;

  /**
   * @brief 重置心跳状态
   * @details 将心跳相关的状态变量恢复到初始状态，包括心跳计时器、
   *          心跳计数器等。此函数通常在连接建立、重新连接或需要清除
   *          心跳记录时调用。
   * @note 这是一个私有方法，仅供类内部使用
   * @return void
   */
  void resetHeartbeatState_();

  /**
   * @brief 发送事件
   * 
   * 将指定的事件发射到系统中，触发相关的事件处理流程。
   * 
   * @param e 要发送的事件对象
   * 
   * @note 这是一个内部方法，通常由类的其他方法调用
   */
  void emit_(Event e);

  /**
   * @brief 发送心跳ping信号
   * 
   * 向远程设备发送心跳ping请求，用于保持连接活跃状态和检测连接是否仍然有效。
   * 
   * @return bool 如果心跳ping发送成功返回true，否则返回false
   * 
   * @note 这是一个私有方法，仅供内部使用
   * 
   * @see receiveHeartbeatPong
   */
  bool sendHeartbeatPing_();

  /**
   * @brief 从接收字节流中识别心跳 ACK
   *
   * 通过增量匹配 "PONG" 关键字，在跨包场景下也能识别。
   *
   * @param data 输入字节流
   * @param len 字节流长度
   */
  void processHeartbeatAckBytes_(const uint8_t *data, size_t len);

private:
  /// @brief 传输层配置对象
  /// @details 用于存储和管理传输层的配置参数，包括通信协议、超时设置、重试策略等相关配置信息
  Config config_{};

  /// @brief 设备连接状态
  /// @details 表示传输层当前的连接状态，初始状态为断开连接
  /// 
  /// @see State
  State state_ = State::Disconnected;

  /// @brief 上次心跳包发送的时间戳（毫秒）
  /// @details 记录最后一次成功发送心跳包的时间，用于心跳超时检测和心跳间隔控制
  uint32_t lastHeartbeatSentAtMs_ = 0;

  /**
   * @brief 上次接收到心跳确认的时间戳（毫秒）
   * @details 记录最后一次收到心跳ACK消息的系统时间，用于判断心跳连接是否超时
   */
  uint32_t lastHeartbeatAckAtMs_ = 0;

  /// @brief 上次接收数据的时间戳（毫秒）
  /// 
  /// 记录最后一次成功接收数据时的系统时间，单位为毫秒。
  /// 用于超时检测和通信状态监控。
  uint32_t lastRxAtMs_ = 0;

  /**
   * @brief 上次尝试重新连接的时间戳（毫秒）
   * @details 记录最后一次尝试重新连接的时间点，用于控制重连的频率和间隔
   */
  uint32_t lastReconnectTryAtMs_ = 0;

  /// @brief 标志位，指示是否正在等待心跳确认消息
  /// @details 当发送心跳包后，将此标志设置为 true，待收到心跳确认响应后设置为 false
  bool awaitingHeartbeatAck_ = false;

  /// @brief 接收缓冲区（原始字节流）
  std::vector<uint8_t> rxBuffer_{};

  /// @brief "PONG" 增量匹配进度（0~4）
  uint8_t heartbeatAckMatchPos_ = 0;
};