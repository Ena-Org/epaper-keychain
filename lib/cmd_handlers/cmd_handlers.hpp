#pragma once

#include "codec.hpp"
#include "router.hpp"

namespace CmdHandlers
{
  /**
   * @brief 注册默认的命令处理程序到路由器中
   * 
   * 该函数将一组预定义的默认命令处理程序注册到提供的路由器实例中。
   * 这些处理程序将使用提供的编解码器来处理命令的编码和解码操作。
   * 
   * @param router 路由器实例的引用，用于注册命令处理程序
   * @param codec 指向编解码器对象的指针，用于处理数据的编码和解码
   * 
   * @note 调用此函数前，router 和 codec 必须已正确初始化
   * @note 该函数不返回值，任何错误应通过 router 或 codec 的内部机制处理
   * 
   * @see Router
   * @see Codec
   */
  void registerDefaultHandlers(Router &router, Codec *codec);
}
