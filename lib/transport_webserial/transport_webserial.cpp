#include "transport_webserial.hpp"

#include <stdlib.h>
#include <string.h>

namespace
{

  /**
   * @brief 无效参数错误代码
   * @details 当函数接收到无效或不合法的参数时返回的错误码常量
   * @note 错误代码值为 1
   */
  constexpr int kErrBadArgs = 1;

  /**
   * @brief 没有流的错误代码
   * @details 当WebSerial传输层无法获取有效的数据流时返回此错误代码
   * @note 错误代码值为 2
   */
  constexpr int kErrNoStream = 2;

  /**
   * @brief 内存不足错误代码
   * @details 表示因内存分配失败或可用内存不足而导致的错误状态
   * @note 错误代码值为 3
   */
  constexpr int kErrNoMemory = 3;

  /**
   * @brief 未连接错误代码
   * @details 表示WebSerial传输层未建立连接时的错误代码。
   *          当尝试通过未连接的传输层进行通信操作时，会返回此错误代码。
   * @note 错误代码值为 4
   */
  constexpr int kErrNotConnected = 4;

  /**
   * @brief 写入失败错误码。
   * @details 当通过 WebSerial 进行数据发送时，如果底层写入操作未成功完成
   *          （例如设备断开、缓冲区错误或传输中断），返回此错误码表示写入失败。
   * @note 错误代码值为 5
   */
  constexpr int kErrWriteFail = 5;

  /**
   * @brief 解码失败错误代码
   * @details 表示在数据传输过程中发生解码失败的错误类型
   * @note 错误代码值为 6
   */
  constexpr int kErrDecodeFail = 6;

  /**
   * @brief 编码失败的错误代码
   * @details 当数据编码操作失败时返回的错误码常量
   * @note 错误代码值为 7
   */
  constexpr int kErrEncodeFail = 7;

  /**
   * @brief SLIP协议的帧结束标记
   * @details SLIP (Serial Line Internet Protocol) 使用此字节值表示数据帧的结束。
   *          当传输数据时，0xC0 字节用于标记一个完整数据包的边界。
   * @note 这是一个常量值，在SLIP编码/解码过程中不可修改。
   * @see RFC 1055 - Serial Line Internet Protocol (SLIP)
   */
  constexpr uint8_t kSlipEnd = 0xC0;

  /**
   * @brief SLIP协议的转义字节常量
   * @details SLIP(Serial Line Internet Protocol)协议中用于转义特殊字符的字节值。
   *          当数据中出现需要转义的字节时，会使用此字节作为转义标记。
   * @note 此常量的值为0xDB，符合SLIP协议规范。
   */
  constexpr uint8_t kSlipEsc = 0xDB;

  /**
   * @brief SLIP协议的转义字符结束标记
   *
   * 用于SLIP (Serial Line Internet Protocol) 协议中,表示转义序列的结束字符。
   * 当原始数据包含0xC0字节时,会被转义为0xDB 0xDC的两字节序列。
   *
   * @note 这是SLIP协议的标准定义常量
   * @see RFC 1055
   */
  constexpr uint8_t kSlipEscEnd = 0xDC;

  /**
   * @brief SLIP协议中的转义字符转义值
   *
   * 用于SLIP（Serial Line Internet Protocol）协议中，
   * 表示转义字符本身的转义编码值。
   * 当需要在数据中传输转义字符0xDB时，应使用此值进行转义。
   *
   * @see kSlipEsc
   */
  constexpr uint8_t kSlipEscEsc = 0xDD;

