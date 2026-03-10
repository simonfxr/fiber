cmake_minimum_required(VERSION 3.10)

set(FIBER_STANDALONE_PROJECT FALSE)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(FIBER_STANDALONE_PROJECT TRUE)
endif()

if(FIBER_STANDALONE_PROJECT)
  project(fiber LANGUAGES C)
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_EXTENSIONS False)

  # Required under windows when building a dll and running tests
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/cmake-utils/cmake-utils.cmake")

if(FIBER_STANDALONE_PROJECT)
  cmu_configure()
endif()

option(FIBER_SHARED "build shared fiber library" ${BUILD_SHARED_LIBS})

option(FIBER_ASM_CHECK_ALIGNMENT "add extra stack alignment checks to asm functions" True)

option(FIBER_M32 "force 32bit compile on x86 via -m32" False)

if(NOT COMMAND check_ipo_supported)
  include(CheckIPOSupported)
endif()
check_ipo_supported(RESULT FIBER_HAVE_LTO)

option(FIBER_LTO "enable lto" False)

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

if(CMU_COMP_GNUC)
  list(APPEND priv_cflags -Wall -Wextra)
  if(FIBER_STANDALONE_PROJECT)
    list(APPEND cflags ${CMU_FLAGS_FP_IEEE})
  endif()
endif()

if(FIBER_ASM_CHECK_ALIGNMENT)
  list(APPEND defines "-DFIBER_ASM_CHECK_ALIGNMENT=1")
endif()

set(asm_sources False)
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
    set_source_files_properties(${asm_sources} PROPERTIES COMPILE_OPTIONS "/safeseh")
  else()
    set(asm_sources src/fiber_asm_x86_cdecl.S)
  endif()
elseif(CMU_OS_POSIX AND CMU_ARCH_RISCV)
  set(asm_sources src/fiber_asm_riscv_elf.S)
elseif(CMU_OS_POSIX AND CMU_ARCH_PPC AND FIBER_BITS_64 AND CMU_LITTLE_ENDIAN)
  set(asm_sources src/fiber_asm_ppc64le_elf2.S)
endif()

if(CMU_COMP_MSVC)
  # make sure it gets passed to the assembler
  list(APPEND priv_cflags "/nologo")
endif()

if(NOT asm_sources)
  message(FATAL_ERROR "fiber: this platform is not supported ARCH=${CMU_ARCH} BITS=${CMU_BITS} OS=${CMAKE_SYSTEM_NAME}")
endif()

if(FIBER_M32)
  list(APPEND cflags "-m32")
  list(APPEND ldflags "-m32")
endif()

enable_language(${asm_lang})
if(NOT CMAKE_${asm_lang}_COMPILER_LOADED)
  message(FATAL_ERROR "no compatible assembler found")
endif()

if(NOT DEFINED FIBER_SHARED)
  set(FIBER_SHARED "${BUILD_SHARED_LIBS}")
endif()

if(NOT TARGET header-utils::header-utils)
  add_subdirectory(deps/cxx-header-utils)
endif()

set(_fiber_lib_mode)
if(FIBER_OBJECT)
  set(_fiber_lib_mode OBJECT)
elseif(FIBER_SHARED)
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

target_compile_options(fiber PUBLIC ${cflags})
target_compile_options(fiber PRIVATE ${priv_cflags} ${CMU_FLAGS_W4})

if(ldflags)
  cmu_target_link_options(fiber PUBLIC ${ldflags})
endif()

if(FIBER_SHARED)
  target_compile_definitions(fiber PUBLIC -DFIBER_SHARED=1)
  set_target_properties(fiber PROPERTIES C_VISIBILITY_PRESET hidden)
endif()

if(FIBER_OBJECT)
  set_target_properties(fiber PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Set C standard on the target so it applies in both standalone and subdirectory
# mode. The standalone code path above sets CMAKE_C_STANDARD, but that does not
# propagate when fiber is consumed as a subdirectory.
set_property(TARGET fiber PROPERTY C_STANDARD 99)
set_property(TARGET fiber PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET fiber PROPERTY C_EXTENSIONS OFF)

if(CMU_COMP_MSVC AND NOT CMU_COMP_GNUC)
  # Pure MSVC (not clang-cl) needs /std:c11 to define __STDC_VERSION__, and
  # never defines __STDC__ without /Za. Both are required by cxx-header-utils
  # (hu/lang.h) for C language detection.
  set_property(TARGET fiber PROPERTY C_STANDARD 11)
  target_compile_definitions(fiber PRIVATE __STDC__=1)
endif()

if(FIBER_STANDALONE_PROJECT)
  enable_testing()
  add_subdirectory(test)
endif()
