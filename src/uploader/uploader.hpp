#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include "epf_protocol.hpp"
#include "storage.hpp"
#include "transport_usb.hpp"

class Uploader
{
public:
  /**
   * @enum State
   * @brief 上传器的状态枚举
   *
   * 定义了文件上传过程中的各个状态阶段。
   *
   * @var State::Idle
   *      空闲状态，等待接收文件
   *
   * @var State::Receiving
   *      接收状态，正在接收文件数据
   *
   * @var State::Verifying
   *      验证状态，正在验证接收到的文件
   *
   * @var State::Done
   *      完成状态，文件上传和验证成功
   *
   * @var State::Error
   *      错误状态，上传或验证过程中发生错误
   */
  enum class State : uint8_t
  {
    Idle,
    Receiving,
    Verifying,
    Done,
    Error,
  };

  /**
   * @enum ErrorCode
   * @brief 文件上传操作的错误代码枚举
   *
   * @var ErrorCode::None
   * 无错误，操作成功
   *
   * @var ErrorCode::Busy
   * 设备忙，无法处理当前请求
   *
   * @var ErrorCode::BadArgs
   * 无效的参数或参数格式错误
   *
   * @var ErrorCode::NoSpace
   * 存储空间不足，无法完成操作
   *
   * @var ErrorCode::OpenFail
   * 打开文件失败
   *
   * @var ErrorCode::WriteFail
   * 写入文件失败
   *
   * @var ErrorCode::CrcMismatch
   * CRC校验不匹配，数据传输可能出错
   *
   * @var ErrorCode::SizeMismatch
   * 文件大小不匹配，上传的数据大小与预期不符
   *
   * @var ErrorCode::BadSession
   * 无效的会话，会话已过期或不存在
   */
  enum class ErrorCode : uint32_t
  {
    None = 0,
    Busy = 1,
    BadArgs = 2,
    NoSpace = 3,
    OpenFail = 4,
    WriteFail = 5,
    CrcMismatch = 6,
    SizeMismatch = 7,
    BadSession = 8,
  };

  /**
   * @struct PutRequest
   * @brief 文件上传请求的数据结构
   *
   * 用于表示一个文件上传请求，包含文件路径、大小和校验信息。
   *
   * @member path 文件在目标位置的路径
   * @member size 文件的大小，单位为字节，默认值为0
   * @member crc32 文件的CRC32校验码，用于验证文件完整性，默认值为0
   */
  struct PutRequest
  {
    String path;
    uint32_t size = 0;
    uint32_t crc32 = 0;
  };

  /**
   * @struct PutResponse
   * @brief 上传PUT请求的响应结构体
   *
   * 用于表示文件上传操作的响应数据，包含会话标识和数据块信息。
   *
   * @member session 会话ID，用于标识一次上传会话，默认值为0
   * @member chunk 数据块大小或块编号，默认值为1024字节
   */
  struct PutResponse
  {
    uint16_t session = 0;
    uint16_t chunk = 1024;
  };

  /**
   * @struct Stats
   * @brief 用于跟踪上传操作的统计信息
   *
   * 该结构体记录了文件上传过程中的关键指标,包括已接收的字节数
   * 和已确认的数据块数量。
   *
   * @member receivedBytes 已接收的字节总数，初始值为0
   * @member ackedChunks 已被确认的数据块数量，初始值为0
   */
  struct Stats
  {
    uint32_t receivedBytes = 0;
    uint32_t ackedChunks = 0;
  };

  class UploadSession
  {
  public:
    /**
     * @brief UploadSession 类的构造函数
     * 
     * 初始化一个上传会话对象。该构造函数负责设置上传会话所需的初始状态和资源。
     * 
     * @details
     * 该构造函数用于创建一个新的上传会话实例。通过此构造函数创建的对象
     * 可以用于管理文件上传的相关操作和状态维护。
     * 
     * @note
     * 确保在使用上传会话前已正确初始化所有必要的资源。
     * 
     * @see UploadSession 类的其他成员函数
     */
    UploadSession();

    /**
     * @brief 检查上传器是否处于活跃状态
     * 
     * @return true 如果上传器当前处于活跃状态，false 否则
     */
    bool isActive() const;

    /**
     * @brief 获取当前上传器的状态
     * 
     * @return State 返回上传器的当前状态
     */
    State state() const;

    /**
     * @brief 获取最后发生的错误代码
     * 
     * 返回上传器执行过程中最后发生的错误代码。
     * 可用于诊断和处理上传操作中遇到的问题。
     * 
     * @return ErrorCode 最后发生的错误代码
     * 
     * @note 如果没有发生错误，返回值应为成功状态码
     */
    ErrorCode lastError() const;

    /**
     * @brief 获取上传器的统计信息
     * 
     * @return Stats 返回包含上传操作统计数据的Stats对象，
     *         包括已上传字节数、上传次数、错误数等信息
     */
    Stats stats() const;

    /**
     * @brief 启动文件上传请求
     * 
     * 该函数处理一个 PUT 请求，开始上传操作。
     * 
     * @param req 常量引用，包含上传请求的详细信息（如文件路径、内容等）
     * @param out 引用，用于存储上传响应的结果（如状态码、错误信息等）
     * 
     * @return bool 返回上传操作是否成功
     *         - true: 上传请求已成功启动
     *         - false: 上传请求启动失败
     * 
     * @note 调用者应检查返回值以确定上传操作的状态
     */
    bool start(const PutRequest &req, PutResponse &out);

    /**
     * @brief 中止当前的上传操作
     * 
     * 此函数用于终止正在进行的文件上传过程。调用此函数后，
     * 上传器将停止当前的上传任务，并释放相关资源。
     * 
     * @note 中止操作可能是异步的，调用此函数不保证立即停止上传。
     * 
     * @return void
     */
    void abort();

    /**
     * @brief 处理接收到的EPF协议帧
     * 
     * 当USB CDC传输接收到新的协议帧时调用此函数。该函数负责解析和处理
     * 来自EPF协议的帧数据，并通过提供的传输接口进行相应的响应或操作。
     * 
     * @param frame 接收到的EPF协议帧视图，包含帧的所有信息
     * @param io USB CDC传输接口，用于发送响应或输出数据
     * 
     * @return void
     * 
     * @note 此函数可能在中断或事件驱动的上下文中被调用
     */
    void onFrame(const EpfProtocol::FrameView &frame, UsbCdcTransport &io);

    /**
     * @brief 轮询 USB CDC 传输设备
     * 
     * 通过给定的 USB CDC 传输接口进行轮询操作。该方法用于检查和处理
     * 来自 USB 设备的数据或状态更新。
     * 
     * @param io USB CDC 传输接口引用，用于与设备进行通信
     * 
     * @return 无返回值
     */
    void poll(UsbCdcTransport &io);

    /**
     * @brief 检查当前是否处于二进制模式
     * @return true 如果处于二进制模式，false 否则
     */
    bool inBinaryMode() const;
  };
};
