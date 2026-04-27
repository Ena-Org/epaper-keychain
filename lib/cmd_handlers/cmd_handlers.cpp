#include "cmd_handlers.hpp"

#include "cmd_handlers_registry.hpp"

namespace CmdHandlers
{
  /**
   * @brief 注册默认的命令处理程序
   * 
   * 将应用程序所需的所有默认命令处理程序注册到路由器中。
   * 包括信息处理程序和日志处理程序的注册。
   * 
   * @param router 路由器引用，用于注册命令处理程序
   * @param codec 编解码器指针，用于处理数据的编码和解码
   * 
   * @return void
   */
  void registerDefaultHandlers(Router &router, Codec *codec)
  {
    registerInfoHandlers(router);
    registerBrowserHandlers(router);
    registerLogHandlers(router, codec);
    registerImageHandlers(router);
  }
}
