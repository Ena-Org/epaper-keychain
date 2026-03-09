#include "cmd.hpp"
#include <vector>
#include "browser_connection_state.hpp"
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

  constexpr uint32_t kImageStorageBytes = 64u * 1024u;
}

/**
 * @brief Cmd 类的默认构造函数
 *
 * 初始化 Cmd 对象，使用编译器生成的默认构造函数。
 * 该构造函数不执行任何显式初始化操作。
 */
Cmd::Cmd()
    : image_storage_(kImageStorageBytes),
      image_store_port_(&image_storage_)
{
}

/**
 * @brief 构造函数 - 初始化图像存储端口适配器
 *
 * @param storage 指向 IImageStorage 接口实现的指针，用于处理图像存储操作
 *
 * @note 该构造函数将传入的 storage 指针保存到成员变量 storage_ 中
 */
Cmd::ImageStorePortAdapter::ImageStorePortAdapter(Storage::IImageStorage *storage)
    : storage_(storage)
{
}

/**
 * @brief 将存储错误代码转换为图像错误代码
 *
 * 该函数将底层存储模块返回的错误代码映射到图像处理模块使用的错误代码。
 * 实现了两个错误枚举类型之间的转换。
 *
 * @param code 来自存储模块的错误代码
 *
 * @return Image::ErrorCode 对应的图像错误代码
 *         - Ok: 操作成功
 *         - Busy: 存储模块忙碌
 *         - InvalidArg: 参数无效
 *         - BadState: 存储模块未就绪
 *         - OutOfRange: 存储空间不足
 *         - StorageError: 存储I/O错误或未找到对应映射的错误
 *         - NotFound: 资源未找到
 *
 * @note 未知的存储错误代码将被映射为 StorageError
 */
Image::ErrorCode Cmd::ImageStorePortAdapter::toImageCode_(Storage::ErrorCode code)
{
  switch (code)
  {
  case Storage::ErrorCode::Ok:
    return Image::ErrorCode::Ok;
  case Storage::ErrorCode::Busy:
    return Image::ErrorCode::Busy;
  case Storage::ErrorCode::InvalidArg:
    return Image::ErrorCode::InvalidArg;
  case Storage::ErrorCode::NotReady:
    return Image::ErrorCode::BadState;
  case Storage::ErrorCode::NoSpace:
    return Image::ErrorCode::OutOfRange;
  case Storage::ErrorCode::IoError:
    return Image::ErrorCode::StorageError;
  case Storage::ErrorCode::NotFound:
    return Image::ErrorCode::NotFound;
  default:
    return Image::ErrorCode::StorageError;
  }
}

/**
 * @brief 开始写入图像数据到存储设备
 * 
 * @param meta 图像元数据，包含传输ID、尺寸、总字节数、像素格式和CRC32校验值
 * 
 * @return Image::Result 操作结果，包含错误码和接受的字节数
 * 
 * @details
 * 该函数将Image命名空间中的图像元数据转换为Storage命名空间格式，
 * 并调用底层存储适配器开始写入操作。
 * 
 * @retval ErrorCode::Success 写入初始化成功
 * @retval ErrorCode::StorageError 存储设备为空或不可用
 * @retval ErrorCode::InvalidArg 不支持的像素格式
 * 
 * @note
 * - 支持的像素格式：Mono1Bpp、Gray2Bpp、Gray4Bpp
 * - 若格式不被支持，函数将返回InvalidArg错误
 * - result.accepted_bytes 表示存储设备准备接受的字节数
 */
