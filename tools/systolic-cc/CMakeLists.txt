# systolic-cc: the compiler driver (C program -> offloaded GEMMs on the FPGA,
# the rest compiled by clang 24). Plain C. It runs systolic-cfront from its
# own directory, systolic-opt from the build, mlir-translate / llc from the
# LLVM 24 build and clang 24, and compiles runtime/offload from the source
# tree, whose location it is built with.
add_executable(systolic-cc systolic-cc.c)
target_link_libraries(systolic-cc PRIVATE systolic_cjson)
target_compile_definitions(systolic-cc PRIVATE
  SYSTOLIC_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
set_target_properties(systolic-cc PROPERTIES C_STANDARD 11
                      C_STANDARD_REQUIRED ON)
target_compile_options(systolic-cc PRIVATE -Wall -Wextra)
add_dependencies(systolic-cc systolic-cfront)
# The clang 24 systolic-cc uses when neither --clang nor SYSTOLIC_CFRONT_CLANG
# names one: the one this build was configured with (-DSYSTOLIC_CFRONT_CLANG).
target_compile_definitions(systolic-cc PRIVATE
  SYSTOLIC_DEFAULT_CLANG="${SYSTOLIC_CFRONT_CLANG}")
