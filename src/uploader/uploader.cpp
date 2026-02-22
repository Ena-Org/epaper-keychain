#include "uploader.hpp"

#include <algorithm>
#include <vector>

namespace
{
  /**
   * @brief 上传会话的内部状态结构体
   * 
   * 用于管理单个文件上传过程中的所有相关状态信息,包括会话
   * 身份识别、传输模式、错误处理和数据验证等。
   * 
   * @member owner 指向所属上传会话的指针
   * @member active 标记当前上传会话是否活跃
   * @member binaryMode 标记是否使用二进制传输模式
   * @member sessionId 上传会话的唯一标识符
   * @member state 当前上传会话的状态(空闲、传输中等)
   * @member lastError 上一次发生的错误代码
   * @member stats 上传统计信息,包括传输字节数、速度等
   * @member request 当前待处理的上传请求
   * @member rollingCrc 滚动计算的循环冗余校验码,用于数据完整性验证
   * @member parser 帧解析器,缓冲区大小为4096字节,用于解析接收到的数据帧
   * @member writer 文件写入器,负责将接收到的数据写入存储设备
   */
  struct SessionState
  {
    SessionState() = default;
    SessionState(const SessionState &) = delete;
    SessionState &operator=(const SessionState &) = delete;
    SessionState(SessionState &&) = delete;
    SessionState &operator=(SessionState &&) = delete;

    const Uploader::UploadSession *owner = nullptr;
    bool active = false;
    bool binaryMode = false;
    uint16_t sessionId = 0;
    Uploader::State state = Uploader::State::Idle;
    Uploader::ErrorCode lastError = Uploader::ErrorCode::None;
    Uploader::Stats stats{};
    Uploader::PutRequest request{};
    uint32_t rollingCrc = 0;
    EpfProtocol::FrameParser parser{4096};
    Storage::FileWriter writer{};
  };

  /**
   * @brief 全局会话状态向量容器
   * 
   * 存储所有活跃的会话状态对象。该向量维护了整个应用程序中
   * 并发会话的生命周期和状态信息。
   * 
   * @details
   * - 用于管理多个客户端会话
   * - 线程安全访问需要外部同步机制
   * - 会话的添加和移除应通过相应的接口函数进行
   * 
   * @note 
   * 此全局变量在多线程环境下使用时需谨慎，
   * 建议使用互斥锁或其他同步原语保护并发访问。
   */
  std::vector<SessionState *> g_sessions;

  /**
   * @brief 获取或创建指定上传会话的状态对象
   * 
   * 根据给定的上传会话所有者指针，在全局会话列表中查找对应的会话状态。
   * 如果找到，则返回该会话状态的引用；如果未找到，则创建一个新的会话状态
   * 对象，设置其所有者，并返回其引用。
   * 
   * @param owner 指向 Uploader::UploadSession 的指针，用作会话的唯一标识符
   * @return SessionState& 返回匹配或新创建的会话状态对象的引用
   * 
   * @note 返回的引用在会话对象被从全局列表中移除之前一直有效
   */
  SessionState &getOrCreate(const Uploader::UploadSession *owner)
  {
    for (auto &s : g_sessions)
    {
      if (s->owner == owner)
      {
        return *s;
      }
    }

    g_sessions.push_back(new SessionState());
    SessionState &created = *g_sessions.back();
    created.owner = owner;
    return created;
  }

  /**
   * @brief 根据上传会话所有者查找对应的会话状态
   * 
   * @param owner 指向 Uploader::UploadSession 的指针，用作查找的所有者标识
   * 
   * @return 如果找到匹配的会话，返回指向 SessionState 的常指针；
   *         如果未找到任何匹配的会话，返回 nullptr
   * 
   * @note 返回的指针指向全局会话容器 g_sessions 中的元素，
   *       调用者需确保在使用返回的指针期间，对应的会话对象仍然存在
   */
  const SessionState *findSession(const Uploader::UploadSession *owner)
  {
    for (const auto &s : g_sessions)
    {
      if (s->owner == owner)
      {
        return s;
      }
    }
    return nullptr;
  }

