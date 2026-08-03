# test/CMakeLists.txt -- wires the .mlir files under test/Systolic into lit.
#
# Until this existed there was no check-systolic target, so every .mlir in
# the tree was dead weight: two of them had been written against a
# matmul_tile syntax the op never accepted and had therefore never parsed,
# let alone had their CHECK lines evaluated.

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

set(SYSTOLIC_TEST_DEPENDS
  FileCheck count not
  systolic-opt
  systolic-translate
)

add_lit_testsuite(check-systolic "Running the systolic regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${SYSTOLIC_TEST_DEPENDS}
)
set_target_properties(check-systolic PROPERTIES FOLDER "Tests")

add_lit_testsuites(SYSTOLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${SYSTOLIC_TEST_DEPENDS}
)
