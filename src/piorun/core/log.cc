#include "core/log.h"

#include "core/smartptr.h"

namespace pio {
Ref<Logger> Logger::Create(const std::string &logpath) {
  return CreateRef<Logger>(logpath);
}

Ref<Logger> Logger::Create() { return CreateRef<Logger>(); }

std::string Logger::LevelToString(Level l) {
  switch (l) {
    case INFO:
      return "[Info]";
    case WARNING:
      return "[Warning]";
    case ERROR:
      return "[Error]";
    case FATAL:
      return "[Fatal]";
    default:
      return "[Unknown]";
  }
}
}  // namespace pio
