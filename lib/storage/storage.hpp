#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Storage

{
  /**
   * @enum PixelFormat
   * @brief 图像像素格式枚举
   *
   * 定义了电子墨水屏支持的像素格式，用于描述图像数据的位深度和颜色表示方式。
   *
   * @var Mono1Bpp
   * 单色1位/像素格式，每个像素用1比特表示，仅支持黑白两种颜色。
   *
   * @var Gray2Bpp
   * 灰度2位/像素格式，每个像素用2比特表示，支持4级灰度。
   *
   * @var Gray4Bpp
   * 灰度4位/像素格式，每个像素用4比特表示，支持16级灰度。
   */
  enum class PixelFormat : uint8_t
  {
    Mono1Bpp = 1,
    Gray2Bpp = 2,
    Gray4Bpp = 3,
  };

  /**
   * @brief 图像传输数据结构
   *
   * 用于存储图像传输过程中的元数据信息。
   *
   * @member transfer_id 传输ID，用于标识和追踪本次图像传输
   * @member width 图像宽度（像素）
   * @member height 图像高度（像素）
   * @member format 像素格式，详见 PixelFormat 枚举定义
   * @member total_bytes 图像数据的总字节数
   * @member crc32 CRC32校验码，用于验证数据完整性
   */
  struct ImageMeta
  {
    uint32_t transfer_id = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    PixelFormat format = PixelFormat::Mono1Bpp;
    uint32_t total_bytes = 0;
    uint32_t crc32 = 0;
  };

  /**
   * @enum ErrorCode
   * @brief 错误代码枚举
   *
   * 定义了图像存储操作中可能返回的各种错误代码，用于指示操作的执行结果状态。
   *
   * @var Ok
   * 操作成功，无错误发生。
   *
   * @var InvalidArg
   * 无效的参数，传入的参数不符合要求或超出有效范围。
   *
   * @var Busy
   * 设备或资源繁忙，无法立即完成操作，建议稍后重试。
   *
   * @var NotReady
   * 设备或资源未就绪，可能未初始化或处于不可用状态。
   *
   * @var NoSpace
   * 存储空间不足，无法完成数据写入操作。
   *
   * @var IoError
   * I/O操作错误，数据读写过程中发生错误。
   *
   * @var NotFound
   * 请求的资源或数据不存在。
   */
  enum class ErrorCode : uint16_t
  {
    Ok = 0,
    InvalidArg = 1,
    Busy = 2,
    NotReady = 3,
    NoSpace = 4,
    IoError = 5,
    NotFound = 6,
  };

  struct Result
  /**
   * @brief 操作结果结构体
   * @details 用于表示存储操作的执行结果，包含错误代码和操作的字节数
   *
   * @var code 操作的错误代码，默认为 ErrorCode::Ok 表示操作成功
   * @var bytes 操作涉及的字节数
   *
   * @fn ok() 检查操作是否成功
   * @return true 表示操作成功（code == ErrorCode::Ok），false 表示操作失败
   */
  {
    ErrorCode code = ErrorCode::Ok;
    uint32_t bytes = 0;

    bool ok() const
    {
      return code == ErrorCode::Ok;
    }
  };

  class IImageStorage
  {
  public:
    /**
     * @brief 虚析构函数
     *
     * 为接口类提供虚析构函数，确保派生类的析构函数能够被正确调用。
     * 使用默认实现，允许编译器自动生成析构函数体。
     */
    virtual ~IImageStorage() = default;

    /**
     * @brief 开始写入图像数据
     *      * 初始化一个新的写入操作，为即将写入的图像数据做准备。
     * 此方法应在实际写入图像数据之前调用。
     *
     * @param meta 图像元数据，包含图像的相关信息（如尺寸、格式等）
     *
     * @return Result 操作结果，指示写入初始化是否成功
     *
     * @note 调用此方法后，应通过相应的写入方法来写入实际的图像数据。
     *       完成写入后，应调用相应的提交或关闭方法来完成写入操作。
     */
    virtual Result begin_write(const ImageMeta &meta) = 0;

    /**
     * @brief 将数据块写入存储设备
     *
     * 将指定长度的数据写入到存储设备的指定偏移位置。
     *
     * @param offset 写入数据的起始偏移位置（字节为单位）
     * @param data 指向待写入数据的指针，不能为nullptr
     * @param len 待写入数据的长度（字节为单位）
     *
     * @return Result 操作结果，表示写入是否成功
     *
     * @note 调用者需确保offset和len的组合不会超出存储设备的有效范围
     * @note data指针必须有效且指向至少len字节的可读内存
     */
    virtual Result write_chunk(uint32_t offset, const uint8_t *data, size_t len) = 0;

    /**
     * @brief 提交存储操作     *
     * 将之前进行的存储操作永久提交到存储介质中。该方法应确保所有待处理的
     * 数据变更都被正确写入并持久化。
     *
     * @return Result 提交操作的结果状态，表示成功或失败
     *
     * @note 这是一个纯虚函数，需要由派生类实现具体的提交逻辑
     */
    virtual Result commit() = 0;

    /**
     * @brief 中止当前的写入操作
     *
     * 该函数用于中止正在进行的写入操作。当写入操作进行到一半时，
     * 可以调用此函数来取消剩余的写入操作，并释放相关资源。
     *
     * @return Result 操作的结果状态，指示中止操作是否成功
     *
     * @note 此函数为纯虚函数，需要由派生类实现具体逻辑
     *
     * @see begin_write()
     */
    virtual Result abort_write() = 0;

    /**
     * @brief 读取最新的图像元数据
     *
     * 从存储中读取最新保存的图像元数据信息。
     *
     * @param[out] meta 用于存储读取的图像元数据的引用。
     *                   该参数会被填充为最新的元数据内容。
     *
     * @return Result 操作结果。返回成功表示元数据读取成功，
     *                返回失败表示读取失败或不存在元数据。
     *
     * @note 这是一个纯虚函数，需要由子类实现具体的读取逻辑。
     */
    virtual Result read_latest_meta(ImageMeta &meta) = 0;

    /**
     * @brief 从存储介质读取最新数据
     *
     * @param offset 相对于存储区起始位置的偏移量（字节）
     * @param out 指向输出缓冲区的指针，用于存储读取的数据
     * @param len 要读取的数据长度（字节数）
     *
     * @return Result 操作结果，表示读取成功或失败
     *
     * @details 该函数从指定的偏移位置读取指定长度的最新数据。
     *          调用者需确保 out 缓冲区大小至少为 len 字节。
     */
    virtual Result read_latest_data(uint32_t offset, uint8_t *out, size_t len) = 0;
  };

  class MemoryImageStorage : public IImageStorage
  {
  public:
    /**
     * @brief 内存图像存储的构造函数
     *
     * @param max_bytes 存储器能容纳的最大字节数。用于限制内存中存储的图像数据的大小。
     *
     * @details 初始化一个内存图像存储对象，并设置其最大容量限制。
     *          该构造函数为显式构造函数，防止隐式类型转换。
     */
    explicit MemoryImageStorage(uint32_t max_bytes);

    /**
     * @param meta 包含图像元数据的ImageMeta对象，用于描述待写入的图像信息
     *
     * @return Result 操作结果，表示写入操作是否成功启动
     *
     * @details 该方法初始化写入操作，准备存储系统以接收图像数据。
     *          必须在实际写入图像数据之前调用此方法。
     */
    Result begin_write(const ImageMeta &meta) override;

    /**
     * @brief 向存储设备写入数据块
     *
     * @param offset 写入数据的起始偏移量（字节）
     * @param data 指向要写入数据的指针
     * @param len 要写入数据的长度（字节）
     *
     * @return Result 操作结果，表示写入是否成功
     *
     * @details
     * 此函数将指定长度的数据从给定指针写入到存储设备中的指定偏移位置。
     * 调用者需要确保提供的数据指针有效，且写入的数据不超过存储设备的容量。
     *
     * @note 该函数是虚函数的重写实现，具体行为由派生类定义。
     */
    Result write_chunk(uint32_t offset, const uint8_t *data, size_t len) override;

    /**
     * @details 将缓存的数据更改持久化到存储设备中。此函数确保
     *          所有待处理的写入操作都被完成，并将改动保存到底层
     *          存储介质（如闪存或EEPROM）中。
     * @return Result 操作结果，表示提交是否成功。返回值包含
     *         成功/失败状态以及相关的错误信息（如适用）。
     * @note   应在对存储数据进行重要修改后调用此函数，以确保
     *         数据不会因设备断电或异常关闭而丢失。
     * @see    Result 了解可能的返回值和错误状态。
     */
    Result commit() override;
    
    Result abort_write() override;
    
    Result read_latest_meta(ImageMeta &meta) override;

    /**
     * @brief 读取最新的数据
     * 
     * 从存储中读取最新的数据。该函数会从指定的偏移量开始读取数据到输出缓冲区中。
     * 
     * @param offset 读取数据的起始偏移量（字节单位）
     * @param out 指向输出缓冲区的指针，用于存储读取的数据
     * @param len 要读取的数据长度（字节单位）
     * 
     * @return Result 操作结果，返回状态码表示读取是否成功
     * 
     * @note 调用者需确保 out 缓冲区的大小足以容纳 len 字节的数据
     */
    Result read_latest_data(uint32_t offset, uint8_t *out, size_t len) override;

  private:
    /// @brief 存储区域的最大字节数
    /// 
    /// 表示该存储对象能够容纳的最大数据量（以字节为单位）
    uint32_t max_bytes_ = 0;

    /// @brief 标志位，表示存储器是否正在进行写入操作
    /// @details 当值为 true 时表示正在写入数据，false 表示未进行写入操作
    bool writing_ = false;

    /// @brief 表示是否已有最新的数据
    /// 
    /// 该标志位用于记录存储中是否包含最新的数据版本。
    /// 当值为 true 时，表示存储的数据是最新的；
    /// 当值为 false 时，表示存储的数据已过期或未初始化。
    bool has_latest_ = false;

    /**
     * @brief 临时存储的图像元数据
     * 
     * 用于存储待处理或临时上传的图像的元数据信息。
     * 在正式提交到存储之前，图像的相关信息会先保存到此成员变量中。
     */
    ImageMeta staging_meta_{};

    /// @brief 存储最新的图像元数据信息
    /// @details 该成员变量用于保存当前设备中最新加载或处理的图像的元数据，
    ///          包括图像的属性、尺寸、格式等相关信息
    ImageMeta latest_meta_{};

    /**
     * @brief 临时数据缓冲区
     * @details 用于存储待写入或正在处理的字节数据。
     *          此向量作为中间缓冲区，在数据持久化到存储设备前进行暂存。
     */
    std::vector<uint8_t> staging_data_{};

    /// @brief 存储最新的数据缓冲区
    /// @details 使用动态数组存储从设备读取或待写入的最新数据。
    ///          采用 uint8_t 类型以支持任意二进制数据的存储。
    std::vector<uint8_t> latest_data_{};
  };
}
