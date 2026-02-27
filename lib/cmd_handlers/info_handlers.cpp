#include "cmd_handlers_registry.hpp"

#include "cmd_ids.hpp"

namespace CmdHandlers
{
  using Packet = Protocol::Packet;

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
                                       reinterpret_cast<const uint8_t *>(kInfo), sizeof(kInfo) - 1);
                           });
  }
}
