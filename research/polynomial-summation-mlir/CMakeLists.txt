cmake_minimum_required(VERSION 3.20)
project(polynomial-summation-p1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Separate research build. Never included from the production CMake graph.
get_filename_component(P1_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
enable_testing()
add_subdirectory("${P1_ROOT}/research/polynomial-summation" p0)

add_library(polynomial-cfg STATIC
  "${P1_ROOT}/lib/Systolic/Analysis/MLIRPolynomialRecurrence.cpp"
  "${P1_ROOT}/lib/Systolic/Analysis/SymbolicPolynomial.cpp")
target_include_directories(polynomial-cfg PUBLIC "${P1_ROOT}/include")
target_include_directories(polynomial-cfg SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
target_compile_options(polynomial-cfg PRIVATE -Wall -Wextra -Wpedantic)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(polynomial-cfg PUBLIC -fno-rtti)
endif()
target_link_libraries(polynomial-cfg PUBLIC polynomial-summation
  MLIRAnalysis MLIRArithDialect MLIRControlFlowDialect MLIRFuncDialect
  MLIRControlFlowInterfaces MLIRSideEffectInterfaces MLIRIR MLIRSupport)

add_executable(polynomial-cfg-report PolynomialCFGReport.cpp)
target_link_libraries(polynomial-cfg-report PRIVATE polynomial-cfg
  MLIRParser MLIRMemRefDialect MLIRSCFDialect)

add_executable(polynomial-cfg-test "${P1_ROOT}/test/polynomial-summation-mlir/PolynomialCFGTest.cpp")
target_link_libraries(polynomial-cfg-test PRIVATE polynomial-cfg
  MLIRParser MLIRMemRefDialect MLIRSCFDialect)
add_test(NAME polynomial-cfg-identities
  COMMAND polynomial-cfg-test "${P1_ROOT}/test/polynomial-summation-mlir/Inputs")

add_executable(polynomial-cfg-extui-nneg-test
  "${P1_ROOT}/test/polynomial-summation-mlir/ExtUINNegTest.cpp")
target_link_libraries(polynomial-cfg-extui-nneg-test PRIVATE polynomial-cfg MLIRParser)
add_test(NAME polynomial-cfg-extui-nneg
  COMMAND polynomial-cfg-extui-nneg-test
    "${P1_ROOT}/test/polynomial-summation-mlir/Inputs/extui-nneg.mlir")

add_executable(polynomial-cfg-symbolic-test "${P1_ROOT}/test/polynomial-summation-p2a/SymbolicCFGTest.cpp")
target_link_libraries(polynomial-cfg-symbolic-test PRIVATE polynomial-cfg MLIRParser MLIRMemRefDialect)
add_test(NAME polynomial-cfg-symbolic
  COMMAND polynomial-cfg-symbolic-test "${P1_ROOT}/test/polynomial-summation-p2a/Inputs"
    "${P1_ROOT}/test/polynomial-summation-mlir/Inputs/extui-nneg.mlir")

find_program(P1_MLIR_OPT mlir-opt HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
find_program(P1_FILECHECK FileCheck HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
find_program(P1_LIT llvm-lit HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED)
find_program(P1_NOT not HINTS "${LLVM_TOOLS_BINARY_DIR}" "/usr/lib/llvm-18/bin" REQUIRED)
add_test(NAME polynomial-cfg-independent-oracle
  COMMAND "${Python3_EXECUTABLE}"
    "${P1_ROOT}/test/polynomial-summation-mlir/differential.py"
    --driver $<TARGET_FILE:polynomial-cfg-report> --mlir-opt "${P1_MLIR_OPT}")
configure_file(lit.site.cfg.py.in lit/lit.site.cfg.py @ONLY)
add_test(NAME polynomial-cfg-lit COMMAND "${P1_LIT}" -v "${CMAKE_CURRENT_BINARY_DIR}/lit")
add_test(NAME polynomial-cfg-symbolic-oracle
  COMMAND "${Python3_EXECUTABLE}" "${P1_ROOT}/test/polynomial-summation-p2a/differential.py"
    --driver $<TARGET_FILE:polynomial-cfg-report> --mlir-opt "${P1_MLIR_OPT}")
configure_file(p2a-lit.site.cfg.py.in p2a-lit/lit.site.cfg.py @ONLY)
add_test(NAME polynomial-cfg-symbolic-lit COMMAND "${P1_LIT}" -v "${CMAKE_CURRENT_BINARY_DIR}/p2a-lit")
add_custom_target(check-polynomial-cfg
  COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure
  DEPENDS polynomial-cfg-report polynomial-cfg-test polynomial-cfg-extui-nneg-test
    polynomial-cfg-symbolic-test
    polynomial-summation-test polynomial-summation-driver)