Image::Result Cmd::ImageStorePortAdapter::begin_write(const Image::UploadMeta &meta)
{
  Image::Result result;
  if (storage_ == nullptr)
  {
    result.code = Image::ErrorCode::StorageError;
    return result;
  }

  Storage::ImageMeta image_meta;
  image_meta.transfer_id = meta.transfer_id;
  image_meta.width = meta.width;
  image_meta.height = meta.height;
  image_meta.total_bytes = meta.total_bytes;
  image_meta.crc32 = meta.expected_crc32;

  switch (meta.format)
  {
  case Image::PixelFormat::Mono1Bpp:
    image_meta.format = Storage::PixelFormat::Mono1Bpp;
    break;
  case Image::PixelFormat::Gray2Bpp:
    image_meta.format = Storage::PixelFormat::Gray2Bpp;
    break;
  case Image::PixelFormat::Gray4Bpp:
    image_meta.format = Storage::PixelFormat::Gray4Bpp;
    break;
  default:
    result.code = Image::ErrorCode::InvalidArg;
    return result;
  }

  const Storage::Result store_result = storage_->begin_write(image_meta);
  result.code = toImageCode_(store_result.code);
  result.accepted_bytes = store_result.bytes;
  return result;
}

/**
 * @brief 将数据块写入存储设备
 *
 * 这个函数将指定长度的数据写入到存储设备的指定偏移位置。
 * 如果存储设备未初始化，将返回存储错误。
 *
 * @param offset 数据写入的起始偏移位置（字节为单位）
 * @param data 指向要写入数据的指针
 * @param len 要写入数据的长度（字节为单位）
 *
 * @return Image::Result 包含操作结果码和实际接受的字节数
 *         - result.code: 操作结果状态码（成功或错误类型）
 *         - result.accepted_bytes: 实际写入或接受的字节数
 *
 * @note 如果storage_为nullptr，将返回StorageError错误码
 */
Image::Result Cmd::ImageStorePortAdapter::write_chunk(uint32_t offset, const uint8_t *data, size_t len)
{
  Image::Result result;
  if (storage_ == nullptr)
  {
    result.code = Image::ErrorCode::StorageError;
    return result;
  }

  const Storage::Result store_result = storage_->write_chunk(offset, data, len);
  result.code = toImageCode_(store_result.code);
  result.accepted_bytes = store_result.bytes;
  return result;
}

/**
 * @brief 提交图像存储操作到底层存储设备
 *
 * 将待存储的图像数据正式提交到存储介质中。在提交前会检查存储接口的有效性。
 *
 * @return Image::Result 包含提交操作的结果信息
 *         - result.code: 操作状态码，若存储接口为空则返回StorageError，
 *                        否则返回转换后的底层存储操作状态码
 *         - result.accepted_bytes: 成功提交的字节数
 *
 * @note 如果存储接口(storage_)未初始化或为空指针，则返回StorageError错误码
 */
Image::Result Cmd::ImageStorePortAdapter::commit()
{
  Image::Result result;
  if (storage_ == nullptr)
  {
    result.code = Image::ErrorCode::StorageError;
    return result;
  }

  const Storage::Result store_result = storage_->commit();
  result.code = toImageCode_(store_result.code);
  result.accepted_bytes = store_result.bytes;
  return result;
}

/**
 * @brief 中止当前的写入操作
 *
 * 该函数用于中止存储适配器中正在进行的图像写入操作。
 * 如果存储对象不可用，则返回存储错误。
 *
 * @return Image::Result 操作结果，包含：
 *         - code: 操作状态码，若存储对象为空则为 StorageError，
 *                 否则为转换后的存储结果状态码
 *         - accepted_bytes: 在中止前已接受的字节数
 *
 * @note 如果 storage_ 为 nullptr，函数会返回 StorageError 错误码
 */
Image::Result Cmd::ImageStorePortAdapter::abort_write()
{
  Image::Result result;
  if (storage_ == nullptr)
  {
    result.code = Image::ErrorCode::StorageError;
    return result;
  }

  const Storage::Result store_result = storage_->abort_write();
  result.code = toImageCode_(store_result.code);
  result.accepted_bytes = store_result.bytes;
  return result;
}

/**
 * @brief 初始化命令处理器
 *
 * 该函数完成命令处理器的初始化工作，包括设置传输层、编解码器和路由器的引用，
 * 初始化浏览器连接状态，配置并启动传输层，初始化图像服务，以及注册命令处理器。
 *
 * @param transport 传输层实例的引用，用于处理通信
 * @param codec 编解码器实例的引用，用于编解码操作
 * @param router 路由器实例的引用，用于消息路由
 *
 * @note 调用此函数后，传输层将被初始化并自动连接
 * @note 浏览器连接状态将被重置为未连接
 * @note 如果图像服务初始化失败，将记录错误日志但不会影响整体初始化流程
 *
 * @see registerHandlers()
 * @see Transport::init()
 * @see ImageService::init()
 */
