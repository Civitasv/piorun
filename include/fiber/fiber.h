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

#include <cstdio>
#include <chrono>
#include <functional>
#include <type_traits>

// TODO: move
#if defined(__GNUC__) && (__GNUC__ > 3 ||(__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
# define ALWAYS_INLINE __attribute__ ((always_inline)) inline 
#else
# define ALWAYS_INLINE inline
#endif


namespace pio::fiber {

struct __go {

  __go(const char* file, int lineno)
  : file_(file), lineno_(lineno) {}

  template<typename Function>
  ALWAYS_INLINE void operator-(Function) {
    printf("hello fiber\n");
    printf("fileName: %s\n", file_);
    printf("lineNo: %d\n", lineno_);
  }

  const char* file_;
  int lineno_;

};

  
/**
 * 线程所管理的协程的运行环境
 * 一个线程只有一个这个属性
 */
struct StCoRoutineEnv;

struct StStackMem {
  int   stackSize_; /*> 栈的大小 */
  char*        bp_; /*> 栈基址 */
  char*    buffer_; /*> 栈顶，stack_bp = stack_buffer + stack_size */

  StStackMem(int size = 128 * 1024) : stackSize_(size) {
    this->buffer_ = new char[stackSize_];
    this->bp_ = buffer_ + stackSize_;
  }

 ~StStackMem() {
    delete[] buffer_;
    buffer_ = nullptr;
    bp_ = nullptr;
  }

};

class StCoRoutine {
  friend class StCoRoutineEnv;
 public:
  using pfn_co_routine_t = void*(*)(void*);

  StCoRoutine(pfn_co_routine_t pfn, void* arg);
 ~StCoRoutine();

  void Yield ();
  void Resume();
  void Reset ();
  bool Done() const;

 private:
  StCoRoutineEnv*     env_; /*> 协程所在的运行环境，即该协程所属的协程管理器 */
  pfn_co_routine_t    pfn_; /*> 协程对应的函数 */
  void*               arg_; /*> 协程对应的函数的参数 */
  StCoContext         ctx_; /*> 协程上下文，包括寄存器和栈 */

  char             cStart_; /*> 该协程是否已经开始运行 */
  char               cEnd_; /*> 该协程是否已经执行完毕 */
  char            cIsMain_; /*> 该协程是否是主协程 */
  char     cEnableSysHook_; /*> 是否打开钩子标识，默认关闭 */

  StStackMem*   stack_mem_; /*> 指向栈内存 */
  char*          stack_sp_; /*> 当前协程的栈的栈顶 */

 private:
  StCoRoutineEnv* CoGetCurrThreadEnv() const;
  void CoInitCurrThreadEnv();
  void SwapContext(StCoRoutine* pendingCo);

  friend int Run(StCoRoutine* co, void*);

};

// TODO: modify.
typedef int (*pfn_co_eventloop_t)(void *);

void co_eventloop(pfn_co_eventloop_t pfn, void *arg);

}


namespace pio::this_coroutine {

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
pio::fiber::StCoRoutine* co_self();

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

}

#define go pio::fiber::__go(__FILE__, __LINE__)-

#endif