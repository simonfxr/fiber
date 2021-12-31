get_filename_component(target ${CMAKE_CURRENT_LIST_FILE} NAME)

string(REPLACE ".cmake" "" target "${target}")
string(REGEX REPLACE "^.*-([^-]*)\$" "\\1" cc "${target}")
string(REGEX REPLACE "-[^-]*\$" "" target "${target}")
string(REGEX REPLACE "-.*" "" CMAKE_SYSTEM_PROCESSOR "${target}")

if(target MATCHES linux)
  set(CMAKE_SYSTEM_NAME Linux)
elseif(target MATCHES "mingw|win")
  set(CMAKE_SYSTEM_NAME Windows)
else()
  message(FATAL_ERROR "Failed to detect CMAKE_SYSTEM_NAME from target ${target}")
  return()
endif()
set(CMAKE_CROSSCOMPILING TRUE)

if(cc STREQUAL "gcc")
  set(cxx "g++")
elseif(cc STREQUAL "cc")
  set(cxx "c++")
elseif(cc STREQUAL "icc")
  set(cxx "icpc")
else()
  set(cxx "${cc}++")
endif()

set(cross ${target}-)

set(CMAKE_AR ${cross}ar)
#set(CMAKE_ASM_COMPILER ${cross}as)
set(CMAKE_C_COMPILER ${cross}${cc})
set(CMAKE_CXX_COMPILER ${cross}${cxx})
set(CMAKE_LINKER ${cross}ld)
set(CMAKE_OBJCOPY ${cross}objcopy CACHE INTERNAL "")
set(CMAKE_RANLIB ${cross}ranlib CACHE INTERNAL "")
set(CMAKE_SIZE ${cross}size CACHE INTERNAL "")
set(CMAKE_STRIP ${cross}strip CACHE INTERNAL "")
set(CMAKE_GCOV ${cross}gcov CACHE INTERNAL "")


# Where to look for the target environment. (More paths can be added here)
set(CMAKE_FIND_ROOT_PATH /usr/${target})
set(CMAKE_INCLUDE_PATH /usr/include/${target})
set(CMAKE_LIBRARY_PATH /usr/lib/${target})
set(CMAKE_PROGRAM_PATH /usr/${target}/bin)

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

unset(cross)
unset(cxx)
unset(cc)
unset(target)