void Cmd::init(Transport &transport, Codec &codec, Router &router)
{
  transport_ = &transport;
  codec_ = &codec;
  router_ = &router;
  browser_connected_ = false;
  BrowserConnectionState::set_connected(false);

  Transport::Config transportCfg;
  transportCfg.heartbeatEnabled = false;
  transport_->init(transportCfg);
  transport_->connect();

  if (!image_service_.init(&image_store_port_))
    LOGE(TAG, "image service init failed");

  registerHandlers();
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
 * @brief 设置浏览器连接状态
 *
 * 该函数用于更新浏览器的连接状态。当连接状态改变时，会同步更新
 * 内部状态标志和浏览器连接状态管理器，并根据连接状态启用或禁用
 * 传输层的心跳检测功能。
 *
 * @param connected 布尔值，表示浏览器是否已连接
 *                  true: 浏览器已连接
 *                  false: 浏览器已断开连接
 *
 * @note 如果传输层对象为空，函数会提前返回，不会设置心跳状态
 *
 * @see BrowserConnectionState::set_connected()
 * @see transport_->setHeartbeatEnabled()
 */
void Cmd::setBrowserConnected(bool connected)
{
  browser_connected_ = connected;
  BrowserConnectionState::set_connected(connected);

  if (transport_ == nullptr)
  {
    return;
  }

  transport_->setHeartbeatEnabled(connected);
}

/**
 * @brief 检查浏览器是否已连接
 * @return bool 如果浏览器已连接返回 true，否则返回 false
 */
bool Cmd::isBrowserConnected() const
{
  return browser_connected_;
}

/**
 * @brief 开始图像上传过程
 * @param meta 图像上传的元数据信息，包含图像的相关配置参数
 * @return Image::Result 返回图像操作的结果状态
 * @details 该函数将初始化图像服务的上传流程，为后续的图像数据传输做准备
 */
Image::Result Cmd::imageBegin(const Image::UploadMeta &meta)
{
  return image_service_.begin(meta);
}

/**
 * @brief 向图像服务追加数据块
 *
 * 该函数将指定的数据块追加到图像服务中的指定偏移位置。
 *
 * @param chunk_offset 数据块在图像中的偏移量（字节）
 * @param data 指向要追加的数据的指针
 * @param len 要追加的数据长度（字节）
 *
 * @return Image::Result 操作结果，表示追加是否成功
 *
 * @note 调用者需确保 data 指针有效且 len 不超过实际数据长度
 */
Image::Result Cmd::imageAppendChunk(uint32_t chunk_offset, const uint8_t *data, size_t len)
{
  return image_service_.append_chunk(chunk_offset, data, len);
}

/**
 * @brief 结束图像处理操作
 *
 * 调用图像服务的 end() 方法来完成当前图像的处理。
 *
 * @return Image::Result 图像处理的结果状态
 */
Image::Result Cmd::imageEnd()
{
  return image_service_.end();
}

/**
 * @brief 应用图像处理操作
 *
 * 将待处理的图像应用到图像服务中，执行实际的图像处理逻辑。
 *
 * @return Image::Result 图像处理操作的结果，包含处理状态和相关信息
 *
 * @details
 * 此方法作为命令类与图像服务之间的桥梁，负责触发图像服务的应用操作。
 * 具体的图像处理逻辑由 image_service_ 对象实现。
 */
Image::Result Cmd::imageApply()
{
  return image_service_.apply();
}

/**
 * @brief 中止图像操作
 *
 * 向图像服务发送中止请求，停止当前正在进行的图像处理操作。
 *
 * @return Image::Result 图像服务返回的结果，表示中止操作是否成功
 * @retval Image::Result::SUCCESS 中止操作成功
 * @retval 其他值 中止操作失败的相应错误码
 */
Image::Result Cmd::imageAbort()
{
  return image_service_.abort();
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
