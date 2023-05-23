#ifndef PIORUN_CORE_LOG_H_
#define PIORUN_CORE_LOG_H_

#include <ostream>
#include <string_view>

namespace pio {
namespace logger {

enum Level {
  FATAL = 0,
  ERROR,
  WARNING,
};

void Begin(const std::string &logpath);
void End();

void Log(Level l, std::string_view msg);

inline void Fatal(std::string_view msg) { Log(FATAL, msg); }
inline void Error(std::string_view msg) { Log(ERROR, msg); }
inline void Warning(std::string_view msg) { Log(WARNING, msg); }

}  // namespace logger
}  // namespace pio

#endif  // !PIORUN_CORE_LOG_H_
