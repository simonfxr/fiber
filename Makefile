
COMP_C ?= clang
COMP_CXX ?= clang++
COMP_ASM ?= yasm

CC := $(COMP_C)
CXX := $(COMP_CXX)
ASM := $(COMP_ASM)

DEBUG_CFLAGS = -ggdb -fstack-protector

CFLAGS := -Wall -Wextra -std=c99 $(EXTRA_CFLAGS)
LDFLAGS := $(EXTRA_LDFLAGS)
ASMFLAGS := -f elf64 -g dwarf2 $(EXTRA_ASMFLAGS)

all: test

test: test.o fiber.o fiber_asm.o

%.o: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<
