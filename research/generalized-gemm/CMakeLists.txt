cmake_minimum_required(VERSION 3.20)
project(generalized-gemm-g1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

# Research build. Never included from the production CMake graph, and the
# production build references none of these files.
get_filename_component(GG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
# The mutation test points this at a mutated copy of lib/GeneralizedGemm.
set(GG_LIB_DIR "${GG_ROOT}/lib/GeneralizedGemm" CACHE PATH "Library sources")
find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_path(GG_Z3_INCLUDE z3++.h REQUIRED)
find_library(GG_Z3_LIBRARY NAMES z3 REQUIRED)

add_library(generalized-gemm STATIC
  "${GG_LIB_DIR}/ProofStatus.cpp"
  "${GG_LIB_DIR}/SemanticTerm.cpp"
  "${GG_LIB_DIR}/SemanticImport.cpp"
  "${GG_LIB_DIR}/ConcreteEvaluator.cpp"
  "${GG_LIB_DIR}/SymbolicEvaluator.cpp"
  "${GG_LIB_DIR}/RelationalEncoding.cpp"
  "${GG_LIB_DIR}/SemanticPrinter.cpp")
target_include_directories(generalized-gemm PUBLIC "${GG_ROOT}/include")
target_include_directories(generalized-gemm SYSTEM PUBLIC
  ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
target_compile_options(generalized-gemm PRIVATE -Wall -Wextra -Wpedantic)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(generalized-gemm PUBLIC -fno-rtti)
endif()
target_link_libraries(generalized-gemm PUBLIC
  MLIRAffineToStandard MLIRSCFToControlFlow MLIRAffineDialect
  MLIRArithDialect MLIRControlFlowDialect MLIRFuncDialect MLIRMemRefDialect
  MLIRSCFDialect MLIRPass MLIRIR MLIRSupport MLIRParser MLIRLLVMDialect)

add_executable(generalized-gemm-report SemanticReport.cpp)
target_link_libraries(generalized-gemm-report PRIVATE generalized-gemm)

add_executable(generalized-gemm-unit test/unit/SemanticModelTest.cpp)
target_link_libraries(generalized-gemm-unit PRIVATE generalized-gemm)
add_test(NAME generalized-gemm-unit COMMAND generalized-gemm-unit)

# Z3 is a test-time dependency only: the library emits text.
add_executable(generalized-gemm-relational test/unit/RelationalAgreementTest.cpp)
target_include_directories(generalized-gemm-relational SYSTEM PRIVATE "${GG_Z3_INCLUDE}")
target_link_libraries(generalized-gemm-relational PRIVATE generalized-gemm "${GG_Z3_LIBRARY}")
add_test(NAME generalized-gemm-relational COMMAND generalized-gemm-relational)

find_program(GG_MLIR_OPT mlir-opt HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(GG_MLIR_TRANSLATE mlir-translate HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(GG_CLANG clang HINTS "$ENV{HOME}/work/clang24-build/bin" NO_DEFAULT_PATH REQUIRED)
find_program(GG_FILECHECK FileCheck HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(GG_NOT not HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(GG_LIT llvm-lit HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)

set(GG_FUZZ "${CMAKE_CURRENT_SOURCE_DIR}/test/fuzz")
add_test(NAME generalized-gemm-oracle-fuzz COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/differential.py" --driver $<TARGET_FILE:generalized-gemm-report>
  --programs 1500 --seed 1)
set_tests_properties(generalized-gemm-oracle-fuzz PROPERTIES TIMEOUT 300)
add_test(NAME generalized-gemm-native-differential COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/differential.py" --driver $<TARGET_FILE:generalized-gemm-report>
  --programs 120 --seed 2 --native --mlir-opt "${GG_MLIR_OPT}"
  --mlir-translate "${GG_MLIR_TRANSLATE}" --clang "${GG_CLANG}")
set_tests_properties(generalized-gemm-native-differential PROPERTIES TIMEOUT 600)

configure_file(test/lit/lit.site.cfg.py.in lit/lit.site.cfg.py @ONLY)
add_test(NAME generalized-gemm-lit COMMAND "${GG_LIT}" -v "${CMAKE_CURRENT_BINARY_DIR}/lit")
set_tests_properties(generalized-gemm-lit PROPERTIES TIMEOUT 120)

add_custom_target(check-generalized-gemm
  COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure
  DEPENDS generalized-gemm-report generalized-gemm-unit generalized-gemm-relational)
