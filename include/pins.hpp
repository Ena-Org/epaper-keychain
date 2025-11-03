// include/pins.hpp
#pragma once
#include <Arduino.h>

/*
  引脚策略说明（别跳过，省你一晚抓狂）：
  1) 不瞎猜：不同开发板的 LED、按键、I2C/SPI 脚法不统一。默认给 -1 或保守值，强制你在 build_flags 里覆盖。
  2) 可覆盖：所有引脚都支持用 -DXXX=... 覆盖，仓库不绑死某一块板。
  3) 自检：编译期 static_assert + #pragma message，提醒你哪些还没配。
  4) 规避雷区：ESP32-S3 的 GPIO0/3/45/46 有启动/输入限制，尽量别拿去做输出。
*/

namespace Board {

// 如果没在 build_flags 里定义，就给个占位默认。你应该覆盖它们。
#ifndef LED_PIN
  // 很多板根本没有“普通 LED”，只有 RGB（WS2812）灯，那就别用这个 PIN 点灯，改用 NeoPixel 库。
  #define LED_PIN 2
#endif

#ifndef LED_ACTIVE_HIGH
  #define LED_ACTIVE_HIGH 1
#endif

#ifndef BTN_PIN
  // 没有按键就保持 -1
  #define BTN_PIN -1
#endif

#ifndef I2C_SDA
  #define I2C_SDA -1
#endif

#ifndef I2C_SCL
  #define I2C_SCL -1
#endif

#ifndef SPI_MOSI
  #define SPI_MOSI -1
#endif
#ifndef SPI_MISO
  #define SPI_MISO -1
#endif
#ifndef SPI_SCK
  #define SPI_SCK  -1
#endif
#ifndef SPI_CS
  #define SPI_CS   -1
#endif

#ifndef UART_TX
  #define UART_TX -1
#endif
#ifndef UART_RX
  #define UART_RX -1
#endif

// 统一的引脚包，主程序只依赖这一份，不关心板子差异
struct Pins {
  int8_t  led        = LED_PIN;
  bool    ledActiveHigh = (LED_ACTIVE_HIGH != 0);
  int8_t  button     = BTN_PIN;
  int8_t  i2cSda     = I2C_SDA;
  int8_t  i2cScl     = I2C_SCL;
  int8_t  spiMosi    = SPI_MOSI;
  int8_t  spiMiso    = SPI_MISO;
  int8_t  spiSck     = SPI_SCK;
  int8_t  spiCs      = SPI_CS;
  int8_t  uartTx     = UART_TX;
  int8_t  uartRx     = UART_RX;
};

// 全局常量实例，直接 #include 使用
static constexpr Pins pins{};

// ====== 编译期自检与提醒 ======
static_assert(!(pins.led == 0 || pins.led == 3 || pins.led == 45 || pins.led == 46),
              "LED_PIN 落在 ESP32-S3 启动/输入限制脚(0/3/45/46)，换一个。");

#if (LED_PIN == 2)
  // 只是提醒：很多 S3 板子并没有 GPIO2 上的普通 LED
  #pragma message("提示：LED_PIN 仍是默认 2。若你的板是 RGB 灯，请改用 NeoPixel，而不是普通数字输出。")
#endif

#if (I2C_SDA < 0 || I2C_SCL < 0)
  #pragma message("提示：I2C 引脚未配置(I2C_SDA/I2C_SCL)。如需 I2C，请在 build_flags 中 -DI2C_SDA=xx -DI2C_SCL=yy")
#endif

#if (SPI_MOSI < 0 || SPI_MISO < 0 || SPI_SCK < 0)
/* SPI 可延后配置，这里只提醒一次 */
  #pragma message("提示：SPI 引脚未配置。")
#endif

#if (UART_TX < 0 || UART_RX < 0)
  #pragma message("提示：额外 UART 引脚未配置。若只用 USB CDC 可忽略。")
#endif

} 
