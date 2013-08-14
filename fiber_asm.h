#ifndef FIBER_ASM_H
#define FIBER_ASM_H

#include <stddef.h>

typedef void *StackPtr;

void fiber_asm_switch(Regs *from, Regs *to);

/*
 * before this function is called,
 * an array containing the arguments is written onto the stack
 * (padded to fit stack alignment)
 * followed by old sp
 * followed by pointer to args
 * followed by function to call
 */ 
void fiber_asm_invoke(void);

void fiber_asm_exec_on_stack(StackPtr, FiberFunc, void *);

#endif
