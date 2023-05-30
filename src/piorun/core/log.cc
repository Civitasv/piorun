#include "core/log.h"

#include "core/smartptr.h"

namespace pio {
namespace logger {

std::ostream &operator<<(std::ostream &os, const Level &l) {
  switch (l) {
    case INFO:
      os << "[Info]";
      break;
    case WARNING:
      os << "[Warning]";
      break;
    case ERROR:
      os << "[Error]";
      break;
    case FATAL:
      os << "[Fatal]";
      break;
    default:
      break;
  }

  return os;
}

Ref<Logger> Logger::Create(const std::string &logpath) {
  return CreateRef<Logger>(logpath);
}

void Logger::Log(Level l, const std::string &msg) {
  std::lock_guard<std::mutex> lk(mt_);
  if (logfile_.is_open()) {
    logfile_ << l << ": " << msg << '\n';
  }
}

}  // namespace logger
}  // namespace pio
