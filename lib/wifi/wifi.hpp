#pragma once
#include <stdint.h>
#include <string>

class Wifi
{
public:
  /**
   * @brief Wi-Fi 连接配置参数。
   *
   * 用于描述设备连接无线网络时所需的基础信息以及重试策略。
   *
   * @var ssid
   * Wi-Fi 网络名称（SSID）。
   *
   * @var password
   * Wi-Fi 网络密码。
   *
   * @var hostname
   * 设备主机名，默认值为 "esp32"。
   *
   * @var maxRetries
   * 连接失败后的最大重试次数，默认值为 10。
   *
   * @var retryDelayMs
   * 每次重试之间的延迟时间（毫秒），默认值为 1000 ms。
   */
  struct Config
  {
    std::string ssid;
    std::string password;
    std::string hostname = "esp32";
    uint8_t maxRetries = 10;
    uint32_t retryDelayMs = 1000;
  };

  /**
   * @brief Wi-Fi 连接状态枚举。
   *
   * 用于描述 Wi-Fi 模块在生命周期中的状态流转：
   * - Idle：空闲状态，尚未开始连接流程。
   * - Starting：启动中，正在初始化 Wi-Fi 子系统。
   * - Connecting：连接中，正在尝试连接到目标接入点。
   * - WaitingIP：已建立链路，等待获取 IP 地址。
   * - Connected：已连接并成功获取网络配置，可进行网络通信。
   * - Failed：连接失败，通常表示初始化、认证或网络协商失败。
   * - Disconnected：已断开连接（可能由主动断开或链路丢失导致）。
   */
  enum class State : uint8_t
  {
    Idle,
    Starting,
    Connecting,
    WaitingIP,
    Connected,
    Failed,
    Disconnected
  };

public:
  /**
   * @brief 初始化 Wi-Fi 模块并应用给定配置。
   *
   * 根据传入的配置参数完成 Wi-Fi 子系统的准备工作（如参数校验、模式设置及连接初始化等）。
   *
   * @param cfg Wi-Fi 初始化配置对象，包含连接所需的参数与行为选项。
   * @return true 初始化成功。
   * @return false 初始化失败（例如配置无效、底层模块初始化失败或连接准备失败）。
   */
  bool init(const Config &cfg);

  /**
   * @brief 连接到已配置的 Wi-Fi 网络。
   *
   * 该函数会尝试使用当前配置的 SSID 与密码建立连接，
   * 并在连接流程完成后返回结果。
   *
   * @return
   * - true：连接成功。
   * - false：连接失败（如配置无效、超时或网络不可用）。
   */
  bool connect();

  /**
   * @brief 断开当前 Wi-Fi 连接。
   *
   * 主动断开设备与已连接接入点（AP）的连接，并停止当前会话。
   * 该操作通常用于网络切换、进入低功耗模式或执行重连流程前的清理。
   *
   * @note 调用后设备将不再具备网络连接能力，直到再次发起连接。
   */
  void disconnect();

  /**
   * @brief 检查当前 Wi-Fi 是否处于已连接状态。
   *
   * 该方法仅查询当前连接状态，不会触发连接流程或修改对象内部状态。
   *
   * @return
   * - `true`：设备当前已连接到 Wi-Fi 网络；
   * - `false`：设备当前未连接到 Wi-Fi 网络。
   */
  bool isConnected() const;

  /**
   * @brief 在指定超时时间内阻塞等待 Wi-Fi 连接成功。
   *
   * 该函数会持续检测当前连接状态，直到设备成功连接到网络，
   * 或达到给定的超时时间后返回失败。
   *
   * @param timeoutMs 等待超时时间（毫秒）。
   * @return true  在超时前已成功连接。
   * @return false 在超时前仍未连接成功。
   */
  bool waitConnected(uint32_t timeoutMs);

  /**
   * @brief WiFi 模块的主循环处理函数。
   *
   * 该函数应在程序主循环中被持续调用，用于驱动 WiFi 相关状态机、
   * 处理连接维护与后台任务（如重连、超时检查、事件轮询等）。
   *
   * @note 此函数为非阻塞设计（如实现遵循该约定），应尽可能频繁调用以保证网络稳定性。
   */
  void loop();

  /**
   * @brief 确保设备已连接到 Wi-Fi，在指定超时时间内尝试完成连接。
   *
   * 若当前已处于连接状态，通常会立即返回；否则会在 `timeoutMs` 毫秒内持续尝试连接，
   * 直到连接成功或超时为止。
   *
   * @param timeoutMs 等待连接成功的最长时间（单位：毫秒）。
   * @return true 在超时前已成功连接（或本来就已连接）。
   * @return false 在 `timeoutMs` 时间内仍未建立连接。
   */
  bool ensureConnected(uint32_t timeoutMs);
  
  /**
   * @brief 获取当前设备的 IP 地址字符串表示。
   *
   * 以可读文本形式返回设备当前的 IPv4/IPv6 地址（具体格式取决于实现）。
   * 当网络未连接或地址不可用时，可能返回空字符串或实现约定的默认值。
   *
   * @return std::string 当前 IP 地址的字符串表示。
   */
  std::string ipString() const;

  /**
   * @brief 获取当前 Wi-Fi 连接的信号强度（RSSI）。
   *
   * RSSI（Received Signal Strength Indicator）通常以 dBm 为单位返回，
   * 数值一般为负数：越接近 0 表示信号越强，越小表示信号越弱。
   *
   * @return int 当前连接的 RSSI 值（单位：dBm）。
   */
  int rssi() const;

  /**
   * @brief 获取当前 Wi-Fi 模块的状态。
   *
   * 该方法为只读访问，不会修改对象内部状态。
   *
   * @return State 当前状态枚举值。
   */
  State state() const;

  /**
   * @brief 获取最近一次 Wi-Fi 断开连接或连接失败的原因码。
   *
   * 该方法返回底层 Wi-Fi 子系统记录的“最近原因”整数值，
   * 可用于日志记录、错误诊断和重连策略判断。
   *
   * @return int 最近一次状态变更的原因码；具体含义请参考所用平台/SDK 的错误码定义。
   */
  int lastReason() const;

private:
  /// @brief Wi-Fi 模块的配置对象。
  /// @details 保存网络连接与运行参数（如 SSID、密码、重连策略、超时等），
  ///          供本类在初始化、连接与状态维护过程中统一读取与使用。
  Config cfg_;

  /// @brief 当前 Wi-Fi 状态机状态。
  /// @details
  /// 使用 `State` 枚举表示模块当前所处阶段（如空闲、连接中、已连接、错误等）。
  /// 该成员通常由状态机逻辑更新，用于驱动后续网络操作与流程控制。
  State state_ = State::Idle;

  /// @brief 记录最近一次 Wi-Fi 状态变化的原因码。
  /// @details 该值通常来自底层网络栈的错误/事件代码，用于调试、日志记录与重连策略判断；
  ///          0 一般表示尚未发生可记录的原因或已重置为默认状态。
  int lastReason_ = 0;
};
