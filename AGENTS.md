# Agent Guidelines for epaper-keychain

## Project Overview
ESP32-based e-paper keychain project using PlatformIO framework. Written in C++ with Arduino framework for embedded development.

## Build Commands

### Setup
```bash
# Install Python venv and PlatformIO
make python:install

# Full environment setup (includes OpenCode)
make init-all
```

### Building
```bash
# Build the project
make python:run

# Or directly with PlatformIO
.pio/venv/bin/platformio run

# Build for specific environment
.pio/venv/bin/platformio run -e seeed_xiao_esp32s3
```

### Testing
```bash
# Run all tests
.pio/venv/bin/platformio test

# Run tests for specific environment
.pio/venv/bin/platformio test -e seeed_xiao_esp32s3

# Verbose test output
.pio/venv/bin/platformio test -v
```

### Cleanup
```bash
# Clean build artifacts
.pio/venv/bin/platformio run --target clean

# Remove Python venv
make python:clean

# Full cleanup (venv + OpenCode)
make clean-all
```

## Code Style Guidelines

### Formatting
- **Indentation**: 2 spaces (no tabs)
- **Line endings**: LF
- **Braces**: Allman style (opening brace on new line)
- **Max line length**: ~120 characters

### Naming Conventions
- **Constants**: `kCamelCase` (e.g., `kLogBufferSize`)
- **Classes/Structs**: `PascalCase` (e.g., `LoggerConfig`)
- **Functions**: `snake_case` (e.g., `format_prefix`)
- **Variables**: `snake_case` (e.g., `buffer_size`)
- **Private members**: trailing underscore (e.g., `member_`)
- **Type aliases**: `PascalCase` (e.g., `using Packet = Protocol::Packet`)
- **Macros**: `UPPER_CASE` (e.g., `LOGE`, `LOGI`)

### Includes
```cpp
// System headers first
#include <Arduino.h>
#include <cstdint>
#include <cstddef>
#include <vector>

// Project headers (use quotes)
#include "logger.hpp"
#include "router.hpp"
```

### Documentation
- Use Doxygen-style comments with Chinese language
- Document all public APIs with `@brief`, `@param`, `@return`
- Include implementation notes with `@note` and `@warning`

```cpp
/**
 * @brief Brief description
 * 
 * Detailed description here
 * 
 * @param param1 Description
 * @return Description of return value
 * @note Important notes
 * @warning Warning notes
 */
```

### Types & Constants
- Use `constexpr` for compile-time constants
- Prefer `uint8_t`, `uint16_t`, `uint32_t` over raw types
- Use `enum class` for strongly-typed enums
- Use `nullptr` instead of `NULL`

### Error Handling
- Check for `nullptr` before dereferencing
- Use early returns for error cases
- Log errors using provided macros: `LOGE`, `LOGW`, `LOGI`, `LOGD`, `LOGV`

```cpp
if (ptr == nullptr) {
  LOGE("TAG", "Null pointer encountered");
  return;
}
```

### Memory Management
- Prefer stack allocation for small buffers
- Use `std::vector` for dynamic arrays
- Be mindful of embedded constraints (limited RAM)

### Namespaces
- Use anonymous namespaces for internal linkage
- Avoid `using namespace` directives in headers

```cpp
namespace {
  constexpr size_t kBufferSize = 256;
  
  void internal_function() {
    // implementation
  }
}
```

### Header Guards
- Use `#pragma once` (preferred over include guards)

### Class Design
- Use static methods for singleton-like patterns
- Initialize members at declaration when possible
- Keep public interface minimal

```cpp
class Example {
public:
  static void init(const Config& cfg);
  static void do_something();

private:
  static Config config_;
  static bool initialized_;
};
```

## Project Structure
- `src/` - Main application code
- `lib/` - Reusable library components
- `include/` - Shared header files
- `test/` - PlatformIO unit tests
- `platformio.ini` - Build configuration

## Dependencies
Key libraries (managed via PlatformIO):
- Adafruit SSD1306/GFX/GC9A01A/BusIO
- GxEPD2 (e-paper display)
- ArduinoJson

## Target Hardware
- Board: Seeed XIAO ESP32S3
- Upload/Monitor: `/dev/ttyACM0` at 115200 baud
