# -*- Python -*-
import os

import lit.formats
from lit.llvm import llvm_config

config.name = "SYSTOLIC"
# lit >= 23 removes the external-shell option; the internal shell runs these
# RUN lines (plain pipes) identically everywhere.
config.test_format = lit.formats.ShTest()
config.suffixes = [".mlir"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.systolic_obj_root, "test")

config.substitutions.append(("%PATH%", config.environment["PATH"]))

llvm_config.with_system_environment(["HOME", "INCLUDE", "LIB", "TMP", "TEMP"])
llvm_config.use_default_substitutions()

# Everything here that is not itself a test.
config.excludes = [
    "CMakeLists.txt",
    "lit.cfg.py",
    "lit.site.cfg.py",
    "lit.site.cfg.py.in",
    # test/e2e needs the FPGA (the board or the RTL co-simulation) and is
    # run by test/e2e/run_e2e.sh, not by lit.
    "e2e",
]

# Inputs for running the affine-GEMM discovery passes by hand
# (--test-affine-matmul-discovery, --affine-matmul-to-systolic): they carry
# no RUN line, so lit would report each as UNRESOLVED. Listed by name rather
# than by "has no RUN line", so a new test that forgets its RUN line still
# shows up as UNRESOLVED.
config.excludes += [
    "affine_gemm.mlir",
    "affine_gemm_access_stride.mlir",
    "affine_gemm_access_stride5.mlir",
    "affine_gemm_bad_reduction.mlir",
    "affine_gemm_batched.mlir",
    "affine_gemm_composed.mlir",
    "affine_gemm_loop_3_5.mlir",
    "affine_gemm_loop_3_5_access_7_2.mlir",
    "affine_gemm_nested.mlir",
    "affine_gemm_nonunimodular.mlir",
    "affine_gemm_nonzero_origin.mlir",
    "affine_gemm_offset.mlir",
    "affine_gemm_partition_coupled.mlir",
    "affine_gemm_permuted.mlir",
    "affine_gemm_singular_mapping.mlir",
    "affine_gemm_step2.mlir",
    "affine_matmul_k.mlir",
    "affine_matmul_x.mlir",
    "livermore_kernel21.mlir",
]

tool_dirs = [config.systolic_tools_dir, config.llvm_tools_dir]
tools = ["systolic-opt", "systolic-translate"]
llvm_config.add_tool_substitutions(tools, tool_dirs)
