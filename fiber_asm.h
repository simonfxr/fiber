#ifndef FIBER_ASM_H
#define FIBER_ASM_H

#include <stddef.h>

typedef void *StackPtr;

void fiber_asm_switch(StackPtr *from, StackPtr to);

/*
 * before this function is called,
 * an array containing the arguments is written onto the stack
 * (padded to fit stack alignment)
 * followed by the address of the function to be invoked,
 * followed by the size of argument array (plus padding)
 */ 
void fiber_asm_push_invoker(StackPtr *);

void fiber_asm_exec_on_stack(StackPtr, FiberFunc, const void *);

#endif
