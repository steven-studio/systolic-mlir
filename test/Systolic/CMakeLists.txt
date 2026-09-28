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



add_executable(check_decomposition_dp
  check_decomposition_dp.cpp
)

target_link_libraries(check_decomposition_dp
  PRIVATE
  MLIRSystolicTransforms
)

add_executable(check_scheduling_dp
  check_scheduling_dp.cpp
)

target_link_libraries(check_scheduling_dp
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_scheduling_dp
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)

add_executable(check_dp_optimizer_scaling
  check_dp_optimizer_scaling.cpp
)

target_link_libraries(check_dp_optimizer_scaling
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_dp_optimizer_scaling
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)


add_executable(check_area_count_vectors
  check_area_count_vectors.cpp
)

target_link_libraries(check_area_count_vectors
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_area_count_vectors
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)

add_executable(check_largest_tile_enumerator
  check_largest_tile_enumerator.cpp
)

target_link_libraries(check_largest_tile_enumerator
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_largest_tile_enumerator
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)

add_executable(check_frontier_dp_scaling
  check_frontier_dp_scaling.cpp
)

target_link_libraries(check_frontier_dp_scaling
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_frontier_dp_scaling
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)
