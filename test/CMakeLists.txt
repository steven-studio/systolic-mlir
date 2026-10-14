# test/CMakeLists.txt -- wires the .mlir files under test/Systolic into lit.
#
# Until this existed there was no check-systolic target, so every .mlir in
# the tree was dead weight: two of them had been written against a
# matmul_tile syntax the op never accepted and had therefore never parsed,
# let alone had their CHECK lines evaluated.

# The clang the C front end (tools/systolic-cfront) parses C with: the
# project's LLVM 24 clang, e.g. a standalone clang build against the same
# LLVM (-DSYSTOLIC_CFRONT_CLANG=$HOME/work/clang24-build/bin/clang). Empty:
# the test/CFront tests are UNSUPPORTED.
set(SYSTOLIC_CFRONT_CLANG "" CACHE FILEPATH
    "clang 24 executable for the C front end's tests")

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

set(SYSTOLIC_TEST_DEPENDS
  systolic-opt
  systolic-translate
  systolic-cost
)

# An LLVM built in-tree (or the Debian llvm-*-tools packages) exports
# FileCheck/count/not as CMake targets; a plain installed toolchain does not,
# and then lit simply finds them on PATH. Depend on them only when they exist.
foreach(util FileCheck count not)
  if(TARGET ${util})
    list(APPEND SYSTOLIC_TEST_DEPENDS ${util})
  endif()
endforeach()

add_lit_testsuite(check-systolic "Running the systolic regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${SYSTOLIC_TEST_DEPENDS}
)
set_target_properties(check-systolic PROPERTIES FOLDER "Tests")

add_lit_testsuites(SYSTOLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${SYSTOLIC_TEST_DEPENDS}
)

add_subdirectory(e2e)