  /**
   * @brief COBS（一致性开销字节填充）编码函数
   *
   * 将输入数据进行COBS编码，用于消除数据中的零字节。
   * COBS编码通过插入长度字节来标记零字节的位置，确保编码后的数据不包含零字节。
   *
   * @param input 指向输入数据缓冲区的指针
   * @param len 输入数据的长度（字节数）
   * @param output 指向输出缓冲区的指针，用于存储编码后的数据
   * @param out_max 输出缓冲区的最大容量（字节数）
   *
   * @return 编码后数据的字节数。如果编码失败（输入/输出指针为空、缓冲区溢出等），返回0
   *
   * @note
   *   - 输出缓冲区需要足够大来容纳编码后的数据，最坏情况下需要 len + len/254 + 1 字节
   *   - 如果输入或输出指针为nullptr，或输出缓冲区大小为0，函数返回0
   *   - 如果在编码过程中写入会超出输出缓冲区范围，编码失败并返回0
   *   - 编码遵守标准COBS算法，代码字节最大值为0xFF
   */
  size_t cobsEncode(const uint8_t *input, size_t len, uint8_t *output, size_t out_max)
  {
    if (input == nullptr || output == nullptr || out_max == 0)
    {
      return 0;
    }

    size_t read_idx = 0;
    size_t code_idx = 0;
    size_t write_idx = 1;
    uint8_t code = 1;

    while (read_idx < len)
    {
      if (input[read_idx] == 0)
      {
        if (code_idx >= out_max)
        {
          return 0;
        }
        output[code_idx] = code;
        code = 1;
        code_idx = write_idx;
        if (++write_idx > out_max)
        {
          return 0;
        }
        ++read_idx;
        continue;
      }

      if (write_idx >= out_max)
      {
        return 0;
      }
      output[write_idx++] = input[read_idx++];
      ++code;

      if (code == 0xFF)
      {
        if (code_idx >= out_max)
        {
          return 0;
        }
        output[code_idx] = code;
        code = 1;
        code_idx = write_idx;
        if (++write_idx > out_max)
        {
          return 0;
        }
      }
    }

    if (code_idx >= out_max)
    {
      return 0;
    }
    output[code_idx] = code;
    return write_idx;
  }

  /**
   * @brief 原地解码COBS编码的数据
   *
   * @details
   * 此函数实现了Consistent Overhead Byte Stuffing (COBS)的解码算法。
   * COBS是一种编码方案,用于将任意二进制数据转换为不包含零字节的形式。
   * 本函数在输入缓冲区上原地执行解码操作。
   *
   * @param[in,out] buffer 指向包含COBS编码数据的缓冲区指针。
   *                        解码后的数据将写入同一缓冲区。
   * @param[in] len 缓冲区中编码数据的长度(字节数)。
   * @param[out] out_len 解码后数据的长度(字节数)。
   *
   * @return bool 解码是否成功
   *         - true: 解码成功,out_len中包含解码后的数据长度
   *         - false: 解码失败,输入缓冲区无效或编码数据格式错误
   *
   * @note
   * - 如果buffer为nullptr或len为0,函数返回false
   * - 如果检测到非法的零值编码码或读取超出缓冲区边界,函数返回false
   * - 原地解码意味着写入位置始终不会超过读取位置,因此不会覆盖未处理的数据
   *
   * @warning
   * 调用者必须确保buffer指向的内存块至少为len字节大小
   */
  bool cobsDecodeInPlace(uint8_t *buffer, size_t len, size_t &out_len)
  {
    out_len = 0;
    if (buffer == nullptr || len == 0)
    {
      return false;
    }

    size_t read_idx = 0;
    size_t write_idx = 0;

    while (read_idx < len)
    {
      const uint8_t code = buffer[read_idx++];
      if (code == 0)
      {
        return false;
      }

      const size_t copy_len = static_cast<size_t>(code - 1);
      if (read_idx + copy_len > len)
      {
        return false;
      }

      for (size_t i = 0; i < copy_len; ++i)
      {
        buffer[write_idx++] = buffer[read_idx++];
      }

      if (code != 0xFF && read_idx < len)
      {
        buffer[write_idx++] = 0;
      }
    }

    out_len = write_idx;
    return true;
  }

