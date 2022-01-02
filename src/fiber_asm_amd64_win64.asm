.CODE

fiber_align_check_failed PROTO C


fiber_asm_switch PROC
  pop rax

  mov [rcx], rsp
  mov rsp, [rdx]
  mov [rcx+0x8], rax
  mov rax, [rdx+0x8]
  mov [rcx+0x10], rbx
  mov rbx, [rdx+0x10]
  mov [rcx+0x18], rbp
  mov rbp, [rdx+0x18]
  mov [rcx+0x20], rdi
  mov rdi, [rdx+0x20]
  mov [rcx+0x28], rsi
  mov rsi, [rdx+0x28]
  mov [rcx+0x30], r12
  mov r12, [rdx+0x30]
  mov [rcx+0x38], r13
  mov r13, [rdx+0x38]
  mov [rcx+0x40], r14
  mov r14, [rdx+0x40]
  mov [rcx+0x48], r15
  mov r15, [rdx+0x48]

  lea rcx, [rcx+0x58]
  and rcx, -16
  lea rdx, [rdx+0x58]
  and rdx, -16

  movaps [rcx], xmm6
  movaps xmm6, [rdx]
  movaps [rcx+0x10], xmm7
  movaps xmm7, [rdx+0x10]
  movaps [rcx+0x20], xmm8
  movaps xmm8, [rdx+0x20]
  movaps [rcx+0x30], xmm9
  movaps xmm9, [rdx+0x30]
  movaps [rcx+0x40], xmm10
  movaps xmm10, [rdx+0x40]
  movaps [rcx+0x50], xmm11
  movaps xmm11, [rdx+0x50]
  movaps [rcx+0x60], xmm12
  movaps xmm12, [rdx+0x60]
  movaps [rcx+0x70], xmm13
  movaps xmm13, [rdx+0x70]
  movaps [rcx+0x80], xmm14
  movaps xmm14, [rdx+0x80]
  movaps [rcx+0x90], xmm15
  movaps xmm15, [rdx+0x90]

  jmp    rax
fiber_asm_switch ENDP


fiber_asm_invoke PROC
  mov rcx, [rsp]
  mov rdx, [rsp+8]
  sub rsp, 32
IFDEF FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0Fh
  jnz fiber_align_check_failed
ENDIF
  call rdx
  add rsp, 48
  mov rax, [rsp+8]
  mov rsp, [rsp]
  jmp rax
fiber_asm_invoke ENDP


fiber_asm_exec_on_stack PROC
  push rbp
  mov rbp, rsp
  lea rsp, [r8 - 32]
IFDEF FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0Fh
  jnz fiber_align_check_failed
ENDIF
  call rdx
  mov rsp, rbp
  pop rbp
  ret
fiber_asm_exec_on_stack ENDP

END
