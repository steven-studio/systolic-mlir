import os
import lit.formats

config.name = "Polynomial-MLIR-P1"
config.test_format = lit.formats.ShTest()
config.suffixes = [".mlir"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = config.p1_exec_root
for name, value in [("%cfg-report", config.p1_driver), ("%mlir-opt", config.p1_mlir_opt),
                    ("%FileCheck", config.p1_filecheck), ("%not", config.p1_not),
                    ("%python", config.p1_python), ("%inputs", config.p1_inputs)]:
    config.substitutions.append((name, '"' + value + '"'))
