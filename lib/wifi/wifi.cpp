#include "wifi.hpp"

#include <Arduino.h>
#include <WiFi.h>
#include "logger.hpp"

static constexpr const char *TAG = "WIFI";

/**
 * @brief 将 Wi-Fi 连接状态码转换为可读文本。
 *
 * 该函数用于把 `wl_status_t` 枚举值映射为简短的状态字符串，
 * 便于日志输出、调试或状态展示。对于未覆盖的状态值，返回 `"UNKNOWN"`。
 *
 * @param status Wi-Fi 状态码（`wl_status_t`）。
 * @return const char* 对应的状态文本常量字符串。
 */
 
/**
 * @brief 基于 SSID 名称进行 5GHz 网络的启发式判断。
 *
 * 该函数通过检查 SSID 中是否包含 `"5G"` 或 `"5g"` 子串来推测网络是否为 5GHz。
 * 这是一种简单的命名规则匹配，不保证结果绝对准确（取决于路由器命名习惯）。
 *
 * @param ssid 待检查的 Wi-Fi SSID 名称。
 * @return bool 若 SSID 看起来像 5GHz 网络则返回 `true`，否则返回 `false`。
 */
namespace
{
  const char *statusText(wl_status_t status)
  {
    switch (status)
    {
    case WL_NO_SHIELD:
      return "NO_SHIELD";
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "SCAN_COMPLETED";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
    }
  }

  bool looksLike5GhzSsid(const std::string &ssid)
  {
    return ssid.find("5G") != std::string::npos || ssid.find("5g") != std::string::npos;
  }
}

/**
 * @brief 初始化 WiFi 模块并应用基础配置。
 *
 * 该函数会保存传入的配置，设置设备为 STA（客户端）模式；
 * 当配置中提供了主机名时，会将其设置到 WiFi 接口。
 * 同时会将内部状态重置为空闲，并清空上一次错误原因码。
 *
 * @param cfg WiFi 初始化配置（包含如主机名等参数）。
 * @return true 初始化流程已成功执行。
 */
bool Wifi::init(const Config &cfg)
{

  cfg_ = cfg;
  WiFi.mode(WIFI_STA);
  if (!cfg.hostname.empty())
    WiFi.setHostname(cfg.hostname.c_str());

  LOGI(TAG, "init sta, hostname=%s", cfg_.hostname.empty() ? "<empty>" : cfg_.hostname.c_str());

  state_ = State::Idle;
  lastReason_ = 0;
  return true;
}


/**
 * @brief 发起 Wi-Fi 连接流程（异步）。
 *
 * 该方法会根据当前配置尝试连接到指定 SSID，并更新内部连接状态。
 * - 若 `ssid` 为空：立即中止连接，记录失败原因，状态置为失败并返回 `false`。
 * - 若 `ssid` 非空：切换到 STA 模式，按需设置主机名，调用 `WiFi.begin(...)` 发起连接，
 *   将状态置为“连接中”并返回 `true`。
 * - 当 SSID 名称疑似 5GHz 网络时会输出警告日志（ESP32 通常仅支持 2.4GHz）。
 *
 * @note 本方法仅负责“发起”连接，不保证在返回时已成功入网；
 *       最终连接结果需通过后续状态/事件或轮询判断。
 *
 * @return
 * - `true`：已成功启动连接流程；
 * - `false`：参数校验失败（如 SSID 为空）导致未启动连接。
 */
bool Wifi::connect()
{
  if (cfg_.ssid.empty())
  {
    lastReason_ = WL_IDLE_STATUS;
    state_ = State::Failed;
    LOGE(TAG, "connect aborted: ssid empty");
    return false;
  }

  state_ = State::Starting;
  LOGI(TAG, "connecting to ssid=%s", cfg_.ssid.c_str());
  if (looksLike5GhzSsid(cfg_.ssid))
  {
    LOGW(TAG, "ssid looks like 5GHz network, ESP32 usually supports only 2.4GHz");
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  if (!cfg_.hostname.empty())
    WiFi.setHostname(cfg_.hostname.c_str());

  WiFi.begin(cfg_.ssid.c_str(), cfg_.password.c_str());
  state_ = State::Connecting;
  return true;
}

/**
 * @brief 断开当前 Wi-Fi 连接并更新内部状态。
 *
 * 调用底层 WiFi 断开接口，并请求关闭 Wi-Fi 模块（参数为 true）。
 * 执行后将对象内部状态设置为 `State::Disconnected`，
 * 用于表示设备已处于断开连接状态。
 */
void Wifi::disconnect()
{
  WiFi.disconnect(true);
  state_ = State::Disconnected;
  LOGI(TAG, "disconnected");
}

/**
 * @brief 检查当前 Wi-Fi 是否已成功连接。
 *
 * 该函数通过查询底层 WiFi 状态，判断当前连接状态是否为 `WL_CONNECTED`。
 *
 * @return `true` 表示设备已连接到 Wi-Fi；`false` 表示未连接。
 */
bool Wifi::isConnected() const
{
  return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief 在指定超时时间内轮询 WiFi 连接状态，等待设备完成连接。
 *
 * 该函数会持续检查 `WiFi.status()`，直到超时或出现明确结果：
 * - 当状态为 `WL_CONNECTED` 时，设置内部状态为 `State::Connected` 并返回 `true`。
 * - 当状态为 `WL_CONNECT_FAILED`、`WL_NO_SSID_AVAIL`、
 *   `WL_CONNECTION_LOST` 或 `WL_DISCONNECTED` 时，
 *   记录失败原因到 `lastReason_`，设置内部状态为 `State::Failed` 并返回 `false`。
 * - 在等待过程中将内部状态设置为 `State::WaitingIP`，并以约 50ms 间隔轮询。
 * - 若超时仍未得到成功结果，记录当前状态到 `lastReason_`，设置为失败并返回 `false`。
 *
 * @param timeoutMs 等待连接的超时时间（毫秒）。
 * @return `true` 表示连接成功；`false` 表示连接失败或超时。
 */
bool Wifi::waitConnected(uint32_t timeoutMs)
{
  const unsigned long start = millis();
  while (millis() - start < timeoutMs)
  {
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
      state_ = State::Connected;
      LOGI(TAG, "connected, ip=%s, rssi=%d", ipString().c_str(), rssi());
      return true;
    }

    if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || status == WL_CONNECTION_LOST)
    {
      lastReason_ = static_cast<int>(status);
      state_ = State::Failed;
      LOGW(TAG, "connect failed, status=%d(%s)", static_cast<int>(status), statusText(status));
      return false;
    }

    state_ = State::WaitingIP;
    delay(50);
  }

  lastReason_ = static_cast<int>(WiFi.status());
  state_ = State::Failed;
  LOGW(TAG, "connect timeout, status=%d(%s)", lastReason_, statusText(WiFi.status()));
  return false;
}

