#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fiber.h"

typedef struct Generator Generator;
typedef struct GeneratorArgs GeneratorArgs;
typedef void (*GeneratorF)(GeneratorArgs *);
typedef unsigned char byte;
typedef union Value Value;

#define STACK_SIZE 4096ul

typedef enum {
    GenActive,
    GenDrained,
} GenState;

union Value {
    long integer;
    unsigned long uinteger;
    double dpfp;
    void *ptr;
};

struct Generator {
    Fiber fiber;
    GenState state;
    Value ret;
    int closing;
    Fiber *caller;
};

struct GeneratorArgs {
    Generator *gen;
    GeneratorF next;
    byte state[];
};

void gen_invoke(void *args);

Generator *gen_new(size_t stack_size, GeneratorF next, size_t state_size, void *state) {
    Generator *gen = malloc(sizeof *gen);
    if (gen == 0)
        return 0;

    fiber_alloc(&gen->fiber, stack_size, true);

    gen->state = GenActive;
    gen->ret.ptr = 0;
    gen->caller = 0;

    GeneratorArgs *args;
    const size_t size = offsetof(GeneratorArgs, state[state_size]);
    fiber_reserve_return(&gen->fiber, gen_invoke, (void **) &args, size);

    args->gen = gen;
    args->next = next;
    memcpy(&args->state[0], state, state_size);
    return gen;
}

int gen_yield(Generator *gen, Value *value) {
    gen->state = GenActive;
    gen->ret = *value;
    fiber_switch(&gen->fiber, gen->caller);
    *value = gen->ret;
    gen->ret.ptr = 0;
    return !gen->closing;
}

void gen_finish(Generator *gen) {
    gen->state = GenDrained;
    fiber_switch(&gen->fiber, gen->caller);
}

void gen_invoke(void *args0) {
    GeneratorArgs *args = (GeneratorArgs *) args0;
    args->next(args);
    gen_finish(args->gen);
}

int gen_next(Fiber *caller, Generator *gen, Value *value) {

    switch (gen->state) {
    case GenActive:
        gen->caller = caller;
        fiber_switch(caller, &gen->fiber);

        switch (gen->state) {
        case GenActive:
            *value = gen->ret;
            return 1;
        case GenDrained:
            return 0;
        default:
            abort();
        }
        
    case GenDrained:
        return 0;

    default:
        abort();
    }
}

void gen_close(Fiber *caller, Generator *gen) {
    gen->closing = 1;
    Value value;
    value.ptr = 0;
    gen_next(caller, gen, &value);
}

void gen_destroy(Fiber *caller, Generator *gen) {
    gen_close(caller, gen);
    fiber_destroy(&gen->fiber);
    free(gen);
}

void fibs_gen(GeneratorArgs *gen) {
    int f0, f1;
    f0 = 0;
    f1 = 1;
    Value val;
    val.integer = f0;
    gen_yield(gen->gen, &val);
    for (;;) {
        val.integer = f1;
        if (!gen_yield(gen->gen, &val))
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

void take_gen(GeneratorArgs *gen_args) {
    TakeState *state = (TakeState *) gen_args->state;
    Generator *gen = gen_args->gen;

    while (state->n --> 0) {
        Value value;
        value.ptr = 0;
        if (!gen_next(&gen->fiber, state->gen, &value))
            break;
        if (!gen_yield(gen, &value))
            break;
    }

    gen_destroy(&gen->fiber, state->gen);
}

Generator *take(int n, Generator *gen) {
    TakeState st;
    st.n = n;
    st.gen = gen;
    return gen_new(STACK_SIZE, take_gen, sizeof st, &st);
}

int main(void) {
    Fiber main_fbr;
    fiber_init_toplevel(&main_fbr);

    Generator *allFibs = gen_new(STACK_SIZE, fibs_gen, 0, NULL);
    Generator *fibs = take(20, allFibs);

    Value value;
    value.ptr = 0;
    while (gen_next(&main_fbr, fibs, &value))
        printf("value: %d\n", (int) value.integer);
    fprintf(stderr, "generator empty\n");
    gen_destroy(&main_fbr, fibs);
    return 0;
}
