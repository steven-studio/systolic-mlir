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
