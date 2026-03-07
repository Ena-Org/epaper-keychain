#pragma once
#include <cstddef>
#include <cstdint>

namespace Image

{
  /**
   * @enum PixelFormat
   * @brief 像素格式枚举类
   *
   * 定义图像数据中像素的色深和编码方式。不同的像素格式
   * 决定了每个像素占用的比特数以及色彩信息的表现方式。
   *
   * @var Mono1Bpp
   *      单色1位每像素格式。每个像素用1比特表示，
   *      通常用于黑白二值图像。存储效率最高。
   *
   * @var Gray2Bpp
   *      灰度2位每像素格式。每个像素用2比特表示，
   *      可表示4种灰度级别（0-3），适合简单的灰度显示
   *
   * @var Gray4Bpp
   *      灰度4位每像素格式。每个像素用4比特表示，
   *      可表示16种灰度级别（0-15），提供更好的灰度效果
   */
  enum class PixelFormat : uint8_t
  {
    Mono1Bpp = 1,
    Gray2Bpp = 2,
    Gray4Bpp = 3,
  };

  /**
   * @struct ImageHeader
   * @brief 图像传输头部信息结构体
   *
   * 用于定义和存储图像传输过程中的元数据信息，包括传输标识、
   * 图像尺寸、像素格式、数据大小和校验值等。
   *
   * @member transfer_id 传输ID，用于标识唯一的图像传输任务
   * @member width 图像宽度（像素）
   * @member height 图像高度（像素）
   * @member format 像素格式，默认为单色1位每像素(Mono1Bpp)
   * @member total_bytes 图像数据的总字节数
   * @member expected_crc32 图像数据的期望CRC32校验值，用于数据完整性验证
   */
  struct UploadMeta
  {
    uint32_t transfer_id = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    PixelFormat format = PixelFormat::Mono1Bpp;
    uint32_t total_bytes = 0;
    uint32_t expected_crc32 = 0;
  };

  /**
   * @enum ErrorCode
   * @brief 图像处理错误代码枚举
   *
   * 定义图像处理和传输操作中可能出现的各种错误类型。
   * 通过标准化的错误代码，调用者可以准确识别操作失败的原因，
   * 进而采取相应的错误处理策略。
   *
   * @var Ok
   *      操作成功完成，无错误发生
   *
   * @var Busy
   *      设备或存储端口当前处于忙碌状态，无法处理新请求。
   *      建议调用者稍后重试
   *
   * @var InvalidArg
   *      传入的参数无效或不符合要求。
   *      可能原因包括：宽高为0、格式不支持或空指针等
   *
   * @var BadState
   *      当前操作状态不符合要求。
   *      例如：尝试在未调用begin()的情况下调用append_chunk()
   *
   * @var OutOfRange
   *      写入数据超出了指定的范围或容量限制。
   *      可能原因包括：偏移量过大、数据长度超标等
   *
   * @var CrcMismatch
   *      接收到的数据CRC32校验值与期望值不匹配。
   *      表示数据在传输或存储过程中可能被损坏或篡改
   *
   * @var StorageError
   *      存储设备出现错误，如读写失败、空间不足等。
   *      建议检查存储设备状态和可用空间
   *
   * @var NotFound
   *      请求的资源或数据不存在。
   *      可能原因包括：指定的图像不存在、传输ID无效等
   */
  enum class ErrorCode : uint16_t
  {
    Ok = 0,
    Busy = 1,
    InvalidArg = 2,
    BadState = 3,
    OutOfRange = 4,
    CrcMismatch = 5,
    StorageError = 6,
    NotFound = 7,
  };

  /**
   * @struct 图像处理结果结构体
   * @brief 用于存储图像操作的执行结果和相关信息
   *
   * @member code 错误代码，表示操作是否成功
   * @member accepted_bytes 已接受的字节数，表示成功处理的数据量
   *
   * @method ok() 检查操作是否成功
   * @return true 如果错误代码为Ok，表示操作成功；false 否则
   */
  struct Result
  {
    ErrorCode code = ErrorCode::Ok;
    uint32_t accepted_bytes = 0;

    bool ok() const
    {
      return code == ErrorCode::Ok;
    }
  };

  class IStorePort
  {
  public:
    /**
     * @brief 虚析构函数
     *
     * 用于确保通过基类指针删除派生类对象时，
     * 派生类的析构函数能够被正确调用。
     */
    virtual ~IStorePort() = default;

    /**
     * @brief 开始写入图像数据
     *
     * 这是一个纯虚函数，用于初始化图像写入操作。
     * 子类必须实现此方法来准备写入过程，例如验证元数据、
     * 分配资源或配置写入参数。
     *
     * @param meta 上传元数据，包含写入操作所需的配置信息
     * @return Result 操作结果，表示初始化是否成功
     *
     * @note 此方法应在调用其他写入相关方法之前被调用
     */
    virtual Result begin_write(const UploadMeta &meta) = 0;

