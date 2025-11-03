#pragma once
#include <Arduino.h>

class Led {
 public:
  explicit Led(uint8_t pin, bool activeHigh = true);
  void begin();
  void on();
  void off();
  void toggle();

 private:
  uint8_t _pin;
  bool _activeHigh;
  bool _state = false;
};
