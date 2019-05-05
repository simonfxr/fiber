.CODE

fiber_align_check_failed PROTO C


fiber_asm_switch PROC
  mov rax, [rdx]
  mov r8, [rax]

  mov [rcx + 0], rsp
  mov [rcx + 8], rbx
  mov [rcx + 16], rbp
  mov [rcx + 24], rdi
  mov [rcx + 32], rsi
  mov [rcx + 40], r12
  mov [rcx + 48], r13
  mov [rcx + 56], r14
  mov [rcx + 64], r15
  lea rcx, [rcx + 72 + 8]
  and rcx, -16
  movaps [rcx + 16 * 0], xmm6
  movaps [rcx + 16 * 1], xmm7
  movaps [rcx + 16 * 2], xmm8
  movaps [rcx + 16 * 3], xmm9
  movaps [rcx + 16 * 4], xmm10
  movaps [rcx + 16 * 5], xmm11
  movaps [rcx + 16 * 6], xmm12
  movaps [rcx + 16 * 7], xmm13
  movaps [rcx + 16 * 8], xmm14
  movaps [rcx + 16 * 9], xmm15

  lea rsp, [rax + 8]
  mov rbx, [rdx + 8]
  mov rbp, [rdx + 16]
  mov rdi, [rdx + 24]
  mov rsi, [rdx + 32]
  mov r12, [rdx + 40]
  mov r13, [rdx + 48]
  mov r14, [rdx + 56]
  mov r15, [rdx + 64]
  lea rdx, [rdx + 72 + 8]
  and rdx, -16
  movaps xmm6, [rdx + 16 * 0]
  movaps xmm7, [rdx + 16 * 1]
  movaps xmm8, [rdx + 16 * 2]
  movaps xmm9, [rdx + 16 * 3]
  movaps xmm10, [rdx + 16 * 4]
  movaps xmm11, [rdx + 16 * 5]
  movaps xmm12, [rdx + 16 * 6]
  movaps xmm13, [rdx + 16 * 7]
  movaps xmm14, [rdx + 16 * 8]
  movaps xmm15, [rdx + 16 * 9]

  jmp r8
fiber_asm_switch ENDP


fiber_asm_invoke PROC
  add rsp, 8
  pop rdx
  mov rcx, [rsp]
  sub rsp, 32

IFDEF FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0Fh
  jnz fiber_align_check_failed
ENDIF
  call rdx
  mov rsp, [rsp + 40]
  ret
fiber_asm_invoke ENDP


fiber_asm_exec_on_stack PROC
  push rbp
  mov rbp, rsp
  lea rsp, [rcx - 40]

IFDEF FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0Fh
  jnz fiber_align_check_failed
ENDIF
  mov rcx, r8
  call rdx
  mov rax, rbp
  mov rsp, rbp
  pop rbp
  ret
fiber_asm_exec_on_stack ENDP

END
