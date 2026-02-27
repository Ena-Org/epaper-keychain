#pragma once
#include <stdint.h>

namespace CmdId
{
  static constexpr uint16_t Info = 0x0001;
  static constexpr uint16_t LogHistory = 0x0101;
  static constexpr uint16_t LogLevelSet = 0x0102;
  static constexpr uint16_t LogClear = 0x0103;
}