  /**
   * @brief 通过USB CDC传输发送协议帧
   * 
   * 该函数构建一个EPF协议帧并通过USB CDC接口发送。首先对帧进行编码，
   * 然后将编码后的数据写入传输接口并刷新缓冲区。
   * 
   * @param io USB CDC传输接口引用，用于发送数据
   * @param type 帧类型，定义帧的用途和处理方式
   * @param session 会话ID，用于标识和关联相关帧
   * @param offset 数据偏移量，表示在整个数据流中的位置
   * @param payload 指向负载数据的指针，默认为nullptr。若为nullptr表示发送无负载的帧
   * @param len 负载数据的长度，默认为0。只在payload非空时有效
   * 
   * @note 输出缓冲区大小为1088字节（帧头+1024字节负载+32位）。
   *       如果编码失败（返回值n==0），则不执行写入操作。
   * 
   * @return void
   */
  void sendFrame(UsbCdcTransport &io,
                 EpfProtocol::FrameType type,
                 uint16_t session,
                 uint32_t offset,
                 const uint8_t *payload = nullptr,
                 uint16_t len = 0)
  {
    uint8_t out[sizeof(EpfProtocol::FrameHeader) + 1024 + sizeof(uint32_t)];
    const size_t n = EpfProtocol::encodeFrame(type, session, offset, payload, len, out, sizeof(out), EpfProtocol::EncodeOptions{});
    if (n > 0)
    {
      io.write(out, n);
      io.flush();
    }
  }

  /**
   * @brief 设置上传器错误状态
   * 
   * 当上传过程中发生错误时调用此函数，用于更新会话状态为错误状态，
   * 并清理相关资源。
   * 
   * @param s 待更新的会话状态对象引用
   * @param code 错误代码，指示发生的具体错误类型
   * 
   * @details
   * 此函数会执行以下操作：
   * - 记录最后发生的错误代码
   * - 将会话状态设置为错误状态
   * - 停用当前会话
   * - 禁用二进制模式
   * - 关闭数据写入器
   */
  void setError(SessionState &s, Uploader::ErrorCode code)
  {
    s.lastError = code;
    s.state = Uploader::State::Error;
    s.active = false;
    s.binaryMode = false;
    s.writer.close(false);
  }
}

/**
 * @brief 初始化上传会话对象
 * 
 * 构造函数，用于创建一个新的上传会话。初始化会话的所有状态变量和
 * 相关数据结构，包括：
 * - 设置会话为非活跃状态
 * - 禁用二进制模式
 * - 初始化状态机为空闲状态
 * - 清空错误代码和统计信息
 * - 重置上传请求和校验和
 * - 重置数据解析器
 */
Uploader::UploadSession::UploadSession()
{
  SessionState &s = getOrCreate(this);
  s.active = false;
  s.binaryMode = false;
  s.state = State::Idle;
  s.lastError = ErrorCode::None;
  s.stats = Stats{};
  s.request = PutRequest{};
  s.rollingCrc = 0;
  s.parser.reset();
}

/**
 * @brief 检查当前上传会话是否处于活跃状态
 * 
 * @return bool 如果会话处于活跃状态返回 true，否则返回 false
 *             当找不到对应的会话时返回 false
 * 
 * @note 此函数通过查找与当前对象关联的会话状态来判断活跃性
 */
bool Uploader::UploadSession::isActive() const
{
  const SessionState *s = findSession(this);
  return (s != nullptr) ? s->active : false;
}

/**
 * @brief 获取当前上传会话的状态
 * 
 * @return Uploader::State 返回当前会话的状态。如果会话存在，返回实际的会话状态；
 *                         如果会话不存在，返回 State::Idle（空闲状态）
 * 
 * @note 该函数通过 findSession 查找会话，时间复杂度取决于会话查找的实现
 */
Uploader::State Uploader::UploadSession::state() const
{
  const SessionState *s = findSession(this);
  return (s != nullptr) ? s->state : State::Idle;
}

/**
 * @brief 获取上传会话的最后一个错误代码
 *
 * @return ErrorCode 返回上次操作产生的错误代码。如果会话不存在或没有错误发生,
 *                   则返回 ErrorCode::None
 *
 * @note 此方法为 const 成员函数,不会修改对象状态
 *
 * @see ErrorCode
 */
Uploader::ErrorCode Uploader::UploadSession::lastError() const
{
  const SessionState *s = findSession(this);
  return (s != nullptr) ? s->lastError : ErrorCode::None;
}

/**
 * @brief 获取当前上传会话的统计信息
 * 
 * @details 通过查找与当前会话对应的会话状态，返回其统计数据。
 *          如果找不到对应的会话状态，则返回一个空的统计信息对象。
 * 
 * @return Uploader::Stats 包含上传会话的统计信息，如已上传字节数、总字节数等。
 *                         若会话不存在，返回默认初始化的Stats对象。
 * 
 * @note 此函数为常量成员函数，不会修改对象状态。
 */
Uploader::Stats Uploader::UploadSession::stats() const
{
  const SessionState *s = findSession(this);
  return (s != nullptr) ? s->stats : Stats{};
}

