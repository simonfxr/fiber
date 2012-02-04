#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fiber.h"


typedef struct Generator Generator;

typedef struct GeneratorArgs GeneratorArgs;

typedef void (*GeneratorF)(const GeneratorArgs *);

typedef unsigned char byte;

typedef enum {
    GenActive,
    GenDrained,
} GenState;

struct Generator {
    Fiber *fiber;
    GenState state;
    void *ret;
    Fiber *caller;
    byte stack[];
};

struct GeneratorArgs {
    Generator *gen;
    GeneratorF next;
    byte state[];
};

void gen_invoke(const void *args);

Generator *gen_new(size_t stack_size, GeneratorF next, size_t state_size, const void *state) {
    Generator *gen = malloc(offsetof(Generator, stack[stack_size]));
    if (gen == 0)
        return 0;

    gen->fiber = fiber_init(&gen->stack[0], stack_size);

    if (gen->fiber == 0) {
        free(gen);
        return 0;
    }

    gen->state = GenActive;
    gen->ret = 0;
    gen->caller = 0;

    const size_t size = offsetof(GeneratorArgs, state[state_size]);
    const size_t asize = (size + (STACK_ALIGNMENT - 1)) & ~(STACK_ALIGNMENT - 1);
    byte data[asize];
    GeneratorArgs *args = (GeneratorArgs *) ((((uintptr_t) data) + (STACK_ALIGNMENT - 1)) & ~(STACK_ALIGNMENT - 1));
    
    args->gen = gen;
    args->next = next;
    memcpy(args + offsetof(GeneratorArgs, state[0]), state, state_size);
    fiber_push_return(gen->fiber, gen_invoke, args, size);
    return gen;
}

int gen_yield(Generator *gen, void *value) {
    gen->state = GenActive;
    gen->ret = value;
    fiber_switch(gen->fiber, gen->caller);
    return gen->ret == 0;
}

void gen_finish(Generator *gen) {
    gen->state = GenDrained;
    fiber_switch(gen->fiber, gen->caller);
    abort();
}

void gen_invoke(const void *args0) {
    const GeneratorArgs *args = (const GeneratorArgs *) args0;
    args->next(args);
    gen_finish(args->gen);
}

int gen_next(Fiber *caller, Generator *gen, void **val) {
    if (gen->state == GenActive) {
        gen->caller = caller;
        fiber_switch(caller, gen->fiber);
    }

    if (gen->state == GenDrained)
        return 0;

    *val = gen->ret;
    gen->ret = 0;
    return 1;
}

void gen_close(Fiber *caller, Generator *gen) {
    if (gen->state == GenActive) {
        gen->caller = caller;
        gen->ret = (void *) (intptr_t) -1;
        fiber_switch(caller, gen->fiber);
    }
}

void gen_destroy(Fiber *caller, Generator *gen) {
    gen_close(caller, gen);
    free(gen);
}

void fibs_gen(const GeneratorArgs *gen) {
    int f0 = 0;
    int f1 = 1;
    gen_yield(gen->gen, &f0);
    for (;;) {
        if (!gen_yield(gen->gen, &f1))
            break;
        int t = f0;
        f0 = f1;
        f1 += t;
    }
}

typedef struct {
    int n;
    Generator *gen;
} TakeState;

void take_gen(const GeneratorArgs *gen) {
    TakeState *state = (TakeState *) gen->state;
    while (state->n --> 0) {
        void *value;
        if (!gen_next(gen->gen->fiber, state->gen, &value))
            break;
        if (!gen_yield(gen->gen, value))
            break;
    }

    gen_destroy(gen->gen->fiber, state->gen);
}

Generator *take(int n, Generator *gen) {
    TakeState st;
    st.n = n;
    st.gen = gen;
    return gen_new(4096, take_gen, sizeof st, &st);
}

int main(void) {
    Fiber main;
    fiber_init_toplevel(&main);
    Generator *fibs = take(20, gen_new(4096, fibs_gen, 0, NULL));
    int *value;
    while (gen_next(&main, fibs, (void **) &value))
        printf("value: %d\n", *value);

    gen_destroy(&main, fibs);
    return 0;
}
