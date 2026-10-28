cmake_minimum_required(VERSION 3.20)
project(polynomial-summation-p0 LANGUAGES CXX)

# Deliberately standalone: the production CMake graph does not include this.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
find_package(Boost REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_library(polynomial-summation STATIC
  "${PROJECT_ROOT}/lib/Systolic/Analysis/PolynomialSummation.cpp")
target_include_directories(polynomial-summation PUBLIC
  "${PROJECT_ROOT}/include" ${Boost_INCLUDE_DIRS})
target_compile_options(polynomial-summation PRIVATE -Wall -Wextra -Wpedantic)

add_executable(polynomial-summation-driver PolynomialSummationDriver.cpp)
target_link_libraries(polynomial-summation-driver PRIVATE polynomial-summation)
add_executable(polynomial-summation-test
  "${PROJECT_ROOT}/test/polynomial-summation/PolynomialSummationTest.cpp")
target_link_libraries(polynomial-summation-test PRIVATE polynomial-summation)

option(POLYNOMIAL_SANITIZERS "Enable address/undefined behavior sanitizers" OFF)
if(POLYNOMIAL_SANITIZERS)
  foreach(target polynomial-summation polynomial-summation-driver polynomial-summation-test)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endforeach()
endif()

enable_testing()
add_test(NAME polynomial-identities COMMAND polynomial-summation-test)
add_test(NAME polynomial-independent-oracle
  COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_ROOT}/test/polynomial-summation/differential.py"
    --driver $<TARGET_FILE:polynomial-summation-driver>)
