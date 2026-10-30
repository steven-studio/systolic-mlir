cmake_minimum_required(VERSION 3.20)
project(polynomial-termination-p2b LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()
get_filename_component(P2B_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# This project is never added by the production build. P2a's build and APIs
# remain unchanged; its regression targets are reused as a subdirectory.
add_subdirectory("${P2B_ROOT}/research/polynomial-summation-mlir" recurrence)
find_path(P2B_Z3_INCLUDE z3++.h REQUIRED)
find_library(P2B_Z3_LIBRARY NAMES z3 REQUIRED)
add_library(polynomial-termination STATIC
  "${P2B_ROOT}/lib/Systolic/Analysis/MLIRPolynomialTermination.cpp"
  "${P2B_ROOT}/lib/Systolic/Analysis/PolynomialTerminationProof.cpp")
target_include_directories(polynomial-termination PRIVATE "${P2B_Z3_INCLUDE}")
target_link_libraries(polynomial-termination PUBLIC polynomial-cfg PRIVATE "${P2B_Z3_LIBRARY}")
target_compile_options(polynomial-termination PRIVATE -Wall -Wextra -Wpedantic)
add_executable(polynomial-termination-report PolynomialTerminationReport.cpp)
target_link_libraries(polynomial-termination-report PRIVATE polynomial-termination MLIRParser MLIRMemRefDialect MLIRSCFDialect)
add_executable(polynomial-termination-test "${P2B_ROOT}/test/polynomial-termination/TerminationTest.cpp")
target_include_directories(polynomial-termination-test PRIVATE "${P2B_ROOT}/lib/Systolic/Analysis")
target_link_libraries(polynomial-termination-test PRIVATE polynomial-termination MLIRParser MLIRMemRefDialect)
add_test(NAME polynomial-termination-certificates COMMAND polynomial-termination-test
  "${P2B_ROOT}/test/polynomial-termination/Inputs/loops.mlir")
set_tests_properties(polynomial-termination-certificates PROPERTIES TIMEOUT 60)
find_program(P2B_MLIR_OPT mlir-opt HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
find_program(P2B_MLIR_TRANSLATE mlir-translate HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
find_program(P2B_CLANG clang HINTS "${LLVM_TOOLS_BINARY_DIR}" "$ENV{HOME}/work/clang24-build/bin" REQUIRED)
find_program(P2B_LIT llvm-lit HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
find_program(P2B_FILECHECK FileCheck HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
find_program(P2B_NOT not HINTS "${LLVM_TOOLS_BINARY_DIR}" "/usr/lib/llvm-18/bin" REQUIRED)
add_test(NAME polynomial-termination-independent COMMAND "${Python3_EXECUTABLE}"
  "${P2B_ROOT}/test/polynomial-termination/differential.py"
  --driver $<TARGET_FILE:polynomial-termination-report> --mlir-opt "${P2B_MLIR_OPT}"
  --mlir-translate "${P2B_MLIR_TRANSLATE}" --clang "${P2B_CLANG}"
  --z3-library "${P2B_Z3_LIBRARY}")
set_tests_properties(polynomial-termination-independent PROPERTIES TIMEOUT 300)
configure_file(lit.site.cfg.py.in lit/lit.site.cfg.py @ONLY)
add_test(NAME polynomial-termination-lit COMMAND "${P2B_LIT}" -v "${CMAKE_CURRENT_BINARY_DIR}/lit")
set_tests_properties(polynomial-termination-lit PROPERTIES TIMEOUT 90)
add_custom_target(check-polynomial-termination
  COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure
  DEPENDS polynomial-termination-report polynomial-termination-test polynomial-cfg-report
    polynomial-cfg-test polynomial-cfg-extui-nneg-test polynomial-cfg-symbolic-test
    polynomial-summation-test polynomial-summation-driver)
