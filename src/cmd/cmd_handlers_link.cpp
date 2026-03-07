// Force-compile local cmd handler implementations into the firmware target.
// PlatformIO LDF currently does not pick up `lib/cmd_handlers` automatically.
#include "cmd_handlers/cmd_handlers.cpp"
#include "cmd_handlers/info_handlers.cpp"
#include "cmd_handlers/log_handlers.cpp"
#include "cmd_handlers/image_handlers.cpp"
