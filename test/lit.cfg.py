# -*- Python -*-
import os
import sys

import lit.formats
from lit.llvm import llvm_config

config.name = "SYSTOLIC"
# lit >= 23 removes the external-shell option; the internal shell runs these
# RUN lines (plain pipes) identically everywhere.
config.test_format = lit.formats.ShTest()
config.suffixes = [".mlir", ".c"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.systolic_obj_root, "test")

config.substitutions.append(("%PATH%", config.environment["PATH"]))

llvm_config.with_system_environment(["HOME", "INCLUDE", "LIB", "TMP", "TEMP"])

# use_default_substitutions() looks for FileCheck, count and not only in
# llvm_tools_dir, and an LLVM build tree configured without
# LLVM_INSTALL_UTILS / LLVM_BUILD_UTILS (the LLVM 24 build this project uses)
# has FileCheck but no count or not, so lit stopped before running anything.
# The same substitutions, searched in llvm_tools_dir first and then on PATH:
# count and not are version-independent helpers.
from lit.llvm.subst import FindTool, ToolSubst

config.substitutions.append(("%python", '"%s"' % (sys.executable)))
llvm_config.add_tool_substitutions(
    [
        ToolSubst("FileCheck", unresolved="fatal"),
        ToolSubst(r"\| \bcount\b", command=FindTool("count"), verbatim=True,
                  unresolved="fatal"),
        ToolSubst(r"\| \bnot\b", command=FindTool("not"), verbatim=True,
                  unresolved="fatal"),
    ],
    [config.llvm_tools_dir] + config.environment["PATH"].split(os.pathsep),
)
llvm_config.add_err_msg_substitutions()

# Everything here that is not itself a test.
config.excludes = [
    "CMakeLists.txt",
    "lit.cfg.py",
    "lit.site.cfg.py",
    "lit.site.cfg.py.in",
    # test/e2e needs the FPGA (the board or the RTL co-simulation) and is
    # run by test/e2e/run_e2e.sh, not by lit.
    "e2e",
    # Inputs other tests' RUN lines read (%S/Inputs/...); not tests.
    "Inputs",
]


# The C front end (tools/systolic-cfront) runs the clang the build names
# (-DSYSTOLIC_CFRONT_CLANG=<clang 24>, see test/CMakeLists.txt). Tests that
# need it say REQUIRES: cfront-clang; without it they are UNSUPPORTED -- no
# other clang on PATH is picked up in its place.
if config.cfront_clang:
    config.available_features.add("cfront-clang")
    cfront = os.path.join(os.path.dirname(config.test_source_root), "tools",
                          "systolic-cfront", "systolic-cfront.py")
    config.substitutions.append(
        ("%cfront", '"%s" "%s" --clang "%s"' % (sys.executable, cfront,
                                                config.cfront_clang)))
    # A plain C compile with the same clang, for comparing a program's
    # output with what systolic-cc made of it.
    config.substitutions.append(("%host-cc", '"%s"' % config.cfront_clang))
    # The driver: the front end, the existing pipeline, the host compile and
    # the link, with the software runtime model unless a test says
    # otherwise. The LLVM build is the one llvm_tools_dir belongs to (llc,
    # mlir-translate, mlir_c_runner_utils).
    cc = os.path.join(os.path.dirname(config.test_source_root), "tools",
                      "systolic-cc", "systolic-cc.py")
    config.substitutions.append(
        ("%systolic-cc", '"%s" "%s" --clang "%s" --build-dir "%s" '
         '--llvm-build "%s"' % (sys.executable, cc, config.cfront_clang,
                                config.systolic_obj_root,
                                os.path.dirname(os.path.realpath(
                                    config.llvm_tools_dir)))))

tool_dirs = [config.systolic_tools_dir, config.llvm_tools_dir]
tools = ["systolic-opt", "systolic-translate", "systolic-cost"]
llvm_config.add_tool_substitutions(tools, tool_dirs)