  /**
   * @brief SLIP协议编码函数
   *
   * 将输入数据按照SLIP（Serial Line Internet Protocol）协议进行编码。
   * 该函数将特殊字符进行转义处理，确保数据能够安全地通过串行链路传输。
   *
   * @param input 指向输入数据缓冲区的指针
   * @param len 输入数据的长度（字节数）
   * @param output 指向输出数据缓冲区的指针，用于存储编码后的数据
   * @param out_max 输出缓冲区的最大容量（字节数）
   *
   * @return 编码后数据的长度（字节数）。如果输入或输出指针为空，或输出缓冲区空间不足，返回0
   *
   * @note 编码规则：
   *       - kSlipEnd 字节被转义为 kSlipEsc + kSlipEscEnd
   *       - kSlipEsc 字节被转义为 kSlipEsc + kSlipEscEsc
   *       - 其他字节保持不变
   *
   * @warning 调用者必须确保输出缓冲区有足够的空间来容纳编码后的数据
   */
  size_t slipEncode(const uint8_t *input, size_t len, uint8_t *output, size_t out_max)
  {
    if (input == nullptr || output == nullptr)
    {
      return 0;
    }

    size_t out_len = 0;
    for (size_t i = 0; i < len; ++i)
    {
      const uint8_t b = input[i];
      if (b == kSlipEnd)
      {
        if (out_len + 2 > out_max)
        {
          return 0;
        }
        output[out_len++] = kSlipEsc;
        output[out_len++] = kSlipEscEnd;
      }
      else if (b == kSlipEsc)
      {
        if (out_len + 2 > out_max)
        {
          return 0;
        }
        output[out_len++] = kSlipEsc;
        output[out_len++] = kSlipEscEsc;
      }
      else
      {
        if (out_len + 1 > out_max)
        {
          return 0;
        }
        output[out_len++] = b;
      }
    }

    return out_len;
  }
}

/**
 * @brief 初始化 WebSerial 传输连接
 *
 * @details 该函数初始化 WebSerial 传输模块，配置接收缓冲区，
 *          设置初始状态并触发连接事件。在调用前会自动调用 end()
 *          以清理任何现有的连接。
 *
 * @param[in] cfg WebSerial 配置参数，包含流对象和缓冲区大小等信息
 *
 * @return bool 初始化是否成功
 *         - true: 初始化成功，WebSerial 连接已建立
 *         - false: 初始化失败，可通过 getLastError() 获取错误原因
 *
 * @retval true 初始化成功
 * @retval false 初始化失败（可能原因：流对象为空、接收缓冲区大小为0、内存分配失败）
 *
 * @note 该函数会分配动态内存用于接收缓冲区，需确保在调用 end() 时释放
 * @note 如果已注册事件回调函数，初始化成功后会触发 Connected 事件
 *
 * @see end()
 * @see getLastError()
 * @see Event
 */
bool TransportWebserial::begin(const WebSerialConfig &cfg)
{
  end();

  _cfg = cfg;
  _lastErr = 0;
  _rxBytes = 0;
  _txBytes = 0;
  _rxLen = 0;
  _slipEsc = false;

  if (_cfg.stream == nullptr || _cfg.rx_max == 0)
  {
    _lastErr = (_cfg.stream == nullptr) ? kErrNoStream : kErrBadArgs;
    return false;
  }

  _rxBuf = static_cast<uint8_t *>(malloc(_cfg.rx_max));
  if (_rxBuf == nullptr)
  {
    _lastErr = kErrNoMemory;
    return false;
  }

  _connected = true;
  if (_onEvt != nullptr)
  {
    _onEvt(Event::Connected, 0, _onEvtUser);
  }

  return true;
}

/**
 * @brief 关闭WebSerial传输连接
 *
 * 该函数用于终止WebSerial传输连接。它会重置连接状态、清空配置参数、
 * 释放接收缓冲区内存，并在之前已连接的情况下触发断开连接事件回调。
 *
 * @details
 * 具体执行以下操作：
 * - 记录连接前的状态
 * - 设置连接标志为false
 * - 重置流指针为nullptr
 * - 恢复波特率为115200
 * - 设置编解码器为Line模式
 * - 设置行分隔符为'\n'
 * - 设置最大接收字节数为2048
 * - 设置连接超时为0
 * - 释放接收缓冲区内存
 * - 重置接收长度和转义标志
 * - 如果之前已连接，则触发断开连接事件回调
 *
 * @return void
 *
 * @note 该函数在断开连接前会自动触发已注册的事件回调函数
 *
 * @see Event::Disconnected
 */
