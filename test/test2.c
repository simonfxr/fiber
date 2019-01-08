#define __STDC_FORMAT_MACROS 1

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fiber/fiber.h>

typedef struct Generator Generator;
typedef struct GeneratorArgs GeneratorArgs;
typedef void (*GeneratorF)(GeneratorArgs *);
typedef union Value Value;

#define STACK_SIZE (4096ul * 128)

typedef enum
{
    GenActive,
    GenDrained,
} GenState;

union Value
{
    uint32_t u32;
    uint64_t u64;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    void *ptr;
};

struct Generator
{
    Fiber fiber;
    GenState state;
    Value ret;
    bool closing;
    Fiber *caller;
};

struct GeneratorArgs
{
    Generator *gen;
    GeneratorF next;
    char state[];
};

static void
gen_invoke(void *args);

static void
gen_finish(Fiber *self, void *args);

static inline void
value_reset(Value *val)
{
    memset(val, 0, sizeof *val);
}

static Generator *
gen_new(size_t stack_size,
        GeneratorF next,
        const void *state,
        size_t state_size)
{
    Generator *gen = malloc(sizeof *gen);
    if (!gen)
        return NULL;

    fiber_alloc(&gen->fiber, stack_size, gen_finish, gen, FIBER_FLAG_GUARD_LO);

    gen->closing = false;
    gen->state = GenActive;
    value_reset(&gen->ret);
    gen->caller = NULL;

    GeneratorArgs *args;
    const size_t size = offsetof(GeneratorArgs, state[0]) + state_size;
    fiber_reserve_return(&gen->fiber, gen_invoke, (void **) &args, size);

    args->gen = gen;
    args->next = next;
    memcpy(&args->state[0], state, state_size);
    return gen;
}

static bool
gen_yield(Generator *gen, Value *value)
{
    gen->state = GenActive;
    gen->ret = *value;
    fiber_switch(&gen->fiber, gen->caller);
    *value = gen->ret;
    value_reset(&gen->ret);
    return !gen->closing;
}

static void
gen_finish(Fiber *self, void *args)
{
    (void) self;
    Generator *gen = (Generator *) args;
    gen->state = GenDrained;
    fiber_switch(&gen->fiber, gen->caller);
}

static void
gen_invoke(void *args0)
{
    GeneratorArgs *args = (GeneratorArgs *) args0;
    args->next(args);
}

static bool
gen_next(Fiber *caller, Generator *gen, Value *value)
{
    switch (gen->state) {
    case GenActive:
        gen->caller = caller;
        fiber_switch(caller, &gen->fiber);

        switch (gen->state) {
        case GenActive:
            *value = gen->ret;
            return true;
        case GenDrained:
            return false;
        default:
            abort();
        }

    case GenDrained:
        return false;

    default:
        abort();
    }
}

static void
gen_close(Fiber *caller, Generator *gen)
{
    gen->closing = true;
    Value value;
    gen_next(caller, gen, &value);
}

static void
gen_destroy(Fiber *caller, Generator *gen)
{
    gen_close(caller, gen);
    fiber_destroy(&gen->fiber);
    free(gen);
}

static void
fibs_gen(GeneratorArgs *gen)
{
    uint64_t f0, f1;
    f0 = 0;
    f1 = 1;
    Value val;
    val.u64 = f0;
    gen_yield(gen->gen, &val);
    for (;;) {
        val.u64 = f1;
        if (!gen_yield(gen->gen, &val))
            break;
        uint64_t t = f0;
        f0 = f1;
        f1 += t;
    }
}

typedef struct
{
    size_t n;
    Generator *gen;
} TakeState;

static void
take_gen(GeneratorArgs *gen_args)
{
    TakeState *state = (TakeState *) gen_args->state;
    Generator *gen = gen_args->gen;

    while (state->n-- > 0) {
        Value value;
        if (!gen_next(&gen->fiber, state->gen, &value))
            break;
        if (!gen_yield(gen, &value))
            break;
    }
}

static Generator *
take(size_t n, Generator *gen)
{
    TakeState st;
    st.n = n;
    st.gen = gen;
    return gen_new(STACK_SIZE, take_gen, &st, sizeof st);
}

int
main(void)
{
    Fiber main_fbr;
    fiber_init_toplevel(&main_fbr);

    Generator *allFibs = gen_new(STACK_SIZE, fibs_gen, NULL, 0);
    Generator *fibs = take(20, allFibs);

    Value value;
    value.ptr = 0;
    while (gen_next(&main_fbr, fibs, &value))
        printf("value: %" PRIu64 "\n", value.u64);
    fprintf(stderr, "generator empty\n");
    return 0;
}
