#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include "protocol_packet.hpp"

class Codec
{
public:
  /**
   * @brief 数据包类型的别名
   *
   * 这是一个类型别名，用于引用协议中定义的数据包类型。
   * 通过使用此别名，可以简化代码中对 Protocol::PacketType 的引用，
   * 提高代码的可读性和可维护性。
   *
   * @see Protocol::PacketType
   */
  using PacketType = Protocol::PacketType;

  /**
   * @brief 数据包类型别名
   * @details 将 Protocol::Packet 类型别名为 Packet，用于简化代码中的类型引用。
   *          这个别名提供了一个更简洁的方式来访问协议层定义的数据包结构。
   */
  using Packet = Protocol::Packet;

  /// @brief 编解码操作的错误类型枚举
  /// @details 用于表示编解码过程中可能出现的各种错误状态
  enum class Error : uint8_t
  {
    None = 0,        ///< 无错误
    InvalidArgument, ///< 无效的参数
    BufferOverflow,  ///< 缓冲区溢出
    BadMagic,        ///< 魔数校验失败
    BadLength,       ///< 长度字段无效
    BadChecksum,     ///< 校验和验证失败
    NeedMore,        ///< 需要更多数据
    EncodeFailed,    ///< 编码操作失败
  };

  /**
   * @struct Config
   * @brief 编解码器配置结构体
   *
   * 用于配置编解码器的缓冲区和负载大小限制。
   *
   * @member maxPayloadBytes 最大有效负载字节数，默认为4096字节。
   *                         用于限制单次编解码操作处理的数据大小。
   *
   * @member maxBufferBytes 最大缓冲区字节数，默认为8192字节。
   *                        用于限制内部缓冲区的最大容量。
   */
  struct Config
  {
    size_t maxPayloadBytes = 4096;
    size_t maxBufferBytes = 8192;
  };

public:
  /**
   * @brief Codec 类的构造函数
   *
   * 初始化 Codec 对象。建立编码解码器的基本状态和必要的资源。
   *
   * @details
   * 该构造函数负责设置 Codec 实例的初始化工作，包括
   * 初始化成员变量和准备编码解码所需的资源。
   */
  Codec();

  /**
   * @brief Codec类的构造函数
   * @details 使用给定的配置对象初始化Codec实例
   * @param cfg 常量引用，包含Codec所需的配置参数
   * @throws 如果配置参数无效，可能抛出异常
   * @note 该构造函数为explicit，防止隐式类型转换
   */
  explicit Codec(const Config &cfg);

  /**
   * @brief 设置编解码器配置
   * @param cfg 编解码器配置对象的常引用，包含所有配置参数
   * @details 该函数将传入的配置应用到编解码器实例中。
   *          新的配置将覆盖之前的设置。
   * @note 调用此函数后，编解码器将使用新的配置参数进行后续的编解码操作。
   * @see Config
   */
  void setConfig(const Config &cfg);

  /**
   * @brief 获取编解码器的配置信息
   *
   * @return const Config& 返回编解码器配置对象的常引用
   */
  const Config &config() const;

  /**
   * @brief 将数据包编码为字节向量
   *
   * 该函数将给定的 Packet 对象编码为二进制格式，并将结果存储在输出向量中。
   *
   * @param packet 待编码的源数据包
   * @param out 存储编码结果的输出字节向量。函数执行完成后，out 将包含编码后的二进制数据
   *
   * @return true 表示编码成功，false 表示编码失败
   *
   * @note 输出向量 out 的内容会被追加到现有数据之后
   */
  bool encode(const Packet &packet, std::vector<uint8_t> &out) const;

  /**
   * @brief 向编解码器输入数据
   *
   * 将指定长度的数据字节流传递给编解码器进行处理。该方法通常用于
   * 逐步输入要编码或解码的数据。
   *
   * @param data 指向输入数据缓冲区的指针，数据将被复制或引用处理
   * @param len 输入数据的长度，单位为字节
   *
   * @note 调用此方法后，编解码器将开始处理输入的数据
   * @note 如果需要处理大量数据，可以多次调用此方法
   */
  void feed(const uint8_t *data, size_t len);

