#pragma once
#include <stdint.h>

namespace CmdId
{
  static constexpr uint16_t Info = 0x0001;

  static constexpr uint16_t BrowserConnect = 0x0002;
  static constexpr uint16_t BrowserDisconnect = 0x0003;
  
  static constexpr uint16_t LogHistory = 0x0101;
  static constexpr uint16_t LogLevelSet = 0x0102;
  static constexpr uint16_t LogClear = 0x0103;

  static constexpr uint16_t ImageBegin = 0x0201;
  static constexpr uint16_t ImageChunk = 0x0202;
  static constexpr uint16_t ImageEnd = 0x0203;
  static constexpr uint16_t ImageApply = 0x0204;
  static constexpr uint16_t ImageAbort = 0x0205;
}