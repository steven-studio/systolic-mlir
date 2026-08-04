# -*- Python -*-
import os

import lit.formats
from lit.llvm import llvm_config

config.name = "SYSTOLIC"
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)
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
]

tool_dirs = [config.systolic_tools_dir, config.llvm_tools_dir]
tools = ["systolic-opt", "systolic-translate"]
llvm_config.add_tool_substitutions(tools, tool_dirs)
