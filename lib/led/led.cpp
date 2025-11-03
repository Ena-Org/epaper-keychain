#include "led.hpp"

Led::Led(uint8_t pin, bool activeHigh) : _pin(pin), _activeHigh(activeHigh) {}
void Led::begin() { pinMode(_pin, OUTPUT); off(); }
void Led::on()    { digitalWrite(_pin, _activeHigh ? HIGH : LOW);  _state = true; }
void Led::off()   { digitalWrite(_pin, _activeHigh ? LOW  : HIGH); _state = false; }
void Led::toggle(){ _state ? off() : on(); }
