# systolic-cfront: the C front end (C -> MLIR func/affine/arith/memref), and
# the JSON reader it shares with systolic-cc. Plain C: the only thing taken
# from the LLVM 24 tool chain is the clang 24 executable it runs, at run time.
add_library(systolic_cjson STATIC json.c)
target_include_directories(systolic_cjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(systolic_cjson PROPERTIES C_STANDARD 11
                      C_STANDARD_REQUIRED ON)

add_executable(systolic-cfront systolic-cfront.c)
target_link_libraries(systolic-cfront PRIVATE systolic_cjson)
set_target_properties(systolic-cfront PROPERTIES C_STANDARD 11
                      C_STANDARD_REQUIRED ON)
target_compile_options(systolic-cfront PRIVATE -Wall -Wextra)
