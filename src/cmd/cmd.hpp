#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "protocol_packet.hpp"
#include "router.hpp"
#include "codec.hpp"
#include "image.hpp"
#include "storage.hpp"
#include "transport.hpp"

class Cmd : public Router::Context
{
public:
  /**
   * @brief 数据包类型别名
   * @details 将 Protocol::Packet 类型定义为 Packet，用于简化代码中的类型引用
   *          避免每次都需要写完整的 Protocol::Packet 限定名称
   */
  using Packet = Protocol::Packet;

  /**
   * @brief Cmd 类的构造函数
   * 
   * 初始化 Cmd 对象，执行必要的初始化操作。
   * 
   * @details
   * 该构造函数用于创建一个新的 Cmd 实例，并进行相关的
   * 初始化工作，如设置默认值、分配资源等。
   */
  Cmd();

  /**
   * @brief 初始化命令处理系统
   * 
   * @param transport 传输层对象，用于处理数据的收发
   * @param codec 编解码器对象，用于数据的编码和解码
   * @param router 路由器对象，用于命令的路由和分发
   * 
   * @return void
   * 
    * @details 该函数会完成以下初始化工作：
    *          1) 绑定传输层、编解码器和路由器依赖；
    *          2) 使用默认配置初始化并连接传输层；
    *          3) 注册默认命令处理器。
    *          必须在使用其他命令处理功能之前调用此函数。
   */
  void init(Transport &transport, Codec &codec, Router &router);

  /**
   * @brief 注册所有命令处理器
   * 
   * 该函数用于初始化和注册系统中所有可用的命令处理器。
   * 在应用程序启动时应调用此函数，以确保所有命令路由能够正确工作。
   * 
   * @return void
   * 
   * @note 该函数应在系统初始化阶段调用，且仅需调用一次
   * @see unregisterHandlers()
   */
  void registerHandlers();

  /**
   * @brief 主循环函数
   * 
   * 该函数实现程序的主循环逻辑。每次调用时执行一次循环迭代，
   * 处理命令或系统事件。应该在主程序的主循环中重复调用此函数。
   * 
   * @note 该函数为非阻塞式调用，每次执行后立即返回。
   */
  void loop();

  /**
   * @brief 发送请求命令到指定的会话
   * 
   * @param session 会话ID，用于标识目标会话
   * @param cmdId 命令ID，指定要执行的命令类型
   * @param payload 指向命令负载数据的指针，可为nullptr如果len为0
   * @param len 命令负载数据的长度（字节数）
   * 
   * @return 如果请求发送成功返回true，否则返回false
   * 
   * @note 调用者负责确保payload指针有效且len与实际数据长度匹配
   */
  bool sendRequest(uint16_t session, uint16_t cmdId, const uint8_t *payload, size_t len);

  /**
   * @brief 回复命令请求
   * 
   * @param session 会话ID，用于标识本次通信会话
   * @param cmdId 命令ID，指示要回复的特定命令类型
   * @param code 响应代码，表示执行结果状态（如成功、失败等）
   * @param payload 指向响应数据的指针，包含返回给请求者的具体数据内容
   * @param len 响应数据的长度（字节数）
   * 
   * @return bool 返回操作是否成功，true表示回复发送成功，false表示失败
   */
  bool reply(uint16_t session, uint16_t cmdId, uint16_t code, const uint8_t *payload, size_t len) override;

  /**
   * @brief 发布事件到事件系统
   * 
   * 将指定的事件ID和负载数据发布到事件系统中，触发相应的事件处理。
   * 
   * @param eventId 事件的唯一标识符，范围为 0 到 65535
   * @param payload 指向事件负载数据的指针，包含要随事件传递的数据
   * @param len 负载数据的长度（字节数）
   * 
   * @return bool 发布是否成功
   *   - true 事件发布成功
   *   - false 事件发布失败（可能原因：内存不足、无效的事件ID等）
   */
  bool publish(uint16_t eventId, const uint8_t *payload, size_t len) override;

  /**
   * @brief 设置浏览器会话连接状态
   *
   * @param connected true 表示浏览器已连接，false 表示浏览器已断开
   */
  void setBrowserConnected(bool connected);

  /**
   * @brief 获取浏览器会话连接状态
   * @return true 浏览器已连接
   * @return false 浏览器未连接
   */
  bool isBrowserConnected() const;

  /**
   * @brief 开始图像上传会话
   */
  Image::Result imageBegin(const Image::UploadMeta &meta);

  /**
   * @brief 追加图像分片
   */
  Image::Result imageAppendChunk(uint32_t chunk_offset, const uint8_t *data, size_t len);

