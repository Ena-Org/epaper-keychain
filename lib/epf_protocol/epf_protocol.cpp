#include "epf_protocol.hpp"
#include <algorithm>
#include <cstring>

/**
 * @brief 计算CRC32校验值
 *
 * 使用标准的CRC32算法（多项式0xEDB88320）对输入数据进行校验。
 * 该实现采用按位计算的方式，对每个字节进行8次迭代。
 *
 * @param data 指向要计算的数据缓冲区的指针
 * @param len 数据的长度（字节数）
 * @param seed 初始种子值，默认为0时初始化为0xFFFFFFFF
 *
 * @return uint32_t 计算得到的CRC32校验值
 *
 * @note
 * - 种子值会被取反后作为初始CRC值
 * - 最终结果也会被取反
 * - 这是标准的CRC32实现，兼容大多数系统的CRC32算法
 *
 * @example
 * uint8_t data[] = {0x01, 0x02, 0x03};
 * uint32_t crc = EpfProtocol::crc32(data, sizeof(data), 0);
 */
uint32_t EpfProtocol::crc32(const uint8_t *data, size_t len, uint32_t seed)
{
  uint32_t crc = ~seed;
  for (size_t i = 0; i < len; ++i)
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; ++j)
    {
      const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1u)));
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

/**
 * @brief 编码协议帧
 *
 * 将帧头、有效负载和CRC校验码组合成完整的协议帧。
 *
 * @param type 帧类型
 * @param session 会话ID
 * @param offset 数据偏移量
 * @param payload 指向有效负载数据的指针，可为nullptr（当len为0时）
 * @param len 有效负载长度（字节）
 * @param out 指向输出缓冲区的指针，用于存储编码后的帧数据
 * @param cap 输出缓冲区的容量（字节）
 * @param opt 编码选项，控制是否包含魔数等参数
 *
 * @return 编码后的帧数据大小（字节）。如果编码失败（如缓冲区不足、
 *         无效参数等），返回0
 *
 * @note 输出缓冲区必须足够容纳完整的帧数据，其中包括：
 *       - 帧头大小
 *       - 有效负载长度
 *       - CRC校验码（4字节）
 *
 * @pre out不能为nullptr；如果len > 0，payload也不能为nullptr
 */
size_t EpfProtocol::encodeFrame(
    FrameType type,
    uint16_t session,
    uint32_t offset,
    const uint8_t *payload,
    uint16_t len,
    uint8_t *out,
    size_t cap,
    EncodeOptions opt)
{
  if (out == nullptr)
  {
    return 0;
  }
  if (len > 0 && payload == nullptr)
  {
    return 0;
  }

  const size_t header_size = sizeof(FrameHeader);
  const size_t total_size = header_size + len + sizeof(uint32_t);
  if (cap < total_size)
  {
    return 0;
  }

  FrameHeader hdr{};
  hdr.magic = opt.include_magic ? MAGIC : 0u;
  hdr.type = static_cast<uint8_t>(type);
  hdr.session = session;
  hdr.offset = offset;
  hdr.len = len;

  std::memcpy(out, &hdr, header_size);
  if (len > 0)
  {
    std::memcpy(out + header_size, payload, len);
  }

  const uint32_t frame_crc = crc32(out, header_size + len);
  std::memcpy(out + header_size + len, &frame_crc, sizeof(frame_crc));
  return total_size;
}

/**
 * @brief 帧解析器的构造函数
 *
 * 初始化FrameParser对象,设置最大有效负载大小和待处理消费量。
 *
 * @param max_payload 帧负载的最大字节数,用于限制单个帧能处理的数据量
 *
 * @details
 * - 将max_payload_成员变量设置为指定的最大有效负载大小
 * - 将pending_consume_初始化为0,表示初始时没有待处理的字节需要消费
 *
 * @note 该构造函数不执行动态内存分配,初始化开销极小
 */
EpfProtocol::FrameParser::FrameParser(size_t max_payload)
    : max_payload_(max_payload), pending_consume_(0) {}

/// @brief 重置帧解析器的状态
///
/// 清空内部缓冲区并重置待处理的数据计数器，
/// 将解析器恢复到初始状态。
void EpfProtocol::FrameParser::reset()
{
  buffer_.clear();
  pending_consume_ = 0;
}

