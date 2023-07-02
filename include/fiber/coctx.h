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
struct FiberContext {

  struct StackMemory {
    
    int   stackSize_; /*> 栈的大小 */
    char*        bp_; /*> 栈基址 */
    char*    buffer_; /*> 栈顶，stack_bp = stack_buffer + stack_size */

    StackMemory(int size = 128 * 1024);

   ~StackMemory();

  };

  FiberContext();

  FiberContext(const FiberContext& rhs);

  FiberContext(FiberContext&& rhs) noexcept;

 ~FiberContext();

  FiberContext& operator=(const FiberContext& rhs);

  FiberContext& operator=(FiberContext&& rhs);

  /**
   * @brief 使用pfn作为协程调度的函数，初始化一个上下文
   *
   * @param ctx 指向一个上下文
   * @param pfn 指向协程准备调度的函数
   * @param s1	@p pfn 的第一个参数
   * @param s2  @p pfn 的第二个参数
   * @return int
   */
  int Make(void*(*pfn)(void*, void*), const void *s1, const void* s2);

  void*     regs_[14]; /*> 存储13个通用寄存器和rip，这里不需要r10，r11以及rax，r10、r11和rax会被调用者保存 */
  StackMemory* stack_; /*> 栈内存 */

};

}

#endif
