# The project-owned exact solver. It links LLVMSupport, for DynamicAPInt, and
# nothing else: no MLIR library and no solver. That is an arithmetic dependency
# on LLVM, which Int.h documents and the provenance declares. test/Exact checks
# that this stays true, and no production target links this library.

add_library(SystolicExact STATIC
  Budget.cpp
  Certificate.cpp
  Checker.cpp
  Decide.cpp
  DeltaRational.cpp
  Int.cpp
  IntegerSearch.cpp
  Lattice.cpp
  Normalize.cpp
  Provenance.cpp
  Rational.cpp
  Session.cpp
  System.cpp
  Tableau.cpp
  Verdict.cpp
)
llvm_update_compile_flags(SystolicExact)
target_link_libraries(SystolicExact PUBLIC LLVMSupport)
target_include_directories(SystolicExact PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# What the build records about itself. See GenerateIdentity.cmake.
find_program(SYSTOLIC_EXACT_GIT git)
foreach(dir ${LLVM_INCLUDE_DIRS})
  if(EXISTS ${dir}/llvm/Support/VCSRevision.h)
    set(exact_vcs_header ${dir}/llvm/Support/VCSRevision.h)
  endif()
endforeach()
set(exact_llvm_source "")
if(LLVM_BUILD_MAIN_SRC_DIR)
  get_filename_component(exact_llvm_source ${LLVM_BUILD_MAIN_SRC_DIR} DIRECTORY)
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" exact_build_upper)
string(SHA256 exact_flags
       "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${exact_build_upper}}")
set(exact_identity ${CMAKE_CURRENT_BINARY_DIR}/SolverIdentity.inc)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/IdentityConfig.cmake
"set(SOURCE_ROOT \"${PROJECT_SOURCE_DIR}\")
set(OUTPUT \"${exact_identity}\")
set(GIT \"${SYSTOLIC_EXACT_GIT}\")
set(VCS_HEADER \"${exact_vcs_header}\")
set(LLVM_SOURCE_DIR \"${exact_llvm_source}\")
set(LLVM_PATCH_SCOPE \"llvm/include/llvm/ADT llvm/lib/Support mlir/include/mlir/Analysis/Presburger mlir/lib/Analysis/Presburger\")
set(COMPILER_ID \"${CMAKE_CXX_COMPILER_ID}\")
set(COMPILER_VERSION \"${CMAKE_CXX_COMPILER_VERSION}\")
set(BUILD_TYPE \"${CMAKE_BUILD_TYPE}\")
set(FLAGS_SHA256 \"${exact_flags}\")
")
add_custom_target(SystolicExactIdentity
  COMMAND ${CMAKE_COMMAND} -DCONFIG=${CMAKE_CURRENT_BINARY_DIR}/IdentityConfig.cmake
          -P ${CMAKE_CURRENT_SOURCE_DIR}/GenerateIdentity.cmake
  BYPRODUCTS ${exact_identity}
  VERBATIM
)
add_dependencies(SystolicExact SystolicExactIdentity)