/**
 * @brief 向帧解析器中输入数据
 *
 * 此函数接收原始字节数据并将其添加到内部缓冲区中进行后续的帧解析。
 * 如果存在待处理的消费数据，函数会先清除缓冲区中对应的已处理部分。
 *
 * @param data 指向输入数据的指针。如果为 nullptr，则忽略此次调用。
 * @param len 输入数据的字节长度。如果为 0，则忽略此次调用。
 *
 * @note
 * - 如果 data 为 nullptr 或 len 为 0，函数将直接返回，不做任何处理
 * - 函数会自动管理内部缓冲区，将新数据追加到缓冲区末尾
 * - pending_consume_ 用于追踪需要从缓冲区前部删除的数据量
 *
 * @return void
 */
void EpfProtocol::FrameParser::feed(const uint8_t *data, size_t len)
{
  if (data == nullptr || len == 0)
  {
    return;
  }

  if (pending_consume_ > 0)
  {
    const size_t consume = std::min(pending_consume_, buffer_.size());
    buffer_.erase(buffer_.begin(), buffer_.begin() + consume);
    pending_consume_ = 0;
  }
  buffer_.insert(buffer_.end(), data, data + len);
}

/**
 * @brief 从缓冲区解析下一个完整帧
 * 
 * 该函数从内部缓冲区中提取并验证一个完整的协议帧。它会检查魔数、
 * 载荷长度和CRC校验和，并将成功解析的帧数据填充到输出参数中。
 * 
 * @param[out] out FrameView 结构体引用，用于返回解析成功的帧数据
 *                 包含帧头、载荷指针和CRC32校验和
 * 
 * @return ParseResult 解析结果状态码：
 *         - ParseResult::Ok        : 成功解析一个完整的帧，out 参数已填充
 *         - ParseResult::NeedMore  : 缓冲区数据不足，需要更多数据
 *         - ParseResult::BadMagic  : 帧头魔数不匹配，该字节已丢弃
 *         - ParseResult::TooLarge  : 帧载荷长度超过最大限制，该字节已丢弃
 *         - ParseResult::BadCrc    : CRC32校验失败，整个帧已丢弃
 * 
 * @note 函数会自动清理已处理的缓冲区数据
 * @note 当返回 BadMagic、TooLarge 或 BadCrc 时，相应的数据已从缓冲区删除
 * @note out 参数中的 payload 指针指向内部缓冲区，在调用下次 nextFrame 前需复制数据
 */
EpfProtocol::ParseResult EpfProtocol::FrameParser::nextFrame(FrameView &out)
{
  if (pending_consume_ > 0)
  {
    const size_t consume = std::min(pending_consume_, buffer_.size());
    buffer_.erase(buffer_.begin(), buffer_.begin() + consume);
    pending_consume_ = 0;
  }

  const size_t header_size = sizeof(FrameHeader);
  const size_t crc_size = sizeof(uint32_t);
  if (buffer_.size() < header_size + crc_size)
  {
    return ParseResult::NeedMore;
  }

  FrameHeader hdr{};
  std::memcpy(&hdr, buffer_.data(), header_size);

  if (hdr.magic != MAGIC)
  {
    buffer_.erase(buffer_.begin());
    return ParseResult::BadMagic;
  }

  if (hdr.len > max_payload_)
  {
    buffer_.erase(buffer_.begin());
    return ParseResult::TooLarge;
  }

  const size_t frame_size = header_size + static_cast<size_t>(hdr.len) + crc_size;
  if (buffer_.size() < frame_size)
  {
    return ParseResult::NeedMore;
  }

  uint32_t recv_crc = 0;
  std::memcpy(&recv_crc, buffer_.data() + header_size + hdr.len, crc_size);
  const uint32_t calc_crc = EpfProtocol::crc32(buffer_.data(), header_size + hdr.len);

  if (recv_crc != calc_crc)
  {
    buffer_.erase(buffer_.begin(), buffer_.begin() + frame_size);
    return ParseResult::BadCrc;
  }

  out.hdr = hdr;
  out.payload = buffer_.data() + header_size;
  out.crc32 = recv_crc;

  pending_consume_ = frame_size;
  return ParseResult::Ok;
}

/**
 * @brief 获取缓冲区中待解析的数据字节数
 *
 * 返回缓冲区中还未被消费的数据长度。当待处理消费量大于等于
 * 缓冲区大小时，表示所有数据都已被消费，返回0。
 *
 * @return size_t 缓冲区中待解析的字节数。如果所有数据都已被消费，
 *         返回0
 *
 * @note 该函数为常成员函数，不修改解析器的状态
 */
size_t EpfProtocol::FrameParser::bufferedBytes() const
{
  if (pending_consume_ >= buffer_.size())
  {
    return 0;
  }
  return buffer_.size() - pending_consume_;
}
