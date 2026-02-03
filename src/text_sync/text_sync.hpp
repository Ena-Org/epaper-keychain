#pragma once
#include <Arduino.h>
#include "http.hpp" 

class TextSync
{
public:
  explicit TextSync(SimpleHttp &http);

  // url 示例: "http://192.168.1.10:8080/api/text/latest"
  void begin(const String &url);

  void setIntervalMs(uint32_t ms);
  void setAuthBearer(const String &token); // 可选：Bearer token
  void setAcceptJson(bool enable);         // 可选：服务端返回 json 时开

  // loop里调用：如果拿到“新文本”，返回 true，并把文本塞到 outText
  bool poll(String &outText);

  // 立刻拉一次（不管间隔），适合启动时
  bool fetchNow(String &outText);

private:
  SimpleHttp &_http;
  String _url;

  uint32_t _intervalMs = 5000;
  uint32_t _lastPollMs = 0;

  String _bearerToken;
  bool _acceptJson = false;

  String _lastText;

  bool _fetch(String &outText, bool force);

  // 如果返回 JSON（例如 {"text":"hello"}），这里做个轻量提取
  // 不依赖 ArduinoJson，避免你引入一堆库后再来骂我
  bool _extractTextFromJson(const String &body, String &text);
};