  /**
   * @brief 结束图像上传会话
   */
  Image::Result imageEnd();

  /**
   * @brief 应用最新图像
   */
  Image::Result imageApply();

  /**
   * @brief 中止图像上传会话
   */
  Image::Result imageAbort();

private:
  class ImageStorePortAdapter : public Image::IStorePort
  {
  public:

    /**
     * @brief ImageStorePortAdapter 的构造函数
     * 
     * @param storage 指向 Storage::IImageStorage 接口的指针，用于执行图像存储操作
     * 
     * @details 该构造函数初始化 ImageStorePortAdapter 对象，并接收一个存储接口的实现指针。
     *          通过依赖注入模式，将具体的存储行为与适配器解耦。
     */
    explicit ImageStorePortAdapter(Storage::IImageStorage *storage);

    /**
     * @brief 开始写入图像数据
     * 
     * @param meta 图像上传的元数据，包含图像信息和配置参数
     * 
     * @return Image::Result 操作结果，表示写入操作是否成功
     * 
     * @note 此方法为虚函数，应由派生类实现具体的写入逻辑
     */
    Image::Result begin_write(const Image::UploadMeta &meta) override;

    /**
     * @brief 写入数据块到指定偏移位置
     * 
     * @param offset 写入操作的起始偏移位置（字节单位）
     * @param data 指向待写入数据的指针
     * @param len 待写入数据的长度（字节单位）
     * 
     * @return Image::Result 写入操作的结果状态
     */
    Image::Result write_chunk(uint32_t offset, const uint8_t *data, size_t len) override;

    /**
     * @brief 提交当前的操作或更改
     * 
     * 将待处理的更改应用到图像或设备中。此方法用于确认并执行
     * 之前进行的所有操作。
     * 
     * @return Image::Result 操作的结果状态，表示提交是否成功
     * @retval Image::Result::Success 提交操作成功
     * @retval Image::Result::Error 提交操作失败
     * 
     * @note 此方法必须由派生类实现
     */
    Image::Result commit() override;

    /**
     * @brief 中止写入操作
     * 
     * 该函数用于中止当前进行的写入操作。当需要停止向存储设备或
     * 缓冲区写入数据时调用此函数。
     * 
     * @return Image::Result 返回操作结果，表示中止写入是否成功。
     *                       成功时返回相应的成功状态码，
     *                       失败时返回相应的错误状态码。
     */
    Image::Result abort_write() override;

  private:
    static Image::ErrorCode toImageCode_(Storage::ErrorCode code);

  private:
    Storage::IImageStorage *storage_ = nullptr;
  };

  /// @brief 传输层指针
  /// @details 用于管理与设备的通信连接，负责数据的发送和接收
  Transport *transport_ = nullptr;

  /// @brief 编解码器指针
  /// @details 用于处理数据的编码和解码操作，默认为空指针
  Codec *codec_ = nullptr;

  /**
   * @brief 路由器指针
   * @details 用于管理和处理命令的路由，初始状态为空指针
   */
  Router *router_ = nullptr;

  /// @brief 浏览器连接状态标志
  /// @details 用于追踪浏览器是否已连接到设备
  /// @note 当浏览器成功建立连接时设置为 true，断开连接时设置为 false
  bool browser_connected_ = false;

  /**
   * @brief 内存图像存储对象
   * 
   * 用于存储和管理电子纸屏幕显示的图像数据。
   * 该成员变量负责维护图像在内存中的缓存，
   * 支持图像的读取、写入和更新操作。
   */
  Storage::MemoryImageStorage image_storage_;

  /// @brief 图像存储端口适配器
  /// 
  /// 用于适配和管理图像存储功能的端口适配器。负责与图像存储服务的通信和数据交互。
  ImageStorePortAdapter image_store_port_;

  /**
   * @brief 图像服务实例
   * 
   * 用于处理图像相关的业务逻辑，包括图像加载、转换、显示等操作。
   * 该服务实例负责管理应用程序中所有与图像相关的功能。
   */
  Image::Service image_service_;

  /**
   * @brief 发送数据包
   * 
   * 将指定的数据包发送到目标设备。此方法为私有方法，
   * 主要用于内部通信协议的实现。
   * 
   * @param packet 要发送的数据包引用，包含完整的数据和头信息
   * 
   * @return bool 如果数据包发送成功返回 true，否则返回 false
   * 
   * @note 这是一个私有方法，不应在类外直接调用
   * @note 发送前请确保相关的通信接口已正确初始化
   * 
   * @see Packet
   */
  bool sendPacket_(const Packet &packet);
};