/**
 * @file thread.h
 * @author horse-dog (horsedog@whu.edu.cn)
 * @brief piorn 线程模块
 * @version 0.1
 * @date 2023-05-31
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef PIORUN_CORE_THREAD_H_
#define PIORUN_CORE_THREAD_H_

#include <functional>
#include <memory>
#include <thread>

#include "mutex.h"

namespace pio {

// 线程类
class Thread {
 public:
  /**
   * @brief Constructor
   *
   * @param cb 线程执行函数
   * @param threadName 线程名称
   */
  Thread(std::function<void()> cb, const std::string& threadName);

  /**
   * @brief Destructor
   */
  ~Thread();

  /**
   * @brief 获取线程ID
   * 
   * @return pid_t 线程ID
   */
  pid_t get_system_id() const { return system_id_; }

  /**
   * @brief 获取线程名称
   * 
   * @return const std::string& 线程名称
   */
  const std::string& get_thread_name() const { return thread_name_; }

  /**
   * @brief Join线程
   */
  void Join();

  /**
   * @brief 获取当前线程的指针
   * 
   * @return Thread* 指向当前线程
   */
  static Thread* GetCurrentThread();

  /**
   * @brief 获取当前线程ID
   * 
   * @return pid_t 线程ID
   */
  static pid_t GetCurrentThreadId();

  /**
   * @brief 获取当前线程的线程名称
   * 
   * @return const std::string& 当前线程的线程名称
   */
  static const std::string& GetCurrentThreadName();

  /**
   * @brief 设置当前线程的线程名称
   * 
   * @param name 线程名称
   */
  static void SetCurrentThreadName(const std::string& name);

  /**
   * @brief 启动线程
   * 
   * @param arg 
   * @return void* 
   */
  static void* run(void* arg);

 private:
  Thread(const Thread&) = delete;
  Thread(const Thread&&) = delete;
  Thread& operator=(const Thread&) = delete;

 private:
  pid_t system_id_;          /*> 系统线程标识 */
  pthread_t thread_;         /*> pthread标识 */
  std::function<void()> cb_; /*> 线程执行函数 */
  std::string thread_name_;  /*> 线程名称 */
  Semaphore semaphore_;      /*> 信号量 */
};

}  // namespace pio

#endif // !PIORUN_CORE_THREAD_H_