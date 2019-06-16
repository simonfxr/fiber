cmake_minimum_required(VERSION 3.10)

set(FIBER_STANDALONE_PROJECT FALSE)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(FIBER_STANDALONE_PROJECT TRUE)
endif()

if(FIBER_STANDALONE_PROJECT)
  project(fiber LANGUAGES C)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/cmake-utils/cmake-utils.cmake")

cmu_configure()

if(NOT DEFINED FIBER_SHARED)
  set(FIBER_SHARED "${BUILD_SHARED_LIBS}")
endif()

set(FIBER_SHARED "${FIBER_SHARED}" CACHE BOOL "build shared fiber library")

set(FIBER_ASM_CHECK_ALIGNMENT
    True
    CACHE BOOL "add extra stack alignment checks to asm functions")

set(FIBER_M32 False CACHE BOOL "force 32bit compile on x86 via -m32")

set(FIBER_HAVE_LTO ${CMU_HAVE_IPO})

set(FIBER_LTO ${FIBER_HAVE_LTO} CACHE BOOL "enable lto")

if(FIBER_M32 OR CMU_BITS_32)
  set(FIBER_BITS_32 True)
elseif(CMU_BITS_64)
  set(FIBER_BITS_64 True)
endif()

set(cflags)
set(priv_cflags)
set(defines)
set(ldflags)
set(asm_lang "ASM")

if(CMU_OS_FREEBSD AND FIBER_BITS_32)
  list(APPEND defines -DFIBER_STACK_ALIGNMENT=4)
endif()

if(CMU_COMP_GNUC)
  list(APPEND priv_cflags -Wall -Wextra)
  if(FIBER_STANDALONE_PROJECT)
    list(APPEND cflags ${CMU_FLAGS_FP_IEEE})
  endif()
endif()

if(FIBER_ASM_CHECK_ALIGNMENT)
  list(APPEND defines "-DFIBER_ASM_CHECK_ALIGNMENT=1")
endif()

set(asm_sources)
if(CMU_OS_POSIX AND CMU_ARCH_X86 AND FIBER_BITS_64)
  set(asm_sources src/fiber_asm_amd64_sysv.S)
elseif(CMU_OS_POSIX AND CMU_ARCH_X86 AND FIBER_BITS_32)
  set(asm_sources src/fiber_asm_x86_cdecl.S)
elseif(CMU_OS_POSIX AND CMU_ARCH_ARM AND FIBER_BITS_64)
  set(asm_sources src/fiber_asm_aarch64_apcs.S)
elseif(CMU_OS_POSIX AND CMU_ARCH_ARM AND FIBER_BITS_32)
  set(asm_sources src/fiber_asm_arm32_eabi.S)
elseif(CMU_OS_WINDOWS AND CMU_ARCH_X86 AND FIBER_BITS_64)
  if(CMU_COMP_MSVC)
    set(asm_lang "ASM_MASM")
    set(asm_sources src/fiber_asm_amd64_win64.asm)
  elseif(CMU_COMP_GNUC)
    set(asm_sources src/fiber_asm_amd64_win64.S)
  endif()
elseif(CMU_OS_WINDOWS AND CMU_ARCH_X86 AND FIBER_BITS_32)
  if(CMU_COMP_MSVC)
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
elseif(CMU_OS_POSIX AND CMU_ARCH_RISCV AND FIBER_BITS_64)
  set(asm_sources src/fiber_asm_riscv64_elf.S)
elseif(CMU_OS_POSIX AND CMU_ARCH_PPC AND FIBER_BITS_64 AND CMU_LITTLE_ENDIAN)
  set(asm_sources src/fiber_asm_ppc64le_elf2.S)
endif()

if(CMU_COMP_MSVC)
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

set(_fiber_lib_mode)
if(FIBER_SHARED)
  set(_fiber_lib_mode SHARED)
else()
  set(_fiber_lib_mode STATIC)
endif()

add_library(fiber ${_fiber_lib_mode} ${asm_sources} src/fiber.c)
target_include_directories(fiber PUBLIC include)
target_include_directories(fiber PRIVATE src)
target_link_libraries(fiber PUBLIC header-utils::header-utils)
target_compile_definitions(fiber PRIVATE ${defines})
if(FIBER_LTO)
  set_property(TARGET fiber PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

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
    if(CMU_COMP_MSVC)
      target_compile_definitions(${tgt} PRIVATE _CRT_SECURE_NO_WARNINGS=1)
    endif()
    if(FIBER_LTO)
      set_property(TARGET ${tgt} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
  endmacro()

  add_test_exec(hello test/hello.c)
  add_test_exec(test_basic test/basic.c)
  add_test_exec(test_coop test/coop.c)
  add_test_exec(test_generators test/generators.c)
  add_test_exec(test_fp_stress test/fp_stress.c)
endif()
