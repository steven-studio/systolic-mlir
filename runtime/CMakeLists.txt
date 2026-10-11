# The FPGA runtime the compiler targets: --lower-systolic-matmul-tile-to-fpga
# emits calls of fpga_matmul_tiled_auto_scheduled (fpga_matmul_tiled.cpp),
# which drives the 1x8x8 + 3x4x4 bitstream over DPTI (dpti_ftdi.cpp: FT2232H
# sync FIFO session; dpti_runtime.c: descriptor register writes).
#
# The older UART runtimes for the HLS/fold bitstreams live under lib/ and are
# built by the Makefile, not here.
add_library(systolic_fpga_runtime STATIC
  dpti_runtime.c
  dpti_ftdi.cpp
  fpga_matmul_tiled.cpp
)

target_include_directories(systolic_fpga_runtime
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBFTDI1 REQUIRED IMPORTED_TARGET libftdi1)

target_link_libraries(systolic_fpga_runtime
  PUBLIC
    PkgConfig::LIBFTDI1
)
