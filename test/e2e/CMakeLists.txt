# End-to-end checks that need the FPGA (the board, or the RTL co-simulation
# built by cosim/build.sh); run_e2e.sh drives them. Nothing here is a lit test.

# The runtime layer on its own: host -> runtime -> DPTI -> FPGA -> result.
add_executable(runtime-dispatch-test runtime_dispatch_test.cpp)
target_link_libraries(runtime-dispatch-test PRIVATE systolic_fpga_runtime)
