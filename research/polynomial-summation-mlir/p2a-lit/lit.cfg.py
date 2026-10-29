import os
import lit.formats
config.name = "Polynomial-MLIR-P2a"
config.test_format = lit.formats.ShTest()
config.suffixes = [".mlir"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = config.p2_exec_root
for name, value in [("%report", config.p2_driver), ("%mlir-opt", config.p2_opt),
                    ("%FileCheck", config.p2_filecheck), ("%inputs", config.p2_inputs)]:
    config.substitutions.append((name, '"' + value + '"'))