/**
 * @brief 轮询并维护 WiFi 连接状态。
 *
 * 当当前内部状态为已连接（State::Connected）但底层 WiFi 实际状态
 * 不再是 WL_CONNECTED 时，本函数会将状态切换为已断开（State::Disconnected），
 * 并记录当前的断开原因码到 lastReason_（来源于 WiFi.status()）。
 *
 * @note 建议在主循环中周期性调用，以便及时检测并响应连接中断。
 */
void Wifi::loop()
{
  if (state_ == State::Connected && WiFi.status() != WL_CONNECTED)
  {
    const wl_status_t status = WiFi.status();
    const int reason = static_cast<int>(status);
    state_ = State::Disconnected;
    lastReason_ = reason;
    LOGW(TAG, "connection lost, status=%d(%s)", reason, statusText(status));
  }
}

/**
 * @brief 确保 WiFi 已连接，并在需要时尝试建立连接。
 *
 * 该方法按以下顺序执行：
 * 1. 若当前已连接，则将内部状态设置为 Connected 并立即返回 true；
 * 2. 若未连接，则先调用 connect() 发起连接；
 * 3. 若 connect() 失败，返回 false；
 * 4. 若 connect() 成功，则在给定超时时间内调用 waitConnected(timeoutMs) 等待连接完成，
 *    并返回其结果。
 *
 * @param timeoutMs 等待连接完成的超时时间（毫秒）。
 * @return true 表示已连接或在超时前连接成功；false 表示连接发起失败或等待超时/失败。
 */
bool Wifi::ensureConnected(uint32_t timeoutMs)
{
  if (isConnected())
  {
    state_ = State::Connected;
    return true;
  }

  if (!connect())
    return false;

  return waitConnected(timeoutMs);
}

/**
 * @brief 获取当前 Wi-Fi 连接的本机 IP 地址字符串。
 *
 * 当设备未连接到 Wi-Fi 时，返回空字符串；
 * 当设备已连接时，返回 `WiFi.localIP()` 对应的点分十进制字符串（如 `"192.168.1.10"`）。
 *
 * @return std::string 当前本机 IP 地址；若未连接则返回空字符串。
 */
std::string Wifi::ipString() const
{
  if (!isConnected())
    return std::string();

  String ip = WiFi.localIP().toString();
  return std::string(ip.c_str());
}

/**
 * @brief 获取当前 Wi-Fi 连接的信号强度（RSSI）。
 *
 * 当设备未连接到 Wi-Fi 时，返回 `0`；
 * 当设备已连接时，返回底层 Wi-Fi 接口提供的 RSSI 值（通常单位为 dBm）。
 *
 * @return int 当前信号强度；未连接时为 `0`。
 */
int Wifi::rssi() const
{
  if (!isConnected())
    return 0;

  return WiFi.RSSI();
}

/**
 * @brief 获取当前 WiFi 状态。
 *
 * 该方法为只读访问器，不会修改对象内部状态。
 *
 * @return Wifi::State 当前保存的 WiFi 状态值。
 */
Wifi::State Wifi::state() const
{
  return state_;
}

/**
 * @brief 获取最近一次 Wi-Fi 连接失败或断开时记录的原因码。
 *
 * 该方法返回内部保存的 `lastReason_` 值，可用于诊断连接异常、
 * 断连原因或重连逻辑中的错误处理分支。
 *
 * @return int 最近一次记录的 Wi-Fi 原因码。
 */
int Wifi::lastReason() const
{
  return lastReason_;
}
