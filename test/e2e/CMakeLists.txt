# End-to-end checks that need the FPGA (the board, or the RTL co-simulation
# built by cosim/build.sh); run_e2e.sh drives them. Nothing here is a lit test.

# The runtime layer on its own: host -> runtime -> DPTI -> FPGA -> result.
add_executable(runtime-dispatch-test runtime_dispatch_test.cpp)
target_link_libraries(runtime-dispatch-test PRIVATE systolic_fpga_runtime)

# The asynchronous runtime on its own (docs/ASYNC.md: modes A, B, C, D, X, V
# against a bitstream with the completion record); run_e2e.sh --mode async
# runs it before the compiler cases.
add_executable(runtime-async-test runtime_async_test.cpp)
target_link_libraries(runtime-async-test PRIVATE systolic_fpga_runtime)

# The harness's handling of the bitstream configuration, on the host only (no
# FPGA, no co-simulation): cmake --build build --target check-e2e-config
add_custom_target(check-e2e-config
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_e2e_config.sh --build-dir ${CMAKE_BINARY_DIR}
  DEPENDS runtime-dispatch-test runtime-async-test systolic-matmul
  USES_TERMINAL
  COMMENT "E2E configuration checks (host only)")

# Regression checks that need the RTL co-simulation (cosim/build.sh first);
# every program runs against the simulation, never a board:
# cmake --build build --target check-e2e-cosim-regressions
add_custom_target(check-e2e-cosim-regressions
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_cosim_regressions.sh --build-dir ${CMAKE_BINARY_DIR}
  DEPENDS runtime-dispatch-test runtime-async-test systolic-matmul systolic-cc
          systolic-cfront systolic-opt systolic_fpga_runtime
  USES_TERMINAL
  COMMENT "E2E co-simulation regressions (needs build/cosim)")

# Phase T1 (descriptor coalescing): the DPTI transport and both runtimes
# against a mock libftdi, on the host only -- no board, no co-simulation.
# The runtime sources are compiled in directly so that the mock's ftdi_*
# definitions take libftdi1's place at link time (the static library would
# pull the real one in). cmake --build build --target check-dpti-coalesce
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBFTDI1_HEADERS REQUIRED libftdi1)
add_executable(dpti-coalesce-test
  dpti_coalesce_test.cpp
  mock_ftdi.cpp
  ${CMAKE_SOURCE_DIR}/runtime/dpti_runtime.c
  ${CMAKE_SOURCE_DIR}/runtime/dpti_ftdi.cpp
  ${CMAKE_SOURCE_DIR}/runtime/fpga_matmul_tiled.cpp
  ${CMAKE_SOURCE_DIR}/runtime/fpga_async_runtime.cpp)
target_include_directories(dpti-coalesce-test PRIVATE
  ${CMAKE_SOURCE_DIR}/runtime ${LIBFTDI1_HEADERS_INCLUDE_DIRS})
target_link_libraries(dpti-coalesce-test PRIVATE Threads::Threads)
add_custom_target(check-dpti-coalesce
  COMMAND dpti-coalesce-test
  COMMAND dpti-coalesce-test --queue-limit-1
  DEPENDS dpti-coalesce-test
  USES_TERMINAL
  COMMENT "DPTI descriptor coalescing against the mock transport (host only)")
