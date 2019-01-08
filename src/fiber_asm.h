#ifndef FIBER_ASM_H
#define FIBER_ASM_H

extern void FIBER_CCONV
fiber_asm_switch(FiberRegs *from, FiberRegs *to);

/*
 * before this function is called,
 * an array containing the arguments is written onto the stack
 * (padded to fit stack alignment)
 * followed by old sp
 * followed by pointer to args
 * followed by function to call
 */
extern void FIBER_CCONV
fiber_asm_invoke(void);

extern void FIBER_CCONV
fiber_asm_exec_on_stack(void *, FiberFunc, void *);

#endif
