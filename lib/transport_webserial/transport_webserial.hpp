#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

class TransportWebserial
{
public:

  /**
   * @enum Event
   * @brief 传输层事件枚举类型
   * 
   * 用于表示WebSerial传输层的各种事件状态。
   * 
   * @var Connected
   *      设备已连接事件
   * 
   * @var Disconnected
   *      设备已断开连接事件
   * 
   * @var Error
   *      传输错误事件
   */
  enum Event : uint8_t {
    Connected,
    Disconnected,
    Error,
  };

  /**
   * @enum Codec
   * @brief 传输协议编码方式枚举
   * 
   * 定义了WebSerial传输层支持的多种编码/封装方式，用于对数据进行编码和解码处理。
   * 
   * @var Codec::Raw
   * 原始数据模式，不进行任何编码处理，直接传输原始字节数据
   * 
   * @var Codec::Line
   * 行编码模式，以行分隔符（通常为换行符）作为帧边界
   * 
   * @var Codec::COBS
   * 一致性字节填充编码模式(Consistent Overhead Byte Stuffing)，
   * 用于透明传输，避免特定字节被误认为帧标志
   * 
   * @var Codec::SLIP
   * 串行行IP协议编码模式(Serial Line Internet Protocol)，
   * 使用特定的转义机制来标识和分隔数据帧
   */
  enum Codec : uint8_t {
    Raw,
    Line,
    COBS,
    SLIP,
  };

  /**
   * @struct WebSerialConfig
   * @brief WebSerial 传输配置结构体
   * 
   * @details 用于配置 WebSerial 串口通信的参数，包括波特率、编码方式、
   *          延迟符号、接收缓冲区大小等设置。
   * 
   * @member stream 指向 Stream 对象的指针，用于实际的串口通信，默认为 nullptr
   * @member baud 通信波特率，默认为 115200
   * @member codec 数据编码方式，默认为 Line 编码
   * @member line_delim 行分隔符，默认为换行符 '\n'
   * @member rx_max 接收缓冲区最大字节数，默认为 2048
   * @member connect_timeout_ms 连接超时时间（毫秒），默认为 0（无超时）
   */
  struct WebSerialConfig {
    Stream* stream = nullptr;
    uint32_t baud = 115200;
    Codec codec = Codec::Line;
    char line_delim = '\n';
    size_t rx_max = 2048;
    uint32_t connect_timeout_ms = 0;
  };

  /**
   * @typedef MessageCallback
   * @brief 消息回调函数指针类型
   * 
   * 用于处理接收到的数据消息的回调函数类型定义。
   * 当WebSerial传输接收到新消息时，将调用该回调函数。
   * 
   * @param data 指向接收到的消息数据缓冲区的指针。数据为字节数组格式。
   * @param len 接收到的消息数据的长度，单位为字节。
   * @param user 用户自定义的指针，用于在回调时传递上下文信息。
   *            该指针会原样传递给回调函数，允许回调函数访问调用者的私有数据。
   */
  using MessageCallback = void (*)(const uint8_t* data, size_t len, void* user);

  /**
   * @brief 事件回调函数指针类型
   * 
   * 用于处理传输事件的回调函数签名。当发生特定事件时，
   * 系统会调用此类型的回调函数来通知调用者。
   * 
   * @param event 发生的事件类型
   * @param err   错误码，0表示无错误，其他值表示特定的错误情况
   * @param user  用户自定义数据指针，由调用者在注册回调时传入
   * 
   * @note 回调函数应该尽可能简短，避免长时间阻塞
   * @note user 指针的生命周期由调用者负责管理
   */
  using EventCallback = void (*)(Event event, int err, void* user);

  /**
   * @brief 初始化Web串口传输
   * @details 根据提供的配置参数初始化Web串口连接。在使用WebSerialConfig进行任何通信前，必须先调用此方法。
   * @param cfg 包含Web串口配置信息的WebSerialConfig结构体引用，包括波特率、数据位等参数
   * @return bool 初始化是否成功
   *   - true: 初始化成功，Web串口已准备好进行通信
   *   - false: 初始化失败，可能原因包括配置参数无效或硬件不可用
   * @note 多次调用此方法会重新初始化连接。建议在begin返回true后再进行数据收发操作。
   * @see WebSerialConfig
   */
  bool begin(const WebSerialConfig& cfg);

