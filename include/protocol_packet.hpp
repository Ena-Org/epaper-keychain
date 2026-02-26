#pragma once
#include <stdint.h>
#include <vector>

namespace Protocol
{
  enum class PacketType : uint8_t
  {
    Request = 1,
    Response = 2,
    Event = 3,
  };

  struct Packet
  {
    PacketType type = PacketType::Request;
    uint16_t session = 0;
    uint16_t cmdId = 0;
    uint16_t code = 0;
    std::vector<uint8_t> payload{};
  };
}
