#ifndef PIORUN_CORE_LOG_H_
#define PIORUN_CORE_LOG_H_

#include <cstdio>
#include <fstream>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>

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
  Logger() : mask_(0xFFFFFFFF) { logfile_ = stdout; }
  Logger(const std::string& logpath) : mask_(0xFFFFFFFF) {
    logfile_ = fopen(logpath.c_str(), "rw");
  }

  ~Logger() {
    if (logfile_ != stdout) fclose(logfile_);
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
  inline void InfoF(const char* __restrict__ fmt, const T&... msg) {
    LogWithFormat(INFO, fmt, msg...);
  }

  template <typename... T>
  inline void WarningF(const char* __restrict__ fmt, const T&... msg) {
    LogWithFormat(WARNING, fmt, msg...);
  }

  template <typename... T>
  inline void ErrorF(const char* __restrict__ fmt, const T&... msg) {
    LogWithFormat(ERROR, fmt, msg...);
  }

  template <typename... T>
  inline void FatalF(const char* __restrict__ fmt, const T&... msg) {
    LogWithFormat(FATAL, fmt, msg...);
  }

  template <typename... T>
  inline void Info(const T&... msg) {
    LogWithoutFormat(INFO, msg...);
  }

  template <typename... T>
  inline void Warning(const T&... msg) {
    LogWithoutFormat(WARNING, msg...);
  }

  template <typename... T>
  inline void Error(const T&... msg) {
    LogWithoutFormat(ERROR, msg...);
  }

  template <typename... T>
  inline void Fatal(const T&... msg) {
    LogWithoutFormat(FATAL, msg...);
  }

  template <typename T>
  friend Ref<Logger> operator<<(Ref<Logger> logger, const T& data) {
    std::stringstream ss;
    logger->Helper(ss, data);

    fprintf(logger->logfile_, "%s", ss.str().c_str());

    return logger;
  }

 public:
  void set_mask(uint32_t mask) { mask_ = mask; }

 private:
  template <typename T0, typename... T>
  void Helper(std::stringstream& ss, const T0& t0, const T&... msg) {
    ss << t0;
    if constexpr (sizeof...(msg) > 0) Helper(ss, msg...);
  }

  template <typename... T>
  void LogWithoutFormat(Level l, const T&... msg) {
    if (l & mask_) {
      std::lock_guard<std::mutex> lk(mt_);
      auto level = LevelToString(l);

      std::stringstream ss;
      Helper(ss, msg...);

      fprintf(logfile_, "%s ", level.c_str());
      fprintf(logfile_, "%s", ss.str().c_str());
      fprintf(logfile_, "\n");
    }
  }

  template <typename... T>
  void LogWithFormat(Level l, const char* __restrict__ fmt, const T&... msg) {
    if (l & mask_) {
      std::lock_guard<std::mutex> lk(mt_);
      auto level = LevelToString(l);

      fprintf(logfile_, "%s ", level.c_str());
      fprintf(logfile_, fmt, msg...);
      fprintf(logfile_, "\n");
    }
  }

 private:
  std::string LevelToString(Level l);

 private:
  FILE* logfile_;
  std::mutex mt_;

  uint32_t mask_;  //< 用于控制输出级别
};
}  // namespace logger
}  // namespace pio

#endif  // !PIORUN_CORE_LOG_H_
