#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "protocol_packet.hpp"
#include "router.hpp"
#include "codec.hpp"
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
   * @details 该函数初始化命令处理系统的各个组件，建立传输层、编解码器和路由器之间的连接。
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

private:
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

  bool sendPacket_(const Packet &packet);
};