/**
 * @brief 启动一个新的上传会话
 * 
 * 初始化上传会话的各项参数，包括验证请求的有效性、检查存储空间、
 * 打开文件写入器，并准备接收数据。
 * 
 * @param req 上传请求参数，包含文件路径和文件大小
 * @param out 上传响应参数，返回分配的会话ID和建议的数据块大小
 * 
 * @return true 如果会话成功启动
 * @return false 如果会话启动失败，具体错误信息存储在 s.lastError 中
 * 
 * @note 可能的错误情况：
 *       - ErrorCode::Busy - 已有活跃的上传会话
 *       - ErrorCode::BadArgs - 请求路径为空
 *       - ErrorCode::OpenFail - 存储系统初始化或文件打开失败
 *       - ErrorCode::NoSpace - 可用存储空间不足
 * 
 * @details 该函数执行以下操作：
 *          1. 检查是否已有活跃会话
 *          2. 验证请求路径不为空
 *          3. 初始化存储系统
 *          4. 检查可用空间是否足以容纳上传文件
 *          5. 生成新的会话ID（从1开始递增）
 *          6. 打开文件写入器
 *          7. 重置会话状态和统计数据
 *          8. 设置为接收数据状态
 */
bool Uploader::UploadSession::start(const PutRequest &req, PutResponse &out)
{
  SessionState &s = getOrCreate(this);
  if (s.active)
  {
    s.lastError = ErrorCode::Busy;
    return false;
  }

  if (req.path.isEmpty())
  {
    s.lastError = ErrorCode::BadArgs;
    s.state = State::Error;
    return false;
  }

  if (!Storage::begin())
  {
    s.lastError = ErrorCode::OpenFail;
    s.state = State::Error;
    return false;
  }

  const Storage::FsInfo fs = Storage::info();
  if (fs.totalBytes > 0)
  {
    const size_t freeBytes = fs.totalBytes - std::min(fs.totalBytes, fs.usedBytes);
    if (req.size > freeBytes)
    {
      s.lastError = ErrorCode::NoSpace;
      s.state = State::Error;
      return false;
    }
  }

  uint16_t next = static_cast<uint16_t>(s.sessionId + 1);
  if (next == 0)
  {
    next = 1;
  }
  s.sessionId = next;

  if (!s.writer.open(req.path, req.size))
  {
    s.lastError = ErrorCode::OpenFail;
    s.state = State::Error;
    return false;
  }

  s.request = req;
  s.stats = Stats{};
  s.rollingCrc = 0;
  s.parser.reset();
  s.state = State::Receiving;
  s.lastError = ErrorCode::None;
  s.active = true;
  s.binaryMode = true;

  out.session = s.sessionId;
  out.chunk = 1024;
  return true;
}

/**
 * @brief 中止当前上传会话
 * 
 * 重置会话的所有状态，包括关闭写入器、清空解析器、禁用
 * 二进制模式，并将会话恢复到空闲状态。同时清除所有错误
 * 信息、统计数据、待处理请求和CRC校验值。
 * 
 * @note 此操作是不可逆的，会丢失当前上传的所有进度信息
 */
void Uploader::UploadSession::abort()
{
  SessionState &s = getOrCreate(this);
  s.writer.close(false);
  s.parser.reset();
  s.active = false;
  s.binaryMode = false;
  s.state = State::Idle;
  s.lastError = ErrorCode::None;
  s.stats = Stats{};
  s.request = PutRequest{};
  s.rollingCrc = 0;
}

/**
 * @brief 处理上传会话中接收到的协议帧
 * 
 * @details 该函数处理EPF协议中的不同类型帧，包括数据帧(DATA)、完成帧(DONE)和其他帧类型。
 *          函数会验证会话状态、帧序列、数据完整性和校验和。
 * 
 * @param frame 接收到的协议帧视图，包含帧头和负载数据
 * @param io USB CDC传输接口，用于发送响应帧
 * 
 * @return void
 * 
 * @note 处理流程：
 *       1. 检查会话是否激活，若未激活则发送NAK
 *       2. 验证帧的会话ID是否匹配
 *       3. 根据帧类型分别处理：
 *          - DATA帧：验证偏移量、写入数据、更新CRC、发送ACK
 *          - DONE帧：验证数据大小和CRC、关闭写入器、发送DONE
 *          - 其他帧：发送NAK
 * 
 * @see SessionState, EpfProtocol::FrameView, UsbCdcTransport
 */
