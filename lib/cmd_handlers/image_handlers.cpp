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

    /**
     * @brief 从路由上下文中获取命令对象指针
     * 
     * 将给定的路由上下文引用强制转换为Cmd指针。
     * 该函数假设Router::Context是Cmd的派生类或兼容类型。
     * 
     * @param ctx 路由上下文的引用
     * @return Cmd* 指向命令对象的指针
     * 
     * @note 该函数使用static_cast进行类型转换，调用者需确保
     *       上下文对象确实可以安全地转换为Cmd指针
     */
    Cmd *getCmdContext_(Router::Context &ctx)
    {
      return static_cast<Cmd *>(&ctx);
    }

    /**
     * @brief 从字节数组中读取小端序的16位无符号整数
     * 
     * @param data 指向包含至少2个字节数据的指针
     * @return uint16_t 读取到的16位无符号整数值
     * 
     * @details
     * 该函数从给定的字节指针处读取两个连续的字节，
     * 并按小端序（Little Endian）格式组合成一个16位无符号整数。
     * 字节顺序：data[0]为低字节，data[1]为高字节。
     * 
     * @note 调用者需确保data指针指向至少2个字节的有效内存区域
     */
    uint16_t read_u16_le_(const uint8_t *data)
    {
      return static_cast<uint16_t>(data[0]) |
             (static_cast<uint16_t>(data[1]) << 8u);
    }

    /**
     * @brief 从字节数组中读取32位无符号整数（小端序）
     * 
     * 该函数从给定的字节数组中读取4个字节，并按小端序（Little Endian）
     * 的方式将其组合成一个32位无符号整数。低地址字节对应低有效位。
     * 
     * @param data 指向字节数组的指针，至少包含4个字节的有效数据
     * 
     * @return 按小端序组合后的32位无符号整数值
     * 
     * @note 调用者需确保data指针指向的内存至少包含4个字节
     */
    uint32_t read_u32_le_(const uint8_t *data)
    {
      return static_cast<uint32_t>(data[0]) |
             (static_cast<uint32_t>(data[1]) << 8u) |
             (static_cast<uint32_t>(data[2]) << 16u) |
             (static_cast<uint32_t>(data[3]) << 24u);
    }

    /**
     * @brief 将32位无符号整数转换为小端字节序并写入输出缓冲区
     * 
     * @param value 要转换的32位无符号整数值
     * @param out 输出缓冲区指针，必须至少能容纳4个字节
     * 
     * @details 该函数将一个32位的无符号整数按照小端字节序（LSB first）
     *          分解为4个8位的字节，并存储到指定的输出缓冲区中。
     *          字节顺序为：最低有效字节在前，最高有效字节在后。
     * 
     * @note 调用者必须确保out指针指向至少4字节大小的有效内存空间
     */
    void write_u32_le_(uint32_t value, uint8_t out[4])
    {
      out[0] = static_cast<uint8_t>(value & 0xFFu);
      out[1] = static_cast<uint8_t>((value >> 8u) & 0xFFu);
      out[2] = static_cast<uint8_t>((value >> 16u) & 0xFFu);
      out[3] = static_cast<uint8_t>((value >> 24u) & 0xFFu);
    }
  }

  /**
   * @brief 注册图像处理命令的处理器
   * 
   * 此函数向路由器注册五个图像处理相关的命令处理器，用于处理图像上传和应用的完整生命周期。
   * 
   * @param router 路由器引用，用于注册各个命令处理器
   * 
   * @details 注册的处理器包括：
   *   - ImageBegin: 初始化图像上传，解析图像元数据（宽度、高度、格式、总字节数、CRC32等）
   *   - ImageChunk: 接收并追加图像数据块，支持指定偏移量的数据写入
   *   - ImageEnd: 完成图像数据接收，进行数据验证和处理
   *   - ImageApply: 应用已上传的图像到设备
   *   - ImageAbort: 中止当前的图像上传操作
   * 
   * @note 所有处理器都要求收到Request类型的数据包，否则将忽略
   * @note 每个处理器都会验证命令上下文的有效性，若无效则返回错误响应
   * 
   * @see Cmd::imageBegin()
   * @see Cmd::imageAppendChunk()
   * @see Cmd::imageEnd()
   * @see Cmd::imageApply()
   * @see Cmd::imageAbort()
   */
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
