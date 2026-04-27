#include "cmd_handlers_registry.hpp"
#include "cmd_ids.hpp"

namespace CmdHandlers
{
  /**
   * @brief 数据包类型别名
   * 
   * 将 Protocol::Packet 类型定义为本地类型别名 Packet，用于简化代码中的类型引用。
   * 通过该别名可以直接使用 Packet 而无需每次都写完整的 Protocol::Packet 限定名。
   * 
   * @using Packet Protocol 命名空间中定义的数据包结构体
   */
  using Packet = Protocol::Packet;

  /**
   * @brief 注册信息处理器到路由器
   * 
   * 该函数向路由器注册一个用于处理信息查询命令的处理器。
   * 当接收到类型为 CmdId::Info 的请求包时，处理器会验证消息类型，
   * 然后返回一个包含 "ok" 字符串的应答消息。
   * 
   * @param router 要注册处理器的路由器引用
   * 
   * @details
   * - 仅处理类型为 Request 的数据包
   * - 返回消息内容为 "ok"
   * - 使用会话ID和命令ID来关联请求和应答
   * 
   * @note 该处理器主要用于检查设备的基本状态或验证连接
   */
  void registerInfoHandlers(Router &router)
  {
    router.registerHandler(CmdId::Info, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             const char kInfo[] = "ok";
                             ctx.reply(packet.session, packet.cmdId, 0,
                                       reinterpret_cast<const uint8_t *>(kInfo), sizeof(kInfo) - 1); });
  }
}
