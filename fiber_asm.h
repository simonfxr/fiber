#ifndef FIBER_ASM_H
#define FIBER_ASM_H

#include <stddef.h>

typedef void *StackPtr;

void fiber_asm_switch(StackPtr *from, StackPtr to);

void fiber_asm_push_thunk(StackPtr *, FiberFunc, const void *, size_t);

void fiber_asm_exec_on_stack(StackPtr, FiberFunc, const void *, size_t);

#endif
