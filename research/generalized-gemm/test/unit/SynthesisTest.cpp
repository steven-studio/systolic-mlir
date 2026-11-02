// G3b: the additive G3a options, each tier's obligations, and the forgeries each
// must refuse. test/fuzz/synthesis_oracle.py recomputes the results independently.
#include "GeneralizedGemm/SynthesisAnalysis.h"
#include "GeneralizedGemm/CoordinateAnalysis.h"
#include "GeneralizedGemm/CoverageSimulation.h"
#include "GeneralizedGemm/SymbolicDomain.h"
#include "GeneralizedGemm/TerminationAnalysis.h"
#include "GeneralizedGemm/TerminationObligations.h"
#include "GeneralizedGemm/Z3Backend.h"

#include "TestSupport.h"

#include <functional>
#include <map>
#include <set>

using namespace mlir::generalized_gemm;
using llvm::APInt;
using V = TerminationVerdict;

static mlir::MLIRContext *context;
static Z3Backend z3Backend;

struct Module {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  explicit Module(const std::string &text) : module(parse(*context, text)) {
    if (!module) {
      fprintf(stderr, "cannot parse:\n%s\n", text.c_str());
      exit(2);
    }
  }
  SemanticProgram import(const std::string &name, unsigned width = 16) {
    ImportOptions options;
    options.indexBitWidth = width;
    options.lowerStructured = true;
    return importFunction(module->lookupSymbol<mlir::func::FuncOp>(name), options);
  }
};

static ParameterScope range(const SemanticProgram &p, int64_t low, int64_t high) {
  ParameterScope scope;
  scope.allParameters = false;
  unsigned w = p.parameters[0].sort.width;
  scope.ranges.push_back({0, APInt(w, low, true), APInt(w, high, true)});
  return scope;
}

static std::optional<TerminationCertificate> certify(const SemanticProgram &p,
                                                     const ParameterScope &scope) {
  TerminationOptions options;
  options.scope = scope;
  options.limits.replayFuel = 1u << 12;
  return analyzeTermination(p, z3Backend, options).certificate;
}

static SynthesisOptions synthesisOptions(const ParameterScope &scope, unsigned level = 0) {
  SynthesisOptions options;
  options.scope = scope;
  options.maxLevel = level;
  options.synthesis.maxCandidates = 5;
  return options;
}

static SymbolicCheckOptions symbolicOptions(const ParameterScope &scope) {
  SymbolicCheckOptions options;
  options.scope = scope;
  return options;
}

static const char *programs = R"(
func.func @gemm(%a: memref<9x9xi32>, %b: memref<9x9xi32>, %c: memref<9x9xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<9x9xi32>
        %y = memref.load %b[%k, %j] : memref<9x9xi32>
        %z = memref.load %c[%i, %j] : memref<9x9xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<9x9xi32>
      }
    }
  }
  return
}
func.func @no_i(%a: memref<9xi32>, %b: memref<9x9xi32>, %c: memref<9x9xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%k] : memref<9xi32>
        %y = memref.load %b[%k, %j] : memref<9x9xi32>
        %z = memref.load %c[%i, %j] : memref<9x9xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<9x9xi32>
      }
    }
  }
  return
}
func.func @hidden(%a: memref<9xi32>, %b: memref<9xi32>, %c: memref<9x9xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i] : memref<9xi32>
        %y = memref.load %b[%j] : memref<9xi32>
        %z = memref.load %c[%i, %j] : memref<9x9xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<9x9xi32>
      }
    }
  }
  return
}
func.func @triangle(%a: memref<9x9xi32>, %b: memref<9x9xi32>, %c: memref<9x9xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %t = arith.addi %i, %c1 : index
    scf.for %j = %c0 to %t step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<9x9xi32>
        %y = memref.load %b[%k, %j] : memref<9x9xi32>
        %z = memref.load %c[%i, %j] : memref<9x9xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<9x9xi32>
      }
    }
  }
  return
}
func.func @cyclic(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %c3 step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c4 step %c1 {
        %t = arith.addi %i, %k : index
        %u = arith.remui %t, %c4 : index
        %x = memref.load %a[%i, %k] : memref<4x4xi32>
        %y = memref.load %b[%u, %j] : memref<4x4xi32>
        %z = memref.load %c[%i, %j] : memref<4x4xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<4x4xi32>
      }
    }
  }
  return
}
func.func @accumulator(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<1xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  %r = scf.for %k = %c0 to %c4 step %c1 iter_args(%s = %zero) -> (i32) {
    %x = memref.load %a[%k] : memref<4xi32>
    %y = memref.load %b[%k] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %t = arith.addi %s, %m : i32
    scf.yield %t : i32
  }
  memref.store %r, %c[%c0] : memref<1xi32>
  return
}
func.func @lookup(%p: memref<4xindex>, %a: memref<4xi32>, %c: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %c4 step %c1 {
    %t = memref.load %p[%i] : memref<4xindex>
    %x = memref.load %a[%t] : memref<4xi32>
    %y = memref.load %a[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
func.func @wrong_read(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2x2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      %x = memref.load %a[%i] : memref<2xi32>
      %y = memref.load %b[%j] : memref<2xi32>
      %z = memref.load %c[%j, %i] : memref<2x2xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %j] : memref<2x2xi32>
    }
  }
  return
}
func.func @two_sites(%a: memref<9x9xi32>, %b: memref<9x9xi32>, %c: memref<9x9xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %p = arith.cmpi eq, %i, %c0 : index
        scf.if %p {
          %x = memref.load %a[%i, %k] : memref<9x9xi32>
          %y = memref.load %b[%k, %j] : memref<9x9xi32>
          %z = memref.load %c[%i, %j] : memref<9x9xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<9x9xi32>
        } else {
          %t = arith.addi %k, %c2 : index
          %x = memref.load %a[%i, %t] : memref<9x9xi32>
          %y = memref.load %b[%k, %j] : memref<9x9xi32>
          %z = memref.load %c[%i, %j] : memref<9x9xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<9x9xi32>
        }
      }
    }
  }
  return
}
func.func @other_root(%a: memref<9x9xi32>, %b: memref<9x9xi32>, %c: memref<9x9xi32>, %d: memref<9x9xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<9x9xi32>
        %y = memref.load %b[%k, %j] : memref<9x9xi32>
        %z = memref.load %d[%i, %j] : memref<9x9xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<9x9xi32>
      }
    }
  }
  return
}
func.func @late_read(%a: memref<9x9xi32>, %b: memref<9x9xi32>, %c: memref<9x9xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c6 = arith.constant 6 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %e = arith.cmpi eq, %i, %c6 : index
        %r = arith.select %e, %c0, %i : index
        %x = memref.load %a[%i, %k] : memref<9x9xi32>
        %y = memref.load %b[%k, %j] : memref<9x9xi32>
        %z = memref.load %c[%r, %j] : memref<9x9xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<9x9xi32>
      }
    }
  }
  return
}
func.func @tainted(%a: memref<9xi32>, %b: memref<1xi32>, %c: memref<9xi32>, %w: memref<1xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %z0 = arith.constant 0 : i32
  %o1 = arith.constant 1 : i32
  scf.for %i = %c0 to %n step %c1 {
    %l = memref.load %w[%c0] : memref<1xi32>
    %m1 = arith.andi %l, %o1 : i32
    %q = arith.index_cast %m1 : i32 to index
    %t = arith.addi %q, %i : index
    %e = arith.cmpi ult, %i, %n : index
    scf.if %e {
      %u = arith.addi %t, %c1 : index
      %x = memref.load %a[%i] : memref<9xi32>
      %y = memref.load %b[%c0] : memref<1xi32>
      %z = memref.load %c[%i] : memref<9xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i] : memref<9xi32>
    }
    memref.store %z0, %w[%c0] : memref<1xi32>
  }
  return
}
func.func @two_loops(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  scf.for %i = %c2 to %c4 step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
)";