void TransportWebserial::end()
{
  const bool wasConnected = _connected;

  _connected = false;
  _cfg.stream = nullptr;
  _cfg.baud = 115200;
  _cfg.codec = Codec::Line;
  _cfg.line_delim = '\n';
  _cfg.rx_max = 2048;
  _cfg.connect_timeout_ms = 0;

  if (_rxBuf != nullptr)
  {
    free(_rxBuf);
    _rxBuf = nullptr;
  }
  _rxLen = 0;
  _slipEsc = false;

  if (wasConnected && _onEvt != nullptr)
  {
    _onEvt(Event::Disconnected, 0, _onEvtUser);
  }
}

/**
 * @brief 处理Web串口传输的主循环函数
 *
 * 该函数在主程序循环中被周期性调用，用于处理接收到的数据。
 * 通过调用handleRx_()方法来处理来自Web串口的传入消息。
 *
 * @details
 * 此函数负责：
 * - 检查并处理接收缓冲区中的数据
 * - 更新传输状态
 * - 处理任何待处理的接收操作
 *
 * @note 该函数应该在主程序的主循环中被频繁调用，以确保及时处理传入数据
 *
 * @see handleRx_()
 */
void TransportWebserial::loop()
{
  handleRx_();
}

/**
 * @brief 检查传输层是否已连接
 *
 * @return true 如果WebSerial传输层已连接
 * @return false 如果WebSerial传输层未连接
 */
bool TransportWebserial::isConnected() const
{
  return _connected;
}

/**
 * @brief 等待WebSerial连接建立
 *
 * @details 该函数会阻塞直到连接建立或超时。函数会周期性地调用loop()方法
 *          以处理连接事件，每次迭代之间延迟1毫秒。
 *
 * @param timeout_ms 等待超时时间（毫秒）。如果为0，表示无限等待直到连接建立
 *
 * @return true 如果连接已建立或在超时前成功建立
 * @return false 如果在指定超时时间内未能建立连接
 *
 * @note 该函数为阻塞式调用，会占用CPU资源进行轮询
 *
 * @see loop()
 */
bool TransportWebserial::waitForConnection(uint32_t timeout_ms)
{
  if (_connected)
  {
    return true;
  }

  const uint32_t start = millis();
  while (!_connected)
  {
    loop();
    if (timeout_ms > 0 && (millis() - start) >= timeout_ms)
    {
      return false;
    }
    delay(1);
  }
  return true;
}

/**
 * @brief 获取最后发生的错误代码
 *
 * @return int 最后发生的错误代码。如果没有错误发生，返回0
 *
 * @note 此方法不会清除错误状态，多次调用会返回相同的错误代码
 */
int TransportWebserial::lastError() const
{
  return _lastErr;
}

/**
 * @brief 设置消息回调函数
 *
 * 用于注册一个回调函数，当接收到消息时调用该函数进行处理。
 *
 * @param cb 消息回调函数指针，该函数在接收到消息时被触发
 * @param user 用户自定义数据指针，将在调用回调函数时作为参数传递
 *
 * @note 调用此函数前，确保传入的回调函数指针有效
 * @see MessageCallback
 */
void TransportWebserial::setMessageCallback(MessageCallback cb, void *user)
{
  _onMsg = cb;
  _onMsgUser = user;
}

/**
 * @brief 设置事件回调函数
 *
 * 注册一个事件回调函数，当传输层发生事件时将调用此回调函数。
 *
 * @param cb 事件回调函数指针，类型为 EventCallback
 * @param user 用户自定义数据指针，将在回调函数被调用时传递给回调函数
 *
 * @note 用户需要确保回调函数的生命周期有效，直到取消注册或对象销毁
 *
 * @see EventCallback
 */
