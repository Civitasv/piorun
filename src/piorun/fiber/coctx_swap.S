.globl coctx_swap
#if !defined( __APPLE__ )
.type  coctx_swap, @function
#endif
coctx_swap:
    # &(curr->ctx) in %rdi, &(pending_co->ctx) in %rsi
    movq %rsp, 96(%rdi)   # 保存 %rax 到 curr->ctx.regs[12]
    movq %rbx, 88(%rdi)   # 保存 %rbx 到 curr->ctx.regs[11]
    movq %rcx, 80(%rdi)   # 保存 %rcx 到 curr->ctx.regs[10]
    movq %rdx, 72(%rdi)   # 保存 %rdx 到 curr->ctx.regs[9]
    movq %rsi, 64(%rdi)   # 保存 %rsi 到 curr->ctx.regs[8]
	movq %rdi, 56(%rdi)   # 保存 %rdi 到 curr->ctx.regs[7]
    movq %rbp, 48(%rdi)   # 保存 %rbp 到 curr->ctx.regs[6]
    movq %r8 , 40(%rdi)   # 保存 %r8  到 curr->ctx.regs[5]
    movq %r9 , 32(%rdi)   # 保存 %r9  到 curr->ctx.regs[4]
    movq %r12, 24(%rdi)   # 保存 %r12 到 curr->ctx.regs[3]
    movq %r13, 16(%rdi)   # 保存 %r13 到 curr->ctx.regs[2]
    movq %r14,  8(%rdi)   # 保存 %r14 到 curr->ctx.regs[1]
    movq %r15,  (%rdi)    # 保存 %r15 到 curr->ctx.regs[0]

    movq 48(%rsi), %rbp   # 恢复 pending_co 的 %rbp
    movq 96(%rsi), %rsp   # 恢复 pending_co 的 %rsp
    movq   (%rsi), %r15   # 恢复 pending_co 的 %r15
    movq  8(%rsi), %r14   # 恢复 pending_co 的 %r14
    movq 16(%rsi), %r13   # 恢复 pending_co 的 %r13
    movq 24(%rsi), %r12   # 恢复 pending_co 的 %r12
    movq 32(%rsi), %r9    # 恢复 pending_co 的 %r9
    movq 40(%rsi), %r8    # 恢复 pending_co 的 %r8
    movq 56(%rsi), %rdi   # 恢复 pending_co 的 %rdi
    movq 72(%rsi), %rdx   # 恢复 pending_co 的 %rdx
    movq 80(%rsi), %rcx   # 恢复 pending_co 的 %rcx
    movq 88(%rsi), %rbx   # 恢复 pending_co 的 %rbx
    movq 64(%rsi), %rsi   # 恢复 pending_co 的 %rsi
	ret