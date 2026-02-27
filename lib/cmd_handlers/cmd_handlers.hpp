#pragma once

#include "codec.hpp"
#include "router.hpp"

namespace CmdHandlers
{
  void registerDefaultHandlers(Router &router, Codec *codec);
}
