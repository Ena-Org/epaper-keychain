#include "wifi.hpp"

static WifiManager *g_wifiSelf = nullptr;

WifiManager::WifiManager()
{
  g_wifiSelf = this;
}

void WifiManager::setHostname(const char *hostname)
{
  _hostname = hostname ? hostname : _hostname;
}

void WifiManager::setAutoReconnect(bool enable)
{
  _autoReconnect = enable;
}

void WifiManager::setPowerSave(bool enable)
{
  _powerSave = enable;
}

void WifiManager::begin(const char *ssid, const char *password)
{
  _ssid = ssid;
  _password = password;

  WiFi.persistent(false); // 不写NVS，少掉一堆奇怪问题
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(_autoReconnect);

  // ESP32 Arduino: hostname要在 connect 前设置
  WiFi.setHostname(_hostname);

  // 省电模式（会影响延迟与吞吐，按需求开）
  WiFi.setSleep(_powerSave);

  // 事件回调，跟踪连接状态
  WiFi.onEvent(_onWiFiEvent);
}

bool WifiManager::connect(uint32_t timeoutMs, uint8_t retry)
{
  if (!_ssid || !_password)
    return false;
  if (isConnected())
    return true;

  for (uint8_t attempt = 0; attempt <= retry; attempt++)
  {
    Serial.printf("[WiFi] Connecting to %s (attempt %u/%u)\n", _ssid, attempt + 1, retry + 1);

    WiFi.disconnect(true, true);
    delay(50);

    WiFi.begin(_ssid, _password);

    const uint32_t start = millis();
    while (millis() - start < timeoutMs)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        _connected = true;
        Serial.printf("[WiFi] Connected. IP=%s RSSI=%d\n", ip().c_str(), rssi());
        return true;
      }
      delay(200);
    }

    Serial.println("[WiFi] Connect timeout.");
  }

  _connected = false;
  return false;
}

void WifiManager::loop()
{
  if (isConnected())
    return;
  if (!_autoReconnect)
    return;

  const uint32_t now = millis();
  if (now - _lastReconnectAttemptMs < _reconnectIntervalMs)
    return;
  _lastReconnectAttemptMs = now;

  // 非阻塞重连策略：短超时，多次尝试
  connect(4000, 0);
}

bool WifiManager::isConnected() const
{
  return _connected && (WiFi.status() == WL_CONNECTED);
}

String WifiManager::ip() const
{
  if (WiFi.status() != WL_CONNECTED)
    return String("0.0.0.0");
  return WiFi.localIP().toString();
}

int8_t WifiManager::rssi() const
{
  if (WiFi.status() != WL_CONNECTED)
    return 0;
  return WiFi.RSSI();
}

void WifiManager::disconnect(bool turnOffWifi)
{
  WiFi.disconnect(true, true);
  _connected = false;
  if (turnOffWifi)
    WiFi.mode(WIFI_OFF);
}

void WifiManager::_onWiFiEvent(WiFiEvent_t event)
{
  if (!g_wifiSelf)
    return;

  switch (event)
  {
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    g_wifiSelf->_connected = true;
    Serial.printf("[WiFi] Got IP: %s\n", WiFi.localIP().toString().c_str());
    break;

  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    g_wifiSelf->_connected = false;
    Serial.println("[WiFi] Disconnected.");
    break;

  default:
    break;
  }
}
