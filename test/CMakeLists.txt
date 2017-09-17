# set(CMAKE_VERBOSE_MAKEFILE 1)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) ## on g++ this ensures: -std=c++11 and not -std=gnu++11

#### Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()
message("Building in ${CMAKE_BUILD_TYPE} mode")


# set runtime directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# declare GTEST_ROOT
set(GTEST_ROOT ../../googletest/googletest/)

# Include files directory
include_directories(${GTEST_ROOT}/include/)

# Library directory
link_directories(${GTEST_ROOT}/build/)

# Generate executable file
add_executable(WBitsVec_Test ${CMAKE_CURRENT_SOURCE_DIR}/WBitsVec_Test.cpp ${CMAKE})
add_executable(BitVec_Test ${CMAKE_CURRENT_SOURCE_DIR}/BitVec_Test.cpp ${CMAKE})
add_executable(RankVec_Test ${CMAKE_CURRENT_SOURCE_DIR}/RankVec_Test.cpp ${CMAKE})
add_executable(SVec_Test ${CMAKE_CURRENT_SOURCE_DIR}/SVec_Test.cpp ${CMAKE})

add_executable(WBitsVec_Benchmark ${CMAKE_CURRENT_SOURCE_DIR}/WBitsVec_Benchmark.cpp ${CMAKE})
add_executable(BasicSearch_Benchmark ${CMAKE_CURRENT_SOURCE_DIR}/BasicSearch_Benchmark.cpp ${CMAKE})

# Link libraries
target_link_libraries(WBitsVec_Test Basics gtest gtest_main pthread)
target_link_libraries(BitVec_Test Basics gtest gtest_main pthread)
target_link_libraries(RankVec_Test Basics gtest gtest_main pthread)
target_link_libraries(SVec_Test Basics gtest gtest_main pthread)

target_link_libraries(WBitsVec_Benchmark Basics)
target_link_libraries(BasicSearch_Benchmark Basics)
