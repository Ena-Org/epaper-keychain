#pragma once

#include "codec.hpp"
#include "router.hpp"

namespace CmdHandlers
{
  void registerInfoHandlers(Router &router);
  void registerLogHandlers(Router &router, Codec *codec);
}
