set(LLVM_LINK_COMPONENTS Support)

# systolic-cost: the cost model (CostModel.cpp) and the binary-search
# optimizer (SystolicOptimizer.cpp) on the command line, for the simulation
# validation and the K_MAX design-space exploration under eval/.
add_llvm_executable(systolic-cost systolic-cost.cpp)

llvm_update_compile_flags(systolic-cost)
target_link_libraries(systolic-cost PRIVATE
  MLIRSystolicTransforms
  MLIRSupport
)

mlir_check_all_link_libraries(systolic-cost)
