.CODE

fiber_align_check_failed PROTO C


fiber_asm_switch PROC
  pop rax

  mov [rcx], rsp
  mov rsp, [rdx]
  mov [rcx+8h], rax
  mov rax, [rdx+8h]
  mov [rcx+10h], rbx
  mov rbx, [rdx+10h]
  mov [rcx+18h], rbp
  mov rbp, [rdx+18h]
  mov [rcx+20h], rdi
  mov rdi, [rdx+20h]
  mov [rcx+28h], rsi
  mov rsi, [rdx+28h]
  mov [rcx+30h], r12
  mov r12, [rdx+30h]
  mov [rcx+38h], r13
  mov r13, [rdx+38h]
  mov [rcx+40h], r14
  mov r14, [rdx+40h]
  mov [rcx+48h], r15
  mov r15, [rdx+48h]

  lea rcx, [rcx+58h]
  and rcx, -16
  lea rdx, [rdx+58h]
  and rdx, -16

  movaps [rcx], xmm6
  movaps xmm6, [rdx]
  movaps [rcx+10h], xmm7
  movaps xmm7, [rdx+10h]
  movaps [rcx+20h], xmm8
  movaps xmm8, [rdx+20h]
  movaps [rcx+30h], xmm9
  movaps xmm9, [rdx+30h]
  movaps [rcx+40h], xmm10
  movaps xmm10, [rdx+40h]
  movaps [rcx+50h], xmm11
  movaps xmm11, [rdx+50h]
  movaps [rcx+60h], xmm12
  movaps xmm12, [rdx+60h]
  movaps [rcx+70h], xmm13
  movaps xmm13, [rdx+70h]
  movaps [rcx+80h], xmm14
  movaps xmm14, [rdx+80h]
  movaps [rcx+90h], xmm15
  movaps xmm15, [rdx+90h]

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
