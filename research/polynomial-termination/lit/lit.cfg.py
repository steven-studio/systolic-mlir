import os
import lit.formats
config.name = "Polynomial-Termination-P2b"
config.test_format = lit.formats.ShTest()
config.suffixes = [".mlir"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = config.p2b_exec_root
for name, value in [("%report", config.p2b_driver), ("%mlir-opt", config.p2b_opt),
                    ("%FileCheck", config.p2b_filecheck), ("%not", config.p2b_not),
                    ("%inputs", config.p2b_inputs)]:
    config.substitutions.append((name, '"' + value + '"'))