/// Programs built so that exactly one obligation stands between a wrong claim and
/// a tier. Each is named after what it does to the checker.
static const char *adversaries = R"(
func.func @clamped(%a: memref<9xi32>, %b: memref<9xi32>, %c: memref<9xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %c4 step %c1 {
    %e = arith.cmpi ult, %i, %c2 : index
    %t = arith.select %e, %i, %c2 : index
    %x = memref.load %a[%t] : memref<9xi32>
    %y = memref.load %b[%t] : memref<9xi32>
    %z = memref.load %c[%t] : memref<9xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%t] : memref<9xi32>
  }
  return
}
func.func @maybe(%a: memref<9xi32>, %b: memref<9xi32>, %c: memref<9xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %e = arith.cmpi ne, %n, %c0 : index
  scf.if %e {
    scf.for %i = %c0 to %c3 step %c1 {
      %x = memref.load %a[%i] : memref<9xi32>
      %y = memref.load %b[%i] : memref<9xi32>
      %z = memref.load %c[%i] : memref<9xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i] : memref<9xi32>
    }
  }
  return
}
func.func @guarded_one(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %c4 = arith.constant 4 : index
  %e = arith.cmpi ult, %n, %c4 : index
  scf.if %e {
    %x = memref.load %a[%n] : memref<4xi32>
    %y = memref.load %b[%n] : memref<4xi32>
    %z = memref.load %c[%n] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%n] : memref<4xi32>
  }
  return
}
func.func @flat(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %x = memref.load %a[%n] : memref<4xi32>
  %y = memref.load %b[%n] : memref<4xi32>
  %z = memref.load %c[%n] : memref<4xi32>
  %m = arith.muli %x, %y : i32
  %s = arith.addi %z, %m : i32
  memref.store %s, %c[%n] : memref<4xi32>
  return
}
func.func @before_loop(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %w: memref<9xindex>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %e = arith.cmpi ult, %n, %c4 : index
  scf.if %e {
    %x = memref.load %a[%n] : memref<4xi32>
    %y = memref.load %b[%n] : memref<4xi32>
    %z = memref.load %c[%n] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%n] : memref<4xi32>
  }
  scf.for %i = %c0 to %c4 step %c1 {
    %t = arith.addi %i, %c1 : index
    memref.store %i, %w[%t] : memref<9xindex>
  }
  return
}
func.func @staircase(%a: memref<2x2xi32>, %b: memref<2x2xi32>, %c: memref<2x2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  %c7 = arith.constant 7 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %di = arith.muli %i, %c2 : index
    scf.for %j = %c0 to %c2 step %c1 {
      %ij = arith.addi %di, %j : index
      %dij = arith.muli %ij, %c2 : index
      scf.for %k = %c0 to %c2 step %c1 {
        %ik = arith.addi %di, %k : index
        %code = arith.addi %dij, %k : index
        %low = arith.cmpi ult, %code, %c3 : index
        %top = arith.cmpi eq, %code, %c7 : index
        %e = arith.ori %low, %top : i1
        scf.if %e {
          %kept = arith.addi %ik, %c1 : index
          %x = memref.load %a[%i, %k] : memref<2x2xi32>
          %y = memref.load %b[%k, %j] : memref<2x2xi32>
          %z = memref.load %c[%i, %j] : memref<2x2xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<2x2xi32>
        }
      }
    }
  }
  return
}
func.func @same_block(%a: memref<9xi32>, %b: memref<9xi32>, %c: memref<9xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<9xi32>
    %y = memref.load %b[%i] : memref<9xi32>
    %z = memref.load %c[%i] : memref<9xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<9xi32>
    %u = memref.load %c[%i] : memref<9xi32>
    %v = arith.addi %u, %m : i32
    memref.store %v, %c[%i] : memref<9xi32>
  }
  return
}
func.func @bounded(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
func.func @xbits(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c4 = arith.constant 4 : index
  %c8 = arith.constant 8 : index
  scf.for %t = %c0 to %c8 step %c1 {
    %low = arith.remui %t, %c2 : index
    %two = arith.remui %t, %c4 : index
    %high = arith.divui %t, %c4 : index
    %half = arith.divui %t, %c2 : index
    %d = arith.muli %high, %c2 : index
    %f = arith.addi %low, %d : index
    %x = memref.load %a[%f] : memref<4xi32>
    %y = memref.load %b[%half] : memref<4xi32>
    %z = memref.load %c[%two] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%two] : memref<4xi32>
  }
  return
}
func.func @mixed(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %d: memref<4xi32>, %e: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
    %w = memref.load %d[%i] : memref<4xi32>
    %v = arith.addi %w, %m : i32
    memref.store %v, %e[%i] : memref<4xi32>
  }
  return
}
func.func @split_j(%a: memref<9x9xi32>, %b: memref<9x9xi32>, %c: memref<9x9xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %p = arith.cmpi eq, %j, %c0 : index
        scf.if %p {
          %x = memref.load %a[%i, %k] : memref<9x9xi32>
          %y = memref.load %b[%k, %j] : memref<9x9xi32>
          %z = memref.load %c[%i, %j] : memref<9x9xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<9x9xi32>
        } else {
          %x = memref.load %a[%i, %k] : memref<9x9xi32>
          %y = memref.load %b[%k, %j] : memref<9x9xi32>
          %z = memref.load %c[%i, %j] : memref<9x9xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<9x9xi32>
        }
      }
    }
  }
  return
}
func.func @jump(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>) {
  %zero = arith.constant 0 : index
  cf.br ^outer(%zero : index)
^outer(%i: index):
  %two = arith.constant 2 : index
  %g = arith.cmpi ult, %i, %two : index
  cf.cond_br %g, ^start, ^exit
^start:
  %z0 = arith.constant 0 : index
  cf.br ^inner(%z0 : index)
^inner(%j: index):
  %z1 = arith.constant 0 : index
  %one = arith.constant 1 : index
  %big = arith.constant -1 : index
  %e = arith.cmpi eq, %i, %z1 : index
  %t = arith.select %e, %big, %one : index
  %h = arith.cmpi ult, %j, %t : index
  cf.cond_br %h, ^body, ^next
^body:
  %x = memref.load %a[%i] : memref<2xi32>
  %y = memref.load %b[%i] : memref<2xi32>
  %w = memref.load %c[%i] : memref<2xi32>
  %m = arith.muli %x, %y : i32
  %s = arith.addi %w, %m : i32
  memref.store %s, %c[%i] : memref<2xi32>
  %o = arith.constant 1 : index
  %nj = arith.addi %j, %o : index
  cf.br ^inner(%nj : index)
^next:
  %p = arith.constant 1 : index
  %ni = arith.addi %i, %p : index
  cf.br ^outer(%ni : index)
^exit:
  return
}
)";

/// Answers from Z3, except that scripts holding `pattern` come back unknown.
struct Selective : SmtBackend {
  std::string pattern;
  unsigned refused = 0;
  explicit Selective(std::string pattern) : pattern(std::move(pattern)) {}
  SmtResponse check(const std::string &script, const SmtLimits &limits, bool model) override {
    if (script.find(pattern) != std::string::npos) {
      ++refused;
      SmtResponse response;
      response.detail = "refused by the test";
      return response;
    }
    return z3Backend.check(script, limits, model);
  }
  std::string version() const override { return "selective"; }
};

static SiteGroup groupOf(const SemanticProgram &p, unsigned site = 0) {
  SiteGroup group;
  group.output = p.sites[site].output->root;
  group.first = p.sites[site].a.root;
  group.second = p.sites[site].b.root;
  for (const MacSite &other : p.sites)
    if (other.output && other.output->root == *group.output &&
        other.a.root == group.first && other.b.root == group.second)
      group.sites.push_back(other.id);
  return group;
}

static EnumerationOrder oneLevel(const SemanticProgram &p, Term value, uint64_t low, uint64_t high) {
  OrderLevel level;
  unsigned width = value->sort.width;
  level.value = value;
  level.lower = p.terms->bv(APInt(width, low));
  level.upper = p.terms->bv(APInt(width, high));
  EnumerationOrder order;
  order.levels = {level};
  return order;
}

static CoordinateFamily batchOnly(Term value) {
  CoordinateFamily family;
  family.axes[AxisP] = {value};
  return family;
}

/// proveCoverage with no V3 before it: what S1 to S3 say on their own.
static bool coverage(const SemanticProgram &p, const TerminationCertificate &proof,
                     const ParameterScope &scope, const EnumerationOrder &order,
                     std::string &why) {
  std::string error;
  Term formula = scopeFormula(p, scope, error);
  std::map<unsigned, Term> invariants;
  for (const auto &[cut, term] : proof.invariants)
    invariants[cut] = adoptProofTerm(p, term, error);
  std::vector<unsigned> ids, locations;
  for (const MacSite &site : p.sites) {
    ids.push_back(site.id);
    locations.push_back(site.location);
  }
  SiteArrivals arrivals = reachSites(p, invariants, proof.cuts, ids, formula, PathLimits());
  CHECK(arrivals.complete);
  AskUnsat ask = [&](const std::string &id, const std::string &where, Term query) {
    return z3Backend.check(obligationScript(id + " " + where, query, {}), SmtLimits(), false).answer;
  };
  return proveCoverage(p, order, arrivals, locations, formula, 64, ask, why);
}

static std::vector<const AccessDomain *> domainsOf(const DomainEnumeration &enumeration,
                                                   unsigned group = 0) {
  std::vector<const AccessDomain *> out;
  for (const PointEvidence &point : enumeration.points)
    if (point.domains[group])
      out.push_back(&*point.domains[group]);
  return out;
}

static DomainEnumeration liveDomains(const SemanticProgram &p, const TerminationCertificate &proof,
                                     const ParameterScope &scope) {
  EnumerationOptions options;
  options.scope = scope;
  options.liveStatePoints = true;
  return enumerateDomain(p, &proof, options, z3Backend);
}

static void testGrammar() {
  // Levels only widen.
  bool add = false, product = false, compare = false;
  for (unsigned level = 0; level <= GrammarSpec::MaxLevel; ++level) {
    GrammarSpec spec = GrammarSpec::atLevel(level);
    CHECK(!add || spec.addition);
    CHECK(!product || spec.product);
    CHECK(!compare || spec.comparison);
    add = spec.addition;
    product = spec.product;
    compare = spec.comparison;
  }
  CHECK(GrammarSpec::atLevel(0).maxSize == 1 && !GrammarSpec::atLevel(0).addition);
  CHECK(GrammarSpec::atLevel(1).addition && !GrammarSpec::atLevel(1).product);
  CHECK(GrammarSpec::atLevel(2).remainder && !GrammarSpec::atLevel(2).comparison);
  CHECK(GrammarSpec::atLevel(3).conditional && !GrammarSpec::atLevel(3).bits);
  CHECK(GrammarSpec::atLevel(4).bits);

  // Definedness is separate from the total value.
  TermContext terms;
  Term x = terms.param(0, Sort::bv(8)), y = terms.param(1, Sort::bv(8));
  SemanticProgram empty;
  auto holds = [&](Term t, int64_t a, int64_t b) {
    EntryState entry;
    entry.parameters = {APInt(8, a, true), APInt(8, b, true)};
    auto value = evaluateTerm(empty, entry, Configuration(), t);
    return value && value->isOne();
  };
  CHECK(holds(definedness(terms, terms.binary(TermOp::Add, x, y)), 0, 0));
  Term quotient = definedness(terms, terms.binary(TermOp::UDiv, x, y));
  CHECK(holds(quotient, 7, 2) && !holds(quotient, 7, 0));
  Term remainder = definedness(terms, terms.binary(TermOp::URem, x, y));
  CHECK(!holds(remainder, 7, 0));
  Term signedDiv = definedness(terms, terms.binary(TermOp::SDiv, x, y));
  CHECK(holds(signedDiv, 7, -1) && !holds(signedDiv, -128, -1) && !holds(signedDiv, 7, 0));
  Term shift = definedness(terms, terms.binary(TermOp::Shl, x, y));
  CHECK(holds(shift, 1, 7) && !holds(shift, 1, 8));
  // Inner terms count, through any depth.
  Term nested = definedness(terms, terms.binary(TermOp::Add, x, terms.binary(TermOp::URem, x, y)));
  CHECK(!holds(nested, 1, 0));
}

