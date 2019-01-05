set(FIBER_STANDALONE_PROJECT FALSE)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(FIBER_STANDALONE_PROJECT TRUE)
endif ()

if(FIBER_STANDALONE_PROJECT)
  cmake_minimum_required(VERSION 3.10)
  project(fiber LANGUAGES C)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/CMakeASM_NASMInformation.cmake")

set(can_use_assembler False)
enable_language(ASM_NASM)
if(CMAKE_ASM_NASM_COMPILER_LOADED)
  set(can_use_assembler True)
endif()

if(NOT can_use_assembler)
  message(FATAL_ERROR "can_use_assembler=${can_use_assembler}")
endif()

set(asm_sources)
set(fiber_defines)
if(UNIX AND CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
  set(asm_sources src/fiber_asm_x64_sysv.asm)
  set(fiber_defines -DFIBER_BITS64)
elseif(UNIX AND CMAKE_C_SIZEOF_DATA_PTR EQUAL 4)
  set(asm_sources src/fiber_asm_x86_cdecl.asm)
  set(fiber_defines -DFIBER_BITS32)
endif()

if(FIBER_STANDALONE_PROJECT)
  add_subdirectory(deps/cxx-header-utils)
endif()

add_library(fiber src/fiber.c ${asm_sources})
target_compile_definitions(fiber PUBLIC ${fiber_defines})
target_include_directories(fiber PUBLIC include src)
target_link_libraries(fiber header-utils)

if(FIBER_STANDALONE_PROJECT)
  add_executable(test test/test.c)
  target_link_libraries(test fiber)

  add_executable(test2 test/test2.c)
  target_link_libraries(test2 fiber)
endif()
