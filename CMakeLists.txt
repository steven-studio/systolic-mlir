cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# 这个专案假设你已经 build 好 LLVM/MLIR (跟你 son-dialect 用的应该是同一份)。
# 用法:
#   mkdir build && cd build
#   cmake -G Ninja .. \
#     -DMLIR_DIR=$LLVM_BUILD_DIR/lib/cmake/mlir \
#     -DLLVM_DIR=$LLVM_BUILD_DIR/lib/cmake/llvm \
#     -DCMAKE_BUILD_TYPE=Release
#   ninja
# ---------------------------------------------------------------------------

project(systolic-mlir LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MLIR REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/include)
link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools)
