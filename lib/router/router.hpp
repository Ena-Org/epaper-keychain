#pragma once
#include <stddef.h>
#include <stdint.h>
#include <functional>
#include <vector>
#include "protocol_packet.hpp"

class Router
{
public:
  /**
   * @brief 数据包类型的类型别名
   * 
   * 将 Protocol::PacketType 定义为 PacketType，用于简化代码中对数据包类型的引用。
   * 这个类型别名提供了一个更简洁的方式来访问协议中定义的数据包类型枚举。
   */
  using PacketType = Protocol::PacketType;

  /**
   * @brief 数据包类型别名
   * 
   * 将 Protocol::Packet 类型定义为别名 Packet，用于简化代码中的类型声明。
   * 该别名表示在协议中定义的数据包数据结构，用于网络通信中的数据传输。
   */
  using Packet = Protocol::Packet;

  class Context
  {
  public:
    /// @brief 虚析构函数
    /// @details 提供多态删除支持，确保派生类对象正确释放资源
    virtual ~Context() = default;

    /**
     * @brief 发送命令回复
     * @param session 会话ID，用于标识该回复对应的请求
     * @param cmdId 命令ID，标识回复的命令类型
     * @param code 回复状态码，0表示成功，非0表示错误
     * @param payload 指向回复数据的指针，如果无数据可为nullptr
     * @param len 回复数据的长度，单位为字节
     * @return true 表示回复发送成功，false 表示发送失败
     */
    virtual bool reply(uint16_t session, uint16_t cmdId, uint16_t code, const uint8_t *payload, size_t len) = 0;

    /**
     * @brief 发布事件到路由系统
     * 
     * @param eventId 事件的唯一标识符，用于区分不同类型的事件
     * @param payload 指向事件负载数据的指针，包含事件相关的具体信息
     * @param len 事件负载数据的长度（字节数）
     * 
     * @return true 事件发布成功
     * @return false 事件发布失败
     * 
     * @note 此为纯虚函数，需要由子类提供具体实现
     * @note payload 指针的生命周期应由调用者管理，确保数据有效性
     */
    virtual bool publish(uint16_t eventId, const uint8_t *payload, size_t len) = 0;
  };

  /**
   * @brief 数据包处理函数的类型定义
   * 
   * 这是一个函数指针类型,用于定义处理网络数据包的回调函数签名。
   * 处理函数接收一个常量数据包引用和一个可修改的上下文引用作为参数,
   * 并且不返回任何值。
   * 
   * @param packet 常量数据包引用,包含待处理的网络数据
   * @param context 可修改的上下文引用,用于存储和传递处理状态信息
   * 
   * @note 该Handler通常用于路由系统中注册特定消息类型的处理函数
   */
  using Handler = std::function<void(const Packet &, Context &)>;

public:
  /**
   * @brief Router 默认构造函数
   * 
   * 使用编译器生成的默认构造函数初始化 Router 对象。
   * 该构造函数不执行任何显式初始化操作，所有成员变量将使用其默认值进行初始化。
   */
  Router() = default;

  /**
   * @brief 注册命令处理器
   * 
   * 将指定的命令ID与其对应的处理器函数进行关联，以便在接收到该命令时
   * 能够调用相应的处理器进行处理。
   * 
   * @param cmdId 命令的唯一标识符，用于区分不同的命令类型
   * @param handler 与命令ID关联的处理器函数，用于处理该命令的业务逻辑
   * 
   * @return bool 处理器注册是否成功
   *             - true: 表示注册成功
   *             - false: 表示注册失败（可能是命令ID已被注册或其他错误原因）
   * 
   * @note 如果使用相同的cmdId多次调用此函数，后续的注册可能会覆盖
   *       之前的处理器，具体行为取决于实现细节。
   */
  bool registerHandler(uint16_t cmdId, Handler handler);

  /**
   * @brief 注销指定命令ID的处理器
   * @param cmdId 要注销的命令ID
   * @return 如果注销成功返回true，如果指定的命令ID不存在返回false
   */
  bool unregisterHandler(uint16_t cmdId);

  /**
   * @brief 清除所有已注册的路由处理器
   * 
   * 该函数会移除路由器中的所有处理器，使其恢复到初始状态。
   * 调用此方法后，之前注册的所有路由将失效，需要重新注册才能使用。
   * 
   * @note 此操作不可撤销，请谨慎调用
   * 
   * @see registerHandler()
   */
  void clearHandlers();

  /**
   * @brief 检查是否存在指定命令ID的处理器
   * @param cmdId 命令ID，用于标识特定的处理请求
   * @return bool 如果存在该命令ID对应的处理器则返回true，否则返回false
   */
  bool hasHandler(uint16_t cmdId) const;

  /**
   * @brief 获取当前路由器中注册的处理器数量
   * 
   * @return size_t 处理器的总数
   */
  size_t handlerCount() const;

  /**
   * @brief 分发数据包到相应的处理器
   * 
   * @param packet 待分发的数据包
   * @param ctx 处理上下文，包含分发过程所需的必要信息
   * 
   * @return bool 如果成功分发数据包则返回 true，否则返回 false
   */
  bool dispatch(const Packet &packet, Context &ctx) const;

private:
  /**
   * @brief 路由表条目结构体
   * 
   * 用于记录命令ID与其对应的处理函数的映射关系。
   * 
   * @member cmdId 命令ID，用于标识不同的路由请求，范围为0-65535
   * @member handler 处理函数对象，用于处理对应命令ID的请求
   */
  struct RouteEntry
  {
    uint16_t cmdId = 0;
    Handler handler{};
  };

  /// @brief 存储所有路由条目的容器
  /// @details 该向量用于管理应用中的所有路由映射关系，
  ///          每个RouteEntry包含路由规则和对应的处理器。
  ///          路由条目的顺序决定了匹配的优先级。
  std::vector<RouteEntry> routes_{};
};