#include "rgb.hpp"
#include "pins.hpp"

bool Rgb::RGB_COMMON_ANODE = false;

void Rgb::setColor(uint8_t r, uint8_t g, uint8_t b)
{
  if (RGB_COMMON_ANODE)
  {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }

  analogWrite(RGB_R_PIN, r);
  analogWrite(RGB_G_PIN, g);
  analogWrite(RGB_B_PIN, b);
}

void Rgb::init()
{
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);

  digitalWrite(RGB_R_PIN, RGB_COMMON_ANODE ? HIGH : LOW);
  digitalWrite(RGB_G_PIN, RGB_COMMON_ANODE ? HIGH : LOW);
  digitalWrite(RGB_B_PIN, RGB_COMMON_ANODE ? HIGH : LOW);
}

void setRgbDigital(bool r, bool g, bool b)
{
  if (Rgb::RGB_COMMON_ANODE)
  {
    digitalWrite(RGB_R_PIN, r ? LOW : HIGH);
    digitalWrite(RGB_G_PIN, g ? LOW : HIGH);
    digitalWrite(RGB_B_PIN, b ? LOW : HIGH);
  }
  else
  {
    digitalWrite(RGB_R_PIN, r ? HIGH : LOW);
    digitalWrite(RGB_G_PIN, g ? HIGH : LOW);
    digitalWrite(RGB_B_PIN, b ? HIGH : LOW);
  }
}

void Rgb::tick()
{

  setRgbDigital(true, false, false);
  delay(1000);

  setRgbDigital(false, true, false);
  delay(1000);

  setRgbDigital(false, false, true);
  delay(1000);

  setRgbDigital(true, true, true);
  delay(1000);
}