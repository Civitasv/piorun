/**
 * @file coctx.h
 * @author horse-dog (horsedog@whu.edu.cn)
 * @brief 有栈协程
 * @version 0.1
 * @date 2023-06-12
 * 
 * @copyright Copyright (c) 2023
 */

#ifndef PIORUN_FIBER_COCTX_H_
#define PIORUN_FIBER_COCTX_H_

#include <stdlib.h>

namespace pio::fiber {

// 协程的上下文
struct StCoContext {

  void * regs_[14]; /*> 存储13个通用寄存器和rip，这里不需要r10，r11以及rax，r10、r11和rax会被调用者保存 */
  size_t size_;     /*> 该上下文使用的栈的大小 */
  char * sp_;	      /*> 该上下文使用的栈的栈顶 */

  using StCoContextFunc = void*(*)(void*, void*);

  StCoContext() : regs_{}, size_{}, sp_{} {}

  StCoContext(StCoContextFunc pfn, const void* s1, const void* s2);

  /**
   * @brief 使用pfn作为协程调度的函数，初始化一个上下文
   *
   * @param ctx 指向一个上下文
   * @param pfn 指向协程准备调度的函数
   * @param s1	@p pfn 的第一个参数
   * @param s2  @p pfn 的第二个参数
   * @return int
   */
  int Make(StCoContextFunc pfn, const void *s1, const void* s2);

};

}

#endif
