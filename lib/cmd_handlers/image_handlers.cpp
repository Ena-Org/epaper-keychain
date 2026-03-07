#include "cmd_handlers_registry.hpp"

#include "cmd/cmd.hpp"
#include "cmd_ids.hpp"

namespace CmdHandlers
{
  namespace
  {
    using Packet = Protocol::Packet;

    constexpr uint16_t kErrorInvalidContext = 1;
    constexpr uint16_t kErrorInvalidPayload = 2;

    Cmd *getCmdContext_(Router::Context &ctx)
    {
      return static_cast<Cmd *>(&ctx);
    }

    uint16_t read_u16_le_(const uint8_t *data)
    {
      return static_cast<uint16_t>(data[0]) |
             (static_cast<uint16_t>(data[1]) << 8u);
    }

    uint32_t read_u32_le_(const uint8_t *data)
    {
      return static_cast<uint32_t>(data[0]) |
             (static_cast<uint32_t>(data[1]) << 8u) |
             (static_cast<uint32_t>(data[2]) << 16u) |
             (static_cast<uint32_t>(data[3]) << 24u);
    }

    void write_u32_le_(uint32_t value, uint8_t out[4])
    {
      out[0] = static_cast<uint8_t>(value & 0xFFu);
      out[1] = static_cast<uint8_t>((value >> 8u) & 0xFFu);
      out[2] = static_cast<uint8_t>((value >> 16u) & 0xFFu);
      out[3] = static_cast<uint8_t>((value >> 24u) & 0xFFu);
    }
  }

  void registerSessionHandlers(Router &router)
  {
    router.registerHandler(CmdId::BrowserConnect, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Cmd *cmd = getCmdContext_(ctx);
                             if (cmd == nullptr)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidContext, nullptr, 0);
                               return;
                             }

                             cmd->setBrowserConnected(true);
                             const uint8_t enabled = 1;
                             ctx.reply(packet.session, packet.cmdId, 0, &enabled, 1); });

    router.registerHandler(CmdId::BrowserDisconnect, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Cmd *cmd = getCmdContext_(ctx);
                             if (cmd == nullptr)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidContext, nullptr, 0);
                               return;
                             }

                             cmd->setBrowserConnected(false);
                             const uint8_t enabled = 0;
                             ctx.reply(packet.session, packet.cmdId, 0, &enabled, 1); });
  }

  void registerImageHandlers(Router &router)
  {
    router.registerHandler(CmdId::ImageBegin, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Cmd *cmd = getCmdContext_(ctx);
                             if (cmd == nullptr)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidContext, nullptr, 0);
                               return;
                             }

                             // Payload layout (17 bytes, little-endian):
                             // transfer_id(4) width(2) height(2) format(1) total_bytes(4) expected_crc32(4)
                             if (packet.payload.size() != 17)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidPayload, nullptr, 0);
                               return;
                             }

                             Image::UploadMeta meta;
                             meta.transfer_id = read_u32_le_(packet.payload.data());
                             meta.width = read_u16_le_(packet.payload.data() + 4);
                             meta.height = read_u16_le_(packet.payload.data() + 6);
                             meta.total_bytes = read_u32_le_(packet.payload.data() + 9);
                             meta.expected_crc32 = read_u32_le_(packet.payload.data() + 13);

                             switch (packet.payload[8])
                             {
                             case 1:
                               meta.format = Image::PixelFormat::Mono1Bpp;
                               break;
                             case 2:
                               meta.format = Image::PixelFormat::Gray2Bpp;
                               break;
                             case 3:
                               meta.format = Image::PixelFormat::Gray4Bpp;
                               break;
                             default:
                               ctx.reply(packet.session, packet.cmdId,
                                         static_cast<uint16_t>(Image::ErrorCode::InvalidArg), nullptr, 0);
                               return;
                             }

                             const Image::Result result = cmd->imageBegin(meta);
                             uint8_t payload[4] = {0, 0, 0, 0};
                             write_u32_le_(result.accepted_bytes, payload);
                             ctx.reply(packet.session, packet.cmdId, static_cast<uint16_t>(result.code), payload, sizeof(payload)); });

    router.registerHandler(CmdId::ImageChunk, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Cmd *cmd = getCmdContext_(ctx);
                             if (cmd == nullptr)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidContext, nullptr, 0);
                               return;
                             }

                             if (packet.payload.size() < 5)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidPayload, nullptr, 0);
                               return;
                             }

                             const uint32_t offset = read_u32_le_(packet.payload.data());
                             const uint8_t *chunk_data = packet.payload.data() + 4;
                             const size_t chunk_len = packet.payload.size() - 4;

                             const Image::Result result = cmd->imageAppendChunk(offset, chunk_data, chunk_len);
                             uint8_t payload[4] = {0, 0, 0, 0};
                             write_u32_le_(result.accepted_bytes, payload);
                             ctx.reply(packet.session, packet.cmdId, static_cast<uint16_t>(result.code), payload, sizeof(payload)); });

    router.registerHandler(CmdId::ImageEnd, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Cmd *cmd = getCmdContext_(ctx);
                             if (cmd == nullptr)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidContext, nullptr, 0);
                               return;
                             }

                             const Image::Result result = cmd->imageEnd();
                             uint8_t payload[4] = {0, 0, 0, 0};
                             write_u32_le_(result.accepted_bytes, payload);
                             ctx.reply(packet.session, packet.cmdId, static_cast<uint16_t>(result.code), payload, sizeof(payload)); });

    router.registerHandler(CmdId::ImageApply, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Cmd *cmd = getCmdContext_(ctx);
                             if (cmd == nullptr)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidContext, nullptr, 0);
                               return;
                             }

                             const Image::Result result = cmd->imageApply();
                             ctx.reply(packet.session, packet.cmdId, static_cast<uint16_t>(result.code), nullptr, 0); });

    router.registerHandler(CmdId::ImageAbort, [](const Packet &packet, Router::Context &ctx)
                           {
                             if (packet.type != Protocol::PacketType::Request)
                             {
                               return;
                             }

                             Cmd *cmd = getCmdContext_(ctx);
                             if (cmd == nullptr)
                             {
                               ctx.reply(packet.session, packet.cmdId, kErrorInvalidContext, nullptr, 0);
                               return;
                             }

                             const Image::Result result = cmd->imageAbort();
                             ctx.reply(packet.session, packet.cmdId, static_cast<uint16_t>(result.code), nullptr, 0); });
  }
}
