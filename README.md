# Fiber: fast and lightweight cross-platform coroutines

- Fast by using custom assembly per platform: switching coroutines is just a few memory/register swaps (e.g. on amd64: 9 registers)
- Minimal: about 300 lines of C and under 100 lines of assembly per platform

## Usecases

- replace your state machine with something more readable
- replace your event loop callbacks with a more natural thread-like control flow
- lazy streams and generators
- agent systems (e.g. AI in games)
- cooperative threading in general (see `test1.c` for an example)

## Supported platforms

- ELF Targets, 32 or 64-bit, running on x86 or ARM (e.g. Linux, BSD's)
- Windows 32 or 64-bit x86
- MacOS 64bit x86 and Apple Silicon (special thanks to [@rsmmr](https://github.com/rsmmr) for Apple Silicon support!)

Tested on the following machines:

- ArchLinux: clang/gcc: 32 and 64-bit builds
- FreeBSD 11: clang: 32 and 64-bit builds
- Windows x86: MSVC: 32 and 64-bit builds
- Windows x86: MinGW: 64-bit builds
- RaspberryPI (armv6): gcc builds
- RaspberryPI 3 (aarch64): gcc/clang builds
- MacOS 10.14: clang 64-bit builds

## Caveats

- Its possible to use very small stacks, but it is not recommended, since signal handlers or linkers might run code on your coroutine stack at unexpected times, better use a more conservative stack size of e.g 32kb.
- Moving fibers from one OS thread to another is problematic: (caching of thread locals and platform specific use of registers)
- If you use C++ exceptions: The top stack frame of each coroutine should catch and rethrow all exceptions in a toplevel fiber (this also applies to Windows SEH)
- Interleaving setjmp/longjmp with fiber switching is not allowed
- Debuggers are confused by the switching of stacks, in particular backtraces only show the frames of the currently active fiber.

## Building

```shell
git clone https://github.com/simonfxr/fiber
git submodule update --init
mkdir build
cd build
cmake ..
make
```

Or integrate it in your cmake build

```cmake
add_subdirectory(path/to/fiber)
target_link_libraries(your-target fiber::fiber)
```

## Example

```c
#include <fiber/fiber.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    Fiber *caller;
    Fiber *self;
    const char *str;
} Args;

/* invoked if coroutine is called after it finished */
void
guard(Fiber *self, void *null)
{
    (void) self;
    (void) null;
    abort();
}

/* entry point */
void
coroutine(void *args0)
{
    Args *args = (Args *) args0;
    printf("From coroutine: %s\n", args->str);
    fiber_switch(args->self, args->caller);
}

int
main()
{
    Fiber main_fiber;
    fiber_init_toplevel(&main_fiber);
    Fiber crt;
    /* allocate a stack of 16kb, additionally add an unmapped page to detect overflows */
    fiber_alloc(&crt, 1024 * 16, guard, NULL, FIBER_FLAG_GUARD_LO);
    Args args;
    args.caller = &main_fiber;
    args.self = &crt;
    args.str = "Hello";
    /* push a new return stack frame, defines the entry point of our coroutine */
    fiber_push_return(&crt, coroutine, &args, sizeof args);
    /* arguments are copied into the coroutine, its safe to destroy them here */
    memset(&args, 0, sizeof args);
    /* run our coroutine */
    fiber_switch(&main_fiber, &crt);
    fiber_destroy(&crt);
    /* main_fiber does not have to be destroyed */
    return 0;
}
```
