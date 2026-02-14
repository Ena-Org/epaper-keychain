#pragma once
#include <Arduino.h>
#include "transport_usb.hpp"
#include "uploader/uploader.hpp"

class CmdRouter
{
public:
  /**
   * @brief 初始化命令路由器
   * 
   * 该函数用于初始化命令路由器的相关资源和状态。
   * 应在使用命令路由器的其他功能之前调用此函数。
   * 
   * @return void
   * 
   * @note 此函数应该只被调用一次，通常在程序启动时调用。
   */
  void begin();

  /**
   * @brief 命令路由器的主循环函数
   * 
   * 该函数处理通过USB CDC传输接收到的命令，并根据命令类型进行相应的路由处理。
   * 同时管理上传会话的状态。
   * 
   * @param io USB CDC传输接口引用，用于与主机进行数据交互
   * @param up 上传会话引用，用于管理文件上传的状态和进度
   * 
   * @return void
   * 
   * @note 该函数应该在主循环中被持续调用，以保持命令接收和处理的响应性
   * @note 函数定义应该在对应的.cpp文件中实现
   */
  void loop(UsbCdcTransport &io, Uploader::UploadSession &up);

private:
  /**
   * @brief 处理单行命令输入
   * 
   * @param line 待处理的命令行内容
   * @param io USB CDC 传输接口，用于与设备进行通信
   * @param up 上传会话对象，管理文件上传的状态和数据
   * 
   * @details 该函数解析并执行用户输入的单行命令，根据命令类型
   *          调用相应的处理逻辑，并通过提供的 USB CDC 接口返回
   *          执行结果。上传会话对象用于处理与文件上传相关的命令。
   */
  void handleLine(const String &line, UsbCdcTransport &io, Uploader::UploadSession &up);

  /**
   * @brief 发送成功响应消息
   * 
   * 通过USB CDC传输接口向主机发送操作成功的回复消息。
   * 
   * @param io USB CDC传输接口引用，用于发送数据
   * @param msg 要发送的成功响应消息内容
   * 
   * @return void
   * 
   * @note 此函数用于命令执行成功后的应答，消息会通过USB CDC协议发送给连接的主机
   */
  void replyOk(UsbCdcTransport &io, const String &msg);

  /**
   * @brief 通过USB CDC传输接口发送错误消息回复
   * 
   * 该函数用于向客户端发送标准的错误响应。错误消息将通过
   * 指定的USB CDC传输接口发送回主机。
   * 
   * @param io USB CDC传输接口对象，用于发送错误消息
   * @param msg 错误消息内容，将被发送给客户端
   * 
   * @return void
   * 
   * @note 该函数应在命令处理过程中发生错误时调用
   * 
   * @see replyOk
   */
  void replyErr(UsbCdcTransport &io, const String &msg);
};
