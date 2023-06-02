#ifndef PIORUN_CORE_LOG_H_
#define PIORUN_CORE_LOG_H_

#include <cstdio>
#include <fstream>
#include <mutex>
#include <ostream>
#include <sstream>

#include "smartptr.h"

namespace pio {
namespace logger {
class Logger {
 public:
  enum Level {
    INFO = 1 << 0,
    WARNING = 1 << 1,
    ERROR = 1 << 2,
    FATAL = 1 << 3,
  };

 public:
  Logger() : use_file_(false) {}
  Logger(const std::string& logpath) : use_file_(true) {
    logfile_ = fopen(logpath.c_str(), "w");
  }

  ~Logger() {
    if (use_file_) fclose(logfile_);
  }

  /**
   * @brief 创建日志实例，输出到文件
   *
   * @param logpath 日志输出文件
   * @return Ref<Logger> 该日志实例可以在多个线程中共享，
   *         引用数量为0时会被自动回收
   */
  static Ref<Logger> Create(const std::string& logpath);

  /**
   * @brief 创建日志实例，输出到标准输出
   *
   * @return Ref<Logger> 该日志实例可以在多个线程中共享，
   *         引用数量为0时会被自动回收
   */
  static Ref<Logger> Create();

  template <typename... T>
  inline void Info(const char* __restrict__ fmt, const T&... msg) {
    Log(INFO, fmt, msg...);
  }

  template <typename... T>
  inline void Warning(const char* __restrict__ fmt, const T&... msg) {
    Log(WARNING, fmt, msg...);
  }

  template <typename... T>
  inline void Error(const char* __restrict__ fmt, const T&... msg) {
    Log(ERROR, fmt, msg...);
  }

  template <typename... T>
  inline void Fatal(const char* __restrict__ fmt, const T&... msg) {
    Log(FATAL, fmt, msg...);
  }

 private:
  template <typename... T>
  void Log(Level l, const char* __restrict__ fmt, const T&... msg) {
    std::lock_guard<std::mutex> lk(mt_);
    auto level = LevelToString(l);

    if (use_file_) {
      fprintf(logfile_, "%s ", level.c_str());
      fprintf(logfile_, fmt, msg...);
      fprintf(logfile_, "\n");
    } else {
      printf("%s ", level.c_str());
      printf(fmt, msg...);
      printf("\n");
    }
  }

 private:
  std::string LevelToString(Level l);

 private:
  FILE* logfile_;
  bool use_file_;
  std::mutex mt_;
};
}  // namespace logger
}  // namespace pio

#endif  // !PIORUN_CORE_LOG_H_
