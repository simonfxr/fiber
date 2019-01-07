cmake_minimum_required(VERSION 3.10)

set(FIBER_STANDALONE_PROJECT FALSE)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(FIBER_STANDALONE_PROJECT TRUE)
endif ()

if(FIBER_STANDALONE_PROJECT)
  project(fiber LANGUAGES C)
endif()

if(NOT DEFINED FIBER_SHARED)
  set(FIBER_SHARED "${BUILD_SHARED_LIBS}")
endif()

set(FIBER_SHARED "${FIBER_SHARED}" CACHE BOOL "build shared fiber library")

set(FIBER_ASM_CHECK_ALIGNMENT True CACHE BOOL "add extra stack alignment checks to asm functions")

set(FIBER_M32 "force 32bit compile on x86 via -m32")

#include("${CMAKE_CURRENT_LIST_DIR}/cmake/CMakeASM_NASMInformation.cmake")

set(cflags)
set(defines)
set(ldflags)

if(FIBER_ASM_CHECK_ALIGNMENT)
  list(APPEND defines "-DFIBER_ASM_CHECK_ALIGNMENT=1")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GCC")
  list(APPEND cflags -Wall -Wextra)
endif()

set(asm_sources)
#set(asm_nasm False)
if(UNIX AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "(x86_64|amd64)" AND NOT FIBER_M32)
  set(asm_sources src/fiber_asm_x64_sysv.S)
  #set(asm_nasm True)
elseif(UNIX AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^i.86$" OR FIBER_M32)
  set(asm_sources src/fiber_asm_x86_cdecl.S)
  set(asm_dialect True)
elseif(UNIX AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  set(asm_sources src/fiber_asm_aarch64.S)
else()
  message(FATAL_ERROR "fiber: combination of system and architecture not supported!")
endif()

if(FIBER_M32)
  list(APPEND cflags "-m32")
  list(APPEND ldflags "-m32")
endif()

# set(asm_dialect ASM)
# if(asm_nasm)
#   set(asm_dialect ASM_NASM)
# endif()

enable_language(ASM)
if(NOT CMAKE_ASM_COMPILER_LOADED)
  message(FATAL_ERROR "no nasm compatible assembler found")
endif()

if(FIBER_STANDALONE_PROJECT)
  add_subdirectory(deps/cxx-header-utils)
endif()

message(STATUS CMAKE_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}")

set(fiber_lib_mode)
if(FIBER_SHARED)
  set(fiber_lib_mode SHARED)
else()
  set(fiber_lib_mode STATIC)
endif()

add_library(fiber ${fiber_lib_mode} src/fiber.c ${asm_sources})
target_include_directories(fiber PUBLIC include)
target_include_directories(fiber PRIVATE src)
target_link_libraries(fiber PUBLIC header-utils)
target_compile_definitions(fiber PRIVATE ${defines})

if(cflags)
  #if(asm_nasm)
  #  #exclude *.asm
  #  set_source_files_properties(src/fiber.c PROPERTIES COMPILE_OPTIONS "${cflags}")
  # else()
  target_compile_options(fiber PRIVATE ${cflags})
  # endif()
endif()

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
  add_executable(test0 test/test0.c)
  target_link_libraries(test0 fiber)
  target_compile_options(test0 PRIVATE ${cflags})

  add_executable(test test/test.c)
  target_link_libraries(test fiber)
  target_compile_options(test PRIVATE ${cflags})

  add_executable(test2 test/test2.c)
  target_link_libraries(test2 fiber)
  target_compile_options(test2 PRIVATE ${cflags})
endif()
