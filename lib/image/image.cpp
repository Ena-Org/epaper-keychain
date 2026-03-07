#include "image.hpp"
#include <cstddef>
#include <cstdint>

namespace Image
{
  namespace
  {
    /// @brief CRC32校验和的初始种子值
    /// @details 用于CRC32算法的初始化，遵循标准CRC32实现约定
    /// 该值为32位无符号整数的最大值，所有比特位均为1
    constexpr uint32_t kCrc32Seed = 0xFFFFFFFFu;

    /**
     * @brief 创建一个Result对象
     * @param code 错误代码，指示操作的结果状态
     * @param accepted_bytes 已接受的字节数，默认为0
     * @return 返回包含指定错误代码和已接受字节数的Result对象
     */
    Result make_result(ErrorCode code, uint32_t accepted_bytes = 0)
    {
      Result result;
      result.code = code;
      result.accepted_bytes = accepted_bytes;
      return result;
    }

    /**
     * @brief 计算数据的CRC32校验值
     *
     * @details 使用标准CRC32多项式(0xEDB88320)计算给定数据的CRC32校验值。
     *          该函数采用反向比特位顺序的CRC算法,逐字节处理输入数据。
     *
     * @param crc 初始CRC32值,通常为0xFFFFFFFF或前一次计算的结果
     * @param data 指向要计算校验值的数据缓冲区的指针
     * @param len 数据缓冲区的长度(字节数)
     *
     * @return uint32_t 计算得到的CRC32校验值
     *
     * @note 如果要计算多段数据的组合CRC32,可以将前一次的返回值作为下一次调用的crc参数
     * @note 最终结果通常需要与0xFFFFFFFF进行异或操作以得到标准CRC32值
     */
    uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len)
    {
      uint32_t value = crc;
      for (size_t i = 0; i < len; ++i)
      {
        value ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
          if ((value & 1u) != 0u)
            value = (value >> 1u) ^ 0xEDB88320u;
          else
            value >>= 1u;
        }
      }
      return value;
    }
  }

  /**
   * @brief 初始化图像服务
   *
   * @param store_port 存储端口指针，用于访问存储设备
   * @return true 初始化成功
   * @return false 初始化失败（store_port为nullptr）
   *
   * @details 该函数将验证传入的存储端口指针有效性，
   *          如果有效则保存指针并重置服务状态。
   */
  bool Service::init(IStorePort *store_port)
  {
    if (store_port == nullptr)
    {
      return false;
    }

    store_port_ = store_port;
    reset();
    return true;
  }

  /**
   * @brief 开始一个图像上传会话
   *
   * 初始化图像上传过程，验证上传元数据的有效性，并准备存储端口进行写入操作。
   *
   * @param meta 上传元数据，包含图像的总字节数、宽度和高度等信息
   *
   * @return Result 操作结果
   *         - ErrorCode::Ok: 会话成功启动
   *         - ErrorCode::StorageError: 存储端口不可用或写入初始化失败
   *         - ErrorCode::Busy: 已存在活跃的上传会话，无法同时开始新会话
   *         - ErrorCode::InvalidArg: 元数据无效（总字节数、宽度或高度为0）
   *
   * @note 此方法会设置内部状态，包括活跃元数据、会话标志、接收字节数和CRC32校验和
   * @note 调用此方法前，请确保存储端口已初始化
   */
  Result Service::begin(const UploadMeta &meta)
  {
    if (store_port_ == nullptr)
      return make_result(ErrorCode::StorageError);

    if (has_active_session_)
      return make_result(ErrorCode::Busy);

    if (meta.total_bytes == 0 || meta.width == 0 || meta.height == 0)
      return make_result(ErrorCode::InvalidArg);

    const Result store_result = store_port_->begin_write(meta);
    if (!store_result.ok())
      return make_result(ErrorCode::StorageError);

    active_meta_ = meta;
    has_active_session_ = true;
    received_bytes_ = 0;
    running_crc32_ = kCrc32Seed;
    return make_result(ErrorCode::Ok);
  }

  /**
   * @brief 追加数据块到活跃的图像传输会话
   *
   * 该函数用于逐块接收图像数据，并将其存储到持久化存储中。每次调用必须按顺序
   * 传入连续的数据块，偏移量必须与已接收字节数匹配。
   *
   * @param chunk_offset 数据块的偏移量（字节），必须等于已接收的字节总数
   * @param data 指向要追加的数据的指针
   * @param len 要追加的数据长度（字节）
   *
   * @return Result 操作结果
   *   - 成功时返回 ErrorCode::Ok，同时返回实际写入的字节数
   *   - 若当前没有活跃的传输会话，返回 ErrorCode::BadState
   *   - 若数据指针为空或长度为0，返回 ErrorCode::InvalidArg
   *   - 若偏移量不连续，返回 ErrorCode::OutOfRange
   *   - 若追加后的总大小超过限制，返回 ErrorCode::OutOfRange
   *   - 若存储操作失败，返回 ErrorCode::StorageError
   *
   * @note 该函数会自动更新运行的CRC32校验值
   * @note 必须先建立活跃的传输会话才能调用此函数
   */
  Result Service::append_chunk(uint32_t chunk_offset, const uint8_t *data, size_t len)
  {
    if (!has_active_session_)
      return make_result(ErrorCode::BadState);

    if (data == nullptr || len == 0)
      return make_result(ErrorCode::InvalidArg);

    if (chunk_offset != received_bytes_)
      return make_result(ErrorCode::OutOfRange);

    if (static_cast<uint64_t>(chunk_offset) + static_cast<uint64_t>(len) > active_meta_.total_bytes)
      return make_result(ErrorCode::OutOfRange);

    const Result store_result = store_port_->write_chunk(chunk_offset, data, len);
    if (!store_result.ok())
      return make_result(ErrorCode::StorageError);

    running_crc32_ = crc32_update(running_crc32_, data, len);
    received_bytes_ += static_cast<uint32_t>(len);
    return make_result(ErrorCode::Ok, static_cast<uint32_t>(len));
  }

  /**
   * @brief 结束当前的数据传输会话
   *
   * 该函数用于完成正在进行的图像数据传输过程。在结束会话前，会进行以下检查和操作：
   * 1. 验证是否存在活跃的会话
   * 2. 验证接收的数据字节数是否与预期总数相符
   * 3. 计算并验证CRC32校验和
   * 4. 将数据提交到存储设备
   *
   * @return Result 返回操作结果，包含以下可能的错误码：
   *         - ErrorCode::Ok - 会话成功结束，数据已提交到存储
   *         - ErrorCode::BadState - 当前没有活跃的会话
   *         - ErrorCode::OutOfRange - 接收的数据字节数与预期不符
   *         - ErrorCode::CrcMismatch - CRC32校验失败，数据已损坏
   *         - ErrorCode::StorageError - 数据提交到存储设备失败
   *         Result对象还包含received_bytes_，表示本次会话接收的总字节数
   *
   * @note 无论操作成功还是失败，该函数都会关闭活跃会话（设置has_active_session_为false）
   * @note 当CRC32校验失败时，会调用store_port_->abort_write()来中止写入操作
   */
  Result Service::end()
  {
    if (!has_active_session_)
      return make_result(ErrorCode::BadState);

    if (received_bytes_ != active_meta_.total_bytes)
      return make_result(ErrorCode::OutOfRange, received_bytes_);

    const uint32_t computed_crc32 = running_crc32_ ^ kCrc32Seed;
    if (computed_crc32 != active_meta_.expected_crc32)
    {
      store_port_->abort_write();
      has_active_session_ = false;
      return make_result(ErrorCode::CrcMismatch, received_bytes_);
    }

    const Result store_result = store_port_->commit();
    if (!store_result.ok())
    {
      has_active_session_ = false;
      return make_result(ErrorCode::StorageError, received_bytes_);
    }

    has_active_session_ = false;
    return make_result(ErrorCode::Ok, received_bytes_);
  }

  /**
   * @brief 应用服务操作
   *
   * @details 此函数尝试应用当前服务的操作。如果服务已有活跃会话，
   *          则返回忙碌错误；否则返回成功状态。
   *
   * @return Result 操作结果，包含以下可能的错误码：
   *         - ErrorCode::Busy 当前存在活跃会话，服务忙碌
   *         - ErrorCode::Ok 操作成功
   *
   * @note 调用此函数前应确保没有其他并发操作正在进行
   */
  Result Service::apply()
  {
    if (has_active_session_)
      return make_result(ErrorCode::Busy);

    return make_result(ErrorCode::Ok);
  }

  /**
   * @brief 中止当前的活跃会话
   *
   * 中止正在进行的数据传输会话。该函数将重置会话状态、接收字节计数器和CRC32校验值。
   *
   * @return Result 操作结果
   *         - ErrorCode::Ok: 成功中止会话
   *         - ErrorCode::BadState: 当前没有活跃会话，无法中止
   *         - ErrorCode::StorageError: 存储端口中止写入操作失败
   *
   * @note 调用此函数后，has_active_session_ 标志将被设置为 false，
   *       received_bytes_ 和 running_crc32_ 将被重置为初始值。
   */
  Result Service::abort()
  {
    if (!has_active_session_)
      return make_result(ErrorCode::BadState);

    const Result store_result = store_port_->abort_write();
    has_active_session_ = false;
    received_bytes_ = 0;
    running_crc32_ = kCrc32Seed;

    if (!store_result.ok())
      return make_result(ErrorCode::StorageError);

    return make_result(ErrorCode::Ok);
  }

  /**
   * @brief 检查服务是否处于忙碌状态
   * @return true 如果服务有活动会话，false 否则
   * @note 该方法用于判断服务当前是否正在处理任务或维持活跃连接
   */
  bool Service::is_busy() const
  {
    return has_active_session_;
  }

  /**
   * @brief 获取当前活跃传输的ID
   *
   * @return uint32_t 返回当前活跃会话的传输ID。如果没有活跃会话，返回0
   *
   * @note 此函数仅在有活跃会话时返回有效的传输ID值
   */
  uint32_t Service::active_transfer_id() const
  {
    if (!has_active_session_)
      return 0;

    return active_meta_.transfer_id;
  }

  /**
   * @brief 获取已接收的字节数
   *
   * @return uint32_t 已接收的字节数
   */
  uint32_t Service::received_bytes() const
  {
    return received_bytes_;
  }

  /**
   * @brief 重置服务的状态
   * 
   * 此函数用于清除服务的所有活动状态。如果当前存在活动会话且存储端口有效，
   * 将中止写入操作。随后重置上传元数据、活动会话标志、已接收字节数和CRC32校验值。
   * 
   * @note 此函数通常在上传完成、中止或出错时调用，以恢复到初始状态。
   */
  void Service::reset()
  {
    if (has_active_session_ && store_port_ != nullptr)
      store_port_->abort_write();

    active_meta_ = UploadMeta{};
    has_active_session_ = false;
    received_bytes_ = 0;
    running_crc32_ = kCrc32Seed;
  }
}