static void testLiveState() {
  Module m(programs);
  SemanticProgram p = m.import("hidden");
  ParameterScope scope = range(p, 2, 2);
  auto certificate = certify(p, scope);
  CHECK(certificate.has_value());
  if (!certificate)
    return;
  EnumerationOptions plain;
  plain.scope = scope;
  DomainEnumeration base = enumerateDomain(p, &*certificate, plain, z3Backend);
  CHECK(!base.sampled);
  EnumerationOptions live = plain;
  live.liveStatePoints = true;
  DomainEnumeration extended = enumerateDomain(p, &*certificate, live, z3Backend);
  const AccessTable &a = base.points[0].domains[0]->table();
  const AccessTable &b = extended.points[0].domains[0]->table();
  // k reaches no address, so without live state two events share a point.
  CHECK(a.events().size() == 12 && b.events().size() == 12);
  CHECK(a.pointCount() == 6);
  CHECK(b.pointCount() == 12);
  CHECK(b.coordinatesUniform);
  // Identical accesses, in the same order, at the same steps.
  for (size_t e = 0; e < a.events().size(); ++e) {
    CHECK(a.events()[e].step == b.events()[e].step);
    CHECK(a.view(a.events()[e].h) == b.view(b.events()[e].h));
    CHECK(a.view(a.events()[e].f) == b.view(b.events()[e].f));
    CHECK(a.view(a.events()[e].g) == b.view(b.events()[e].g));
  }

  // A value read from memory is no coordinate.
  SemanticProgram accumulate = m.import("accumulator");
  auto independent = memoryIndependentState(accumulate);
  unsigned tainted = 0;
  for (bool free : independent)
    tainted += !free;
  CHECK(tainted >= 1);
  SemanticProgram gemm = m.import("gemm");
  auto free = memoryIndependentState(gemm);
  CHECK(std::count(free.begin(), free.end(), false) == 0); // Loads stay in one block.

  // Samples: inside the scope, under the limit, and marked as such.
  SemanticProgram g = m.import("gemm");
  ParameterScope wide = range(g, 0, 4);
  auto proof = certify(g, wide);
  CHECK(proof.has_value());
  EnumerationOptions sample;
  sample.scope = wide;
  sample.onlyPoints = {{APInt(16, 2)}, {APInt(16, 4)}};
  DomainEnumeration two = enumerateDomain(g, &*proof, sample, z3Backend);
  CHECK(two.gate == GateStatus::Open && two.sampled && two.points.size() == 2);
  sample.onlyPoints = {{APInt(16, 9)}};
  CHECK(enumerateDomain(g, &*proof, sample, z3Backend).gate == GateStatus::Unknown);
  sample.onlyPoints = {{APInt(16, 1), APInt(16, 1)}};
  CHECK(enumerateDomain(g, &*proof, sample, z3Backend).gate == GateStatus::Unknown);
  sample.onlyPoints = {{APInt(16, 1)}, {APInt(16, 2)}};
  sample.limits.maxPoints = 1;
  CHECK(enumerateDomain(g, &*proof, sample, z3Backend).gate == GateStatus::Unknown);
}

static void testStructure() {
  // A view may ignore an axis: not genuine, and exactly as full.
  AccessTable t;
  t.coordinatesUniform = true;
  CoordinateCandidate c;
  uint64_t step = 0;
  for (int64_t i = 0; i < 2; ++i)
    for (int64_t j = 0; j < 2; ++j)
      for (int64_t k = 0; k < 2; ++k) {
        IndexTuple x{APInt(64, i), APInt(64, j), APInt(64, k)};
        IndexTuple h{APInt(64, i), APInt(64, j)}, f{APInt(64, k)}, g{APInt(64, k), APInt(64, j)};
        t.add(0, step++, x, h, h, f, g);
        c.p.push_back(0);
        c.i.push_back(i);
        c.j.push_back(j);
        c.k.push_back(k);
      }
  CandidateReport r = inspectCandidate(t, c);
  CHECK(r.decompositionValid && !r.injectiveF && !r.genuine);
  CHECK(r.structurallyFull);
  CHECK(r.eligibility == Eligibility::ForLaterVerification);
  // Each of its conditions.
  CoordinateCandidate flat = c;
  std::fill(flat.j.begin(), flat.j.end(), 0u);
  CHECK(!inspectCandidate(t, flat).structurallyFull);
  CoordinateCandidate batches = c;
  batches.p = c.k;
  CHECK(!inspectCandidate(t, batches).structurallyFull); // K is 1 in each batch.
}

static void testFinite() {
  Module m(programs);
  SemanticProgram p = m.import("gemm");
  ParameterScope scope = range(p, 1, 3);
  auto certificate = certify(p, scope);
  CHECK(certificate.has_value());
  SynthesisResult r = synthesizeProgram(p, &*certificate, synthesisOptions(scope), z3Backend);
  CHECK(r.groups.size() == 1 && !r.groups[0].candidates.empty());
  const VerifiedCandidate &best = r.groups[0].candidates[0];
  const CoordinateFamily &family = best.family;
  CHECK(best.finite.tier() == EvidenceTier::ScopeEnumerated);
  CHECK(best.finite.equivalence().kind() == ProofStatus::Kind::Unknown);
  CHECK(best.finite.points().size() == 3);
  TermContext &terms = *p.terms;

  auto finite = [&](const CoordinateFamily &f) {
    return checkFamilyFinite(p, r.enumeration, 0, f);
  };
  CHECK(finite(family).status() == FamilyStatus::Verified);

  // Dropping a coordinate breaks a dependence, and names the point.
  CoordinateFamily wrong = family;
  wrong.axes[AxisI].clear();
  CheckedFamily refuted = finite(wrong);
  CHECK(refuted.status() == FamilyStatus::Refuted && refuted.failingPoint().has_value());
  CHECK(refuted.tier() == EvidenceTier::Proposed);

  // A term undefined at an event is refused with no value used.
  CoordinateFamily undefined = family;
  Term i = family.axes[AxisI][0];
  undefined.axes[AxisI][0] = terms.binary(TermOp::UDiv, terms.bv(APInt(16, 12)), i);
  CheckedFamily division = finite(undefined);
  CHECK(division.status() == FamilyStatus::Refuted);
  CHECK(division.reason().find("undefined") != std::string::npos);

  // Memory and floats are not coordinates.
  CoordinateFamily memory = family;
  memory.axes[AxisI][0] = terms.load(terms.memVar(p.roots[0].sort), {i, i});
  CHECK(finite(memory).status() == FamilyStatus::Refuted);
  CHECK(finite(memory).tier() == EvidenceTier::Proposed);

  // A state not in the table has no value.
  CoordinateFamily missing = family;
  missing.axes[AxisI][0] = terms.state(9999, Sort::bv(16));
  CHECK(finite(missing).status() != FamilyStatus::Verified);

  // A sample is a sample: a verified family stays at Point.
  SynthesisOptions options = synthesisOptions(scope);
  EnumerationOptions sampled;
  sampled.scope = scope;
  sampled.liveStatePoints = true;
  sampled.onlyPoints = {{APInt(16, 2)}};
  DomainEnumeration one = enumerateDomain(p, &*certificate, sampled, z3Backend);
  CheckedFamily point = checkFamilyFinite(p, one, 0, family);
  CHECK(point.status() == FamilyStatus::Verified && point.tier() == EvidenceTier::Point);

  // A closed gate gives nothing.
  DomainEnumeration closed = enumerateDomain(p, nullptr, sampled, z3Backend);
  CHECK(checkFamilyFinite(p, closed, 0, family).status() == FamilyStatus::Unknown);

  // A point that stops leaves the scope unfinished, whatever the others say.
  Module small(R"(
func.func @bounds(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<2xi32>
    %y = memref.load %b[%i] : memref<2xi32>
    %z = memref.load %c[%i] : memref<2xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<2xi32>
  }
  return
}
)");
  SemanticProgram b = small.import("bounds");
  ParameterScope bscope = range(b, 1, 3);
  auto bproof = certify(b, bscope);
  CHECK(bproof.has_value());
  SynthesisResult bounded = synthesizeProgram(b, &*bproof, synthesisOptions(bscope), z3Backend);
  CHECK(bounded.enumeration.points[2].run == RunStatus::UndefinedBehavior);
  CHECK(!bounded.groups[0].candidates.empty());
  for (const VerifiedCandidate &candidate : bounded.groups[0].candidates)
    CHECK(candidate.finite.tier() == EvidenceTier::Point);
}

