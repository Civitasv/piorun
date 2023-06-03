/**
 * @file mutex.h
 * @author horse-dog (horsedog@whu.edu.cn)
 * @brief piorun mutex模块
 * @version 0.1
 * @date 2023-05-31
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef PIORUN_CORE_MUTEX_H_
#define PIORUN_CORE_MUTEX_H_

#include <memory>
#include <shared_mutex>
#include <semaphore.h>

namespace pio {

// 线程级信号量
class Semaphore {
 public:
  /**
   * @brief Constructor
   * 
   * @param count 信号量初始计数
   */
  Semaphore(uint32_t count = 0);

  /**
   * @brief Destructor
   */
  ~Semaphore();

  /**
   * @brief 获取信号量
   */
  void Wait();

  /**
   * @brief 释放信号量
   */
  void Signal();

 private:
  sem_t semaphore_; /*> posix信号量 */
};


// 用户态自旋锁
class SpinLock {
 public:
  SpinLock() noexcept 
  { pthread_spin_init(&locker_, 0); };

  ~SpinLock() noexcept 
  { pthread_spin_destroy(&locker_); }

  void Lock() noexcept 
  { pthread_spin_lock(&locker_); }

  void Unlock() noexcept 
  { pthread_spin_unlock(&locker_); }

  bool TryLock() noexcept 
  { return pthread_spin_trylock(&locker_) == 0; }

 private:
  pthread_spinlock_t locker_; /*> posix自旋锁 */
};


// 用户态原子锁
class CASLock {
 public:
  CASLock() noexcept {};

  void Lock() noexcept {
    while (flag_.test_and_set(std::memory_order_acquire))
      ;
  }
  
  void Unlock() noexcept { flag_.clear(std::memory_order_release); }
  
  bool TryLock() noexcept {
    return !flag_.test_and_set(std::memory_order_acquire);
  }

 private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT; /*> 原子布尔值 */
};

} // namespace pio

#endif // !PIORUN_CORE_MUTEX_H_