void Uploader::UploadSession::onFrame(const EpfProtocol::FrameView &frame, UsbCdcTransport &io)
{
  SessionState &s = getOrCreate(this);
  if (!s.active)
  {
    sendFrame(io, EpfProtocol::FrameType::NAK, 0, 0);
    return;
  }

  if (frame.hdr.session != s.sessionId)
  {
    s.lastError = ErrorCode::BadSession;
    sendFrame(io, EpfProtocol::FrameType::NAK, s.sessionId, s.stats.receivedBytes);
    return;
  }

  const EpfProtocol::FrameType frameType = static_cast<EpfProtocol::FrameType>(frame.hdr.type);

  if (frameType == EpfProtocol::FrameType::DATA)
  {
    if (frame.hdr.offset != s.stats.receivedBytes)
    {
      setError(s, ErrorCode::WriteFail);
      sendFrame(io, EpfProtocol::FrameType::NAK, s.sessionId, s.stats.receivedBytes);
      return;
    }

    const uint32_t newHighWater = frame.hdr.offset + frame.hdr.len;
    if (newHighWater > s.request.size)
    {
      setError(s, ErrorCode::SizeMismatch);
      sendFrame(io, EpfProtocol::FrameType::NAK, s.sessionId, s.stats.receivedBytes);
      return;
    }

    if (!s.writer.writeAt(frame.hdr.offset, frame.payload, frame.hdr.len))
    {
      setError(s, ErrorCode::WriteFail);
      sendFrame(io, EpfProtocol::FrameType::NAK, s.sessionId, s.stats.receivedBytes);
      return;
    }

    s.rollingCrc = EpfProtocol::crc32(frame.payload, frame.hdr.len, s.rollingCrc);
    s.stats.receivedBytes = newHighWater;
    s.stats.ackedChunks += 1;

    sendFrame(io, EpfProtocol::FrameType::ACK, s.sessionId, s.stats.receivedBytes);
    return;
  }

  if (frameType == EpfProtocol::FrameType::DONE)
  {
    s.state = State::Verifying;

    if (s.stats.receivedBytes != s.request.size)
    {
      setError(s, ErrorCode::SizeMismatch);
      sendFrame(io, EpfProtocol::FrameType::NAK, s.sessionId, s.stats.receivedBytes);
      return;
    }

    if (s.request.crc32 != 0 && s.rollingCrc != s.request.crc32)
    {
      setError(s, ErrorCode::CrcMismatch);
      sendFrame(io, EpfProtocol::FrameType::NAK, s.sessionId, s.stats.receivedBytes);
      return;
    }

    s.writer.close(true);
    s.active = false;
    s.binaryMode = false;
    s.lastError = ErrorCode::None;
    s.state = State::Done;

    sendFrame(io, EpfProtocol::FrameType::DONE, s.sessionId, s.stats.receivedBytes);
    return;
  }

  sendFrame(io, EpfProtocol::FrameType::NAK, s.sessionId, s.stats.receivedBytes);
}

/**
 * @brief 轮询处理USB CDC传输中的数据帧
 * 
 * 该函数在二进制模式下，从USB CDC传输中读取数据并解析协议帧。
 * 对每个成功解析的帧调用处理函数，如果解析失败且会话处于活跃状态，
 * 则发送NAK(负确认)响应。
 * 
 * @param io USB CDC传输接口，用于读取接收数据和发送响应
 * 
 * @details
 * - 首先检查会话是否处于二进制模式，若不是则直接返回
 * - 从传输接口读取所有可用数据并送入解析器
 * - 循环尝试从解析器获取完整的协议帧
 * - 对于成功解析的帧，调用onFrame进行处理
 * - 对于解析失败且会话活跃的情况，发送NAK帧表示接收失败
 * 
 * @note 该函数应定期调用以保持数据处理的实时性
 */
void Uploader::UploadSession::poll(UsbCdcTransport &io)
{
  SessionState &s = getOrCreate(this);
  if (!s.binaryMode)
  {
    return;
  }

  uint8_t in[256];
  while (io.available() > 0)
  {
    const size_t n = io.read(in, sizeof(in));
    if (n == 0)
    {
      break;
    }
    s.parser.feed(in, n);
  }

  for (;;)
  {
    EpfProtocol::FrameView frame{};
    const EpfProtocol::ParseResult result = s.parser.nextFrame(frame);
    if (result == EpfProtocol::ParseResult::NeedMore)
    {
      break;
    }

    if (result == EpfProtocol::ParseResult::Ok)
    {
      onFrame(frame, io);
      continue;
    }

    if (s.active)
    {
      sendFrame(io, EpfProtocol::FrameType::NAK, s.sessionId, s.stats.receivedBytes);
    }
  }
}

/**
 * @brief 检查当前上传会话是否处于二进制模式
 * 
 * 通过查找与当前会话相关联的会话状态，判断该会话是否启用了二进制模式。
 * 如果找到对应的会话状态，返回其二进制模式标志；否则返回false。
 * 
 * @return bool 如果会话处于二进制模式返回true，否则返回false
 */
bool Uploader::UploadSession::inBinaryMode() const
{
  const SessionState *s = findSession(this);
  return (s != nullptr) ? s->binaryMode : false;
}
