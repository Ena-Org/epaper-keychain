#pragma once
#include <Arduino.h>

struct HttpHeader
{
  const char *key;
  const char *value;
};

struct HttpResponse
{
  int status = -1;    // HTTP status code，失败时为负数
  String body;        // 返回内容
  String contentType; // Content-Type
};

class SimpleHttp
{
public:
  void setTimeout(uint32_t ms);
  void setUserAgent(const char *ua);

  // TLS 方案二选一：
  // 1) setInsecureTLS(true) 省事但不安全（测试可用）
  // 2) setCACert(pem)        生产建议
  void setInsecureTLS(bool insecure);
  void setCACert(const char *pem);

  HttpResponse get(const String &url, const HttpHeader *headers = nullptr, size_t headerCount = 0);
  HttpResponse postJson(const String &url, const String &json,
                        const HttpHeader *headers = nullptr, size_t headerCount = 0);

  HttpResponse request(const String &method, const String &url,
                       const String &payload, const char *contentType,
                       const HttpHeader *headers = nullptr, size_t headerCount = 0);

private:
  uint32_t _timeoutMs = 8000;
  const char *_userAgent = "epaper-keychain/1.0";

  bool _insecureTLS = false;
  const char *_caCertPem = nullptr;
};
