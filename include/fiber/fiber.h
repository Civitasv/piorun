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

class MultiThreadFiberScheduler;

class Fiber;

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

class shared_mutex {

 public:
  shared_mutex() : state() {}
 ~shared_mutex() {}
  shared_mutex(const shared_mutex&) = delete;
  shared_mutex& operator=(const shared_mutex&) = delete;

  void lock();
  bool try_lock();
  void unlock();
  void lock_shared();
  bool try_lock_shared();
  void unlock_shared();

 private:
  std::deque<Fiber*> rwaiters;
  std::deque<Fiber*> wwaiters;
  SpinLock                mtx;
  int                   state;

};

class condition_variable {

 public:
  void wait(std::unique_lock<fiber::mutex>& lock);

  template<typename _Predicate>
  void wait(std::unique_lock<fiber::mutex>& lock, _Predicate pred) {
	  while (!pred())
	    wait(lock);
  }

  void notify_one();
  void notify_all();

 private:
  std::deque<Fiber*> waiters;
  SpinLock               mtx;

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

 public:
  std::deque<Fiber*> waiters;
  SpinLock               mtx;
  size_t               count;

};

template <typename T>
class channel {

 public:
  channel(int sz = 1) : full(0), space(sz) {}
 ~channel() {}

  friend void operator>>(const T& x, channel& c)
  { c.write(x); }

  friend void operator>>(T&& x, channel& c)
  { c.write(std::move(x)); }

  friend void operator<<(T&x, channel&c)
  { x = c.read(); }

  void operator>>(T& x)
  { x = this->read(); }

  void operator<<(const T& x) 
  { this->write(x); }

  void operator<<(T&& x)
  { this->write(std::move(x)); }

  void write(const T& x) {
    space.wait();
    mtx.lock();
    items.push_back(x);
    mtx.unlock();
    full.signal();
  }

  void write(T&& x) {
    space.wait();
    mtx.lock();
    items.push_back(std::move(x));
    mtx.unlock();
    full.signal();
  }

  T read() {
    full.wait();
    mtx.lock();
    T x = std::move(items.front());
    items.pop_front();
    mtx.unlock();
    space.signal();
    return x;
  }

 private:
  mutex mtx;
  semaphore full;
  semaphore space;
  std::deque<T> items;

};


using Mutex = mutex;
using RWmutex = shared_mutex;
using Semaphore = semaphore;
using ConditionVariable = condition_variable;

template <class T>
using Channel = channel<T>;


class Fiber {

friend class FiberEnvironment;
friend class mutex;
friend class shared_mutex;
friend class semaphore;
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
  void Reset (std::function<void()>&& pfn);
  bool Done() const { return cEnd_ != 0; }
  bool IsMain() const { return cIsMain_ == 1; }
  bool IsHooked() const { return cEnableSysHook_ == 1; }
  void EnableHook() { cEnableSysHook_ = 1; }
  void DisableHook() { cEnableSysHook_ = 0; }

 private:
  FiberEnvironment*     env_; /*> 协程所在的协程环境 */
  std::function<void()> pfn_; /*> 协程对应的函数 */
  FiberContext          ctx_; /*> 协程上下文，包括寄存器和栈 */

  bool               cStart_; /*> 该协程是否已经开始运行 */
  bool                 cEnd_; /*> 该协程是否已经执行完毕 */
  bool              cIsMain_; /*> 该协程是否是主协程 */
  bool       cEnableSysHook_; /*> 是否打开钩子标识，默认打开 */
  bool          cCreateByEnv; /*> 该协程是否是有协程池创建的，默认为否 */

 private:
  Fiber(bool);
  void SwapContext(Fiber* pendingCo);

};


class MultiThreadFiberScheduler {

friend class mutex;
friend class shared_mutex;
friend class semaphore;
friend class condition_variable;
friend void threadRoutine(int, MultiThreadFiberScheduler*);

 private:
  MultiThreadFiberScheduler(int threadNum = 7);
  MultiThreadFiberScheduler(const MultiThreadFiberScheduler&) = delete;
  MultiThreadFiberScheduler(MultiThreadFiberScheduler&&) = delete;
 ~MultiThreadFiberScheduler();

 public:
  static MultiThreadFiberScheduler& GetInstance();
  void Schedule(const std::function<void()>& fn);
  void Schedule(std::function<void()>&& fn);

 private:
  
  std::deque<std::function<void()>> commTasks;
  SpinLock mutex;
  std::deque<std::thread> threads;
  int wannaQuitThreadCount;
  const int threadNum;
  // int turn = 0;
};

struct __go {

  __go() {}

 ~__go() {}

  inline void operator-(const std::function<void()>& fn) {
    MultiThreadFiberScheduler::GetInstance().Schedule(fn);
  }

  inline void operator-(std::function<void()>&& fn) {
    MultiThreadFiberScheduler::GetInstance().Schedule(std::move(fn));
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
 * @brief get ID of current thread in FiberEnv.
 * 
 * @return thread ID
 */
int get_thread_id();

/**
 * @brief get pointer of current coroutine.
 * 
 * @return pio::fiber::StCoRoutine* 
 */
pio::fiber::Fiber* co_self();

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