#include "core/log.h"

#include <fstream>
#include <mutex>

namespace pio {
namespace logger {

namespace {
std::ofstream logfile;
std::mutex mt;
}  // namespace

std::ostream &operator<<(std::ostream &os, const Level &l) {
  switch (l) {
    case FATAL:
      os << "Fatal";
      break;
    case ERROR:
      os << "Error";
      break;
    case WARNING:
      os << "Warning";
      break;
    default:
      break;
  }

  return os;
}

void Setup(const std::string &logpath) { logfile.open(logpath); }
void Close() { logfile.close(); }

void Log(Level l, std::string_view msg) {
  std::lock_guard<std::mutex> lk(mt);
  if (logfile.is_open()) {
    logfile << l << ": " << msg << '\n';
  }
}

}  // namespace logger
}  // namespace pio