  /**
   * @brief 终止 WebSerial 传输连接
   * 
   * 关闭 WebSerial 传输通道，释放相关资源，停止所有进行中的通信操作。
   * 调用此方法后，传输对象将进入关闭状态。
   * 
   * @note 在重新使用传输对象前，需要重新初始化连接。
   * 
   * @see begin()
   */
  void end();

  /**
   * @brief 主循环函数
   * 
   * 处理WebSerial传输的事件循环，包括接收数据、处理通信状态等。
   * 应该在主程序的循环中定期调用此函数，以确保WebSerial传输的正常运行。
   * 
   * @details
   * 此函数负责：
   * - 检查待发送的数据
   * - 处理接收到的数据
   * - 更新通信状态
   * - 处理超时和错误条件
   * 
   * @note 此函数应该被频繁调用以保证实时性
   * 
   * @see setup()
   */
  void loop();

  /**
   * @brief 检查WebSerial连接状态
   * @details 查询当前WebSerial传输层是否已连接到设备
   * @return true 如果连接已建立，false 如果未连接或连接已断开
   * @note 此为const成员函数，不会修改对象状态
   */
  bool isConnected() const;

  /**
   * @brief 等待连接建立
   * 
   * 阻塞等待WebSerial连接建立。可以指定超时时间，在超时后如果连接未建立则返回false。
   * 
   * @param timeout_ms 等待超时时间，单位为毫秒。如果为0，则无限期等待直到连接建立。
   *                   默认值为0。
   * 
   * @return true 如果连接成功建立
   * @return false 如果在指定的超时时间内连接未建立
   * 
   * @note 该函数是阻塞调用，会一直等待直到连接建立或超时。
   */
  bool waitForConnection(uint32_t timeout_ms = 0);

  /**
   * @brief 获取最后发生的错误代码
   * @return int 返回最后一次操作的错误代码。若无错误，返回0；否则返回具体的错误代码
   * @note 此方法为常量方法，不会修改对象状态
   * @see setError() 用于设置错误代码
   */
  int lastError() const;

  /**
   * @brief 设置消息回调函数
   * 
   * @details 
   * 该函数用于注册一个回调函数，当接收到来自WebSerial的消息时会被触发。
   * 回调函数将在消息到达时被调用，允许应用程序处理接收到的数据。
   * 
   * @param cb 消息回调函数指针，类型为 MessageCallback。
   *           当收到消息时，该函数将被调用以处理消息数据。
   * 
   * @param user 可选的用户自定义数据指针，默认为nullptr。
   *             该指针将被传递给回调函数，用于保存用户特定的上下文信息。
   * 
   * @return void
   * 
   * @note 
   * - 必须在建立WebSerial连接之前调用此函数来注册回调
   * - 同一时间只能注册一个回调函数，再次调用将覆盖之前的设置
   * - 回调函数执行时应避免阻塞操作，以防止影响消息接收
   * 
   * @see MessageCallback
   */
  void setMessageCallback(MessageCallback cb, void* user = nullptr);

  /**
   * @brief 设置事件回调函数
   * 
   * @details 该函数用于注册一个事件回调函数，当WebSerial传输层发生事件时
   *          会调用注册的回调函数来通知应用层。
   * 
   * @param cb 事件回调函数指针，类型为EventCallback。当事件发生时，
   *           此回调函数将被调用
   * @param user 可选的用户自定义数据指针，默认为nullptr。该指针会在
   *             调用回调函数时作为参数传递给cb，允许应用层维护上下文信息
   * 
   * @return void
   * 
   * @note 该函数应在WebSerial初始化后、启动通信前调用
   * @note 用户需要确保cb函数指针有效且在使用期间不被释放
   * @note user指针的生命周期由调用者负责管理
   */
  void setEventCallback(EventCallback cb, void* user = nullptr);