static void testSymbolic() {
  Module m(programs);
  SemanticProgram p = m.import("gemm");
  ParameterScope scope = range(p, 1, 3);
  auto certificate = certify(p, scope);
  SynthesisResult r = synthesizeProgram(p, &*certificate, synthesisOptions(scope), z3Backend);
  const VerifiedCandidate &best = r.groups[0].candidates[0];
  CHECK(best.symbolic.tier() == EvidenceTier::SymbolicExact);
  CHECK(best.symbolic.status() == FamilyStatus::Verified);
  CHECK(best.symbolic.coverage().form == DomainForm::ExactClosedForm);
  CHECK(admitsClaim(best.symbolic.coverage(), DomainClaim::Coverage).admitted);
  CHECK(best.symbolic.symbolicBox());
  CHECK(best.symbolic.equivalence().kind() == ProofStatus::Kind::Unknown);
  CHECK(best.order.has_value());
  const SiteGroup &group = r.enumeration.groups[0];
  const CoordinateFamily &family = best.family;
  const EnumerationOrder &order = *best.order;
  TermContext &terms = *p.terms;
  SymbolicCheckOptions options = symbolicOptions(scope);

  auto check = [&](const CoordinateFamily &f, const EnumerationOrder *o,
                   const TerminationCertificate *c, SmtBackend &backend) {
    return checkFamilySymbolic(p, c, group, f, o, options, backend);
  };
  auto tierOf = [&](const CoordinateFamily &f, const EnumerationOrder &o) {
    return check(f, &o, &*certificate, z3Backend).tier();
  };
  CHECK(tierOf(family, order) == EvidenceTier::SymbolicExact);
  // The bound of a nest is a kept fact, and its proof is in the record.
  {
    SemanticProgram tri = m.import("triangle");
    ParameterScope triangle = range(tri, 2, 3);
    auto proof = certify(tri, triangle);
    SynthesisResult result = synthesizeProgram(tri, &*proof, synthesisOptions(triangle), z3Backend);
    bool definition = false;
    for (const VerifiedCandidate &candidate : result.groups[0].candidates)
      for (const ObligationRecord &record : candidate.symbolic.records())
        definition = definition || record.id == "I";
    CHECK(definition);
    for (const VerifiedCandidate &candidate : result.groups[0].candidates)
      for (const ObligationRecord &record : candidate.symbolic.records())
        if (record.id == "I")
          CHECK(record.discharged);
  }
  bool seen[6] = {};
  for (const ObligationRecord &record : best.symbolic.records()) {
    CHECK(record.discharged);
    for (unsigned n = 0; n < 6; ++n)
      seen[n] = seen[n] || record.id == std::vector<std::string>{"V0", "V1h", "V2", "V3", "S1", "S2"}[n];
  }
  for (bool one : seen)
    CHECK(one);

  // No certificate, a forged one, or one for another program: no tier.
  CHECK(check(family, &order, nullptr, z3Backend).status() == FamilyStatus::Unknown);
  TerminationCertificate forged = *certificate;
  forged.levels.clear();
  CHECK(check(family, &order, &forged, z3Backend).tier() == EvidenceTier::Proposed);
  TerminationCertificate other = *certificate;
  other.fingerprint = "other";
  CHECK(check(family, &order, &other, z3Backend).tier() == EvidenceTier::Proposed);
  // A proof for a narrow scope does not reach a wider one.
  options.scope = range(p, 1, 3);
  SymbolicCheckOptions wide = symbolicOptions(range(p, -8, 200));
  CheckedFamily beyond = checkFamilySymbolic(p, &*certificate, group, family, &order, wide, z3Backend);
  CHECK(beyond.tier() != EvidenceTier::SymbolicExact);

  // A group must be all the sites of its roots, and only those.
  SiteGroup partial = group;
  partial.sites.clear();
  CHECK(checkFamilySymbolic(p, &*certificate, partial, family, &order, options, z3Backend).status() ==
        FamilyStatus::Unknown);
  SiteGroup misnamed = group;
  misnamed.first = *group.output;
  CHECK(checkFamilySymbolic(p, &*certificate, misnamed, family, &order, options, z3Backend).status() ==
        FamilyStatus::Unknown);
  {
    SemanticProgram two = m.import("two_sites");
    CHECK(two.sites.size() == 2);
    ParameterScope all;
    auto proof = certify(two, all);
    SynthesisResult both = synthesizeProgram(two, &*proof, synthesisOptions(all), z3Backend);
    CHECK(both.enumeration.groups.size() == 1 && both.enumeration.groups[0].sites.size() == 2);
    SiteGroup half = both.enumeration.groups[0];
    half.sites.pop_back();
    CHECK(!both.groups[0].candidates.empty());
    const VerifiedCandidate &candidate = both.groups[0].candidates[0];
    CheckedFamily part = checkFamilySymbolic(
        two, &*proof, half, candidate.family,
        candidate.order ? &*candidate.order : nullptr, symbolicOptions(all), z3Backend);
    CHECK(part.status() == FamilyStatus::Unknown);
    CHECK(part.reason().find("group") != std::string::npos);
  }

  // Without an order there is nothing to range over, so no tier at all.
  CHECK(check(family, nullptr, &*certificate, z3Backend).tier() == EvidenceTier::Proposed);

  // The order must be exactly the coordinates.
  EnumerationOrder fewer = order;
  fewer.levels.pop_back();
  CHECK(tierOf(family, fewer) == EvidenceTier::Proposed);
  EnumerationOrder extra = order;
  extra.levels.push_back(order.levels[0]);
  CHECK(tierOf(family, extra) == EvidenceTier::Proposed);
  EnumerationOrder foreign = order;
  foreign.levels[0].value = terms.state(9999, Sort::bv(16));
  CHECK(tierOf(family, foreign) == EvidenceTier::Proposed);
  EnumerationOrder stateBound = order;
  stateBound.levels[1].upper = order.levels[0].value;
  CHECK(tierOf(family, stateBound) == EvidenceTier::Proposed);
  EnumerationOrder inner = order;
  inner.levels[0].upperOuter = 2; // A level can only read outer levels.
  CHECK(tierOf(family, inner) == EvidenceTier::Proposed);

  // A set too wide contains the coordinates, so V3 holds. The events do not cover
  // it, so coverage must fail: Valid, and never Exact.
  EnumerationOrder tooWide = order;
  unsigned fixed = 0;
  for (unsigned n = 0; n < tooWide.levels.size(); ++n)
    if (tooWide.levels[n].upper->op == TermOp::Const)
      fixed = n;
  tooWide.levels[fixed].upper = terms.binary(TermOp::Add, order.levels[fixed].upper,
                                             terms.bv(APInt(16, 1)));
  CheckedFamily valid = check(family, &tooWide, &*certificate, z3Backend);
  CHECK(valid.tier() == EvidenceTier::SymbolicValid);
  CHECK(valid.status() == FamilyStatus::Verified);
  CHECK(valid.reason().find("coverage is open") != std::string::npos);
  CHECK(valid.coverage().form != DomainForm::ExactClosedForm);
  // A set too narrow fails V3, and no coverage is tried on a range that fails.
  EnumerationOrder tooNarrow = order;
  tooNarrow.levels[fixed].upper = terms.binary(TermOp::Sub, order.levels[fixed].upper,
                                               terms.bv(APInt(16, 1)));
  CheckedFamily narrow = check(family, &tooNarrow, &*certificate, z3Backend);
  CHECK(narrow.tier() == EvidenceTier::Proposed && narrow.failingPoint().has_value());
  // The wrong level order enumerates the same set in another order. Coverage
  // says the events follow this order, so it must fail.
  EnumerationOrder swapped = order;
  std::swap(swapped.levels[1], swapped.levels[2]);
  for (OrderLevel &level : swapped.levels)
    level.upperOuter = -1;
  CheckedFamily reordered = check(family, &swapped, &*certificate, z3Backend);
  CHECK(reordered.tier() == EvidenceTier::SymbolicValid);

  // A wrong family: each dependence is its own obligation.
  auto without = [&](unsigned axis) {
    CoordinateFamily f = family;
    Term moved = f.axes[axis][0];
    f.axes[axis].clear();
    (void)moved;
    return f;
  };
  for (unsigned axis : {AxisI, AxisJ, AxisK}) {
    CoordinateFamily f = without(axis);
    EnumerationOrder o;
    for (const OrderLevel &level : order.levels)
      if (level.value != family.axes[axis][0])
        o.levels.push_back(level);
    for (OrderLevel &level : o.levels)
      level.upperOuter = -1;
    CheckedFamily failed = check(f, &o, &*certificate, z3Backend);
    CHECK(failed.tier() == EvidenceTier::Proposed);
    CHECK(failed.status() != FamilyStatus::Verified);
    bool undischarged = false;
    for (const ObligationRecord &record : failed.records())
      undischarged = undischarged || (!record.discharged && record.id.rfind("V1", 0) == 0);
    CHECK(undischarged);
  }

  // Undefined on the domain: i divides, and i can be zero.
  CoordinateFamily division = family;
  Term i = family.axes[AxisI][0];
  division.axes[AxisI][0] = terms.binary(TermOp::UDiv, terms.bv(APInt(16, 12)), i);
  EnumerationOrder divided = order;
  for (OrderLevel &level : divided.levels)
    if (level.value == i)
      level.value = division.axes[AxisI][0];
  CheckedFamily undefined = check(division, &divided, &*certificate, z3Backend);
  CHECK(undefined.tier() == EvidenceTier::Proposed);
  CHECK(undefined.reason().find("V0") != std::string::npos);

  // A shadow is no coordinate: it says nothing about the iteration.
  CoordinateFamily poison = family;
  poison.axes[AxisI][0] = terms.zext(terms.shadow(0), 16);
  CheckedFamily shadowed = check(poison, &order, &*certificate, z3Backend);
  CHECK(shadowed.status() == FamilyStatus::Refuted);
  CHECK(shadowed.reason().find("shadow") != std::string::npos);

  // Memory leaves are refused before any solver is asked.
  CoordinateFamily memory = family;
  memory.axes[AxisI][0] = terms.load(terms.memVar(p.roots[0].sort), {i, i});
  CHECK(check(memory, &order, &*certificate, z3Backend).status() == FamilyStatus::Refuted);

  // Unknown is never proof, wherever it lands.
  for (const char *name : {"; V0", "; V1h", "; V1f", "; V1g", "; V3"}) {
    Selective backend(name);
    CheckedFamily answer = check(family, &order, &*certificate, backend);
    CHECK(backend.refused > 0);
    CHECK(answer.tier() == EvidenceTier::Proposed);
    CHECK(answer.status() == FamilyStatus::Unknown);
  }
  for (const char *name : {"; S1", "; S2", "; prune"}) {
    Selective backend(name);
    CheckedFamily answer = check(family, &order, &*certificate, backend);
    CHECK(backend.refused > 0);
    CHECK(answer.tier() == EvidenceTier::SymbolicValid);
    CHECK(answer.coverage().form != DomainForm::ExactClosedForm);
  }
  // V2 alone open: the simulation gives the bijection by itself.
  {
    Selective backend("; V2");
    CheckedFamily answer = check(family, &order, &*certificate, backend);
    CHECK(backend.refused > 0);
    CHECK(answer.tier() == EvidenceTier::SymbolicExact);
    bool open = false;
    for (const ObligationRecord &record : answer.records())
      open = open || (record.id == "V2" && !record.discharged);
    CHECK(open);
    CHECK(answer.reason().find("could not") != std::string::npos);
  }
  // V2 and coverage both open: nothing says that coordinates identify events.
  {
    struct Both : Selective {
      using Selective::Selective;
      SmtResponse check(const std::string &script, const SmtLimits &limits, bool model) override {
        if (script.find("; V2") != std::string::npos || script.find("; S2") != std::string::npos) {
          ++refused;
          return SmtResponse();
        }
        return z3Backend.check(script, limits, model);
      }
    } backend("");
    CheckedFamily answer = check(family, &order, &*certificate, backend);
    CHECK(answer.tier() == EvidenceTier::Proposed && answer.status() == FamilyStatus::Unknown);
  }
  // A silent solver leaves nothing at all.
  Selective silent("(check-sat)");
  CHECK(check(family, &order, &*certificate, silent).tier() == EvidenceTier::Proposed);
  // Limits end in Unknown.
  options.limits.maxQueries = 3;
  CHECK(check(family, &order, &*certificate, z3Backend).tier() == EvidenceTier::Proposed);
  options.limits = SymbolicLimits2();
  options.limits.maxPairs = 0;
  CHECK(check(family, &order, &*certificate, z3Backend).tier() == EvidenceTier::Proposed);
  options.limits = SymbolicLimits2();
  options.limits.paths.maxPathLength = 0;
  CHECK(check(family, &order, &*certificate, z3Backend).tier() == EvidenceTier::Proposed);
  options.limits = SymbolicLimits2();
  options.limits.maxUnfolding = 1;
  CheckedFamily shallow = check(family, &order, &*certificate, z3Backend);
  CHECK(shallow.tier() == EvidenceTier::SymbolicValid);
  CHECK(shallow.reason().find("limit") != std::string::npos);

  // A non-oblivious program or an accumulator group has no symbolic tier.
  SemanticProgram lookup = m.import("lookup");
  SiteGroup any;
  any.output = 2;
  any.sites = {0};
  CHECK(checkFamilySymbolic(lookup, &*certificate, any, family, &order, options, z3Backend).status() ==
        FamilyStatus::Unknown);
  SiteGroup scalar;
  scalar.unsupported = "no output";
  CHECK(checkFamilySymbolic(p, &*certificate, scalar, family, &order, options, z3Backend).status() ==
        FamilyStatus::Unknown);
}

