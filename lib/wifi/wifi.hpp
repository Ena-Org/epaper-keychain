#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WifiManager
{
public:
  WifiManager();

  void setHostname(const char *hostname);
  void setAutoReconnect(bool enable);
  void setPowerSave(bool enable);

  void begin(const char *ssid, const char *password);

  // 阻塞式连接（建议只在启动阶段用）
  bool connect(uint32_t timeoutMs = 15000, uint8_t retry = 2);

  // 非阻塞式维护（loop里调用）
  void loop();

  bool isConnected() const;
  String ip() const;
  int8_t rssi() const;

  void disconnect(bool turnOffWifi = false);

private:
  const char *_ssid = nullptr;
  const char *_password = nullptr;

  const char *_hostname = "epaper-keychain";
  bool _autoReconnect = true;
  bool _powerSave = false;

  uint32_t _lastReconnectAttemptMs = 0;
  uint32_t _reconnectIntervalMs = 3000;

  volatile bool _connected = false;

  static void _onWiFiEvent(WiFiEvent_t event);
};
