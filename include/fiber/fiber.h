/**
 * @file fiber.h
 * @author horse-dog (horsedog@whu.edu.cn)
 * @brief 有栈协程
 * @version 0.1
 * @date 2023-06-12
 * 
 * @copyright Copyright (c) 2023
 */

#ifndef PIORUN_FIBER_FIBER_H_
#define PIORUN_FIBER_FIBER_H_

#include "coctx.h"
#include "core/mutex.h"

#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdio>
#include <chrono>
#include <functional>
#include <type_traits>

namespace pio::fiber {
  
/**
 * 线程所管理的协程的运行环境
 * 一个线程只有一个这个属性
 */
class FiberEnvironment;

class Fiber {

  friend class FiberEnvironment;
  friend class condition_variable;
  friend int GoRoutine(Fiber* co, void*);

 public:
  Fiber(const std::function<void()>& pfn = nullptr);
  Fiber(const Fiber& rhs) = default;
  Fiber(Fiber&& rhs) noexcept = default;
 ~Fiber();

  Fiber& operator=(const Fiber& rhs) = default;
  Fiber& operator=(Fiber&& rhs) noexcept = default;

  void Yield ();
  void Resume();
  void Reset (const std::function<void()>& pfn = nullptr);
  void Reset (std::function<void()>&& pfn = nullptr);
  bool Done() const { return cEnd_ != 0; }
  bool IsMain() const { return cIsMain_ == 1; }
  bool IsHooked() const { return cEnableSysHook_ == 1; }
  void EnableHook() { cEnableSysHook_ = 1; }
  void DisableHook() { cEnableSysHook_ = 0; }

 private:
  FiberEnvironment*     env_; /*> 协程所在的运行环境，即该协程所属的协程管理器 */
  std::function<void()> pfn_; /*> 协程对应的函数 */
  FiberContext          ctx_; /*> 协程上下文，包括寄存器和栈 */

  char               cStart_; /*> 该协程是否已经开始运行 */
  char                 cEnd_; /*> 该协程是否已经执行完毕 */
  char              cIsMain_; /*> 该协程是否是主协程 */
  char       cEnableSysHook_; /*> 是否打开钩子标识，默认打开 */
  char          cCreateByEnv;

 private:
  Fiber(bool);
  void SwapContext(Fiber* pendingCo);

};

class condition_variable {

 public:
  void wait();

  // template<typename _Predicate>
  // void wait(_Predicate pred);

  void notify_one();
  void notify_all();

 private:
  std::deque<Fiber*> waiters;
  SpinLock               mtx;

};

class mutex {

 public:
  mutex() : locked(false) {}
 ~mutex() {}
  mutex(const mutex&) = delete;
  mutex& operator=(const mutex&) = delete;

  void lock();
  bool try_lock();
  void unlock();

 private:
  std::deque<Fiber*> waiters;
  SpinLock               mtx;
  bool                locked;
};

class semaphore {

 public:
  semaphore(size_t count = 0) : count(count) {}
 ~semaphore() {}
  semaphore(const semaphore&) = delete;
  semaphore& operator=(const semaphore&) = delete;

  void wait();
  bool try_wait();
  void signal();

 private:
  std::deque<Fiber*> waiters;
  SpinLock               mtx;
  size_t               count;
};

class FiberScheduler {

 private:
  FiberScheduler();
  FiberScheduler(const FiberScheduler&) = delete;
  FiberScheduler(FiberScheduler&&) = delete;
 ~FiberScheduler();

 public:
  static FiberScheduler& GetInstance();
  void Start(std::function<int(void)> pfn = nullptr);

};

class MultiThreadFiberScheduler {

 private:
  MultiThreadFiberScheduler(int threadNum = 4);
  MultiThreadFiberScheduler(const MultiThreadFiberScheduler&) = delete;
  MultiThreadFiberScheduler(MultiThreadFiberScheduler&&) = delete;
 ~MultiThreadFiberScheduler();

 public:
  static MultiThreadFiberScheduler& GetInstance();
  void Schedule(const std::function<void()>& fn);

 private:
  
  std::deque<std::function<void()>> commTasks;
  SpinLock mutex;
  std::deque<std::thread> threads;
  std::atomic_bool* wq;
  const int threadNum;
  std::atomic_bool stop;
  static const int TASK_THRESHOLD = 1;
};

struct __go {

  __go() {}

 ~__go() {}

  inline void operator-(const std::function<void()>& fn) {
    MultiThreadFiberScheduler::GetInstance().Schedule(fn);
  }

};

}


namespace pio::this_fiber {

/**
 * @brief get ID of current coroutine.
 * 
 * @return coroutine ID
 */
unsigned long long get_id();

/**
 * @brief get pointer of current coroutine.
 * 
 * @return pio::fiber::StCoRoutine* 
 */
pio::fiber::Fiber* co_self();

int get_thread_id();

/**
 * @brief yield current coroutine.
 */
void yield();

/**
 * @brief enable system hook for current coroutine.
 */
void enable_system_hook();

/**
 * @brief disable system hook for current coroutine.
 */
void disable_system_hook();

/**
 * @brief sleep current coroutine.
 * 
 * @param ms sleep time
 */
void sleep_for(const std::chrono::milliseconds& ms);

/**
 * @brief sleep current coroutine.
 * 
 * @param time_point
 */
void sleep_until(const std::chrono::high_resolution_clock::time_point& time_point);

}

#define go pio::fiber::__go()-

#endif