  /**
   * @brief 获取下一个数据包
   *
   * 从编码器或解码器中获取下一个处理结果的数据包。
   * 该函数会依次处理数据并返回每个生成的数据包。
   *
   * @param[out] out 输出参数，用于存储获取到的数据包
   *
   * @return true 表示成功获取到下一个数据包
   * @return false 表示没有更多的数据包可供获取
   *
   * @note 调用者应该在返回 true 的情况下处理 out 参数中的数据包，
   *       当返回 false 时表示所有数据包都已处理完毕
   */
  bool next(Packet &out);

  /**
   * @brief 重置编解码器的状态
   *
   * 将编解码器恢复到初始状态，清除任何之前处理的数据和内部缓冲区。
   * 调用此方法后，编解码器可以开始处理新的数据流。
   */
  void reset();

  /**
   * @brief 获取缓冲区中当前缓存的字节数
   *
   * @return size_t 缓冲区中已缓存的字节数
   */
  size_t bufferedBytes() const;

  /**
   * @brief 获取最后发生的错误
   *
   * 返回编解码器执行过程中最后发生的错误信息。
   * 如果没有发生错误，返回表示成功的错误码。
   *
   * @return Error 最后发生的错误对象，包含错误类型和详细信息
   *
   * @note 此函数为常成员函数，不会修改对象状态
   */
  Error lastError() const;

  /**
   * @brief 获取最后一次操作的错误信息文本
   *
   * @return const char* 指向错误信息文本的常量指针。
   *                     如果没有发生错误，返回值可能为空指针或空字符串。
   */
  const char *lastErrorText() const;

  /**
   * @brief 计算给定数据的32位校验和。
   *
   * 此函数根据输入的数据指针和长度，结合可选的种子值，计算并返回一个32位无符号整数的校验和。
   *
   * @param data 指向待计算校验和的数据缓冲区的指针。
   * @param len 数据缓冲区的字节长度。
   * @param seed 可选参数，校验和的初始种子值，默认为0。
   * @return uint32_t 计算得到的32位校验和。
   */
  static uint32_t checksum32(const uint8_t *data, size_t len, uint32_t seed = 0);

private:
#pragma pack(push, 1)
  /**
   * @brief 表示数据帧的头部结构体。
   *
   * 该结构体用于描述通信协议中数据帧的头部信息，包括魔术数、类型、会话号、命令ID、返回码以及数据长度等字段。
   *
   * 字段说明：
   * - magic: 魔术数，用于标识数据帧的有效性。
   * - type: 帧类型，区分不同的数据帧类别。
   * - session: 会话号，用于标识一次会话过程。
   * - cmdId: 命令ID，表示具体的命令类型。
   * - code: 返回码，表示命令执行的结果状态。
   * - len: 数据长度，表示后续数据部分的字节数。
   */
  struct FrameHeader
  {
    uint32_t magic;
    uint8_t type;
    uint16_t session;
    uint16_t cmdId;
    uint16_t code;
    uint16_t len;
  };

#pragma pack(pop)

  /**
   * @brief 魔数常量，用于标识特定的数据格式或文件类型。
   * 
   * 该常量的值为 0x31434443u，通常用于数据校验或文件头部的唯一标识，
   * 以确保数据的完整性和正确性。
   */
  static constexpr uint32_t kMagic = 0x31434443u; 

  /**
   * @brief 解码一个数据包。
   *
   * 尝试从输入流中解码一个数据包，并将结果存储到 out 参数中。
   *
   * @param[out] out 解码后的数据包对象。
   * @return 如果解码成功返回 true，否则返回 false。
   */
  bool decodeOne_(Packet &out);

  /**
   * @brief 设置错误状态。
   * 
   * 此函数用于设置当前对象的错误状态。传入的错误类型将被记录，用于后续的错误处理或调试。
   * 
   * @param err 要设置的错误类型，类型为 Error。
   */
  void setError_(Error err) const;

private:
  // Config 配置结构体的实例，用于存储和管理相关的配置信息。
  // 该成员变量在类内部使用，负责初始化和维护配置信息的状态。
  Config config_{};

  /**
   * @brief 用于存储接收数据的缓冲区。
   *
   * 该缓冲区以字节（uint8_t）为单位，动态存储接收到的数据内容。
   * 适用于需要按字节处理数据流的场景，如串口通信、网络数据接收等。
   */
  std::vector<uint8_t> rxBuffer_{};

  /**
   * @brief 表示错误类型的枚举类。
   *
   * 该枚举用于描述在编解码过程中可能出现的各种错误类型。
   * 例如：无错误、参数错误、数据损坏、内存不足等。
   */
  mutable Error lastError_ = Error::None;
};
