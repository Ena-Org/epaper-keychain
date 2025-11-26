#include "led.hpp"
#include "pins.hpp"

void Led::init()
{
    pinMode(USER_LED_PIN, OUTPUT);
    digitalWrite(USER_LED_PIN, HIGH);
}
void Led::on() { digitalWrite(USER_LED_PIN, LOW); }
void Led::off() { digitalWrite(USER_LED_PIN, HIGH); }
void Led::loop()
{
    static unsigned long lastToggle = 0;
    static bool isOn = false;
    unsigned long now = millis();
    if (now - lastToggle >= 1000)
    {
        lastToggle = now;
        if (isOn)
            off();
        else
            on();

        isOn = !isOn;
    }
}