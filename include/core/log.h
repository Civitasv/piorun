#ifndef PIORUN_CORE_LOG_H_
#define PIORUN_CORE_LOG_H_

#include <fstream>
#include <mutex>
#include <ostream>

#include "smartptr.h"

namespace pio {
namespace logger {

enum Level {
  INFO = 1 << 0,
  WARNING = 1 << 1,
  ERROR = 1 << 2,
  FATAL = 1 << 3,
};

class Logger {
 public:
  Logger(const std::string& logpath) { logfile_.open(logpath); }
  ~Logger() {
    if (logfile_.is_open()) logfile_.close();
  }

  static Ref<Logger> Create(const std::string& logpath);

  inline void Info(const std::string& msg) { Log(INFO, msg); }
  inline void Warning(const std::string& msg) { Log(WARNING, msg); }
  inline void Error(const std::string& msg) { Log(ERROR, msg); }
  inline void Fatal(const std::string& msg) { Log(FATAL, msg); }

 private:
  void Log(Level l, const std::string& msg);

 private:
  std::ofstream logfile_;
  std::mutex mt_;
};

}  // namespace logger
}  // namespace pio

#endif  // !PIORUN_CORE_LOG_H_
