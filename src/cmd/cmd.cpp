#include "cmd.hpp"

#include <vector>

#include "cmd_handlers/cmd_handlers.hpp"
#include "logger.hpp"

namespace
{
  /**
   * @brief 命令模块的日志标签
   * @details 用于在日志输出中标识来自命令处理模块的消息，便于调试和跟踪
   * @note 此标签在整个cmd.cpp文件中作为日志系统的标识使用
   */
  constexpr const char *TAG = "CMD";

  /**
   * @brief 接收数据块的大小常量
   * 
   * 定义了单次接收操作中数据块的大小，设置为256字节。
   * 用于控制串口或网络通信中每次读取的数据量，
   * 平衡内存使用和传输效率。
   * 
   * @note 该常量在编译期确定，无法在运行时修改
   */
  constexpr size_t kRxChunkSize = 256;
}

/**
 * @brief Cmd 类的默认构造函数
 * 
 * 初始化 Cmd 对象，使用编译器生成的默认构造函数。
 * 该构造函数不执行任何显式初始化操作。
 */
Cmd::Cmd() = default;

/**
 * @brief 初始化Cmd对象，设置其依赖的传输、编解码和路由器
 * 
 * @param transport 传输层对象的引用，用于处理数据传输
 * @param codec 编解码器对象的引用，用于数据序列化和反序列化
 * @param router 路由器对象的引用，用于处理消息路由
 * 
 * @note 该方法必须在使用Cmd对象的其他功能前调用
 * @warning 传入的transport、codec和router对象的生命周期必须长于Cmd对象
 */
void Cmd::init(Transport &transport, Codec &codec, Router &router)
{
  transport_ = &transport;
  codec_ = &codec;
  router_ = &router;
}

/**
 * @brief 注册命令处理器
 * 
 * 通过外部处理器模块为路由器注册各类命令处理函数。
 * 
 * @details
 * - 若路由器为空指针，则直接返回
 * - 具体命令处理逻辑位于独立的 handler 实现文件中
 * 
 * @return void
 * 
 * @note 该函数依赖于router_成员变量已被正确初始化
 */
void Cmd::registerHandlers()
{
  if (router_ == nullptr)
  {
    return;
  }

  CmdHandlers::registerDefaultHandlers(*router_, codec_);
}

/**
 * @brief 处理命令循环的主函数
 * 
 * 该函数执行以下步骤：
 * 1. 检查传输层、编解码器和路由器是否已初始化
 * 2. 调用传输层的loop()进行必要的维护处理
 * 3. 从传输层接收数据并放入缓冲区
 * 4. 将接收到的数据馈入编解码器进行解析
 * 5. 逐个获取解析后的数据包并将其分派给对应的处理器
 * 6. 监测编解码过程中的错误，并根据错误类型采取相应处理：
 *    - 缓冲区溢出或长度错误时重置编解码器
 *    - 其他错误记录警告日志
 * 
 * @note 该函数应定期调用以保持命令处理的实时性
 * @note 如果路由器找不到对应命令ID的处理器，会输出调试日志但不中断处理流程
 * 
 * @see transport_->loop()
 * @see transport_->receive()
 * @see codec_->feed()
 * @see codec_->next()
 * @see router_->dispatch()
 */
void Cmd::loop()
{
  if (transport_ == nullptr || codec_ == nullptr || router_ == nullptr)
  {
    return;
  }

  transport_->loop();

  uint8_t buffer[kRxChunkSize];
  while (transport_->available() > 0)
  {
    const size_t n = transport_->receive(buffer, sizeof(buffer));
    if (n == 0)
    {
      break;
    }

    codec_->feed(buffer, n);
    Packet packet;
    while (codec_->next(packet))
    {
      if (!router_->dispatch(packet, *this))
      {
        LOGD(TAG, "no handler for cmd=0x%04X", static_cast<unsigned>(packet.cmdId));
      }
    }

    const Codec::Error err = codec_->lastError();
    if (err != Codec::Error::None && err != Codec::Error::NeedMore)
    {
      LOGW(TAG, "codec error=%u(%s)", static_cast<unsigned>(err), codec_->lastErrorText());
      if (err == Codec::Error::BufferOverflow || err == Codec::Error::BadLength)
      {
        codec_->reset();
      }
    }
  }
}

