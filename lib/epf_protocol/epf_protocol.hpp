#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <vector>

class EpfProtocol
{
public:
  /**
   * @brief EPF协议的魔数常量
   *
   * 用于标识和验证EPF（E-Paper Format）协议数据包的有效性。
   * 该魔数值为 0x31465045，对应ASCII字符 "1FPE"。
   *
   * @details
   * 在EPF协议通信中，接收端可以通过检查数据包开头的魔数来确认
   * 数据包是否为有效的EPF格式，防止数据损坏或误识别。
   */
  static constexpr uint32_t MAGIC = 0x31465045u;

  /**
   * @enum FrameType
   * @brief 定义协议中使用的帧类型枚举
   *
   * 这个枚举类用于表示EPF协议中不同的帧类型，每种类型对应特定的通信功能。
   *
   * @var FrameType::DATA
   * 数据帧，用于传输实际的数据内容
   *
   * @var FrameType::ACK
   * 确认帧，用于表示数据接收成功
   *
   * @var FrameType::NAK
   * 否定确认帧，用于表示数据接收失败或错误
   *
   * @var FrameType::DONE
   * 完成帧，用于表示传输过程完成
   *
   * @var FrameType::LOG
   * 日志帧，用于传输日志信息
   */
  enum class FrameType : uint8_t
  {
    DATA = 1,
    ACK = 2,
    NAK = 3,
    DONE = 4,
    LOG = 5,
  };

#pragma pack(push, 1)
  /**
   * @struct FrameHeader
   * @brief 帧协议的头部结构体
   *
   * 定义了EPF协议中帧的头部信息，包含魔数、帧类型、会话ID、数据偏移量和数据长度。
   *
   * @member magic 魔数，用于标识和验证帧的有效性
   * @member type 帧的类型，指示该帧所包含的数据类型或操作命令
   * @member session 会话ID，用于识别和关联相关的通信会话
   * @member offset 数据在整个传输中的字节偏移量，支持分片传输
   * @member len 当前帧中有效数据的长度（字节）
   */
  struct FrameHeader
  {
    uint32_t magic;
    uint8_t type;
    uint16_t session;
    uint32_t offset;
    uint16_t len;
  };

#pragma pack(pop)

  /**
   * @struct FrameView
   * @brief 帧数据视图结构体
   *
   * 用于表示一个完整的协议帧，包含帧头信息、有效负载数据和校验码。
   *
   * @member hdr 帧头，包含帧的元数据信息
   * @member payload 指向有效负载数据的常量指针
   * @member crc32 32位循环冗余校验码，用于验证帧数据的完整性
   */
  struct FrameView
  {
    FrameHeader hdr;
    const uint8_t *payload;
    uint32_t crc32;
  };

  /**
   * @enum ParseResult
   * @brief EPF协议解析结果枚举类
   *
   * 表示EPF协议数据包解析操作的各种可能结果。
   *
   * @var ParseResult::NeedMore
   *      需要更多数据才能完成解析
   *
   * @var ParseResult::Ok
   *      解析成功
   *
   * @var ParseResult::BadMagic
   *      魔数验证失败，数据包头部无效
   *
   * @var ParseResult::BadCrc
   *      CRC校验失败，数据包可能已损坏
   *
   * @var ParseResult::TooLarge
   *      数据包大小超过允许的最大限制
   */
  enum class ParseResult : uint8_t
  {
    NeedMore,
    Ok,
    BadMagic,
    BadCrc,
    TooLarge,
  };

  /**
   * @struct EncodeOptions
   * @brief 编码选项配置结构体
   *
   * 用于配置编码过程中的各种选项参数。
   *
   * @member include_magic 是否在编码数据中包含魔数标识
   *                       - true: 包含魔数（默认值）
   *                       - false: 不包含魔数
   */
  struct EncodeOptions
  {
    bool include_magic;
    EncodeOptions() : include_magic(true) {}
  };

  /**
   * @brief 计算数据的CRC32校验值
   *
   * @param data 指向要计算的数据缓冲区的指针
   * @param len 数据的长度（字节数）
   * @param seed 初始CRC值，默认为0。可用于计算多个数据块的连续CRC
   *
   * @return uint32_t CRC32校验值
   *
   * @details
   * 该函数使用标准CRC32算法计算输入数据的校验值。
   * 支持分段计算：可将前一次计算的返回值作为seed参数传入，
   * 以计算多个数据块组合后的CRC32值。
   *
   * @example
   * uint32_t crc = crc32(data, sizeof(data));
   * uint32_t crc_part1 = crc32(data1, size1);
   * uint32_t crc_full = crc32(data2, size2, crc_part1);
   */
  static uint32_t crc32(const uint8_t *data, size_t len, uint32_t seed = 0);

