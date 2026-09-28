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

add_executable(check_compute_cost check_compute_cost.cpp)

target_link_libraries(check_compute_cost
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_compute_cost
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)

add_executable(
  check_assignment_aware_schedule
  check_assignment_aware_schedule.cpp
)

target_link_libraries(
  check_assignment_aware_schedule
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(
  check_assignment_aware_schedule
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)

add_executable(
  check_exact_optimizer
  check_exact_optimizer.cpp
)

target_link_libraries(
  check_exact_optimizer
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(
  check_exact_optimizer
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)

