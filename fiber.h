#ifndef FIBER_H
#define FIBER_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t FiberState;

static const FiberState FS_EXECUTING = 1;
static const FiberState FS_TOPLEVEL = 2;
static const FiberState FS_DONE = 4;

/*
 *
 * memory layout is as follows:
 * fiber_stack is a ptr to the beginning of an array
 * of byte size sz.
 * the array is used to allocate space for the Fiber structure to
 * keep track of some state.
 * stack grows downward so we place the Fiber struct at the end of the
 * array (aligned to word boundary (sizeof (void *)))
 *
 */ 
typedef struct {
    size_t cookie;
    size_t size;
    void *stack_ptr;
    FiberState state;
    uint16_t offset_to_stack;
} Fiber;

typedef void (*FiberFunc)(const void *);

Fiber *fiber_init(void *, size_t);

void fiber_init_toplevel(Fiber *);

void *fiber_stack(Fiber *);

int fiber_is_toplevel(Fiber *);

void fiber_switch(Fiber *from, Fiber *to);

void fiber_push_return(Fiber *, FiberFunc, const void *, size_t);

void fiber_exec_on(Fiber *active, Fiber *temp, FiberFunc, const void *, size_t);

#endif
