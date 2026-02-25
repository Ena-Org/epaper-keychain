#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

class Transport
{
public:
  enum Event : uint8_t
  {
    Connected,
    Disconnected,
    Error,
  };
};