void TransportWebserial::setEventCallback(EventCallback cb, void *user)
{
  _onEvt = cb;
  _onEvtUser = user;
}

/**
 * @brief 通过WebSerial传输发送数据
 *
 * @param[in] data 指向待发送数据的指针，不能为空指针
 * @param[in] len 待发送数据的长度（字节数），必须大于0
 *
 * @return true 数据发送成功
 * @return false 数据发送失败，可通过 _lastErr 获取错误信息
 *
 * @note 当 data 为空指针或 len 为0时，函数会设置错误码为 kErrBadArgs 并返回false
 *
 * @see encodeAndWrite_
 */
bool TransportWebserial::send(const uint8_t *data, size_t len)
{
  if (data == nullptr || len == 0)
  {
    _lastErr = kErrBadArgs;
    return false;
  }
  return encodeAndWrite_(data, len);
}

/**
 * @brief 通过Web Serial发送字符串数据
 *
 * @param s 待发送的C风格字符串指针，不能为nullptr
 *
 * @return true 如果字符串成功编码并写入，或字符串为空；false 如果参数无效
 *
 * @retval true 数据发送成功或字符串长度为0
 * @retval false 传入的字符串指针为nullptr，此时_lastErr将被设置为kErrBadArgs
 *
 * @note 该函数会自动计算字符串长度，并将其转换为uint8_t字节数组进行编码和传输
 * @note 空字符串（长度为0）被视为有效输入，函数直接返回true不进行写操作
 */
bool TransportWebserial::send(const char *s)
{
  if (s == nullptr)
  {
    _lastErr = kErrBadArgs;
    return false;
  }

  const size_t len = strlen(s);
  if (len == 0)
  {
    return true;
  }
  return encodeAndWrite_(reinterpret_cast<const uint8_t *>(s), len);
}

/**
 * @brief 发送字符串数据
 *
 * 将给定的字符串编码后通过 WebSerial 传输发送。
 * 如果字符串为空，则直接返回成功。
 *
 * @param s 要发送的字符串引用
 *
 * @return bool 如果字符串为空返回 true；否则返回编码和写入操作的结果
 *
 * @note 字符串会被转换为 uint8_t 数组后进行编码和传输
 */
