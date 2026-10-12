# End-to-end checks that need the FPGA (the board, or the RTL co-simulation
# built by cosim/build.sh); run_e2e.sh drives them. Nothing here is a lit test.

# The runtime layer on its own: host -> runtime -> DPTI -> FPGA -> result.
add_executable(runtime-dispatch-test runtime_dispatch_test.cpp)
target_link_libraries(runtime-dispatch-test PRIVATE systolic_fpga_runtime)

# The harness's handling of the bitstream configuration, on the host only (no
# FPGA, no co-simulation): cmake --build build --target check-e2e-config
add_custom_target(check-e2e-config
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_e2e_config.sh --build-dir ${CMAKE_BINARY_DIR}
  DEPENDS runtime-dispatch-test systolic-matmul
  USES_TERMINAL
  COMMENT "E2E configuration checks (host only)")
