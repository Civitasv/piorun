#include "core/log.h"

#include "core/smartptr.h"

namespace pio {
Ref<Logger> Logger::Create(const std::string &logpath, Level min_log_level) {
  return CreateRef<Logger>(logpath, min_log_level);
}

Ref<Logger> Logger::Create(Level min_log_level) {
  return CreateRef<Logger>(min_log_level);
}

std::string Logger::LevelToString(Level l) {
  switch (l) {
    case VERBOSE:
      return "[Verbose]";
    case DEBUG:
      return "[Debug]";
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