/**
 * @brief 发送请求数据包
 * 
 * 构造一个请求型数据包并通过底层接口发送。
 * 
 * @param session 会话ID，用于标识请求的会话
 * @param cmdId 命令ID，用于标识请求的命令类型
 * @param payload 指向载荷数据的指针，若为nullptr则表示无载荷数据
 * @param len 载荷数据的长度（字节数），若为0则表示无载荷数据
 * 
 * @return bool 返回true表示数据包发送成功，返回false表示发送失败
 * 
 * @note 若payload为nullptr或len为0，则发送的数据包不包含载荷数据
 */
bool Cmd::sendRequest(uint16_t session, uint16_t cmdId, const uint8_t *payload, size_t len)
{
  Packet packet;
  packet.type = Protocol::PacketType::Request;
  packet.session = session;
  packet.cmdId = cmdId;
  packet.code = 0;
  if (payload != nullptr && len > 0)
  {
    packet.payload.assign(payload, payload + len);
  }
  return sendPacket_(packet);
}

/**
 * @brief 发送命令响应包
 * 
 * @param session 会话ID，用于匹配请求和响应
 * @param cmdId 命令ID
 * @param code 响应状态码
 * @param payload 响应数据负载指针，可为nullptr
 * @param len 响应数据长度，当payload为nullptr时应为0
 * 
 * @return true 表示数据包发送成功
 * @return false 表示数据包发送失败
 * 
 * @note 当payload不为nullptr且len大于0时，负载数据将被复制到数据包中
 */
bool Cmd::reply(uint16_t session, uint16_t cmdId, uint16_t code, const uint8_t *payload, size_t len)
{
  Packet packet;
  packet.type = Protocol::PacketType::Response;
  packet.session = session;
  packet.cmdId = cmdId;
  packet.code = code;
  if (payload != nullptr && len > 0)
  {
    packet.payload.assign(payload, payload + len);
  }
  return sendPacket_(packet);
}

/**
 * @brief 发布一个事件数据包
 * 
 * @param eventId 事件ID，作为数据包的命令ID
 * @param payload 指向事件负载数据的指针，可为nullptr
 * @param len 事件负载数据的长度（字节数），当payload为nullptr时应为0
 * 
 * @return true 数据包发送成功
 * @return false 数据包发送失败
 * 
 * @note 函数会将负载数据复制到数据包中，调用者需要保证payload指针的有效性
 * @note 若payload为nullptr或len为0，则发送的数据包不包含负载数据
 */
bool Cmd::publish(uint16_t eventId, const uint8_t *payload, size_t len)
{
  Packet packet;
  packet.type = Protocol::PacketType::Event;
  packet.session = 0;
  packet.cmdId = eventId;
  packet.code = 0;
  if (payload != nullptr && len > 0)
  {
    packet.payload.assign(payload, payload + len);
  }
  return sendPacket_(packet);
}

/**
 * @brief 发送数据包
 * @details 将数据包进行编码，然后通过传输层发送编码后的数据
 * @param packet 参考要发送的数据包，包含待编码的信息
 * @return true 数据包编码并发送成功
 * @return false 传输层或编码器为空指针、编码失败或编码结果为空
 * @note 编码失败时会输出警告日志，包含具体的错误信息
 */
bool Cmd::sendPacket_(const Packet &packet)
{
  if (transport_ == nullptr || codec_ == nullptr)
  {
    return false;
  }

  std::vector<uint8_t> out;
  if (!codec_->encode(packet, out))
  {
    LOGW(TAG, "encode failed: %s", codec_->lastErrorText());
    return false;
  }

  if (out.empty())
  {
    return false;
  }

  transport_->send(out.data(), out.size());
  return true;
}
