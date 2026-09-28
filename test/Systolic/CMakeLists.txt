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

add_executable(check_decomposition
  check_decomposition.cpp
)

target_link_libraries(check_decomposition PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_decomposition PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)
