cmake_minimum_required(VERSION 3.10)

set(FIBER_STANDALONE_PROJECT FALSE)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(FIBER_STANDALONE_PROJECT TRUE)
endif()

if(FIBER_STANDALONE_PROJECT)
  project(fiber LANGUAGES C)
endif()

if(NOT DEFINED FIBER_SHARED)
  set(FIBER_SHARED "${BUILD_SHARED_LIBS}")
endif()

set(FIBER_SHARED "${FIBER_SHARED}" CACHE BOOL "build shared fiber library")

set(FIBER_ASM_CHECK_ALIGNMENT True
    CACHE BOOL "add extra stack alignment checks to asm functions")

set(FIBER_M32 False CACHE BOOL "force 32bit compile on x86 via -m32")

message(STATUS CMAKE_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS CMAKE_C_COMPILER_ID="${CMAKE_C_COMPILER_ID}")
message(STATUS CMAKE_C_SIZEOF_DATA_PTR="${CMAKE_C_SIZEOF_DATA_PTR}")
message(STATUS CMAKE_C_COMPILER_ID="${CMAKE_C_COMPILER_ID}")
message(STATUS CMAKE_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}")

if(FIBER_M32 OR CMAKE_C_SIZEOF_DATA_PTR STREQUAL 4)
  set(FIBER_BITS32 True)
elseif(CMAKE_C_SIZEOF_DATA_PTR STREQUAL 8)
  set(FIBER_BITS64 True)
else()
  message(ERROR "CMAKE_C_SIZEOF_DATA_PTR invalid: ${CMAKE_C_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64|(^i.86$)")
  set(FIBER_X86 True)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|^arm")
  set(FIBER_ARM True)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv")
  set(FIBER_RISCV True)
else()
  message(
    ERROR
    "fiber: architecture not supported: CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}"
    )
endif()

set(cflags)
set(priv_cflags)
set(defines)
set(ldflags)
set(asm_lang "ASM")

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND FIBER_BITS32)
  list(APPEND defines -DFIBER_STACK_ALIGNMENT=4)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU|Intel")
  list(APPEND priv_cflags -Wall -Wextra)
  if(FIBER_STANDALONE_PROJECT)
    if(FIBER_X86
       AND NOT
           CMAKE_SYSTEM_NAME
           STREQUAL
           "FreeBSD")
      list(APPEND cflags "-mfpmath=sse")
    endif()
    list(APPEND cflags "-ffp-contract=off")
  endif()
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)

if(FIBER_ASM_CHECK_ALIGNMENT)
  list(APPEND defines "-DFIBER_ASM_CHECK_ALIGNMENT=1")
endif()

set(asm_sources)
if(UNIX AND FIBER_X86 AND FIBER_BITS64)
  set(asm_sources src/fiber_asm_amd64_sysv.S)
elseif(UNIX AND FIBER_X86 AND FIBER_BITS32)
  set(asm_sources src/fiber_asm_x86_cdecl.S)
elseif(UNIX AND FIBER_ARM AND FIBER_BITS64)
  set(asm_sources src/fiber_asm_aarch64_apcs.S)
elseif(UNIX AND FIBER_ARM AND FIBER_BITS32)
  set(asm_sources src/fiber_asm_arm32_eabi.S)
elseif(WIN32 AND FIBER_X86 AND FIBER_BITS64)
  if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(asm_lang "ASM_MASM")
    set(asm_sources src/fiber_asm_amd64_win64.asm)
  elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU|Intel")
    set(asm_sources src/fiber_asm_amd64_win64.S)
  endif()
elseif(WIN32 AND FIBER_X86 AND FIBER_BITS32)
  if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(asm_lang "ASM_MASM")
    set(asm_sources src/fiber_asm_x86_win32.asm)
    set_source_files_properties(${asm_sources}
                                PROPERTIES
                                COMPILE_OPTIONS
                                "/safeseh")
  else()
    message(
      ERROR
      "compiling fiber under X86 WIN32 only supported when using MSVC toolchain"
      )
  endif()
elseif(UNIX AND FIBER_RISCV AND FIBER_BITS64)
  set(asm_sources src/fiber_asm_riscv64_elf.S)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  # make sure it gets passed to the assembler
  list(APPEND priv_cflags "/nologo")
endif()

if(NOT asm_sources)
  message(ERROR "fiber: this platform is not supported")
endif()

if(FIBER_M32)
  list(APPEND cflags "-m32")
  list(APPEND ldflags "-m32")
endif()

enable_language(${asm_lang})
if(NOT CMAKE_${asm_lang}_COMPILER_LOADED)
  set(have_asm True)
  message(ERROR "no compatible assembler found")
endif()

if(NOT TARGET header-utils::header-utils)
  add_subdirectory(deps/cxx-header-utils)
endif()

set(fiber_lib_mode)
if(FIBER_SHARED)
  set(fiber_lib_mode SHARED)
else()
  set(fiber_lib_mode STATIC)
endif()

add_library(fiber ${fiber_lib_mode} src/fiber.c ${asm_sources})
target_include_directories(fiber PUBLIC include)
target_include_directories(fiber PRIVATE src)
target_link_libraries(fiber PUBLIC header-utils::header-utils)
target_compile_definitions(fiber PRIVATE ${defines})

add_library(fiber::fiber ALIAS fiber)
target_compile_options(fiber PUBLIC ${cflags})
target_compile_options(fiber PRIVATE ${priv_cflags})

if(ldflags)
  if(COMMAND target_link_options)
    target_link_options(fiber PUBLIC ${ldflags})
  else()
    target_link_libraries(fiber PUBLIC ${ldflags})
  endif()
endif()

if(FIBER_SHARED)
  target_compile_definitions(fiber PUBLIC -DFIBER_SHARED=1)
  set_target_properties(fiber PROPERTIES C_VISIBILITY_PRESET hidden)
endif()

if(FIBER_STANDALONE_PROJECT)

  macro(add_test_exec tgt)
    add_executable(${tgt} ${ARGN})
    target_link_libraries(${tgt} fiber::fiber)
    target_compile_options(${tgt} PRIVATE ${cflags})
    if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
      target_compile_definitions(${tgt} PRIVATE _CRT_SECURE_NO_WARNINGS=1)
    endif()
  endmacro()

  add_test_exec(hello test/hello.c)
  add_test_exec(test_basic test/basic.c)
  add_test_exec(test_coop test/coop.c)
  add_test_exec(test_generators test/generators.c)
  add_test_exec(test_fp_stress test/fp_stress.c)
endif()
