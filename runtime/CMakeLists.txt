cmake_minimum_required(VERSION 3.13)

add_library(systolic_runtime STATIC
  systolic_runtime.cpp
)

target_include_directories(systolic_runtime
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_library(systolic_fpga_runtime STATIC
  dpti_runtime.cpp
  dpti_runtime.c
  dpti_fpga_transport.cpp
  dpti_ftdi.cpp
  fpga_matmul4x4.cpp
  fpga_matmul_tiled.cpp
)

target_include_directories(systolic_fpga_runtime
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBFTDI1 REQUIRED IMPORTED_TARGET libftdi1)

target_link_libraries(systolic_fpga_runtime
  PUBLIC
    PkgConfig::LIBFTDI1
)

add_executable(dpti-runtime-test
  test_dpti_runtime.cpp
)

target_link_libraries(dpti-runtime-test
  PRIVATE
    systolic_fpga_runtime
)

add_executable(dpti-ftdi-test
  test_dpti_ftdi.cpp
)

target_link_libraries(dpti-ftdi-test
  PRIVATE
    systolic_fpga_runtime
)

add_executable(fpga-matmul16x16-test
  test_fpga_matmul16x16.c
)

target_link_libraries(fpga-matmul16x16-test
  PRIVATE
    systolic_fpga_runtime
)

add_executable(dpti-16x16-test
  test_dpti_16x16.cpp
)

target_link_libraries(dpti-16x16-test
  PRIVATE
    systolic_fpga_runtime
)

add_executable(dpti-job0-numerical-test
  test_dpti_job0_numerical.cpp
)

target_link_libraries(dpti-job0-numerical-test
  PRIVATE
    systolic_fpga_runtime
)

add_executable(dpti-job0-compiler-b-test
  test_dpti_job0_compiler_b.cpp
)

target_link_libraries(dpti-job0-compiler-b-test
  PRIVATE
    systolic_fpga_runtime
)
