
#include "pins.hpp"
#include "rgb.hpp"
#include "touch.hpp"
#include "touch_rgb.hpp"

static Rgb rgb;
static Touch touch;

int TouchRGBFeature::CH_R = 0;
int TouchRGBFeature::CH_G = 1;
int TouchRGBFeature::CH_B = 2;

int TouchRGBFeature::PWM_FREQ = 5000;
int TouchRGBFeature::PWM_RES = 8; // 0-255

bool TouchRGBFeature::ledOn = false;
bool TouchRGBFeature::lastTouched = false;

void TouchRGBFeature::init()
{
    Serial.begin(115200);

    ledcSetup(CH_R, PWM_FREQ, PWM_RES);
    ledcSetup(CH_G, PWM_FREQ, PWM_RES);
    ledcSetup(CH_B, PWM_FREQ, PWM_RES);

    ledcAttachPin(RGB_R_PIN, CH_R);
    ledcAttachPin(RGB_G_PIN, CH_G);
    ledcAttachPin(RGB_B_PIN, CH_B);

    rgb.setColor(0, 0, 0);
}

void TouchRGBFeature::begin()
{
    bool raw = digitalRead(TOUCH_PIN);
    bool touched = Touch::TOUCH_ACTIVE_HIGH ? raw : !raw;

    if (touched && !lastTouched)
    {
        ledOn = !ledOn;
        Serial.print("Toggle, ledOn = ");
        Serial.println(ledOn ? "true" : "false");

        if (ledOn)
        {
            Serial.println("Set color to (255, 0, 128)");
            rgb.setColor(255, 0, 128);
        }
        else
        {
            Serial.println("Set color to (0, 0, 0)");
            rgb.setColor(0, 0, 0);
        }
    }

    lastTouched = touched;
    delay(10); // 简单防抖
}