#include "cmd_handlers.hpp"

#include "cmd_handlers_registry.hpp"

namespace CmdHandlers
{
  void registerDefaultHandlers(Router &router, Codec *codec)
  {
    registerInfoHandlers(router);
    registerLogHandlers(router, codec);
  }
}
