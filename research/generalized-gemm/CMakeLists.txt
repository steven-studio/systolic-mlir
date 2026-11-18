cmake_minimum_required(VERSION 3.20)
project(generalized-gemm LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

# Research build. Never included from the production CMake graph, and the
# production build references none of these files.
get_filename_component(GG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
# The mutation test points this at a mutated copy of lib/GeneralizedGemm.
set(GG_LIB_DIR "${GG_ROOT}/lib/GeneralizedGemm" CACHE PATH "Library sources")
find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_path(GG_Z3_INCLUDE z3++.h REQUIRED)
find_library(GG_Z3_LIBRARY NAMES z3 REQUIRED)

add_library(generalized-gemm STATIC
  "${GG_LIB_DIR}/ProofStatus.cpp"
  "${GG_LIB_DIR}/SemanticTerm.cpp"
  "${GG_LIB_DIR}/SemanticImport.cpp"
  "${GG_LIB_DIR}/ConcreteEvaluator.cpp"
  "${GG_LIB_DIR}/SymbolicEvaluator.cpp"
  "${GG_LIB_DIR}/RelationalEncoding.cpp"
  "${GG_LIB_DIR}/SemanticPrinter.cpp")
target_include_directories(generalized-gemm PUBLIC "${GG_ROOT}/include")
target_include_directories(generalized-gemm SYSTEM PUBLIC
  ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
target_compile_options(generalized-gemm PRIVATE -Wall -Wextra -Wpedantic)
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(generalized-gemm PUBLIC -fno-rtti)
endif()
target_link_libraries(generalized-gemm PUBLIC
  MLIRAffineToStandard MLIRSCFToControlFlow MLIRAffineDialect
  MLIRArithDialect MLIRControlFlowDialect MLIRFuncDialect MLIRMemRefDialect
  MLIRSCFDialect MLIRPass MLIRIR MLIRSupport MLIRParser MLIRLLVMDialect)

# G2a. The checker links neither the proposers nor a solver: the only authority
# for a verdict cannot call the code that guesses, and a proposer that needs the
# checker's verdict constructors would not link.
add_library(generalized-gemm-checker STATIC
  "${GG_LIB_DIR}/ControlStructure.cpp"
  "${GG_LIB_DIR}/CutPaths.cpp"
  "${GG_LIB_DIR}/LoopCounters.cpp"
  "${GG_LIB_DIR}/TerminationObligations.cpp"
  "${GG_LIB_DIR}/TerminationCertificate.cpp")
target_link_libraries(generalized-gemm-checker PUBLIC generalized-gemm)
target_compile_options(generalized-gemm-checker PRIVATE -Wall -Wextra -Wpedantic)

add_library(generalized-gemm-termination STATIC
  "${GG_LIB_DIR}/TerminationProposers.cpp"
  "${GG_LIB_DIR}/TerminationAnalysis.cpp")
target_link_libraries(generalized-gemm-termination PUBLIC generalized-gemm-checker)
target_compile_options(generalized-gemm-termination PRIVATE -Wall -Wextra -Wpedantic)

add_library(generalized-gemm-z3 STATIC "${GG_LIB_DIR}/Z3Backend.cpp")
target_include_directories(generalized-gemm-z3 SYSTEM PRIVATE "${GG_Z3_INCLUDE}")
target_link_libraries(generalized-gemm-z3 PUBLIC generalized-gemm-checker "${GG_Z3_LIBRARY}")
target_compile_options(generalized-gemm-z3 PRIVATE -Wall -Wextra -Wpedantic)

# P2b as a source of hints only. Its CMake project stays untouched.
add_subdirectory("${GG_ROOT}/research/polynomial-summation-mlir" polynomial-recurrence EXCLUDE_FROM_ALL)
add_library(generalized-gemm-p2b-candidates STATIC
  "${GG_ROOT}/lib/Systolic/Analysis/MLIRPolynomialTermination.cpp"
  "${GG_ROOT}/lib/Systolic/Analysis/PolynomialTerminationProof.cpp")
target_include_directories(generalized-gemm-p2b-candidates PRIVATE "${GG_Z3_INCLUDE}")
target_link_libraries(generalized-gemm-p2b-candidates PUBLIC polynomial-cfg PRIVATE "${GG_Z3_LIBRARY}")
add_library(generalized-gemm-polynomial-adapter STATIC "${GG_LIB_DIR}/PolynomialAdapter.cpp")
target_link_libraries(generalized-gemm-polynomial-adapter
  PUBLIC generalized-gemm-termination PRIVATE generalized-gemm-p2b-candidates)

add_executable(generalized-gemm-termination-report TerminationReport.cpp)
target_link_libraries(generalized-gemm-termination-report PRIVATE
  generalized-gemm-polynomial-adapter generalized-gemm-z3)

# G3a. The checker links neither the proposer nor union-find, and holds the only
# constructor of a found status. The analysis layer is what connects them.
add_library(generalized-gemm-coordinate-model STATIC "${GG_LIB_DIR}/CoordinateModel.cpp")
target_link_libraries(generalized-gemm-coordinate-model PUBLIC generalized-gemm)
add_library(generalized-gemm-candidate-checker STATIC "${GG_LIB_DIR}/CandidateChecker.cpp")
target_link_libraries(generalized-gemm-candidate-checker PUBLIC generalized-gemm-coordinate-model)
add_library(generalized-gemm-canonical-recovery STATIC "${GG_LIB_DIR}/CanonicalRecovery.cpp")
target_link_libraries(generalized-gemm-canonical-recovery PUBLIC generalized-gemm-coordinate-model)
add_library(generalized-gemm-finite-domain STATIC "${GG_LIB_DIR}/FiniteDomain.cpp"
  "${GG_LIB_DIR}/SiteSets.cpp")
target_link_libraries(generalized-gemm-finite-domain PUBLIC
  generalized-gemm-coordinate-model generalized-gemm-checker)
add_library(generalized-gemm-coordinates STATIC "${GG_LIB_DIR}/CoordinateAnalysis.cpp")
target_link_libraries(generalized-gemm-coordinates PUBLIC
  generalized-gemm-candidate-checker generalized-gemm-canonical-recovery
  generalized-gemm-finite-domain)
foreach(target generalized-gemm-coordinate-model generalized-gemm-candidate-checker
        generalized-gemm-canonical-recovery generalized-gemm-finite-domain
        generalized-gemm-coordinates)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

add_executable(generalized-gemm-coordinate-report CoordinateReport.cpp)
target_link_libraries(generalized-gemm-coordinate-report PRIVATE
  generalized-gemm-coordinates generalized-gemm-polynomial-adapter generalized-gemm-z3)

# G3b. The grammar and the search can issue nothing: the tier authority links
# neither of them, and the G3a checker stays the judge of every finite claim.
add_library(generalized-gemm-tier-checker STATIC
  "${GG_LIB_DIR}/CoordinateFamily.cpp"
  "${GG_LIB_DIR}/IndexProgram.cpp"
  "${GG_LIB_DIR}/IndexSlice.cpp"
  "${GG_LIB_DIR}/LoopInvariant.cpp"
  "${GG_LIB_DIR}/IndexInvariant.cpp"
  "${GG_LIB_DIR}/SymbolicDomain.cpp"
  "${GG_LIB_DIR}/CoverageSimulation.cpp"
  "${GG_LIB_DIR}/SymbolicCandidateChecker.cpp")
target_link_libraries(generalized-gemm-tier-checker PUBLIC
  generalized-gemm-candidate-checker generalized-gemm-finite-domain)
add_library(generalized-gemm-synthesis STATIC
  "${GG_LIB_DIR}/CoordinateGrammar.cpp"
  "${GG_LIB_DIR}/IndexProgramSemgus.cpp"
  "${GG_LIB_DIR}/ClosedFormSynthesis.cpp"
  "${GG_LIB_DIR}/InvariantSynthesis.cpp"
  "${GG_LIB_DIR}/IndexInvariantSynthesis.cpp"
  "${GG_LIB_DIR}/CoordinateSynthesis.cpp"
  "${GG_LIB_DIR}/SynthesisAnalysis.cpp")
target_link_libraries(generalized-gemm-synthesis PUBLIC
  generalized-gemm-tier-checker generalized-gemm-coordinates)
foreach(target generalized-gemm-tier-checker generalized-gemm-synthesis)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

add_executable(generalized-gemm-synthesis-report SynthesisReport.cpp)
target_link_libraries(generalized-gemm-synthesis-report PRIVATE
  generalized-gemm-synthesis generalized-gemm-polynomial-adapter generalized-gemm-z3)

# G4a. The computation authority links the tier checker and neither the grammar
# nor the search: nothing a proposer computes can reach a verdict.
add_library(generalized-gemm-computation-checker STATIC
  "${GG_LIB_DIR}/HerbrandReplay.cpp"
  "${GG_LIB_DIR}/ComputationChecker.cpp")
target_link_libraries(generalized-gemm-computation-checker PUBLIC generalized-gemm-tier-checker)
add_library(generalized-gemm-computation STATIC "${GG_LIB_DIR}/ComputationAnalysis.cpp")
target_link_libraries(generalized-gemm-computation PUBLIC
  generalized-gemm-computation-checker generalized-gemm-synthesis)
foreach(target generalized-gemm-computation-checker generalized-gemm-computation)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

add_executable(generalized-gemm-computation-report ComputationReport.cpp)
target_link_libraries(generalized-gemm-computation-report PRIVATE
  generalized-gemm-computation generalized-gemm-polynomial-adapter generalized-gemm-z3)

# G5a. The domain authority links the computation checker and the tier checker, and
# neither a proposer nor any Presburger or Simplex code.
add_library(generalized-gemm-domain-checker STATIC
  "${GG_LIB_DIR}/GuardedOrbit.cpp"
  "${GG_LIB_DIR}/ReductionSemantics.cpp"
  "${GG_LIB_DIR}/DomainChecker.cpp")
target_link_libraries(generalized-gemm-domain-checker PUBLIC generalized-gemm-computation-checker)
add_library(generalized-gemm-domain STATIC
  "${GG_LIB_DIR}/DomainProposer.cpp"
  "${GG_LIB_DIR}/DomainAnalysis.cpp")
target_link_libraries(generalized-gemm-domain PUBLIC
  generalized-gemm-domain-checker generalized-gemm-synthesis)
foreach(target generalized-gemm-domain-checker generalized-gemm-domain)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

add_executable(generalized-gemm-domain-report DomainReport.cpp)
target_link_libraries(generalized-gemm-domain-report PRIVATE
  generalized-gemm-domain generalized-gemm-polynomial-adapter generalized-gemm-z3)

# G5b-M4-0: what the shared site-set rules answer, for an oracle with no library code.
add_executable(generalized-gemm-identity-report IdentityReport.cpp)
target_link_libraries(generalized-gemm-identity-report PRIVATE
  generalized-gemm-domain generalized-gemm-polynomial-adapter generalized-gemm-z3)

# Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md: one record per loop nest. The outliner copies a
# nest into a function of its own, and the record reads what the analyses above say of
# the copy. No target above links this library, so none of their results can depend on it.
# Stage 2: the binding of a nest is computed by the production pass's own file, compiled here
# as it is, so that both sides compute it with one piece of code.
add_library(generalized-gemm-nest STATIC
  "${GG_LIB_DIR}/NestOutline.cpp"
  "${GG_LIB_DIR}/NestRecord.cpp"
  "${GG_ROOT}/lib/Systolic/Transforms/NestBinding.cpp")
target_link_libraries(generalized-gemm-nest PUBLIC
  generalized-gemm-domain generalized-gemm-polynomial-adapter)
target_compile_options(generalized-gemm-nest PRIVATE -Wall -Wextra -Wpedantic)
add_executable(generalized-gemm-nest-report NestReport.cpp)
target_link_libraries(generalized-gemm-nest-report PRIVATE generalized-gemm-nest generalized-gemm-z3)
add_executable(generalized-gemm-nest-test test/unit/NestTest.cpp)
target_link_libraries(generalized-gemm-nest-test PRIVATE generalized-gemm-nest generalized-gemm-z3)
add_test(NAME generalized-gemm-nest COMMAND generalized-gemm-nest-test)
set_tests_properties(generalized-gemm-nest PROPERTIES TIMEOUT 900)

add_executable(generalized-gemm-report SemanticReport.cpp)
target_link_libraries(generalized-gemm-report PRIVATE generalized-gemm)

add_executable(generalized-gemm-unit test/unit/SemanticModelTest.cpp)
target_link_libraries(generalized-gemm-unit PRIVATE generalized-gemm)
add_test(NAME generalized-gemm-unit COMMAND generalized-gemm-unit)

# Z3 is a test-time dependency only: the library emits text.
add_executable(generalized-gemm-relational test/unit/RelationalAgreementTest.cpp)
target_include_directories(generalized-gemm-relational SYSTEM PRIVATE "${GG_Z3_INCLUDE}")
target_link_libraries(generalized-gemm-relational PRIVATE generalized-gemm "${GG_Z3_LIBRARY}")
add_test(NAME generalized-gemm-relational COMMAND generalized-gemm-relational)

# The loop catalogue, printed at the widths the tests use.
set(GG_LOOPS "${CMAKE_CURRENT_SOURCE_DIR}/test/lit/Inputs/gen_loops.py")
foreach(width 4 8)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/loops${width}.mlir"
    COMMAND "${Python3_EXECUTABLE}" "${GG_LOOPS}" ${width} > "${CMAKE_CURRENT_BINARY_DIR}/loops${width}.mlir"
    DEPENDS "${GG_LOOPS}")
endforeach()
add_custom_target(generalized-gemm-loops DEPENDS
  "${CMAKE_CURRENT_BINARY_DIR}/loops4.mlir" "${CMAKE_CURRENT_BINARY_DIR}/loops8.mlir")

add_executable(generalized-gemm-termination-test test/unit/TerminationTest.cpp)
target_link_libraries(generalized-gemm-termination-test PRIVATE
  generalized-gemm-termination generalized-gemm-z3)
add_dependencies(generalized-gemm-termination-test generalized-gemm-loops)
add_test(NAME generalized-gemm-termination COMMAND generalized-gemm-termination-test
  "${CMAKE_CURRENT_BINARY_DIR}/loops4.mlir" "${CMAKE_CURRENT_BINARY_DIR}/loops8.mlir")
set_tests_properties(generalized-gemm-termination PROPERTIES TIMEOUT 600)

add_executable(generalized-gemm-coordinate-test test/unit/CoordinateRecoveryTest.cpp)
target_link_libraries(generalized-gemm-coordinate-test PRIVATE
  generalized-gemm-coordinates generalized-gemm-termination generalized-gemm-z3)
add_test(NAME generalized-gemm-coordinates COMMAND generalized-gemm-coordinate-test)
set_tests_properties(generalized-gemm-coordinates PROPERTIES TIMEOUT 300)

add_executable(generalized-gemm-synthesis-test test/unit/SynthesisTest.cpp)
target_link_libraries(generalized-gemm-synthesis-test PRIVATE
  generalized-gemm-synthesis generalized-gemm-termination generalized-gemm-z3)
add_test(NAME generalized-gemm-synthesis COMMAND generalized-gemm-synthesis-test)
# 502 s in Release on 2026-11-18, with the invariant, null-space and index sections.
set_tests_properties(generalized-gemm-synthesis PROPERTIES TIMEOUT 900)

add_executable(generalized-gemm-computation-test test/unit/ComputationTest.cpp)
target_link_libraries(generalized-gemm-computation-test PRIVATE
  generalized-gemm-computation generalized-gemm-termination generalized-gemm-z3)
add_test(NAME generalized-gemm-computation COMMAND generalized-gemm-computation-test)
set_tests_properties(generalized-gemm-computation PROPERTIES TIMEOUT 1200)

add_executable(generalized-gemm-domain-test test/unit/DomainTest.cpp)
target_link_libraries(generalized-gemm-domain-test PRIVATE
  generalized-gemm-domain generalized-gemm-termination generalized-gemm-z3)
add_test(NAME generalized-gemm-domain COMMAND generalized-gemm-domain-test core)
set_tests_properties(generalized-gemm-domain PROPERTIES TIMEOUT 1500)
add_test(NAME generalized-gemm-domain-holes COMMAND generalized-gemm-domain-test holes)
set_tests_properties(generalized-gemm-domain-holes PROPERTIES TIMEOUT 3000)

find_program(GG_MLIR_OPT mlir-opt HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(GG_MLIR_TRANSLATE mlir-translate HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(GG_CLANG clang HINTS "$ENV{HOME}/work/clang24-build/bin" NO_DEFAULT_PATH REQUIRED)
find_program(GG_FILECHECK FileCheck HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(GG_NOT not HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(GG_LIT llvm-lit HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH REQUIRED)

set(GG_FUZZ "${CMAKE_CURRENT_SOURCE_DIR}/test/fuzz")
add_test(NAME generalized-gemm-oracle-fuzz COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/differential.py" --driver $<TARGET_FILE:generalized-gemm-report>
  --programs 1500 --seed 1)
set_tests_properties(generalized-gemm-oracle-fuzz PROPERTIES TIMEOUT 300)
add_test(NAME generalized-gemm-native-differential COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/differential.py" --driver $<TARGET_FILE:generalized-gemm-report>
  --programs 120 --seed 2 --native --mlir-opt "${GG_MLIR_OPT}"
  --mlir-translate "${GG_MLIR_TRANSLATE}" --clang "${GG_CLANG}")
set_tests_properties(generalized-gemm-native-differential PROPERTIES TIMEOUT 600)

add_test(NAME generalized-gemm-termination-oracle COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/termination_exhaustive.py" --driver $<TARGET_FILE:generalized-gemm-termination-report>
  --programs 600 --seed 1 --jobs 4 --catalogue "${GG_LOOPS}")
set_tests_properties(generalized-gemm-termination-oracle PROPERTIES TIMEOUT 1200)
add_test(NAME generalized-gemm-termination-replay COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/replay_obligations.py" --driver $<TARGET_FILE:generalized-gemm-termination-report>
  --catalogue "${GG_LOOPS}" --z3-library "${GG_Z3_LIBRARY}")
set_tests_properties(generalized-gemm-termination-replay PROPERTIES TIMEOUT 600)

add_test(NAME generalized-gemm-coordinate-tables COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/coordinate_bruteforce.py" --driver $<TARGET_FILE:generalized-gemm-coordinate-report>
  --random 20000 --hidden 2000 --subsets 5000 --seed 1)
set_tests_properties(generalized-gemm-coordinate-tables PROPERTIES TIMEOUT 600)
add_test(NAME generalized-gemm-coordinate-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/coordinate_oracle.py" --driver $<TARGET_FILE:generalized-gemm-coordinate-report>
  --programs 120 --seed 1 --jobs 6)
set_tests_properties(generalized-gemm-coordinate-programs PROPERTIES TIMEOUT 1200)

add_test(NAME generalized-gemm-synthesis-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/synthesis_oracle.py" --driver $<TARGET_FILE:generalized-gemm-synthesis-report>
  --programs 60 --seed 1 --jobs 6)
set_tests_properties(generalized-gemm-synthesis-programs PROPERTIES TIMEOUT 1500)

add_test(NAME generalized-gemm-computation-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/computation_oracle.py" --driver $<TARGET_FILE:generalized-gemm-computation-report>
  --programs 60 --fills 40 --seed 1 --jobs 6)
set_tests_properties(generalized-gemm-computation-programs PROPERTIES TIMEOUT 1200)

# G5b-M3: claims over sets of groups, against the source trace.
add_test(NAME generalized-gemm-multigroup-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/multigroup_oracle.py" --driver $<TARGET_FILE:generalized-gemm-computation-report>
  --programs 24 --fills 12 --seed 1 --jobs 2)
set_tests_properties(generalized-gemm-multigroup-programs PROPERTIES TIMEOUT 3600)

add_test(NAME generalized-gemm-domain-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/domain_oracle.py" --driver $<TARGET_FILE:generalized-gemm-domain-report>
  --programs 40 --fills 16 --seed 1 --jobs 2)
# G5b-M2 doubled the oracle's specs (segmented nests, and a second run for those with
# repeat loops): 40 programs take about 25 minutes with two jobs.
set_tests_properties(generalized-gemm-domain-programs PROPERTIES TIMEOUT 3600)

# G5b-M4-0: group gates, cells, refutation scope and tiers, against the source trace.
add_test(NAME generalized-gemm-identity-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/identity_oracle.py" --driver $<TARGET_FILE:generalized-gemm-identity-report>
  --programs 60 --seed 1 --jobs 2)
set_tests_properties(generalized-gemm-identity-programs PROPERTIES TIMEOUT 1800)

# G5b-M4a: domain claims over unions of groups, against the source trace.
add_test(NAME generalized-gemm-setdomain-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/setdomain_oracle.py" --driver $<TARGET_FILE:generalized-gemm-domain-report>
  --programs 30 --seed 1 --jobs 2)
set_tests_properties(generalized-gemm-setdomain-programs PROPERTIES TIMEOUT 3600)

# G5b-M4b-1: Theorem 17 over sets of groups, folds against the source trace.
add_test(NAME generalized-gemm-setreduction-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/setreduction_oracle.py" --driver $<TARGET_FILE:generalized-gemm-domain-report>
  --programs 30 --fills 6 --seed 1 --jobs 2)
set_tests_properties(generalized-gemm-setreduction-programs PROPERTIES TIMEOUT 3600)

# Stage 1: records of loop nests, recomputed from the printed programs, and composition in
# place against the function's own run. The existing domain driver is read for the
# whole-function claims of the same programs.
add_test(NAME generalized-gemm-nest-programs COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/nest_oracle.py" --driver $<TARGET_FILE:generalized-gemm-nest-report>
  --domain-driver $<TARGET_FILE:generalized-gemm-domain-report>
  --programs 30 --fills 4 --seed 1 --jobs 4)
set_tests_properties(generalized-gemm-nest-programs PROPERTIES TIMEOUT 3600)
# The same programs with every product written with scf.for (bounds and steps arith.constant
# values before the nest, memref.load and memref.store, indices by arith in the body): each
# product over roots apart from its output must have the same complete, VERIFIED record.
add_test(NAME generalized-gemm-nest-programs-scf COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/nest_oracle.py" --driver $<TARGET_FILE:generalized-gemm-nest-report>
  --domain-driver $<TARGET_FILE:generalized-gemm-domain-report>
  --programs 30 --fills 4 --seed 1 --jobs 4 --loops scf)
set_tests_properties(generalized-gemm-nest-programs-scf PROPERTIES TIMEOUT 3600)
# The same programs with every product's loops counting scf.while loops (the counter the one
# carried value, compared in the before region, stepped in the after region), and with each
# loop one of affine.for, scf.for and scf.while: the same complete, VERIFIED records.
add_test(NAME generalized-gemm-nest-programs-while COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/nest_oracle.py" --driver $<TARGET_FILE:generalized-gemm-nest-report>
  --domain-driver $<TARGET_FILE:generalized-gemm-domain-report>
  --programs 30 --fills 4 --seed 1 --jobs 4 --loops while)
set_tests_properties(generalized-gemm-nest-programs-while PROPERTIES TIMEOUT 3600)
add_test(NAME generalized-gemm-nest-programs-mixed COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/nest_oracle.py" --driver $<TARGET_FILE:generalized-gemm-nest-report>
  --domain-driver $<TARGET_FILE:generalized-gemm-domain-report>
  --programs 30 --fills 4 --seed 1 --jobs 4 --loops mixed)
set_tests_properties(generalized-gemm-nest-programs-mixed PROPERTIES TIMEOUT 3600)

# Stage 1: a nest's record against the production affine recognizer's report, field by
# field. The production tool is not built here: point GG_SYSTOLIC_OPT at one built from
# the same sources, and the test exists. It is only run.
set(GG_SYSTOLIC_OPT "" CACHE FILEPATH "systolic-opt of this source tree, to compare nest records with")
if(GG_SYSTOLIC_OPT)
  add_test(NAME generalized-gemm-nest-affine COMMAND "${Python3_EXECUTABLE}"
    "${GG_FUZZ}/nest_affine_compare.py" --nest-driver $<TARGET_FILE:generalized-gemm-nest-report>
    --systolic-opt "${GG_SYSTOLIC_OPT}" --index-bit-width 64
    --functions mm,strict,mmi,offsets,offsetsi,allocated,restrict --twin mmi=mm --twin offsetsi=offsets
    "${CMAKE_CURRENT_SOURCE_DIR}/test/lit/nest_product.mlir")
  set_tests_properties(generalized-gemm-nest-affine PROPERTIES TIMEOUT 1200)
endif()

# Stage 2: the records the production gate's tests read (test/Systolic/Inputs/nest_records*.json)
# must be what the research tool says of their input today.
add_test(NAME generalized-gemm-nest-gate-records COMMAND "${Python3_EXECUTABLE}"
  "${GG_FUZZ}/nest_gate_records.py" --driver $<TARGET_FILE:generalized-gemm-nest-report>
  --root "${GG_ROOT}" --check)
set_tests_properties(generalized-gemm-nest-gate-records PROPERTIES TIMEOUT 1800)

# Stage 2, end to end: records, the production pass with nest-records, the FPGA lowering, a
# run against the software runtime model, results by each nest's numeric contract. Needs the
# production systolic-opt of this source tree (GG_SYSTOLIC_OPT, above); it is only run.
find_program(GG_LLC llc HINTS "${LLVM_TOOLS_BINARY_DIR}" NO_DEFAULT_PATH)
if(GG_SYSTOLIC_OPT AND GG_LLC)
  add_test(NAME generalized-gemm-nest-e2e COMMAND "${Python3_EXECUTABLE}"
    "${GG_FUZZ}/nest_e2e.py" --nest-driver $<TARGET_FILE:generalized-gemm-nest-report>
    --systolic-opt "${GG_SYSTOLIC_OPT}" --mlir-translate "${GG_MLIR_TRANSLATE}" --llc "${GG_LLC}"
    --clang "${GG_CLANG}" --runtime "${GG_ROOT}/runtime/offload" --llvm-lib "${LLVM_LIBRARY_DIR}")
  set_tests_properties(generalized-gemm-nest-e2e PROPERTIES TIMEOUT 3600)
  # Stage 3, end to end: nests the production recognizer finds no GEMM in, lowered from their
  # records (formulas and tables), run, and compared with the original.
  add_test(NAME generalized-gemm-nest-driven-e2e COMMAND "${Python3_EXECUTABLE}"
    "${GG_FUZZ}/nest_driven_e2e.py" --nest-driver $<TARGET_FILE:generalized-gemm-nest-report>
    --systolic-opt "${GG_SYSTOLIC_OPT}" --mlir-translate "${GG_MLIR_TRANSLATE}" --llc "${GG_LLC}"
    --clang "${GG_CLANG}" --runtime "${GG_ROOT}/runtime/offload" --llvm-lib "${LLVM_LIBRARY_DIR}")
  set_tests_properties(generalized-gemm-nest-driven-e2e PROPERTIES TIMEOUT 3600)
  # Stage 3 on scf.for, end to end: GEMMs written with scf.for, verified by the research,
  # lowered from their records (formulas and tables), run, and compared with the original.
  add_test(NAME generalized-gemm-nest-scf-e2e COMMAND "${Python3_EXECUTABLE}"
    "${GG_FUZZ}/nest_scf_e2e.py" --nest-driver $<TARGET_FILE:generalized-gemm-nest-report>
    --systolic-opt "${GG_SYSTOLIC_OPT}" --mlir-translate "${GG_MLIR_TRANSLATE}" --llc "${GG_LLC}"
    --clang "${GG_CLANG}" --runtime "${GG_ROOT}/runtime/offload" --llvm-lib "${LLVM_LIBRARY_DIR}")
  set_tests_properties(generalized-gemm-nest-scf-e2e PROPERTIES TIMEOUT 3600)
  # Stage 3 on scf.while, end to end: GEMMs written with counting scf.while loops, and nests
  # that mix them with for loops, verified by the research, lowered from their records, run,
  # and compared with the original.
  add_test(NAME generalized-gemm-nest-while-e2e COMMAND "${Python3_EXECUTABLE}"
    "${GG_FUZZ}/nest_while_e2e.py" --nest-driver $<TARGET_FILE:generalized-gemm-nest-report>
    --systolic-opt "${GG_SYSTOLIC_OPT}" --mlir-translate "${GG_MLIR_TRANSLATE}" --llc "${GG_LLC}"
    --clang "${GG_CLANG}" --runtime "${GG_ROOT}/runtime/offload" --llvm-lib "${LLVM_LIBRARY_DIR}")
  set_tests_properties(generalized-gemm-nest-while-e2e PROPERTIES TIMEOUT 3600)
  # Stage 3 on subscripts a loop carries, end to end: GEMMs whose K, or I, subscripts are
  # carried by an scf.for (iter_args) or an scf.while, verified by the research with index
  # invariants (--index-invariants), lowered from their records, run, and compared with the
  # original.
  add_test(NAME generalized-gemm-nest-carried-e2e COMMAND "${Python3_EXECUTABLE}"
    "${GG_FUZZ}/nest_carried_e2e.py" --nest-driver $<TARGET_FILE:generalized-gemm-nest-report>
    --systolic-opt "${GG_SYSTOLIC_OPT}" --mlir-translate "${GG_MLIR_TRANSLATE}" --llc "${GG_LLC}"
    --clang "${GG_CLANG}" --runtime "${GG_ROOT}/runtime/offload" --llvm-lib "${LLVM_LIBRARY_DIR}")
  set_tests_properties(generalized-gemm-nest-carried-e2e PROPERTIES TIMEOUT 3600)
endif()

configure_file(test/lit/lit.site.cfg.py.in lit/lit.site.cfg.py @ONLY)
add_test(NAME generalized-gemm-lit COMMAND "${GG_LIT}" -v "${CMAKE_CURRENT_BINARY_DIR}/lit")
# Phase 4 added dom_phase4.mlir (about 30 s); the suite was at 109 s of 120. Stage 1 of
# docs/GENERALIZED_GEMM_SCOPE.md added six nest_*.mlir files, each a few analyses of ten
# seconds: the limit is for a loaded machine, and no check depends on it.
set_tests_properties(generalized-gemm-lit PROPERTIES TIMEOUT 900)

# Phase 5: the table oracle recomputes every claim about the synthesized while and
# table programs with its own parser and interpreter of the G.5 rules.
add_test(NAME generalized-gemm-table-programs COMMAND "${Python3_EXECUTABLE}" -I
  "${GG_FUZZ}/table_oracle.py" --driver $<TARGET_FILE:generalized-gemm-synthesis-report>
  --cases 12 --seed 1)
set_tests_properties(generalized-gemm-table-programs PROPERTIES TIMEOUT 900)

# The inductive invariant path (Section 13): every claim against an interpreter of its
# own, every input of the scope at widths 3 and 4, certificates by brute force at width 3.
add_test(NAME generalized-gemm-invariant-programs COMMAND "${Python3_EXECUTABLE}" -I
  "${GG_FUZZ}/invariant_oracle.py" --driver $<TARGET_FILE:generalized-gemm-synthesis-report>
  --cases 24 --seed 1)
set_tests_properties(generalized-gemm-invariant-programs PROPERTIES TIMEOUT 1800)

# Index invariants for a GEMM's subscripts (Section 14): every claim of --index-invariants
# against an interpreter of its own, at every point of each scope.
add_test(NAME generalized-gemm-index-invariant-programs COMMAND "${Python3_EXECUTABLE}" -I
  "${GG_FUZZ}/index_invariant_oracle.py" --driver $<TARGET_FILE:generalized-gemm-synthesis-report>
  --programs 12 --seed 1)
set_tests_properties(generalized-gemm-index-invariant-programs PROPERTIES TIMEOUT 1800)

add_custom_target(check-generalized-gemm
  COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure -R "^generalized-gemm-"
  DEPENDS generalized-gemm-report generalized-gemm-unit generalized-gemm-relational
    generalized-gemm-termination-report generalized-gemm-termination-test
    generalized-gemm-coordinate-report generalized-gemm-coordinate-test
    generalized-gemm-synthesis-report generalized-gemm-synthesis-test
    generalized-gemm-computation-report generalized-gemm-computation-test
    generalized-gemm-domain-report generalized-gemm-domain-test
    generalized-gemm-identity-report
    generalized-gemm-nest-report generalized-gemm-nest-test)
