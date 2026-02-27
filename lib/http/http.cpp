#include "http.hpp"

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

void SimpleHttp::setTimeout(uint32_t ms)
{
  _timeoutMs = ms;
}

void SimpleHttp::setUserAgent(const char *ua)
{
  _userAgent = ua ? ua : _userAgent;
}

void SimpleHttp::setInsecureTLS(bool insecure)
{
  _insecureTLS = insecure;
}

void SimpleHttp::setCACert(const char *pem)
{
  _caCertPem = pem;
}

static bool isHttps(const String &url)
{
  return url.startsWith("https://");
}

HttpResponse SimpleHttp::get(const String &url, const HttpHeader *headers, size_t headerCount)
{
  return request("GET", url, "", nullptr, headers, headerCount);
}

HttpResponse SimpleHttp::postJson(const String &url, const String &json,
                                  const HttpHeader *headers, size_t headerCount)
{
  return request("POST", url, json, "application/json", headers, headerCount);
}

HttpResponse SimpleHttp::request(const String &method, const String &url,
                                 const String &payload, const char *contentType,
                                 const HttpHeader *headers, size_t headerCount)
{
  HttpResponse res;

  HTTPClient http;
  http.setTimeout(_timeoutMs);
  http.setUserAgent(_userAgent);

  int code = -1;

  if (isHttps(url))
  {
    WiFiClientSecure client;
    if (_insecureTLS)
      client.setInsecure();
    else if (_caCertPem)
      client.setCACert(_caCertPem);
    else
    {
      // 没有 CA 又没开 insecure，等于你想让它“安全地不工作”
      // 这里给个明确提示
      Serial.println("[HTTP] https without CA cert. Use setCACert() or setInsecureTLS(true).");
      res.status = -2;
      return res;
    }

    if (!http.begin(client, url))
    {
      res.status = -3;
      return res;
    }
  }
  else
  {
    WiFiClient client;
    if (!http.begin(client, url))
    {
      res.status = -3;
      return res;
    }
  }

  // 加 headers
  for (size_t i = 0; i < headerCount; i++)
  {
    if (!headers[i].key || !headers[i].value)
      continue;
    http.addHeader(headers[i].key, headers[i].value);
  }

  if (method == "GET")
  {
    code = http.GET();
  }
  else if (method == "POST")
  {
    if (contentType)
      http.addHeader("Content-Type", contentType);
    code = http.POST((uint8_t *)payload.c_str(), payload.length());
  }
  else if (method == "PUT")
  {
    if (contentType)
      http.addHeader("Content-Type", contentType);
    code = http.PUT((uint8_t *)payload.c_str(), payload.length());
  }
  else if (method == "DELETE")
  {
    code = http.sendRequest("DELETE");
  }
  else
  {
    // 兜底：允许自定义 method
    code = http.sendRequest(method.c_str(), (uint8_t *)payload.c_str(), payload.length());
  }

  res.status = code;

  if (code > 0)
  {
    res.contentType = http.header("Content-Type");
    res.body = http.getString();
  }
  else
  {
    // code <= 0 表示连接层失败（DNS/超时/断网）
    Serial.printf("[HTTP] Request failed: %s\n", http.errorToString(code).c_str());
  }

  http.end();
  return res;
}
