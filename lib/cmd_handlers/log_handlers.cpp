#include "cmd_handlers_registry.hpp"

#include <algorithm>
#include <vector>

#include "cmd_ids.hpp"
#include "logger.hpp"

namespace CmdHandlers
{
  using Packet = Protocol::Packet;

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
                             ctx.reply(packet.session, packet.cmdId, 0, payload.data(), copied);
                           });

    router.registerHandler(CmdId::LogClear, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Logger::clear_history();
                             ctx.reply(packet.session, packet.cmdId, 0, nullptr, 0);
                           });

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
                             ctx.reply(packet.session, packet.cmdId, 0, &raw, 1);
                           });
  }
}
