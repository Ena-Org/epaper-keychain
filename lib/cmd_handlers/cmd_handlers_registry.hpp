#pragma once

#include "codec.hpp"
#include "router.hpp"

namespace CmdHandlers
{
  /**
   * @brief 注册信息处理器到路由器
   * 
   * 该函数将所有与信息查询相关的命令处理器注册到指定的路由器中。
   * 注册后，路由器可以根据不同的请求类型将其分发到对应的处理程序。
   * 
   * @param router 路由器引用，用于注册处理器
   * 
   * @return void
   * 
   * @note 这个函数应该在应用程序初始化阶段被调用
   * 
   * @see Router
   */
  void registerInfoHandlers(Router &router);

  /**
   * @brief 注册浏览器连接相关处理器
   *
   * @param router 路由器引用
   */
  void registerBrowserHandlers(Router &router);

  /**
   * @brief 注册日志处理器到路由器
   * 
   * 将日志相关的命令处理器注册到指定的路由器中，使路由器能够
   * 处理与日志操作相关的命令请求。
   * 
   * @param router 路由器引用，用于注册处理器
   * @param codec 编解码器指针，用于处理命令编码和解码
   * 
   * @note 此函数应在应用程序初始化阶段调用
   * @see Router, Codec
   */
  void registerLogHandlers(Router &router, Codec *codec);

  /**
   * @brief 注册图像上传相关处理器
   *
   * @param router 路由器引用
   */
  void registerImageHandlers(Router &router);
}