static GroupSynthesis synthesize(const SemanticProgram &p, const ParameterScope &scope,
                                 unsigned level = 0) {
  auto certificate = certify(p, scope);
  CHECK(certificate.has_value());
  SynthesisResult r = synthesizeProgram(p, &*certificate, synthesisOptions(scope, level), z3Backend);
  CHECK(r.groups.size() == 1);
  return r.groups.empty() ? GroupSynthesis() : r.groups[0];
}

static void testPrograms() {
  Module m(programs);

  // G3a cannot split this: A ignores I. Here it is full and exact.
  SemanticProgram noI = m.import("no_i");
  GroupSynthesis g = synthesize(noI, range(noI, 2, 3));
  CHECK(g.status == RecoveryStatus::CandidateFound);
  CHECK(!g.candidates.empty());
  if (!g.candidates.empty()) {
    const VerifiedCandidate &best = g.candidates[0];
    CHECK(best.symbolic.tier() == EvidenceTier::SymbolicExact);
    for (const PointFacts &point : best.finite.points()) {
      CHECK(point.report.structurallyFull);
      CHECK(!point.report.genuine);
      CHECK(!point.report.injectiveF);
      CHECK(point.report.eligibility == Eligibility::ForLaterVerification);
    }
  }

  // Alternatives have distinct structures, none a copy of another.
  std::set<std::array<size_t, 4>> shapes;
  for (const VerifiedCandidate &candidate : g.candidates) {
    std::array<size_t, 4> shape;
    for (unsigned axis = 0; axis < 4; ++axis)
      shape[axis] = candidate.family.axes[axis].size();
    shapes.insert(shape);
  }
  CHECK(g.candidates.size() > 1);
  CHECK(shapes.size() > 1);

  // k reaches no address, and is a real axis.
  SemanticProgram hidden = m.import("hidden");
  GroupSynthesis h = synthesize(hidden, range(hidden, 2, 3));
  CHECK(h.status == RecoveryStatus::CandidateFound);
  if (!h.candidates.empty()) {
    CHECK(h.candidates[0].symbolic.tier() == EvidenceTier::SymbolicExact);
    const CandidateReport &report = h.candidates[0].finite.points()[0].report;
    CHECK(report.multiplicity == MultiplicityForm::Unit);
    CHECK(report.batches[0].k == 2);
  }

  // Triangular: the axes are right, the domain is partial, and exact coverage is
  // of that domain. Not a box, and not presented as one.
  SemanticProgram tri = m.import("triangle");
  GroupSynthesis t = synthesize(tri, range(tri, 2, 4));
  bool natural = false;
  for (const VerifiedCandidate &candidate : t.candidates) {
    const auto &axes = candidate.family.axes;
    if (axes[AxisP].empty() && axes[AxisI].size() == 1 && axes[AxisJ].size() == 1 &&
        axes[AxisK].size() == 1) {
      natural = true;
      CHECK(candidate.symbolic.tier() == EvidenceTier::SymbolicExact);
      CHECK(!candidate.symbolic.symbolicBox());
      CHECK(candidate.finite.points().back().report.cartesian == CartesianForm::Partial);
      CHECK(!candidate.finite.points().back().report.structurallyFull);
    }
  }
  CHECK(natural);

  // A modular coordinate needs the wider grammar, and the search reaches it.
  SemanticProgram cyclic = m.import("cyclic");
  ParameterScope one = range(cyclic, 0, 0);
  GroupSynthesis flat = synthesize(cyclic, one, 0);
  GroupSynthesis wider = synthesize(cyclic, one, 2);
  auto fullCount = [](const GroupSynthesis &group) {
    unsigned n = 0;
    for (const VerifiedCandidate &candidate : group.candidates)
      n += candidate.finite.points()[0].report.structurallyFull;
    return n;
  };
  CHECK(fullCount(wider) > 0);
  CHECK(wider.level >= flat.level);

  // Two sibling loops share a group and no state: i of one loop is undefined at
  // the other's events, so no one term is a coordinate of both. Unknown is the
  // whole answer, which says nothing about G3a's labels for the same table.
  SemanticProgram loops = m.import("two_loops");
  GroupSynthesis l = synthesize(loops, ParameterScope{});
  CHECK(l.status == RecoveryStatus::Unknown && l.candidates.empty());

  // Scalar accumulators stay unsupported, with no output invented.
  SemanticProgram scalar = m.import("accumulator");
  SynthesisResult s = synthesizeProgram(scalar, nullptr, synthesisOptions({}), z3Backend);
  CHECK(s.enumeration.gate == GateStatus::Unsupported);
  CHECK(s.groups.empty());

  // A trace that depends on memory is not enumerated.
  SemanticProgram data = m.import("lookup");
  CHECK(synthesizeProgram(data, nullptr, synthesisOptions({}), z3Backend).enumeration.gate ==
        GateStatus::Unsupported);

  // No certificate, no search.
  SemanticProgram p = m.import("gemm");
  ParameterScope scope = range(p, 1, 2);
  SynthesisResult none = synthesizeProgram(p, nullptr, synthesisOptions(scope), z3Backend);
  CHECK(none.enumeration.gate == GateStatus::Unknown && none.groups.empty());

  // C0 fails on an exact event: no += exists, whatever the coordinates.
  SemanticProgram wrong = m.import("wrong_read");
  GroupSynthesis w = synthesize(wrong, ParameterScope{});
  CHECK(w.status == RecoveryStatus::CandidateRefuted);
  CHECK(w.candidates.empty());

  // A search with no room says Unknown, and that is all it says.
  auto certificate = certify(p, scope);
  SynthesisOptions tight = synthesisOptions(scope);
  tight.synthesis.maxRounds = 0;
  SynthesisResult limited = synthesizeProgram(p, &*certificate, tight, z3Backend);
  CHECK(limited.groups.size() == 1);
  // The seed may already hold; what matters is that nothing is claimed unchecked.
  for (const VerifiedCandidate &candidate : limited.groups[0].candidates)
    CHECK(candidate.finite.status() == FamilyStatus::Verified);
  Selective silent("(check-sat)");
  EnumerationOptions enumerate;
  enumerate.scope = scope;
  SynthesisResult quiet = synthesizeProgram(p, &*certificate, synthesisOptions(scope), silent);
  CHECK(quiet.enumeration.gate == GateStatus::Unknown);

  // A scope too large to run is sampled, stays Point, and may still reach a
  // symbolic tier: the two are separate evidence.
  SemanticProgram large = m.import("gemm");
  ParameterScope big = range(large, 1, 9);
  auto bigProof = certify(large, big);
  SynthesisOptions sampled = synthesisOptions(big);
  sampled.limits.maxPoints = 3;
  SynthesisResult wide = synthesizeProgram(large, &*bigProof, sampled, z3Backend);
  CHECK(wide.enumeration.sampled);
  CHECK(!wide.groups[0].candidates.empty());
  const VerifiedCandidate &best = wide.groups[0].candidates[0];
  CHECK(best.finite.tier() == EvidenceTier::Point);
  CHECK(best.symbolic.tier() == EvidenceTier::SymbolicExact);

  // The JSON repeats that nothing here authorizes a transformation.
  std::string text = toJSON(large, wide, false);
  CHECK(text.find("\"authorizes_lowering\": false") != std::string::npos);
  CHECK(text.find("\"equivalence\": \"UNKNOWN\"") != std::string::npos);
  CHECK(text == toJSON(large, wide, false));
}

