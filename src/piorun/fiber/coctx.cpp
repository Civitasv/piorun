#include "fiber/coctx.h"

#include <stdio.h>
#include <string.h>

// low | regs[0] : r15 |
//     | regs[1] : r14 |
//     | regs[2] : r13 |
//     | regs[3] : r12 |
//     | regs[4] : r9  |
//     | regs[5] : r8  |
//     | regs[6] : rbp |
//     | regs[7] : rdi |
//     | regs[8] : rsi |
//     | regs[9] : rdx |
//     | regs[10]: rcx |
//     | regs[11]: rbx |
// hig | regs[12]: rsp |
enum {
  kRDI = 7,
  kRSI = 8,
  kRSP = 12,
};

namespace pio::fiber {

StCoContext::StCoContext(StCoContextFunc pfn, const void* s1, const void* s2)
: regs_{}, size_{}, sp_{} {
  Make(pfn, s1, s2);
}

int StCoContext::Make(StCoContextFunc pfn, const void *s1, const void *s2) {
  // 这里腾出8字节，用于存放返回地址，新得到的sp指针开始向下生长的栈就是新的过程的栈基址了，因为返回地址属于调用者的栈帧而非被调用者的
  char* sp = this->sp_ + this->size_ - sizeof(void*);
  // 这里是为了16字节对齐（见深入理解计算机系统），过程调用的栈基址进行16字节对齐
  // -16LL = fffffffffffffff0
  sp = (char*)((unsigned long)sp & -16LL);
  memset(regs_, 0, sizeof(regs_));

  // 设置返回地址
  void** ret_addr = (void**)(sp);
  *ret_addr = (void*)pfn;

  // 协程运行开始前，最重要的信息只有rdi，rsi，rsp和返回地址
  this->regs_[kRSP] = sp;
  this->regs_[kRDI] = (char*)s1;
  this->regs_[kRSI] = (char*)s2;
  return 0;
}

}
