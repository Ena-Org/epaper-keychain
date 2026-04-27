#include "storage.hpp"

#include <algorithm>
#include <cstring>

namespace Storage
{
  namespace
  {
    /**
     * @brief 创建一个Result对象
     *
     * @param code 错误代码，用于标识操作的结果状态
     * @param bytes 字节数，表示处理的数据量，默认为0
     *
     * @return Result 包含错误代码和字节数的结果对象
     */
    Result make_result(ErrorCode code, uint32_t bytes = 0)
    {
      Result result;
      result.code = code;
      result.bytes = bytes;
      return result;
    }
  }

  /**
   * @brief MemoryImageStorage 构造函数
   * @param max_bytes 内存存储的最大字节数
   *
   * 初始化内存图像存储对象，设置最大可用字节数限制。
   */
  MemoryImageStorage::MemoryImageStorage(uint32_t max_bytes)
      : max_bytes_(max_bytes)
  {
  }

  /**
   * @brief 开始写入图像数据
   * @details 初始化内存中的image存储，准备接收新的图像数据。该函数会检查存储空间
   *          是否充足，并初始化用于暂存图像元数据和数据的内部缓冲区。
   *
   * @param meta 图像元数据，包含图像的总字节数等信息
   *
   * @return Result 操作结果
   *         - ErrorCode::Ok      成功开始写入操作
   *         - ErrorCode::Busy    存储器正在进行写入操作，无法启动新的写入
   *         - ErrorCode::NoSpace 请求的存储空间无效或超过最大可用空间
   *
   * @note 调用此函数后，writing_ 标志位会被设置为 true。
   *       必须在调用 write() 或 end_write() 之前调用此函数。
   */
  Result MemoryImageStorage::begin_write(const ImageMeta &meta)
  {
    if (writing_)
      return make_result(ErrorCode::Busy);

    if (meta.total_bytes == 0 || meta.total_bytes > max_bytes_)
      return make_result(ErrorCode::NoSpace);

    staging_meta_ = meta;
    staging_data_.assign(meta.total_bytes, 0);
    writing_ = true;
    return make_result(ErrorCode::Ok);
  }

  /**
   * @brief 向暂存缓冲区的指定偏移位置写入数据块
   *
   * 将指定长度的数据复制到内部暂存缓冲区中的指定偏移位置。该操作仅在
   * 写入模式激活时执行。
   *
   * @param offset 目标缓冲区的字节偏移位置
   * @param data 指向待写入数据的指针
   * @param len 待写入数据的字节长度
   *
   * @return Result 操作结果，包含：
   *         - 若成功：返回 ErrorCode::Ok，value 为实际写入的字节数
   *         - 若未处于写入模式：返回 ErrorCode::NotReady
   *         - 若数据指针为空或长度为0：返回 ErrorCode::InvalidArg
   *         - 若写入范围超出缓冲区大小：返回 ErrorCode::InvalidArg
   *
   * @note 必须确保 offset + len 不超过暂存缓冲区的总大小
   * @note 调用此函数前应确保写入模式已启动
   */
  Result MemoryImageStorage::write_chunk(uint32_t offset, const uint8_t *data, size_t len)
  {
    if (!writing_)
      return make_result(ErrorCode::NotReady);

    if (data == nullptr || len == 0)
      return make_result(ErrorCode::InvalidArg);

    const size_t end = static_cast<size_t>(offset) + len;
    if (end > staging_data_.size())
      return make_result(ErrorCode::InvalidArg);

    std::memcpy(staging_data_.data() + offset, data, len);
    return make_result(ErrorCode::Ok, static_cast<uint32_t>(len));
  }

  /**
   * @brief 提交暂存的图像数据到最新存储
   *
   * 将暂存区(staging)的元数据和图像数据提交到最新存储区(latest),
   * 完成一次写入操作。
   *
   * @return Result 操作结果
   *         - ErrorCode::Ok: 提交成功,返回已提交的数据字节数
   *         - ErrorCode::NotReady: 未处于写入状态,提交失败
   *
   * @note 此函数会:
   *       1. 检查是否处于写入状态
   *       2. 复制暂存元数据到最新元数据
   *       3. 复制暂存图像数据到最新图像数据
   *       4. 设置has_latest_标志为true,表示存在有效的最新数据
   *       5. 重置writing_标志为false
   *       6. 清空暂存数据
   */
  Result MemoryImageStorage::commit()
  {
    if (!writing_)
      return make_result(ErrorCode::NotReady);

    latest_meta_ = staging_meta_;
    latest_data_ = staging_data_;
    has_latest_ = true;
    writing_ = false;
    staging_data_.clear();
    return make_result(ErrorCode::Ok, latest_meta_.total_bytes);
  }

  /**
   * @brief 中止当前的写入操作
   *
   * 该函数用于取消正在进行的图像写入操作。如果当前没有进行写入操作，
   * 则返回NotReady错误。成功中止写入后，会清空暂存数据和元数据。
   *
   * @return Result 操作结果
   *         - ErrorCode::Ok：成功中止写入操作
   *         - ErrorCode::NotReady：当前没有进行中的写入操作
   *
   * @note 调用此函数后，所有未提交的数据将被丢弃
   */
  Result MemoryImageStorage::abort_write()
  {
    if (!writing_)
      return make_result(ErrorCode::NotReady);

    writing_ = false;
    staging_data_.clear();
    staging_meta_ = ImageMeta{};
    return make_result(ErrorCode::Ok);
  }

  /**
   * @brief 读取最新的图像元数据
   *
   * @param[out] meta 用于存储读取的图像元数据的引用
   *
   * @return Result 操作结果
   *   - ErrorCode::Ok: 成功读取元数据
   *   - ErrorCode::NotFound: 不存在最新的元数据
   *
   * @note 此方法从内存中获取最新缓存的图像元数据。
   *       在调用此方法前，请确保已通过其他操作设置过元数据。
   */
  Result MemoryImageStorage::read_latest_meta(ImageMeta &meta)
  {
    if (!has_latest_)
      return make_result(ErrorCode::NotFound);

    meta = latest_meta_;
    return make_result(ErrorCode::Ok);
  }

  /**
   * @brief 读取最新存储的数据
   * 
   * 从内存中读取最新保存的图像数据。支持从指定偏移量开始读取，
   * 可以部分读取数据。
   * 
   * @param offset 读取的起始偏移量（单位：字节）
   * @param out 输出缓冲区指针，用于存储读取的数据
   * @param len 请求读取的字节数
   * 
   * @return Result 操作结果
   *         - ErrorCode::NotFound 如果没有最新数据可读取
   *         - ErrorCode::InvalidArg 如果 out 为 nullptr、len 为 0 或 offset 超出数据范围
   *         - ErrorCode::Ok 读取成功，返回值包含实际读取的字节数
   * 
   * @note 如果请求的读取长度超过可用数据长度，将只读取可用的部分
   */
  Result MemoryImageStorage::read_latest_data(uint32_t offset, uint8_t *out, size_t len)
  {
    if (!has_latest_)
      return make_result(ErrorCode::NotFound);

    if (out == nullptr || len == 0)
      return make_result(ErrorCode::InvalidArg);

    if (offset >= latest_data_.size())
      return make_result(ErrorCode::InvalidArg);

    const size_t readable = latest_data_.size() - offset;
    const size_t copy_len = std::min(readable, len);
    std::memcpy(out, latest_data_.data() + offset, copy_len);
    return make_result(ErrorCode::Ok, static_cast<uint32_t>(copy_len));
  }
}
