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
#define BIT(n) 1 << n
class Logger {
 public:
  enum Level {
    VERBOSE = BIT(0),
    DEBUG = BIT(1),
    INFO = BIT(2),
    WARNING = BIT(3),
    ERROR = BIT(4),
    FATAL = BIT(5),
    NO_LOG = BIT(6)
  };

 public:
  Logger(Level min_log_level)
      : min_log_level_(min_log_level), use_mask_(false), mask_(0xFFFFFFFF) {
    logfile_ = stdout;
  }
  Logger(const std::string& logpath, Level min_log_level)
      : min_log_level_(min_log_level), use_mask_(false), mask_(0xFFFFFFFF) {
    logfile_ = fopen(logpath.c_str(), "w");
  }

  ~Logger() {
    if (logfile_ != stdout) fclose(logfile_);
  }

  /**
   * @brief 创建日志实例，输出到文件
   *
   * @param logpath 日志输出文件
   * @param min_log_level 日志级别下限
   * @return Ref<Logger> 该日志实例可以在多个线程中共享，
   *         引用数量为0时会被自动回收
   */
  static Ref<Logger> Create(const std::string& logpath,
                            Level min_log_level = Level::VERBOSE);

  /**
   * @brief 创建日志实例，输出到标准输出
   *
   * @param min_log_level 日志级别下限
   * @return Ref<Logger> 该日志实例可以在多个线程中共享，
   *         引用数量为0时会被自动回收
   */
  static Ref<Logger> Create(Level min_log_level = Level::VERBOSE);

  template <typename... T>
  inline void VerboseF(const char* __restrict__ fmt, const T&... msg) {
    LogWithFormat(VERBOSE, fmt, msg...);
  }

  template <typename... T>
  inline void DebugF(const char* __restrict__ fmt, const T&... msg) {
    LogWithFormat(DEBUG, fmt, msg...);
  }

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
  inline void Verbose(const T&... msg) {
    LogWithoutFormat(VERBOSE, msg...);
  }

  template <typename... T>
  inline void Debug(const T&... msg) {
    LogWithoutFormat(DEBUG, msg...);
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
  void set_min_log_level(Level min_log_level) {
    min_log_level_ = min_log_level;
  }

  void set_use_mask(bool use_mask) { use_mask_ = use_mask; }

  void set_mask(uint32_t mask) { mask_ = mask; }

 private:
  template <typename T0, typename... T>
  void Helper(std::stringstream& ss, const T0& t0, const T&... msg) {
    ss << t0;
    if constexpr (sizeof...(msg) > 0) Helper(ss, msg...);
  }

  template <typename... T>
  void LogWithoutFormat(Level l, const T&... msg) {
    bool check = (use_mask_ && ((l & mask_) != 0) ||
                  !use_mask_ && (l >= min_log_level_));
    if (check) {
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
    bool check = (use_mask_ && ((l & mask_) != 0)) ||
                 (!use_mask_ && (l >= min_log_level_));
    if (check) {
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

  Level min_log_level_;  //< 用于控制输出级别下限

  bool use_mask_;  //< 是否使用 mask
  uint32_t mask_;  //< 用于更细粒度的控制输出级别
};
}  // namespace pio

#endif  // !PIORUN_CORE_LOG_H_