  /**
   * @brief 通过WebSerial传输发送数据
   * 
   * @param data 指向要发送的数据缓冲区的指针
   * @param len 要发送的数据长度（字节数）
   * 
   * @return true 发送成功
   * @return false 发送失败
   * 
   * @note 该函数通过WebSerial协议将指定长度的数据发送到目标设备或服务
   * @warning 确保data指针指向有效的内存地址，且缓冲区大小至少为len字节
   */
  bool send(const uint8_t* data, size_t len);

  /**
   * @brief 通过Web Serial发送数据
   * 
   * 将指定的字符串数据通过Web Serial接口发送到设备。
   * 
   * @param s 指向以null结尾的字符串的指针，包含要发送的数据
   * 
   * @return true 如果数据发送成功
   * @return false 如果数据发送失败或Web Serial连接未建立
   * 
   * @note 该函数是非阻塞的，数据被放入发送队列后立即返回
   * @note 字符串必须以null字符('\0')结尾
   */
  bool send(const char* s);

  /**
   * @brief 通过WebSerial传输发送字符串数据
   * 
   * @param s 要发送的字符串内容
   * 
   * @return bool 发送是否成功
   *             true  - 字符串发送成功
   *             false - 字符串发送失败
   * 
   * @note 该函数为异步操作，返回true仅表示数据已进入发送队列
   */
  bool send(const String& s);

  /**
   * @brief 刷新串行通信缓冲区
   * @details 将发送缓冲区中的所有待发送数据立即发送出去，
   *          确保数据不会被延迟处理。此方法会阻塞直到缓冲区被清空。
   * @return void
   * @note 在进行关键操作或需要确保数据及时发送时应该调用此方法
   */
  void flush();

  /**
   * @brief 向串行端口写入原始数据
   * 
   * 将指定长度的原始字节数据写入到Web Serial连接。
   * 
   * @param data 指向要写入数据的字节数组指针
   * @param len 要写入的数据长度（字节数）
   * 
   * @return 实际写入的字节数。如果写入失败，返回值可能小于 len
   * 
   * @note 该函数为阻塞操作，需要确保串行连接已建立
   * @note 调用者需确保 data 指针有效且至少包含 len 字节的数据
   */
  size_t writeRaw(const uint8_t* data, size_t len);

  /**
   * @brief 获取原始可用数据的字节数
   * @details 返回当前可读的原始数据字节数，不进行任何数据解析或转换
   * @return int 可用的原始字节数，如果无可用数据则返回0
   */
  int availableRaw() const;

  /**
   * @brief 从串行端口读取原始数据
   * 
   * 从WebSerial传输层读取最多max_len字节的原始数据到指定的缓冲区。
   * 
   * @param out 指向输出缓冲区的指针，用于存储读取的数据
   * @param max_len 最多读取的字节数
   * 
   * @return 成功读取的字节数，如果出错返回负值
   * 
   * @note 调用者需要确保out指针指向的缓冲区大小至少为max_len字节
   */
  int readRaw(uint8_t* out, size_t max_len);

  /**
   * @brief 获取接收的字节数
   * 
   * 返回通过 WebSerial 传输接收到的总字节数。
   * 
   * @return uint32_t 接收的字节总数
   */
  uint32_t bytesRx() const;

  /**
   * @brief 获取已发送的字节数
   * 
   * @return uint32_t 通过 WebSerial 传输已发送的字节总数
   */
  uint32_t bytesTx() const;

  /**
   * @brief 重置解析器
   * @details 清除解析器的内部状态，将其恢复到初始状态。
   *          该函数用于清空所有待处理的数据和解析状态，
   *          为下一次解析任务做准备。
   * @return void
   */
  void resetParser();

private:
  /**
   * @brief 处理接收数据的回调函数
   * 
   * 此函数用于处理通过 WebSerial 接收到的数据。
   * 当有新数据到达时，该函数会被调用以进行数据处理。
   * 
   * @note 这是一个私有成员函数，仅在类内部使用。
   * 
   * @see handleTx_()
   */
  void handleRx_();

