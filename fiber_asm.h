#ifndef FIBER_ASM_H
#define FIBER_ASM_H

typedef void *StackPtr;

extern void fiber_asm_switch(Regs *from, Regs *to);

/*
 * before this function is called,
 * an array containing the arguments is written onto the stack
 * (padded to fit stack alignment)
 * followed by old sp
 * followed by pointer to args
 * followed by function to call
 */ 
extern void fiber_asm_invoke(void);

extern void fiber_asm_exec_on_stack(StackPtr, FiberFunc, void *);

#endif