    /**
     * @brief 向指定偏移位置写入数据块
     *
     * @param offset 写入的起始偏移位置（单位：字节）
     * @param data 指向待写入数据的指针
     * @param len 待写入数据的长度（单位：字节）
     *
     * @return Result 写入操作的结果状态
     */
    virtual Result write_chunk(uint32_t offset, const uint8_t *data, size_t len) = 0;

    /**
     * @brief 提交更改
     *
     * 将所有待处理的更改应用到图像上。此方法必须在修改完成后调用，
     * 以确保所有操作生效。
     *
     * @return Result 操作结果，指示提交是否成功
     */
    virtual Result commit() = 0;

    /**
     * @brief 中止写入操作
     *
     * 该方法用于中止当前正在进行的写入操作。调用此方法后，
     * 写入过程应该被停止，资源应该被释放。
     *
     * @return Result 操作的结果状态，表示中止操作是否成功
     *
     * @note 这是一个纯虚函数，必须由派生类实现
     */
    virtual Result abort_write() = 0;
  };

  class Service
  {
  public:
    /**
     * @brief 初始化图像模块
     *
     * @param store_port 指向存储端口接口的指针，用于访问存储功能
     *
     * @return bool 初始化是否成功
     *             true  - 初始化成功
     *             false - 初始化失败
     *
     * @note 该函数应在使用图像模块的其他功能前调用
     */
    bool init(IStorePort *store_port);

    /**
     * @brief 开始上传图像数据
     *
     * @param meta 上传元数据，包含上传操作所需的配置信息
     * @return Result 操作结果，表示开始上传是否成功
     *
     * @note 此函数用于初始化图像上传过程，必须在发送实际图像数据前调用
     * @see UploadMeta, Result
     */
    Result begin(const UploadMeta &meta);

    /**
     * @brief 向图像数据追加一个数据块
     *
     * @param chunk_offset 数据块在整个图像中的偏移量（字节为单位）
     * @param data 指向要追加的数据的指针
     * @param len 要追加的数据长度（字节为单位）
     *
     * @return Result 操作结果，表示追加是否成功
     *
     * @details
     * 此函数用于分块追加图像数据，通常用于从外部存储设备（如闪存）
     * 逐步加载图像数据到内存中。调用者需要确保提供的偏移量和数据
     * 长度组合不会超过图像的总大小。
     */
    Result append_chunk(uint32_t chunk_offset, const uint8_t *data, size_t len);

    /**
     * @brief 结束图像处理
     *
     * 完成当前图像的处理流程，释放相关资源并完成最后的清理工作。
     *
     * @return Result 操作结果，表示是否成功完成
     */
    Result end();

    /**
     * @brief 应用图像处理操作
     *
     * 执行之前设置的所有图像处理操作，如缩放、旋转、滤镜等。
     * 该函数会将处理结果应用到当前图像对象上。
     *
     * @return Result 返回操作的执行结果，包含成功/失败状态以及相关错误信息
     *
     * @note 在调用此函数前，请确保已正确配置所有必要的参数
     * @see Result 获取更多返回值类型的信息
     */
    Result apply();

    /**
     * @brief 中止当前操作
     * @details 该函数用于中止正在进行的图像处理或传输操作，
     *          立即停止所有后续操作并释放相关资源。
     * @return Result 操作结果，指示中止操作是否成功
     * @note 调用此函数后，对象可能需要重新初始化才能继续使用
     */
    Result abort();

    /**
     * @brief 检查设备是否正在忙碌
     * @return true 如果设备正在忙碌；false 如果设备空闲
     */
    bool is_busy() const;

    /**
     * @brief 获取当前活跃的传输ID
     *
     * @return uint32_t 当前活跃传输的唯一标识符
     */
    uint32_t active_transfer_id() const;

    /**
     * @brief 获取已接收的字节数
     * @return uint32_t 返回已接收的字节数
     */
    uint32_t received_bytes() const;

    /**
     * @brief 重置图像状态
     *
     * 将图像对象恢复到初始状态，清除所有已加载的数据和配置。
     * 调用此方法后，图像对象将可以重新初始化或加载新的图像数据。
     */
    void reset();

  private:
    /// @brief 存储端口指针
    /// @details 用于与存储设备进行通信的端口接口指针，为空指针表示尚未初始化
    IStorePort *store_port_ = nullptr;

    /**
     * @brief 当前活跃的上传元数据
     *
     * 存储与文件上传相关的元数据信息，用于跟踪
     * 当前正在处理或最后上传的文件的相关参数。
     */
    UploadMeta active_meta_{};

    /**
     * @brief 表示是否存在活跃的会话
     * @details 该标志用于跟踪系统中是否当前有活跃的用户会话。
     *          当会话建立时设置为true,会话结束时设置为false。
     */
    bool has_active_session_ = false;

    /// @brief 已接收的字节数
    /// @details 用于追踪从数据源接收到的字节数量，通常用于处理图像数据传输或加载进度
    uint32_t received_bytes_ = 0;

    /// @brief 用于计算CRC32校验和的运行值
    /// @details 存储图像数据在传输或存储过程中的累积CRC32校验和，用于数据完整性验证
    uint32_t running_crc32_ = 0;
  };
}
