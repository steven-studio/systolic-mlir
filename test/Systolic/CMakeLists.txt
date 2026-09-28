add_executable(test_systolic_tiling
  test_systolic_tiling.cpp
)

target_link_libraries(test_systolic_tiling PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(test_systolic_tiling PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)