  /**
   * @brief 编码协议帧数据
   *
   * 将指定的帧类型、会话、偏移量和负载数据编码成二进制帧格式。
   *
   * @param type 帧类型，指定要编码的帧种类
   * @param session 会话ID，用于标识通信会话
   * @param offset 数据偏移量，表示数据在整体流中的位置
   * @param payload 指向负载数据的指针，可为nullptr如果负载长度为0
   * @param len 负载数据的长度，单位为字节
   * @param out 指向输出缓冲区的指针，用于存储编码后的帧数据
   * @param cap 输出缓冲区的容量，单位为字节
   * @param opt 编码选项，包含额外的编码参数配置，默认为空选项
   *
   * @return 返回编码后的帧数据长度（字节数）。如果编码失败或缓冲区空间不足，
   *         返回值可能为0或具体的错误码，应根据实现定义进行判断
   *
   * @note 调用者需确保output缓冲区的容量足以容纳编码后的数据
   * @note 负载数据不会被修改
   *
   * @see FrameType, EncodeOptions
   */
  static size_t encodeFrame(
      FrameType type,
      uint16_t session,
      uint32_t offset,
      const uint8_t *payload,
      uint16_t len,
      uint8_t *out,
      size_t cap,
      EncodeOptions opt);

  class FrameParser
  {
  public:
    /**
     * @brief 帧解析器的构造函数
     * 
     * 初始化一个 FrameParser 对象，用于解析协议帧数据。
     * 
     * @param max_payload 最大负载大小，单位为字节。默认值为 4096 字节。
     *                    该参数定义了解析器能够处理的最大数据负载长度。
     *                    如果接收到的负载数据超过此限制，解析器可能会拒绝或截断数据。
     * 
     * @note 构造函数为显式构造，不允许隐式类型转换。
     * 
     * @see parse() - 用于解析帧数据的成员函数
     */
    explicit FrameParser(size_t max_payload = 4096);

    /**
     * @brief 重置协议状态
     * 
     * 将EPF协议的所有状态重置为初始值，清除任何待处理的操作、
     * 缓冲区数据和内部状态机状态。此方法通常在初始化或错误恢复时调用。
     * 
     * @return void
     */
    void reset();

    /**
     * @brief 向协议处理器输入数据
     * 
     * 将接收到的原始字节数据输入到协议处理器中进行解析和处理。
     * 该函数会逐字节或按块处理数据，更新内部状态机。
     * 
     * @param data 指向数据缓冲区的指针，包含待处理的原始字节数据
     * @param len  待处理数据的长度（字节数）
     * 
     * @note 
     *   - data 指针不能为空
     *   - len 可以为 0，此时函数无操作
     *   - 该函数可被多次调用，适合流式数据处理
     * 
     * @see epf_protocol_parse() 数据解析相关函数
     */
    void feed(const uint8_t *data, size_t len);

    /**
     * @brief 解析下一帧数据
     * 
     * 从协议流中解析下一帧数据，并将结果存储到输出参数中。
     * 
     * @param[out] out 用于存储解析得到的帧数据的引用
     * 
     * @return ParseResult 解析结果，包含操作是否成功的状态信息
     * 
     * @note 该函数会改变内部的解析状态，successive calls 将解析后续的帧数据
     * 
     * @see FrameView, ParseResult
     */
    ParseResult nextFrame(FrameView &out);

    /**
     * @brief 获取缓冲区中的字节数
     * 
     * 返回当前缓冲区中已存储的字节数量。该函数用于
     * 查询缓冲区的使用情况，通常在处理数据之前检查
     * 可用数据量。
     * 
     * @return size_t 缓冲区中的字节数，若缓冲区为空则返回 0
     * 
     * @note 此函数不会修改缓冲区的内容，是一个只读操作
     * 
     * @see clearBuffer(), readBuffer()
     */
    size_t bufferedBytes() const;

  private:
    size_t max_payload_;
    size_t pending_consume_;
    std::vector<uint8_t> buffer_;
  };
};
