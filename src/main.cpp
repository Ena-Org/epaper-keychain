#include <Arduino.h>
#include "pins.hpp"
#include <led.hpp>

// using namespace Board;

// Led led(pins.led, pins.ledActiveHigh);

void setup()
{
  // Serial.begin(115200);
  // delay(300);

  // pinMode(pins.led, OUTPUT);
  // digitalWrite(pins.led, pins.ledActiveHigh ? HIGH : LOW);

  // 如果板载 NeoPixel 可用，则点亮为绿色
  // #if (NEOPIXEL_PIN >= 0)
  //   neopixelWrite(NEOPIXEL_PIN, 0, 255, 0);
  // #endif
  pinMode(USER_LED_PIN, OUTPUT);
  digitalWrite(USER_LED_PIN, LOW);
}

void loop()
{
  // led.toggle();
  // delay(500);
}
