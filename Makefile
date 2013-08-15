
COMP_C ?= gcc
COMP_CXX ?= g++
COMP_ASM ?= yasm

CC := $(COMP_C)
CXX := $(COMP_CXX)
ASM := $(COMP_ASM)

# DEBUG_CFLAGS := 
# DEBUG_ASMFLAGS :=

DEBUG_CFLAGS ?= -ggdb -fstack-protector
DEBUG_ASMFLAGS ?= -g dwarf2 

OPT_CFLAGS := -O3 -march=native

CFLAGS := -Wall -Wextra -std=c99 -DFIBER_BITS64 \
	$(OPT_CFLAGS) \
	$(EXTRA_CFLAGS)

LDFLAGS := $(EXTRA_LDFLAGS)
ASMFLAGS := -f elf64 $(EXTRA_ASMFLAGS)

DYN_CFLAGS := $(CFLAGS) -fPIC -fvisibility=hidden
DYN_LDFLAGS := $(LDFLAGS) -shared

all: test test2

test: libfiber.so test.o
	gcc $(LDFLAGS) -L. -lfiber -o $@ test.o

test2: libfiber.so test2.o
	gcc $(LDFLAGS) -L. -lfiber -o $@ test2.o

fiber_asm.o: fiber_asm_linux_amd64.asm
	$(ASM) $(ASMFLAGS) -o $@ $^

%.o: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $^

fiber_shared.o: fiber.c
	gcc -c $(DYN_CFLAGS) -o $@ $^

libfiber.so: fiber_shared.o fiber_asm.o
	gcc $(DYN_LDFLAGS) -o $@ $^

clean:
	- rm test test2 *.o libfiber.so

.PHONY: clean