/// Regressions of the independent review, each from its own counterexample.
static void testReviewFindings() {
  Module m(programs);

  // A read of another root: no += at any index. Both reports, and any family.
  SemanticProgram root = m.import("other_root");
  ParameterScope all;
  auto rootProof = certify(root, all);
  CHECK(rootProof.has_value());
  SynthesisResult r = synthesizeProgram(root, &*rootProof, synthesisOptions(all), z3Backend);
  CHECK(r.enumeration.gate == GateStatus::Unsupported);
  CHECK(r.enumeration.reason.find("root") != std::string::npos);
  CHECK(r.groups.empty());
  EnumerationOptions rootOptions;
  rootOptions.scope = all;
  CoordinateResult canonical = recoverCoordinates(root, &*rootProof, rootOptions, z3Backend);
  CHECK(canonical.enumeration.gate == GateStatus::Unsupported);
  CHECK(canonical.points.empty());
  // A caller's group cannot bring it back.
  CoordinateFamily any;
  SiteGroup forged;
  forged.output = root.sites[0].output->root;
  forged.first = root.sites[0].a.root;
  forged.second = root.sites[0].b.root;
  forged.sites = {0};
  EnumerationOrder none;
  OrderLevel level;
  level.value = level.lower = level.upper = root.terms->bv(APInt(16, 1));
  none.levels.push_back(level);
  CHECK(checkFamilySymbolic(root, &*rootProof, forged, any, &none, symbolicOptions(all), z3Backend)
            .reason()
            .find("group") != std::string::npos);

  // A wrong read at an unsampled point: C0 holds at every sample, and fails at
  // n = 7. Nothing symbolic may stand, and the run of that point refutes the form.
  SemanticProgram late = m.import("late_read");
  ParameterScope scope = range(late, 1, 8);
  auto lateProof = certify(late, scope);
  CHECK(lateProof.has_value());
  SynthesisOptions sampled = synthesisOptions(scope);
  sampled.limits.maxPoints = 2;
  SynthesisResult l = synthesizeProgram(late, &*lateProof, sampled, z3Backend);
  CHECK(l.enumeration.sampled);
  for (const VerifiedCandidate &candidate : l.groups[0].candidates)
    CHECK(candidate.symbolic.tier() < EvidenceTier::SymbolicValid);
  // The obligation itself, directly on a family that passes the samples.
  ParameterScope small = range(late, 2, 3);
  auto smallProof = certify(late, small);
  SynthesisResult s = synthesizeProgram(late, &*smallProof, synthesisOptions(small), z3Backend);
  CHECK(!s.groups[0].candidates.empty());
  CHECK(s.groups[0].candidates[0].symbolic.tier() == EvidenceTier::SymbolicExact);
  const VerifiedCandidate &good = s.groups[0].candidates[0];
  CheckedFamily wider = checkFamilySymbolic(late, &*lateProof, l.enumeration.groups[0], good.family,
                                            good.order ? &*good.order : nullptr,
                                            symbolicOptions(scope), z3Backend);
  CHECK(wider.tier() == EvidenceTier::Proposed);
  CHECK(wider.reason().find("V4") != std::string::npos);
  CHECK(wider.failingPoint().has_value());

  // A value from memory is no coordinate: the two copies of a query share memory.
  SemanticProgram tainted = m.import("tainted");
  ParameterScope tscope = range(tainted, 1, 3);
  auto taintProof = certify(tainted, tscope);
  auto independent = memoryIndependentState(tainted);
  int id = -1;
  for (const StateVar &var : tainted.state)
    if (!independent[var.id] && var.sort.kind == Sort::BV && var.sort.width == 16)
      id = var.id;
  CHECK(id >= 0);
  CHECK(taintProof.has_value());
  if (id >= 0 && taintProof) {
    CoordinateFamily family;
    Term state = tainted.terms->state(id, tainted.state[id].sort);
    family.axes[AxisI] = {state};
    EnumerationOrder levels;
    OrderLevel one;
    one.value = state;
    one.lower = tainted.terms->bv(APInt(16, 0));
    one.upper = tainted.terms->bv(APInt(16, 9));
    levels.levels = {one};
    SiteGroup group;
    group.output = tainted.sites[0].output->root;
    group.first = tainted.sites[0].a.root;
    group.second = tainted.sites[0].b.root;
    group.sites = {tainted.sites[0].id};
    CheckedFamily refused = checkFamilySymbolic(tainted, &*taintProof, group, family, &levels,
                                                symbolicOptions(tscope), z3Backend);
    CHECK(refused.status() == FamilyStatus::Refuted);
    CHECK(refused.reason().find("memory") != std::string::npos);
  }

  // A tier says what it quantifies over. Beyond the bound of the arrays every run
  // stops, so the claim is about runs that complete, and it says so.
  Module small2(R"(
func.func @bounds(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
)");
  SemanticProgram b = small2.import("bounds");
  ParameterScope wide = range(b, 1, 40);
  auto proof = certify(b, wide);
  SynthesisOptions options = synthesisOptions(wide);
  options.limits.maxPoints = 3;
  SynthesisResult bounded = synthesizeProgram(b, &*proof, options, z3Backend);
  CHECK(bounded.enumeration.termination.kind() == V::ProvenTerminatingIfDefined);
  CHECK(!bounded.groups.empty() && !bounded.groups[0].candidates.empty());
  for (const VerifiedCandidate &candidate : bounded.groups[0].candidates)
    CHECK(!candidate.symbolic.holdsForEveryRun());
  ParameterScope safe = range(b, 1, 4);
  auto safeProof = certify(b, safe);
  SynthesisResult defined = synthesizeProgram(b, &*safeProof, synthesisOptions(safe), z3Backend);
  CHECK(defined.enumeration.termination.kind() == V::ProvenTerminating);
  CHECK(!defined.groups[0].candidates.empty());
  CHECK(defined.groups[0].candidates[0].symbolic.holdsForEveryRun());
}

/// Each case is one obligation or refusal with nothing else to stop the claim.
static void testAdversaries() {
  Module m(programs), bad(adversaries);
  SemanticProgram p = m.import("gemm");
  TermContext &terms = *p.terms;
  ParameterScope scope = range(p, 1, 3);
  auto proof = certify(p, scope);
  SynthesisResult r = synthesizeProgram(p, &*proof, synthesisOptions(scope), z3Backend);
  const VerifiedCandidate &best = r.groups[0].candidates[0];
  const SiteGroup &group = r.enumeration.groups[0];
  const EnumerationOrder &order = *best.order;
  const CoordinateFamily &family = best.family;
  SymbolicCheckOptions options = symbolicOptions(scope);
  auto check = [&](const EnumerationOrder &o) {
    return checkFamilySymbolic(p, &*proof, group, family, &o, options, z3Backend);
  };
  auto says = [](const CheckedFamily &c, const char *text) {
    return c.reason().find(text) != std::string::npos;
  };

  // S3: one element too many at the slowest level. Every step matches until the
  // last event, whose successor the run never reaches.
  EnumerationOrder longer = order;
  longer.levels[0].upper = terms.binary(TermOp::Add, order.levels[0].upper, terms.bv(APInt(16, 1)));
  CheckedFamily tail = check(longer);
  CHECK(tail.tier() == EvidenceTier::SymbolicValid);
  CHECK(says(tail, "S3"));

  // V3 reads the lower bound: the events at k = 0 are below it.
  EnumerationOrder raised = order;
  raised.levels.back().lower = terms.bv(APInt(16, 1));
  CheckedFamily below = check(raised);
  CHECK(below.tier() == EvidenceTier::Proposed);
  CHECK(says(below, "V3"));

  // The scope is a premise: i < n <= 3 needs it, and n = 1 keeps coverage open.
  EnumerationOrder fixed = order;
  fixed.levels[0].upper = terms.bv(APInt(16, 3));
  CHECK(check(fixed).tier() == EvidenceTier::SymbolicValid);

  EnumerationOrder stateBound = order;
  stateBound.levels[1].upper = order.levels[0].value;
  CHECK(says(check(stateBound), "reads state"));

  // A group the enumerator set aside stays aside, whatever else matches.
  SiteGroup aside = group;
  aside.unsupported = "set aside by the test";
  CHECK(checkFamilySymbolic(p, &*proof, aside, family, &order, options, z3Backend).reason() ==
        "set aside by the test");

  // No sites: every universal claim would be vacuous, and an empty order would
  // simulate an empty run.
  SemanticProgram scalar = bad.import("flat");
  scalar.sites.clear();
  ParameterScope anything;
  auto scalarProof = certify(scalar, anything);
  CHECK(scalarProof.has_value());
  SiteGroup nothing;
  nothing.output = 0;
  CheckedFamily none = checkFamilySymbolic(scalar, &*scalarProof, nothing, family, &order,
                                           symbolicOptions(anything), z3Backend);
  CHECK(says(none, "no sites"));

  // A trace that reads memory, with a well-formed group. No certificate exists for
  // it, and the refusal comes before one would be read.
  SemanticProgram lookup = m.import("lookup");
  CHECK(!certify(lookup, anything).has_value());
  {
    Term li = lookup.sites[0].output->index[0];
    EnumerationOrder levels = oneLevel(lookup, li, 0, 4);
    TerminationCertificate blank;
    CheckedFamily memory = checkFamilySymbolic(lookup, &blank, groupOf(lookup), batchOnly(li),
                                               &levels, symbolicOptions(anything), z3Backend);
    CHECK(memory.tier() == EvidenceTier::Proposed);
    CHECK(says(memory, "memory contents"));
  }

  // Finite tiers: a closed gate says why, and no events is no evidence.
  EnumerationOptions sampled;
  sampled.scope = scope;
  DomainEnumeration closed = enumerateDomain(p, nullptr, sampled, z3Backend);
  CHECK(says(checkFamilyFinite(p, closed, 0, family), "gate"));
  SemanticProgram emptyRun = m.import("gemm");
  ParameterScope zero = range(emptyRun, 0, 0);
  auto zeroProof = certify(emptyRun, zero);
  DomainEnumeration quiet = liveDomains(emptyRun, *zeroProof, zero);
  CHECK(quiet.gate == GateStatus::Open);
  CheckedFamily noEvents = checkFamilyFinite(emptyRun, quiet, 0, CoordinateFamily());
  CHECK(noEvents.status() == FamilyStatus::Unknown);
  CHECK(says(noEvents, "no point has events"));

  // Unknown on a path prune is a stop with its own reason, not a kept path.
  {
    Selective backend("; prune");
    CheckedFamily answer = checkFamilySymbolic(p, &*proof, group, family, &order, options, backend);
    CHECK(says(answer, "could not be decided"));
  }

  // Last element repeated: t = min(i, 2) gives 0, 1, 2, 2. Every event is in the
  // set, each step but the last matches, and the set has no successor to break it.
  SemanticProgram clamped = bad.import("clamped");
  auto clampProof = certify(clamped, anything);
  CHECK(clampProof.has_value());
  Term t = clamped.sites[0].output->index[0];
  EnumerationOrder clampOrder = oneLevel(clamped, t, 0, 3);
  CheckedFamily repeated = checkFamilySymbolic(clamped, &*clampProof, groupOf(clamped), batchOnly(t),
                                               &clampOrder, symbolicOptions(anything), z3Backend);
  CHECK(repeated.tier() == EvidenceTier::Proposed);
  CHECK(repeated.status() == FamilyStatus::Unknown);
  bool openV2 = false;
  for (const ObligationRecord &record : repeated.records())
    openV2 = openV2 || (record.id == "V2" && !record.discharged);
  CHECK(openV2);

  // V2 fails on a family that passes everything else: k reaches no address.
  SemanticProgram hidden = m.import("hidden");
  ParameterScope two = range(hidden, 2, 2);
  auto hiddenProof = certify(hidden, two);
  SynthesisResult h = synthesizeProgram(hidden, &*hiddenProof, synthesisOptions(two), z3Backend);
  CHECK(!h.groups[0].candidates.empty());
  if (!h.groups[0].candidates.empty()) {
    const VerifiedCandidate &good = h.groups[0].candidates[0];
    CHECK(good.symbolic.tier() == EvidenceTier::SymbolicExact);
    Term k = good.family.axes[AxisK][0];
    CoordinateFamily lost = good.family;
    lost.axes[AxisK].clear();
    EnumerationOrder fewer;
    for (const OrderLevel &level : good.order->levels)
      if (level.value != k)
        fewer.levels.push_back(level);
    CheckedFamily blind = checkFamilySymbolic(hidden, &*hiddenProof, h.enumeration.groups[0], lost,
                                              &fewer, symbolicOptions(two), z3Backend);
    CHECK(blind.tier() == EvidenceTier::Proposed);
    bool sawV2 = false;
    for (const ObligationRecord &record : blind.records())
      sawV2 = sawV2 || (record.id == "V2" && !record.discharged && record.result == "sat");
    CHECK(sawV2);
    CHECK(blind.failingPoint().has_value());
  }

  // A run with no events: first(v) is in the set and the entry reaches the exit.
  // Only the exit branch of S1 sees it, as every event that happens is right.
  SemanticProgram maybe = bad.import("maybe");
  ParameterScope both = range(maybe, 0, 1);
  auto maybeProof = certify(maybe, both);
  CHECK(maybeProof.has_value());
  Term mi = maybe.sites[0].output->index[0];
  EnumerationOrder maybeOrder = oneLevel(maybe, mi, 0, 3);
  CheckedFamily silent = checkFamilySymbolic(maybe, &*maybeProof, groupOf(maybe), batchOnly(mi),
                                             &maybeOrder, symbolicOptions(both), z3Backend);
  CHECK(silent.tier() == EvidenceTier::SymbolicValid);
  CHECK(says(silent, "S1"));
  // With the empty run ruled out by the scope, the same claim is exact.
  ParameterScope onlyOne = range(maybe, 1, 1);
  auto oneProof = certify(maybe, onlyOne);
  CHECK(checkFamilySymbolic(maybe, &*oneProof, groupOf(maybe), batchOnly(mi), &maybeOrder,
                            symbolicOptions(onlyOne), z3Backend).tier() == EvidenceTier::SymbolicExact);

  // No loop: the only arrival comes from the entry, and V3 must still reach it.
  SemanticProgram guarded = bad.import("guarded_one");
  ParameterScope four = range(guarded, 0, 3);
  auto guardedProof = certify(guarded, four);
  CHECK(guardedProof.has_value());
  Term gn = guarded.sites[0].output->index[0];
  EnumerationOrder tight = oneLevel(guarded, gn, 0, 2);
  CheckedFamily past = checkFamilySymbolic(guarded, &*guardedProof, groupOf(guarded), batchOnly(gn),
                                           &tight, symbolicOptions(four), z3Backend);
  CHECK(past.tier() == EvidenceTier::Proposed);
  CHECK(says(past, "V3"));

  // A site in the entry block is out of the simulation's reach, by name.
  SemanticProgram flat = bad.import("flat");
  ParameterScope low = range(flat, 0, 3);
  auto flatProof = certify(flat, low);
  CHECK(flatProof.has_value());
  Term fn = flat.sites[0].output->index[0];
  EnumerationOrder flatOrder = oneLevel(flat, fn, 0, 4);
  CheckedFamily entry = checkFamilySymbolic(flat, &*flatProof, groupOf(flat), batchOnly(fn),
                                            &flatOrder, symbolicOptions(low), z3Backend);
  CHECK(entry.tier() == EvidenceTier::SymbolicValid);
  CHECK(says(entry, "entry block"));
  // At the entry no state has been written, so none has poison: a state the path
  // has not reached yet is defined, as in the evaluator.
  SemanticProgram early = bad.import("before_loop");
  ParameterScope anyN;
  auto earlyProof = certify(early, anyN);
  CHECK(earlyProof.has_value());
  std::string scopeError;
  SiteArrivals direct = reachSites(early, {}, earlyProof->cuts, {early.sites[0].id},
                                   scopeFormula(early, anyN, scopeError), PathLimits());
  CHECK(direct.complete && direct.arrivals.size() == 1);
  unsigned unwritten = 0;
  if (!direct.arrivals.empty()) {
    const SymbolicPath &path = direct.arrivals[0].path;
    CHECK(path.shadow.size() == early.state.size());
    for (unsigned id = 0; id < path.state.size(); ++id)
      if (!path.state[id]) {
        ++unwritten;
        CHECK(path.shadow[id] == early.terms->boolean(false));
      }
  }
  CHECK(unwritten > 0);

  // Two sites in one block: the walk treats a block as one event.
  SemanticProgram same = bad.import("same_block");
  ParameterScope few = range(same, 1, 2);
  auto sameProof = certify(same, few);
  CHECK(sameProof.has_value() && same.sites.size() == 2);
  Term si = same.sites[0].output->index[0];
  EnumerationOrder sameOrder;
  sameOrder.levels = oneLevel(same, si, 0, 1).levels;
  sameOrder.levels[0].upper = same.terms->param(same.parameters[0].id, same.parameters[0].sort);
  CheckedFamily shared = checkFamilySymbolic(same, &*sameProof, groupOf(same), batchOnly(si),
                                             &sameOrder, symbolicOptions(few), z3Backend);
  CHECK(shared.tier() == EvidenceTier::Proposed);
  CHECK(says(shared, "share a block"));

  // Only a no-stop condition says i < 4: the loop runs to 40, the arrays stop it.
  SemanticProgram bounded = bad.import("bounded");
  ParameterScope wide = range(bounded, 1, 40);
  auto boundedProof = certify(bounded, wide);
  CHECK(boundedProof.has_value());
  Term bi = bounded.sites[0].output->index[0];
  EnumerationOrder boundedOrder = oneLevel(bounded, bi, 0, 4);
  CheckedFamily stops = checkFamilySymbolic(bounded, &*boundedProof, groupOf(bounded), batchOnly(bi),
                                            &boundedOrder, symbolicOptions(wide), z3Backend);
  CHECK(stops.tier() >= EvidenceTier::SymbolicValid);
  CHECK(!stops.holdsForEveryRun());

  // A guard over memory has no premise.
  SymbolicPath forged = startSymbolicPath(p, p.entryLocation);
  Term i = family.axes[AxisI][0];
  forged.condition = terms.icmp(Predicate::Eq, terms.load(terms.memVar(p.roots[0].sort), {i, i}),
                                terms.bv(APInt(32, 0)));
  CHECK(pathPremise(p, forged) == nullptr);

  // Views of different lengths differ, whatever the coordinates. Both sites read
  // a[i, k], and they share i and k, so this pair is reached with equal keys.
  SemanticProgram sites = bad.import("split_j");
  auto sitesProof = certify(sites, anything);
  SynthesisResult split = synthesizeProgram(sites, &*sitesProof, synthesisOptions(anything), z3Backend);
  CHECK(!split.groups[0].candidates.empty());
  if (!split.groups[0].candidates.empty()) {
    const VerifiedCandidate &candidate = split.groups[0].candidates[0];
    CHECK(candidate.order.has_value());
    CHECK(candidate.symbolic.tier() >= EvidenceTier::SymbolicValid);
    sites.sites[1].a.index.pop_back();
    CheckedFamily shorter = checkFamilySymbolic(sites, &*sitesProof, split.enumeration.groups[0],
                                                candidate.family, &*candidate.order,
                                                symbolicOptions(anything), z3Backend);
    CHECK(shorter.tier() == EvidenceTier::Proposed);
    CHECK(says(shorter, "V1f"));
  }
}

/// S1 and S2 read membership again, though V3 already implies it at real events.
/// Called alone, with V3 out of the way, they must hold by themselves.
static void testCoverageAlone() {
  Module m(programs), bad(adversaries);
  SemanticProgram p = m.import("gemm");
  ParameterScope scope = range(p, 1, 3);
  auto proof = certify(p, scope);
  SynthesisResult r = synthesizeProgram(p, &*proof, synthesisOptions(scope), z3Backend);
  const EnumerationOrder &order = *r.groups[0].candidates[0].order;
  std::string why;
  CHECK(coverage(p, *proof, scope, order, why));

  // The first event equals first(v), and first(v) is in no set: an empty level.
  EnumerationOrder empty = order;
  empty.levels[1].upper = empty.levels[1].lower;
  why.clear();
  CHECK(!coverage(p, *proof, scope, empty, why));
  CHECK(why.find("S1") != std::string::npos);

  // An event outside the set is the exact successor of the one before it: at i = 1
  // the range of j wraps to nothing, and the program's own loop runs once there.
  SemanticProgram jump = bad.import("jump", 8);
  ParameterScope anything;
  // The simulation needs only cuts: each loop head, reached by a second edge.
  TerminationCertificate heads;
  std::vector<unsigned> incoming(jump.locationCount, 0);
  for (const Transition &edge : jump.transitions)
    if (edge.to < jump.locationCount && ++incoming[edge.to] == 2)
      heads.cuts.push_back(edge.to);
  CHECK(heads.cuts.size() == 2);
  CHECK(jump.sites.size() == 1);
  Term outerTerm = jump.sites[0].output->index[0], innerTerm = nullptr;
  for (const StateVar &var : jump.state)
    if (var.loopCarried && var.sort == outerTerm->sort && !llvm::is_contained(stateSupport(outerTerm), var.id))
      innerTerm = jump.terms->state(var.id, var.sort);
  CHECK(innerTerm != nullptr);
  EnumerationOrder wrapped;
  OrderLevel outer, inner;
  outer.value = outerTerm;
  outer.lower = jump.terms->bv(APInt(8, 0));
  outer.upper = jump.terms->bv(APInt(8, 2));
  inner.value = innerTerm;
  inner.lower = jump.terms->bv(APInt(8, 0));
  inner.upper = jump.terms->bv(APInt(8, 255));
  inner.upperOuter = 0;
  wrapped.levels = {outer, inner};
  why.clear();
  CHECK(!coverage(jump, heads, anything, wrapped, why));
  CHECK(why.find("S2") != std::string::npos);
}

/// Theorem 6 by brute force: all assignments of a small bank, each judged by the
/// G3a facts, against every clause the search learned.
static void testClauses() {
  Module m(programs), bad(adversaries);
  struct Case {
    const char *name;
    bool adversary;
    int64_t low, high;
  };
  uint64_t valid = 0, clauses = 0, boxClauses = 0, batched = 0, excluded = 0;
  for (Case c : {Case{"gemm", false, 2, 3}, Case{"no_i", false, 2, 3}, Case{"hidden", false, 2, 3},
                 Case{"triangle", false, 2, 3}, Case{"xbits", true, 0, 0}, Case{"two_sites", false, 0, 0},
                 Case{"staircase", true, 0, 0}}) {
    SemanticProgram p = (c.adversary ? bad : m).import(c.name);
    ParameterScope scope;
    if (!p.parameters.empty() && c.high)
      scope = range(p, c.low, c.high);
    auto proof = certify(p, scope);
    CHECK(proof.has_value());
    DomainEnumeration enumeration = liveDomains(p, *proof, scope);
    CHECK(enumeration.gate == GateStatus::Open);
    auto domains = domainsOf(enumeration);
    GrammarSpec spec = GrammarSpec::atLevel(1);
    spec.maxAtoms = 5;
    AtomBank bank = buildAtoms(p, enumeration.groups[0], domains, spec);
    SynthesisLimits limits;
    limits.maxCandidates = 30;
    SynthesisOutcome outcome = synthesizeCoordinates(p, bank, domains, limits, z3Backend);
    clauses += outcome.always.size() + outcome.whenUnit.size() + outcome.whenBox.size();
    boxClauses += outcome.whenBox.size();
    bool foundBoxUnit = false;

    unsigned count = bank.atoms.size();
    std::vector<unsigned> placement(count, 0); // 0: unused. 1 to 4: an axis.
    while (true) {
      std::array<std::vector<unsigned>, 4> axes;
      for (unsigned a = 0; a < count; ++a)
        if (placement[a])
          axes[placement[a] - 1].push_back(a);
      bool isValid = true, isUnit = true, isBox = true;
      for (unsigned d = 0; d < domains.size() && isValid; ++d) {
        CoordinateCandidate candidate;
        std::array<std::vector<uint32_t> *, 4> slots{&candidate.p, &candidate.i, &candidate.j,
                                                     &candidate.k};
        size_t events = domains[d]->table().events().size();
        for (unsigned axis = 0; axis < 4; ++axis) {
          std::map<std::vector<uint64_t>, uint32_t> ids;
          for (size_t e = 0; e < events; ++e) {
            std::vector<uint64_t> key;
            for (unsigned atom : axes[axis])
              key.push_back(bank.atoms[atom].values[d][e]);
            slots[axis]->push_back(ids.try_emplace(std::move(key), (uint32_t)ids.size()).first->second);
          }
        }
        CandidateReport report = inspectCandidate(domains[d]->table(), candidate);
        isValid = report.malformed.empty() && report.decompositionValid;
        isUnit = isUnit && report.multiplicity == MultiplicityForm::Unit;
        isBox = isBox && report.cartesian == CartesianForm::Box;
      }
      if (isValid) {
        ++valid;
        batched += !axes[0].empty();
        foundBoxUnit = foundBoxUnit || (isUnit && isBox);
        auto holds = [&](const SelectorClause &clause) {
          for (const SelectorLiteral &l : clause)
            if ((placement[l.atom] == l.axis + 1) == l.positive)
              return true;
          return false;
        };
        for (const SelectorClause &clause : outcome.always)
          excluded += !holds(clause);
        if (isUnit)
          for (const SelectorClause &clause : outcome.whenUnit)
            excluded += !holds(clause);
        if (isBox)
          for (const SelectorClause &clause : outcome.whenBox)
            excluded += !holds(clause);
      }
      unsigned a = 0;
      for (; a < count; ++a) {
        if (++placement[a] < 5)
          break;
        placement[a] = 0;
      }
      if (a == count)
        break;
    }
    // A finished search that found nothing means there was nothing to find.
    if (outcome.exhausted && foundBoxUnit)
      CHECK(!outcome.candidates.empty());
  }
  CHECK(excluded == 0);
  CHECK(valid > 0 && clauses > 0 && boxClauses > 0 && batched > 0);
}

/// What the search must keep doing, seen from outside it.
static void testSearch() {
  Module m(programs), bad(adversaries);

  // The grammar's operators, each present as an atom of its level.
  SemanticProgram p = m.import("gemm");
  ParameterScope scope = range(p, 2, 3);
  auto proof = certify(p, scope);
  DomainEnumeration enumeration = liveDomains(p, *proof, scope);
  auto domains = domainsOf(enumeration);
  auto atomsAt = [&](unsigned level) {
    GrammarSpec spec = GrammarSpec::atLevel(level);
    spec.maxAtoms = 400;
    spec.maxOperands = 4000;
    spec.extraConstants = {APInt(16, 2)};
    return buildAtoms(p, enumeration.groups[0], domains, spec).atoms;
  };
  auto ops = [&](unsigned level) {
    std::set<TermOp> found;
    for (const Atom &atom : atomsAt(level)) {
      // A constant shift keeps the kernel of its operand, and is never an atom.
      bool twoVariables = atom.term->args.size() == 2 &&
                          !stateSupport(atom.term->args[1]).empty();
      if (atom.term->op != TermOp::Add || twoVariables)
        found.insert(atom.term->op);
    }
    return found;
  };
  CHECK(!ops(0).count(TermOp::Add));
  CHECK(ops(1).count(TermOp::Add));
  CHECK(!ops(1).count(TermOp::UDiv) && !ops(1).count(TermOp::URem));
  CHECK(ops(2).count(TermOp::UDiv));
  CHECK(ops(2).count(TermOp::URem));
  // Each by a constant and by a variable: they come from different productions.
  auto divides = [&](TermOp op, bool byConstant) {
    for (const Atom &atom : atomsAt(2))
      if (atom.term->op == op && (atom.term->args[1]->op == TermOp::Const) == byConstant)
        return true;
    return false;
  };
  CHECK(divides(TermOp::UDiv, true));
  CHECK(divides(TermOp::URem, true));
  CHECK(divides(TermOp::UDiv, false));
  CHECK(divides(TermOp::URem, false));

  // With no rounds at all, the seed alone holds the canonical candidate.
  SynthesisOptions tight = synthesisOptions(scope);
  tight.synthesis.maxRounds = 0;
  SynthesisResult seeded = synthesizeProgram(p, &*proof, tight, z3Backend);
  CHECK(seeded.groups.size() == 1 && !seeded.groups[0].candidates.empty());

  // A box search returns boxes: the second candidate is the box mode's first, or
  // with no seed the first's successor there.
  SemanticProgram tri = m.import("triangle");
  ParameterScope triScope = range(tri, 2, 3);
  auto triProof = certify(tri, triScope);
  DomainEnumeration triDomains = liveDomains(tri, *triProof, triScope);
  auto triList = domainsOf(triDomains);
  GrammarSpec triSpec = GrammarSpec::atLevel(0);
  AtomBank triBank = buildAtoms(tri, triDomains.groups[0], triList, triSpec);
  SynthesisLimits one;
  one.maxCandidates = 1;
  SynthesisOutcome triOutcome = synthesizeCoordinates(tri, triBank, triList, one, z3Backend);
  CHECK(triOutcome.candidates.size() >= 2);
  if (triOutcome.candidates.size() >= 2)
    for (const AccessDomain *domain : triList) {
      std::string why;
      auto labels = familyLabels(tri, *domain, triOutcome.candidates[1], why);
      CHECK(labels.has_value());
      if (labels)
        CHECK(inspectCandidate(domain->table(), *labels).cartesian == CartesianForm::Box);
    }

  // No two candidates are the same partition of the events on every axis.
  SemanticProgram noI = m.import("no_i");
  ParameterScope noScope = range(noI, 2, 3);
  auto noProof = certify(noI, noScope);
  DomainEnumeration noDomains = liveDomains(noI, *noProof, noScope);
  auto noList = domainsOf(noDomains);
  AtomBank noBank = buildAtoms(noI, noDomains.groups[0], noList, GrammarSpec::atLevel(0));
  SynthesisOutcome noOutcome = synthesizeCoordinates(noI, noBank, noList, SynthesisLimits(), z3Backend);
  CHECK(noOutcome.candidates.size() > 1);
  std::set<std::vector<std::vector<uint32_t>>> kernels;
  for (const CoordinateFamily &family : noOutcome.candidates) {
    std::vector<std::vector<uint32_t>> key;
    for (const AccessDomain *domain : noList) {
      std::string why;
      auto labels = familyLabels(noI, *domain, family, why);
      CHECK(labels.has_value());
      if (!labels)
        continue;
      for (const std::vector<uint32_t> *axis : {&labels->p, &labels->i, &labels->j, &labels->k}) {
        std::map<uint32_t, uint32_t> ids;
        std::vector<uint32_t> canonical;
        for (uint32_t label : *axis)
          canonical.push_back(ids.try_emplace(label, (uint32_t)ids.size()).first->second);
        key.push_back(std::move(canonical));
      }
    }
    CHECK(kernels.insert(std::move(key)).second);
  }

  // Bit 1 of t is no state and no subterm: only a wider grammar has a full family.
  SemanticProgram xbits = bad.import("xbits");
  ParameterScope all;
  auto xProof = certify(xbits, all);
  CHECK(xProof.has_value());
  auto full = [&](unsigned level) {
    SynthesisResult result = synthesizeProgram(xbits, &*xProof, synthesisOptions(all, level), z3Backend);
    unsigned n = 0;
    for (const VerifiedCandidate &candidate : result.groups[0].candidates)
      n += candidate.finite.points()[0].report.structurallyFull;
    return n;
  };
  CHECK(full(0) == 0);
  CHECK(full(2) > 0);

  // One group set aside, one searched, and each says which.
  SemanticProgram mixed = bad.import("mixed");
  auto mixedProof = certify(mixed, all);
  CHECK(mixedProof.has_value());
  SynthesisResult split = synthesizeProgram(mixed, &*mixedProof, synthesisOptions(all), z3Backend);
  CHECK(split.enumeration.gate == GateStatus::Open);
  CHECK(split.groups.size() == 2);
  unsigned found = 0, aside = 0;
  for (const GroupSynthesis &group : split.groups) {
    found += group.status == RecoveryStatus::CandidateFound;
    aside += group.status == RecoveryStatus::Unsupported && group.reason.find("root") != std::string::npos;
  }
  CHECK(found == 1 && aside == 1);

  // A value from memory never enters a point, even where it is defined everywhere.
  SemanticProgram tainted = m.import("tainted");
  ParameterScope tscope = range(tainted, 1, 2);
  auto taintProof = certify(tainted, tscope);
  auto independent = memoryIndependentState(tainted);
  CHECK(std::count(independent.begin(), independent.end(), false) > 0);
  DomainEnumeration live = liveDomains(tainted, *taintProof, tscope);
  auto list = domainsOf(live);
  CHECK(!list.empty());
  for (const AccessDomain *domain : list)
    for (unsigned id : domain->table().coordinateNames)
      CHECK(independent[id]);
}

static void testSlices() {
  Module m(programs);
  SemanticProgram p = m.import("two_loops");
  auto live = controlLiveState(p);
  // A loop's variable is live in its own blocks, and the other loop's is not
  // live in the first loop's blocks before that loop starts.
  CHECK(!live.empty());
  std::set<unsigned> every;
  for (const auto &[location, ids] : live)
    every.insert(ids.begin(), ids.end());
  CHECK(!every.empty());
  for (const Transition &t : p.transitions)
    for (unsigned id : stateSupport(t.guard))
      CHECK(live[t.from].count(id) == 1);
  bool differ = false;
  for (const auto &[location, ids] : live)
    differ = differ || ids != every;
  CHECK(differ);

  // The equation of the fixpoint, on nests with temporaries and on branches: what
  // defines a live state is live before the definition.
  for (const char *name : {"gemm", "triangle", "two_sites", "cyclic"}) {
    SemanticProgram q = m.import(name);
    auto facts = controlLiveState(q);
    unsigned used = 0;
    for (const Transition &t : q.transitions) {
      if (t.to >= q.locationCount)
        continue;
      for (const auto &[id, term] : t.assignments)
        if (facts[t.to].count(id))
          for (unsigned read : stateSupport(term)) {
            ++used;
            CHECK(facts[t.from].count(read) == 1);
          }
    }
    CHECK(used > 0);
  }
}

int main() {
  mlir::MLIRContext local;
  configure(local);
  context = &local;
  testGrammar();
  testStructure();
  testLiveState();
  testSlices();
  testFinite();
  testSymbolic();
  testPrograms();
  testReviewFindings();
  testAdversaries();
  testCoverageAlone();
  testClauses();
  testSearch();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
