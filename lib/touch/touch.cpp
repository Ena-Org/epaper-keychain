#include "touch.hpp"
#include "pins.hpp"

bool Touch::TOUCH_ACTIVE_HIGH = false;

void Touch::init()
{
    Serial.begin(115200);
    pinMode(TOUCH_PIN, INPUT);

    pinMode(21, OUTPUT);
    digitalWrite(21, LOW);
}

void Touch::loop()
{
    bool raw = digitalRead(TOUCH_PIN);
    bool touched = TOUCH_ACTIVE_HIGH ? raw : !raw;

    if (touched)
    {
        Serial.println("Touched!");
    }
    else
    {
        Serial.println("Not touched");
    }

    // 触摸就点亮板载 LED
    digitalWrite(21, touched ? LOW : HIGH);

    delay(50); // 简单防抖 + 不要刷屏太快
}