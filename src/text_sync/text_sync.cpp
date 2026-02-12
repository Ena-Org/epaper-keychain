#include "text_sync.hpp"

TextSync::TextSync(SimpleHttp &http) : _http(http) {}

void TextSync::begin(const String &url)
{
  _url = url;
}

void TextSync::setIntervalMs(uint32_t ms)
{
  _intervalMs = ms;
}

void TextSync::setAuthBearer(const String &token)
{
  _bearerToken = token;
}

void TextSync::setAcceptJson(bool enable)
{
  _acceptJson = enable;
}

void TextSync::setPostMode(bool enable, const String &payload, const String &contentType)
{
  _usePost = enable;
  _postPayload = payload;
  _postContentType = contentType;
}

bool TextSync::poll(String &outText)
{
  return _fetch(outText, false);
}

bool TextSync::fetchNow(String &outText)
{
  return _fetch(outText, true);
}

bool TextSync::_fetch(String &outText, bool force)
{
  if (_url.length() == 0)
    return false;

  const uint32_t now = millis();
  if (!force)
  {
    if (now - _lastPollMs < _intervalMs)
      return false;
  }
  _lastPollMs = now;

  // headers
  HttpHeader headers[3];
  size_t headerCount = 0;

  // Accept
  if (_acceptJson)
  {
    headers[headerCount++] = {"Accept", "application/json"};
  }
  else
  {
    headers[headerCount++] = {"Accept", "text/plain"};
  }

  // Authorization
  String auth;
  if (_bearerToken.length() > 0)
  {
    auth = "Bearer " + _bearerToken;
    headers[headerCount++] = {"Authorization", auth.c_str()};
  }

  HttpResponse r;
  if (_usePost)
  {
    const char *ctype = _postContentType.length() ? _postContentType.c_str() : nullptr;
    r = _http.request("POST", _url, _postPayload, ctype, headers, headerCount);
  }
  else
  {
    r = _http.get(_url, headers, headerCount);
  }

  if (r.status != 200)
  {
    Serial.printf("[TextSync] HTTP %d\n", r.status);
    return false;
  }

  String text;

  if (_acceptJson)
  {
    if (!_extractTextFromJson(r.body, text))
    {
      Serial.println("[TextSync] JSON parse failed (lightweight parser).");
      return false;
    }
  }
  else
  {
    text = r.body;
  }

  text.trim();
  if (text.length() == 0)
    return false;

  // 去重：只有文本变化才算“新数据”
  if (text == _lastText)
    return false;

  _lastText = text;
  outText = text;
  return true;
}

bool TextSync::_extractTextFromJson(const String &body, String &text)
{
  // 轻量版：只支持类似 {"text":"hello"} 这种
  // 复杂 JSON 建议直接上 ArduinoJson（但你得自己接受“依赖地狱”的快乐）
  int keyPos = body.indexOf("\"text\"");
  if (keyPos < 0)
    return false;

  int colon = body.indexOf(':', keyPos);
  if (colon < 0)
    return false;

  int firstQuote = body.indexOf('\"', colon + 1);
  if (firstQuote < 0)
    return false;

  int secondQuote = body.indexOf('\"', firstQuote + 1);
  if (secondQuote < 0)
    return false;

  text = body.substring(firstQuote + 1, secondQuote);
  return true;
}
