.CODE

fiber_align_check_failed PROTO C


fiber_asm_switch proc
  mov [rcx + 0], rsp
  mov [rcx + 8], rbx
  mov [rcx + 16], rbp
  mov [rcx + 24], rdi
  mov [rcx + 32], rsi
  mov [rcx + 40], r12
  mov [rcx + 48], r13
  mov [rcx + 56], r14
  mov [rcx + 64], r15

  mov rsp, [rdx + 0]
  mov rbx, [rdx + 8]
  mov rbp, [rdx + 16]
  mov rdi, [rdx + 24]
  mov rsi, [rdx + 32]
  mov r12, [rdx + 40]
  mov r13, [rdx + 48]
  mov r14, [rdx + 56]
  mov r15, [rdx + 64]

  ret
fiber_asm_switch endp


fiber_asm_invoke proc
  add rsp, 8
  pop rdx
  mov rcx, [rsp]
  sub rsp, 32

  test esp, 0Fh
  jz ok
  jmp fiber_align_check_failed
ok:

  call rdx
  mov rsp, [rsp + 40]
  ret
fiber_asm_invoke endp


fiber_asm_exec_on_stack proc
  mov rax, rsp
  mov rsp, rcx
  push rax
  sub rsp, 32

  test esp, 0Fh
  jz ok
  jmp fiber_align_check_failed
ok:

  mov rcx, r8
  call rdx
  mov rsp, [rsp + 32]
  ret
fiber_asm_exec_on_stack endp

end
