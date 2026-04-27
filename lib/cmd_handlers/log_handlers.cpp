#include "cmd_handlers_registry.hpp"

#include <algorithm>
#include <vector>

#include "cmd_ids.hpp"
#include "logger.hpp"

namespace CmdHandlers
{
  /**
   * @typedef Packet
   * @brief 协议数据包类型别名
   * 
   * 将 Protocol::Packet 类型简化为 Packet，用于方便在本文件中引用和使用
   * 协议通信中的数据包结构体。这个别名提高了代码的可读性，避免了重复
   * 书写完整的命名空间限定符。
   * 
   * @see Protocol::Packet
   */
  using Packet = Protocol::Packet;

  /**
   * @brief 注册日志相关的命令处理器
   * 
   * 该函数向路由器注册三个日志处理命令：
   * 
   * 1. LogHistory - 获取日志历史记录
   *    - 返回最近的日志历史数据
   *    - 返回大小受限于codec的最大负载字节数或默认1024字节
   *    - 若无日志数据则返回空响应
   * 
   * 2. LogClear - 清空日志历史记录
   *    - 清除所有保存的日志历史数据
   *    - 返回成功响应
   * 
   * 3. LogLevelSet - 设置日志级别
   *    - 从请求负载的第一个字节读取日志级别值
   *    - 验证日志级别的有效性（必须是定义的有效级别或Off模式）
   *    - 若级别无效则返回错误码1
   *    - 若设置成功则返回设置的日志级别值
   * 
   * @param router 命令路由器引用，用于注册处理器
   * @param codec 编解码器指针，用于获取最大负载字节数配置，可为nullptr
   * 
   * @note 所有处理器仅响应Request类型的数据包
   * @note 若codec为nullptr，则日志历史记录的大小限制默认为1024字节
   */
  void registerLogHandlers(Router &router, Codec *codec)
  {
    router.registerHandler(CmdId::LogHistory, [codec](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             const size_t maxPayload =
                                 (codec != nullptr) ? codec->config().maxPayloadBytes : 1024u;
                             const size_t historySize = Logger::history_size();
                             const size_t outSize = std::min(historySize, maxPayload);

                             if (outSize == 0)
                             {
                               ctx.reply(packet.session, packet.cmdId, 0, nullptr, 0);
                               return;
                             }

                             std::vector<uint8_t> payload(outSize);
                             const size_t copied = Logger::copy_history(payload.data(), payload.size());
                             ctx.reply(packet.session, packet.cmdId, 0, payload.data(), copied); });

    router.registerHandler(CmdId::LogClear, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Logger::clear_history();
                             ctx.reply(packet.session, packet.cmdId, 0, nullptr, 0); });

    router.registerHandler(CmdId::LogLevelSet, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             if (packet.payload.size() < 1)
                             {
                               ctx.reply(packet.session, packet.cmdId, 1, nullptr, 0);
                               return;
                             }

                             const uint8_t raw = packet.payload[0];
                             if (raw > static_cast<uint8_t>(Logger::LogLevel::Verbose) &&
                                 raw != static_cast<uint8_t>(Logger::LogLevel::Off))
                             {
                               ctx.reply(packet.session, packet.cmdId, 1, nullptr, 0);
                               return;
                             }

                             Logger::set_level(static_cast<Logger::LogLevel>(raw));
                             ctx.reply(packet.session, packet.cmdId, 0, &raw, 1); });
  }
}