  /**
   * @brief 将数据帧推送到传输队列
   * 
   * 该函数将指定长度的数据帧添加到内部传输队列中，准备通过Web Serial协议发送。
   * 
   * @param data 指向待发送数据的指针，不能为nullptr
   * @param len 数据的长度，单位为字节
   * 
   * @return bool 如果数据帧成功推送到队列则返回true，否则返回false
   *             常见的失败原因包括：队列已满、数据指针无效等
   * 
   * @note 此函数是私有方法（带下划线后缀），仅供类内部使用
   * @note 调用者需确保data指针指向有效的内存区域，且长度不超过最大帧长度限制
   */
  bool pushFrame_(const uint8_t* data, size_t len);

  /**
   * @brief 对数据进行编码并通过WebSerial写入
   * 
   * 该函数将输入的数据进行编码处理，然后通过WebSerial传输接口写入。
   * 编码过程可能包括序列化、压缩或其他数据转换操作。
   * 
   * @param data 指向待编码数据的指针，不能为nullptr
   * @param len 数据的长度（字节数）
   * 
   * @return true 表示编码和写入操作成功完成
   * @return false 表示编码或写入过程中发生错误
   * 
   * @note 调用者需确保data指针指向的内存有效且包含至少len字节的数据
   * @note 该函数是私有成员函数，仅供类内部使用
   */
  bool encodeAndWrite_(const uint8_t* data, size_t len);

private:
  /**
   * @brief WebSerial 传输配置对象
   * 
   * 用于存储 WebSerial 传输层的配置参数。
   * 该成员变量通过默认构造函数初始化，包含 WebSerial 
   * 连接和通信所需的各项配置信息。
   * 
   * @note 配置对象使用聚合初始化方式创建，所有成员变量
   *       都将被初始化为其默认值。
   * 
   * @see WebSerialConfig
   */
  WebSerialConfig _cfg{};

  /**
   * @brief 消息回调函数指针
   * @details 用于处理接收到的消息的回调函数。当有新消息到达时，
   *          该回调函数会被调用，执行相应的消息处理逻辑。
   * @note 如果未设置回调函数，该指针值为 nullptr
   */
  MessageCallback _onMsg = nullptr;

  /// @brief 用户自定义消息回调函数的指针
  /// @details 存储用户提供的回调函数地址，用于处理Web串口接收到的消息
  /// @note 初始化为nullptr，使用前需确保已赋予有效的函数指针
  /// @see _onMsg
  void* _onMsgUser = nullptr;

  /**
   * @brief 事件回调函数指针
   * @details 用于存储事件处理的回调函数。当特定事件触发时，
   *          此函数指针将被调用以执行相应的事件处理逻辑。
   * @note 初始化为nullptr，使用前需确保已设置有效的回调函数
   */
  EventCallback _onEvt = nullptr;

  /**
   * @brief 用户事件回调函数指针
   * @details 存储用户自定义事件处理的回调函数地址，当相应事件触发时调用此函数指针
   * @note 初始化为 nullptr，需在使用前设置为有效的函数指针
   */
  void* _onEvtUser = nullptr;

  /// @brief 存储上一次发生的错误代码
  /// @details 用于记录最近一次操作中出现的错误信息，便于调试和错误处理
  int _lastErr = 0;

  /// @brief 连接状态标志
  /// @details 表示WebSerial传输层是否已连接到设备。当为true时表示连接已建立，
  ///          当为false时表示未连接或连接已断开。
  bool _connected = false;

  /// @brief 接收的字节数计数器
  /// @details 用于记录通过 WebSerial 传输接收到的总字节数
  uint32_t _rxBytes = 0;

  /**
   * @brief 已发送的字节数
   * 
   * 用于记录通过WebSerial传输已成功发送的总字节数。
   */
  uint32_t _txBytes = 0;

  /**
   * @brief 接收缓冲区指针
   * 
   * 用于存储通过Web Serial接口接收到的数据。
   * 初始化为nullptr，需要在使用前分配内存。
   * 
   * @note 调用者需要负责内存的分配和释放
   */
  uint8_t* _rxBuf = nullptr;

  /// @brief 接收数据的长度
  /// @details 用于记录通过Web Serial传输接收到的数据字节数
  size_t _rxLen = 0;

  /// @brief SLIP 转义状态
  /// @details 为 true 表示上一个字节是 SLIP 转义字节 0xDB
  bool _slipEsc = false;
};