bool TransportWebserial::send(const String &s)
{
  if (s.length() == 0)
  {
    return true;
  }
  return encodeAndWrite_(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
}

/**
 * @brief 刷新输出缓冲区
 *
 * 将待发送的数据从缓冲区刷新到底层串行流中。
 * 如果流对象已初始化，则调用其 flush() 方法以确保
 * 所有待处理的数据都被发送。
 *
 * @note 此操作仅在流对象存在时执行
 *
 * @see _cfg.stream
 */
void TransportWebserial::flush()
{
  if (_cfg.stream != nullptr)
  {
    _cfg.stream->flush();
  }
}

/**
 * @brief 向串行流写入原始数据
 *
 * 将指定长度的数据写入底层串行流。此函数会追踪已发送的字节总数,
 * 并在写入失败时设置错误状态。
 *
 * @param[in] data 指向要写入数据的指针,不能为 nullptr
 * @param[in] len 要写入的字节数,必须大于 0
 *
 * @return 实际写入的字节数。如果返回值小于 len,表示写入不完整;
 *         如果返回 0,表示未写入任何数据
 *
 * @retval 0 在以下情况下返回:
 *         - 传输未连接 (设置 _lastErr = kErrNotConnected)
 *         - 配置的流为空指针 (设置 _lastErr = kErrNotConnected)
 *         - 数据指针为空 (设置 _lastErr = kErrBadArgs)
 *         - 长度为 0 (设置 _lastErr = kErrBadArgs)
 *
 * @note 函数会自动累加已发送字节数到 _txBytes 成员变量
 * @note 若实际写入字节数不等于请求的字节数,会设置 _lastErr = kErrWriteFail
 *
 * @see _lastErr, _txBytes, kErrNotConnected, kErrBadArgs, kErrWriteFail
 */
size_t TransportWebserial::writeRaw(const uint8_t *data, size_t len)
{
  if (!_connected || _cfg.stream == nullptr)
  {
    _lastErr = kErrNotConnected;
    return 0;
  }
  if (data == nullptr || len == 0)
  {
    _lastErr = kErrBadArgs;
    return 0;
  }

  const size_t written = _cfg.stream->write(data, len);
  _txBytes += static_cast<uint32_t>(written);
  if (written != len)
  {
    _lastErr = kErrWriteFail;
  }
  return written;
}

/**
 * @brief 获取原始传输层可用的字节数
 *
 * 检查 WebSerial 传输连接状态和流对象的有效性，
 * 如果连接正常且流对象有效，则返回底层流中可用的字节数。
 *
 * @return int 返回可用的字节数。如果未连接或流对象为空指针，返回 0；
 *             否则返回底层流对象中可用的字节数
 *
 * @note 此函数为常成员函数，不修改对象状态
 * @see _connected, _cfg.stream
 */
int TransportWebserial::availableRaw() const
{
  if (!_connected || _cfg.stream == nullptr)
  {
    return 0;
  }
  return _cfg.stream->available();
}

/**
 * @brief 从WebSerial流中读取原始数据
 *
 * 该函数从连接的WebSerial流中读取最多max_len字节的数据，
 * 并将其存储到输出缓冲区中。该函数会持续读取数据直到
 * 流中没有更多可用数据或达到最大长度限制。
 *
 * @param[out] out 指向输出缓冲区的指针，用于存储读取的数据
 * @param[in] max_len 输出缓冲区的最大长度（字节数）
 *
 * @return 成功读取的字节数（>=0），失败时返回-1
 *
 * @retval -1 发生错误（未连接或参数无效）
 * @retval >=0 实际读取的字节数
 *
 * @note 该函数会自动更新接收字节计数器(_rxBytes)
 *
 * @see _lastErr 查看具体错误信息
 */
int TransportWebserial::readRaw(uint8_t *out, size_t max_len)
{
  if (!_connected || _cfg.stream == nullptr)
  {
    _lastErr = kErrNotConnected;
    return -1;
  }
  if (out == nullptr || max_len == 0)
  {
    _lastErr = kErrBadArgs;
    return -1;
  }

  size_t total = 0;
  while (total < max_len)
  {
    const int v = _cfg.stream->read();
    if (v < 0)
    {
      break;
    }
    out[total++] = static_cast<uint8_t>(v);
  }

  _rxBytes += static_cast<uint32_t>(total);
  return static_cast<int>(total);
}

/**
 * @brief 获取已接收的字节数
 *
 * @return uint32_t 接收的字节总数
 */
uint32_t TransportWebserial::bytesRx() const
{
  return _rxBytes;
}

/**
 * @brief 获取已发送的字节数
 *
 * @return uint32_t 通过 WebSerial 传输已发送的字节总数
 */
uint32_t TransportWebserial::bytesTx() const
{
  return _txBytes;
}

/**
 * @brief 重置解析器状态
 *
 * 将接收缓冲区长度重置为0，并清除SLIP转义标志。
 * 该函数用于初始化或重新开始数据包解析过程。
 *
 * @return void
 */
void TransportWebserial::resetParser()
{
  _rxLen = 0;
  _slipEsc = false;
}

/**
 * @brief 处理接收数据的核心方法
 *
 * 从配置的数据流中读取可用数据，并根据配置的编解码器类型进行相应的处理。
 * 支持四种编解码模式：Raw（原始）、Line（行分隔符）、COBS（一致开销字节填充）和SLIP（串行行网际协议）。
 *
 * 处理流程：
 * - 检查连接状态、数据流和接收缓冲区的有效性
 * - 循环读取可用数据，针对每个字节根据编解码器类型进行处理
 * - Raw模式：直接将字节推送至帧队列
 * - Line模式：累积字节直到遇到分隔符，然后推送完整的帧
 * - COBS模式：累积字节直到遇到0x00，解码后推送帧
 * - SLIP模式：处理转义序列，累积字节直到遇到帧结束标记，然后推送帧
 *
 * @note 此方法应定期调用以确保及时处理接收到的数据
 * @note 如果接收缓冲区溢出，将触发错误回调（如果已设置）
 * @note 统计信息 _rxBytes 会累计所有接收到的字节数
 *
 * @return void
 *
 * @see pushFrame_()
 * @see cobsDecodeInPlace()
 * @see _cfg
 * @see _onEvt
 */
void TransportWebserial::handleRx_()
{
  if (!_connected || _cfg.stream == nullptr || _rxBuf == nullptr)
    return;

  while (_cfg.stream->available() > 0)
  {
    const int v = _cfg.stream->read();
    if (v < 0)
      break;

    const uint8_t byte = static_cast<uint8_t>(v);
    _rxBytes += 1;

    if (_cfg.codec == Codec::Raw)
    {
      (void)pushFrame_(&byte, 1);
      continue;
    }

    auto raiseError = [this](int code)
    {
      _lastErr = code;
      _rxLen = 0;
      _slipEsc = false;
      if (_onEvt != nullptr)
        _onEvt(Event::Error, _lastErr, _onEvtUser);
    };

    if (_cfg.codec == Codec::Line)
    {
      if (byte == static_cast<uint8_t>(_cfg.line_delim))
      {
        if (_rxLen > 0)
        {
          (void)pushFrame_(_rxBuf, _rxLen);
          _rxLen = 0;
        }
        continue;
      }

      if (_rxLen < _cfg.rx_max)
        _rxBuf[_rxLen++] = byte;
      else
        raiseError(kErrBadArgs);

      continue;
    }

    if (_cfg.codec == Codec::COBS)
    {
      if (byte == 0x00)
      {
        if (_rxLen > 0)
        {
          size_t decoded_len = 0;
          if (!cobsDecodeInPlace(_rxBuf, _rxLen, decoded_len))
          {
            raiseError(kErrDecodeFail);
            continue;
          }

          if (decoded_len > 0)
            (void)pushFrame_(_rxBuf, decoded_len);
        }
        _rxLen = 0;
        continue;
      }

      if (_rxLen < _cfg.rx_max)
        _rxBuf[_rxLen++] = byte;
      else
        raiseError(kErrBadArgs);

      continue;
    }

    if (_cfg.codec == Codec::SLIP)
    {
      if (byte == kSlipEnd)
      {
        if (_rxLen > 0)
        {
          (void)pushFrame_(_rxBuf, _rxLen);
          _rxLen = 0;
        }
        _slipEsc = false;
        continue;
      }

      uint8_t decoded = byte;
      if (_slipEsc)
      {
        if (byte == kSlipEscEnd)
        {
          decoded = kSlipEnd;
        }
        else if (byte == kSlipEscEsc)
        {
          decoded = kSlipEsc;
        }
        else
        {
          raiseError(kErrDecodeFail);
          continue;
        }
        _slipEsc = false;
      }
      else if (byte == kSlipEsc)
      {
        _slipEsc = true;
        continue;
      }

      if (_rxLen < _cfg.rx_max)
        _rxBuf[_rxLen++] = decoded;
      else
        raiseError(kErrBadArgs);

      continue;
    }
  }
}

/**
 * @brief 将数据帧推送到消息处理回调函数
 * 
 * 将指定的数据帧通过已注册的消息回调函数进行处理。
 * 如果未设置回调函数或传入参数无效，则不执行任何操作。
 * 
 * @param data 指向待推送数据的指针，不能为空指针
 * @param len 待推送数据的长度（字节数），必须大于0
 * 
 * @return true 数据帧推送成功（参数有效且回调函数已调用）
 * @return false 数据帧推送失败（data为空指针或len为0）
 * 
 * @note 如果 @c _onMsg 为空指针，数据帧虽然通过有效性检查但不会被处理
 * 
 * @see _onMsg
 * @see _onMsgUser
 */
bool TransportWebserial::pushFrame_(const uint8_t *data, size_t len)
{
  if (data == nullptr || len == 0)
    return false;

  if (_onMsg != nullptr)
    _onMsg(data, len, _onMsgUser);

  return true;
}

/**
 * @brief 对数据进行编码并写入传输通道
 * 
 * 根据配置的编码方式（Raw、Line、COBS 或 SLIP）对数据进行编码，
 * 然后通过 writeRaw() 写入到传输流中。
 * 
 * @param[in] data 指向待编码数据的指针
 * @param[in] len 数据的字节长度
 * 
 * @return true 数据编码并写入成功
 * @return false 编码或写入失败，错误代码存储在 _lastErr 中
 * 
 * @retval kErrNotConnected 传输未连接或流对象为空指针
 * @retval kErrBadArgs 输入参数无效（data 为空或 len 为 0）、编码方式不支持
 * @retval kErrNoMemory 动态分配内存失败（COBS 或 SLIP 编码时）
 * @retval kErrEncodeFail 编码过程失败
 * 
 * @note 
 *  - Raw 模式：直接写入原始数据，无编码开销
 *  - Line 模式：写入数据后附加行分隔符（由 _cfg.line_delim 指定）
 *  - COBS 模式：使用一致性开销字节法编码，末尾添加 0x00 帧结束符
 *  - SLIP 模式：使用串行线网际协议编码，末尾添加 kSlipEnd 帧结束符
 * 
 * @warning 对于 COBS 和 SLIP 编码，此函数会动态分配内存，使用者应确保系统有足够内存
 */
bool TransportWebserial::encodeAndWrite_(const uint8_t *data, size_t len)
{
  if (!_connected || _cfg.stream == nullptr)
  {
    _lastErr = kErrNotConnected;
    return false;
  }
  if (data == nullptr || len == 0)
  {
    _lastErr = kErrBadArgs;
    return false;
  }

  if (_cfg.codec == Codec::Raw)
    return writeRaw(data, len) == len;

  if (_cfg.codec == Codec::Line)
  {
    if (writeRaw(data, len) != len)
    {
      return false;
    }
    const uint8_t delim = static_cast<uint8_t>(_cfg.line_delim);
    return writeRaw(&delim, 1) == 1;
  }

  if (_cfg.codec == Codec::COBS)
  {
    const size_t encoded_max = len + (len / 254) + 2;
    uint8_t *encoded = static_cast<uint8_t *>(malloc(encoded_max));
    if (encoded == nullptr)
    {
      _lastErr = kErrNoMemory;
      return false;
    }

    const size_t encoded_len = cobsEncode(data, len, encoded, encoded_max - 1);
    if (encoded_len == 0)
    {
      free(encoded);
      _lastErr = kErrEncodeFail;
      return false;
    }

    const uint8_t frame_end = 0x00;
    const bool ok = (writeRaw(encoded, encoded_len) == encoded_len) &&
                    (writeRaw(&frame_end, 1) == 1);
    free(encoded);
    return ok;
  }

  if (_cfg.codec == Codec::SLIP)
  {
    const size_t encoded_max = (len * 2) + 1;
    uint8_t *encoded = static_cast<uint8_t *>(malloc(encoded_max));
    if (encoded == nullptr)
    {
      _lastErr = kErrNoMemory;
      return false;
    }

    const size_t encoded_len = slipEncode(data, len, encoded, encoded_max - 1);
    if (encoded_len == 0)
    {
      free(encoded);
      _lastErr = kErrEncodeFail;
      return false;
    }

    const uint8_t frame_end = kSlipEnd;
    const bool ok = (writeRaw(encoded, encoded_len) == encoded_len) &&
                    (writeRaw(&frame_end, 1) == 1);
    free(encoded);
    return ok;
  }

  _lastErr = kErrBadArgs;
  return false;
}
