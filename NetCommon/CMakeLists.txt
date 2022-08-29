cmake_minimum_required(VERSION 2.6)
set(CMAKE_CXX_STANDARD 17)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
 
# Link runTests with what we want to test and the GTest and pthread library
add_executable(executeTests net_test.cpp)
target_link_libraries(executeTests ${GTEST_LIBRARIES} pthread)