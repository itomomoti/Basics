# set(CMAKE_VERBOSE_MAKEFILE 1)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF) ## on g++ this ensures: -std=c++11 and not -std=gnu++11

#### Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()
message("Building in ${CMAKE_BUILD_TYPE} mode")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -g -p")
set(CMAKE_CXX_FLAGS_RELEASE "-ggdb -Ofast -fstrict-aliasing -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -ggdb -Ofast -fstrict-aliasing -march=native")

project(Basics)

add_library(${PROJECT_NAME} STATIC
  BitsUtil.cpp
)
