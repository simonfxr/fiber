#include <stdio.h>
#include <stdlib.h>

#include "fiber.h"


typedef struct Thread Thread;

typedef struct Sched Sched;

struct Sched {
    Thread main_thread;
    Fiber main_fiber;
    Thread *running;
    Thread *done; /* only next ptr used */
    size_t fuel;
};

struct Thread {
    Thread *prev, *next;
    void *stack;
    Fiber *fiber;
};

static Sched *sched;

void schedule() {
    Fiber *active = sched->running.fiber;

    if (sched->fuel == 0) {
        fiber_switch(active, &sched->main_fiber);
    } else {
        sched->running = sched->running.next;
        --sched->fuel;
        fiber_switch(active, sched->running.fiber);
    }
}

void thread_destroy(Thread *t) {
    free(fiber_stack(t.fiber));
    free(t);
}

void thread_end() {
    Thread *t = sched->running;
    t->prev->next = t->next;
    t->next->prev = t->prev;
    
    t->next = sched->done;
    t->prev = 0;
    sched->done = t;
    schedule();
}

typedef struct {
    void (*entry)(void);
} ThreadArgs;

void thread_exec(Fiber *fbr, const ThreadArgs *args) {
    printf("Thread started: %p\n", sched->running);
    args->entry();
    thread_end();
}

void thread_start(void (*func)(void)) {
    Thread *t = malloc(sizeof *t);
    void *stack = malloc(STACK_SIZE);
    t->fiber = fiber_init(stack, STACK_SIZE);
    assert(stack == fiber_stack(t->fiber));

    ThreadArgs args;
    args.entry = func;
    args.args = args;
    fiber_push_return(t->fiber, thread_exec, &args, sizeof args);
    
    t->next = sched->running->next;
    t->prev = sched->running;
    t->next->prev = t;
    t->prev->next = t;
}

void sched_init(Sched *s) {
    sched = s;
    s->main_thread.prev = &s->main_thread;
    s->main_thread.next = &s->main_thread;
    s->main_thread.fiber = &s->main_fiber;
    fiber_init_toplevel(&s->main_fiber);

    s->running = &s->main_thread;
    s->done = 0;
}

void put_str(const char *str) {
    puts(str);
}

void thread1() {
    for (;;) {
        put_str("thread1 running");
        
        schedule();
    }
}

void thread2() {
    for (;;) {
        put_str("thread2 running");
        
        schedule();
    }
}

void thread3() {
    int rounds = 10;
    while (rounds --> 0) {
        put_str("thread3 running");
        schedule();
    }
    
    put_str("thread3 exiting");
}

void main() {

    Sched s;
    sched_init(&s);
    s.fuel = 100;

    thread_start(thread1);
    thread_start(thread2);
    thread_start(thread3);

    while (sched->running->next != &sched->main_thread && sched->fuel > 0) {
        schedule();
        
        while (sched->done != 0) {
            Thread *t = sched->done;
            sched->done = sched->done->next;
            printf("thread exiting: %p\n", t);
            thread_destroy(t);
        }
    }
    
    return 0;
}
