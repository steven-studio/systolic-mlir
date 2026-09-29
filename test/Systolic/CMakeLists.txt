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


add_executable(check_area_first_scaling
  check_area_first_scaling.cpp
)

target_link_libraries(check_area_first_scaling
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_area_first_scaling
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)

add_executable(check_packed_count_vector
  check_packed_count_vector.cpp
)

target_link_libraries(check_packed_count_vector
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_packed_count_vector
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)


add_executable(benchmark_decomposition_dp
  benchmark_decomposition_dp.cpp
)

target_link_libraries(benchmark_decomposition_dp
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(benchmark_decomposition_dp
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
)



add_executable(check_greedy_decompose_1to32
  check_greedy_decompose_1to32.cpp
)

target_link_libraries(check_greedy_decompose_1to32
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_greedy_decompose_1to32
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)


add_executable(check_scheduler_25x25
  check_scheduler_25x25.cpp
)

target_link_libraries(check_scheduler_25x25
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_scheduler_25x25
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)



add_executable(check_scheduler_cases
  check_scheduler_cases.cpp
)

target_link_libraries(check_scheduler_cases
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_scheduler_cases
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)



add_executable(check_optimizer_25x25
  check_optimizer_25x25.cpp
)

target_link_libraries(check_optimizer_25x25
  PRIVATE
  MLIRSystolicTransforms
)

target_include_directories(check_optimizer_25x25
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

