// G3b: the additive G3a options, each tier's obligations, and the forgeries each
// must refuse. test/fuzz/synthesis_oracle.py recomputes the results independently.
#include "GeneralizedGemm/SynthesisAnalysis.h"
#include "GeneralizedGemm/CoordinateAnalysis.h"
#include "GeneralizedGemm/CoverageSimulation.h"
#include "GeneralizedGemm/ClosedFormSynthesis.h"
#include "GeneralizedGemm/IndexProgramSemgus.h"
#include "GeneralizedGemm/LoopCounters.h"
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
// Two phases of one counter t in [0, 2n): column 0 reduces in the order t, column
// 1 in the order (t + 1 - n) mod n, so the reduction coordinate is the branch
// K = if t < n then t else (t + 1 - n) mod n. The Appendix G example of this phase.
func.func @rotated(%a: memref<9x9xi32>, %b: memref<9x2xi32>, %c: memref<9x2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %two_n = arith.muli %n, %c2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %t = %c0 to %two_n step %c1 {
      %p = arith.cmpi ult, %t, %n : index
      scf.if %p {
        %x = memref.load %a[%i, %t] : memref<9x9xi32>
        %y = memref.load %b[%t, %c0] : memref<9x2xi32>
        %z = memref.load %c[%i, %c0] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<9x2xi32>
      } else {
        %u = arith.addi %t, %c1 : index
        %v = arith.subi %u, %n : index
        %r = arith.remui %v, %n : index
        %x = memref.load %a[%i, %r] : memref<9x9xi32>
        %y = memref.load %b[%r, %c1] : memref<9x2xi32>
        %z = memref.load %c[%i, %c1] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c1] : memref<9x2xi32>
      }
    }
  }
  return
}
// One counter, two phases, the second shifted: J = if t < n then 0 else 1 and
// K = if t < n then t else t - n, which the expression grammar reaches at level 2 as
// t div n and t mod n. The statement grammar reaches it at level 0, and the family
// keeps a lexicographic order, so every later checker has its say on it.
func.func @two_phase(%a: memref<9x9xi32>, %b: memref<9x2xi32>, %c: memref<9x2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %two_n = arith.muli %n, %c2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %t = %c0 to %two_n step %c1 {
      %p = arith.cmpi ult, %t, %n : index
      scf.if %p {
        %x = memref.load %a[%i, %t] : memref<9x9xi32>
        %y = memref.load %b[%t, %c0] : memref<9x2xi32>
        %z = memref.load %c[%i, %c0] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<9x2xi32>
      } else {
        %r = arith.subi %t, %n : index
        %x = memref.load %a[%i, %r] : memref<9x9xi32>
        %y = memref.load %b[%r, %c1] : memref<9x2xi32>
        %z = memref.load %c[%i, %c1] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c1] : memref<9x2xi32>
      }
    }
  }
  return
}
// Listing E.5 of the thesis as a reduction: the column i steps by 7 when even and
// by 3 when odd, from 0, over k < n. Closed form i(m) = 5m + 2(m mod 2) (Prop. 4.3).
func.func @parity(%a: memref<1x64xi32>, %b: memref<64x1xi32>, %c: memref<1x1xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  %c7 = arith.constant 7 : index
  %i_final = scf.for %k = %c0 to %n step %c1 iter_args(%i = %c0) -> (index) {
    %x = memref.load %a[%c0, %i] : memref<1x64xi32>
    %y = memref.load %b[%i, %c0] : memref<64x1xi32>
    %z = memref.load %c[%c0, %c0] : memref<1x1xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%c0, %c0] : memref<1x1xi32>
    %r = arith.remui %i, %c2 : index
    %even = arith.cmpi eq, %r, %c0 : index
    %step = arith.select %even, %c7, %c3 : index
    %next = arith.addi %i, %step : index
    scf.yield %next : index
  }
  return
}
// Listing G.1 of the thesis as a reduction: a = (t + 1)! steps the column index
// a mod 8 over t < n. No loop-free form of a is in the grammar; a while program is.
func.func @factorial(%a: memref<1x8xi32>, %b: memref<8x1xi32>, %c: memref<1x1xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %final = scf.for %t = %c0 to %n step %c1 iter_args(%acc = %c1) -> (index) {
    %col = arith.remui %acc, %c8 : index
    %x = memref.load %a[%c0, %col] : memref<1x8xi32>
    %y = memref.load %b[%col, %c0] : memref<8x1xi32>
    %z = memref.load %c[%c0, %c0] : memref<1x1xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%c0, %c0] : memref<1x1xi32>
    %t2 = arith.addi %t, %c2 : index
    %next = arith.muli %acc, %t2 : index
    scf.yield %next : index
  }
  return
}
// A reduction loop with no plain counter: the column i steps by 7 when even and 3
// when odd, from 0 while i < n, for each of two output columns j. K = i is a valid
// label but no unit-step order; the iteration count m, with i = 5m + 2(m mod 2), is.
func.func @history(%a: memref<1x64xi32>, %b: memref<64x2xi32>, %c: memref<1x2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %j = %c0 to %c2 step %c1 {
    %r:2 = scf.for %k = %c0 to %n step %c1 iter_args(%f = %c0, %g = %c0) -> (index, index) {
      %x = memref.load %a[%c0, %f] : memref<1x64xi32>
      %y = memref.load %b[%f, %j] : memref<64x2xi32>
      %z = memref.load %c[%c0, %j] : memref<1x2xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%c0, %j] : memref<1x2xi32>
      %fg = arith.addi %f, %g : index
      %nf = arith.addi %fg, %c1 : index
      scf.yield %nf, %f : index, index
    }
  }
  return
}
func.func @stepped(%a: memref<1x64xi32>, %b: memref<64x2xi32>, %c: memref<1x2xi32>, %n: index) {
  %zero = arith.constant 0 : index
  cf.br ^outer(%zero : index)
^outer(%j: index):
  %two = arith.constant 2 : index
  %g = arith.cmpi ult, %j, %two : index
  cf.cond_br %g, ^start, ^exit
^start:
  %z0 = arith.constant 0 : index
  cf.br ^inner(%z0 : index)
^inner(%i: index):
  %h = arith.cmpi ult, %i, %n : index
  cf.cond_br %h, ^body, ^next
^body:
  %c0 = arith.constant 0 : index
  %x = memref.load %a[%c0, %i] : memref<1x64xi32>
  %y = memref.load %b[%i, %j] : memref<64x2xi32>
  %w = memref.load %c[%c0, %j] : memref<1x2xi32>
  %m = arith.muli %x, %y : i32
  %s = arith.addi %w, %m : i32
  memref.store %s, %c[%c0, %j] : memref<1x2xi32>
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  %c7 = arith.constant 7 : index
  %r = arith.remui %i, %c2 : index
  %even = arith.cmpi eq, %r, %c0 : index
  %step = arith.select %even, %c7, %c3 : index
  %ni = arith.addi %i, %step : index
  cf.br ^inner(%ni : index)
^next:
  %one = arith.constant 1 : index
  %nj = arith.addi %j, %one : index
  cf.br ^outer(%nj : index)
^exit:
  return
}
// The same with the second half walked backwards: K = if t < n then t else 2n-1-t.
// Here the expression grammar also has a coordinate, t * (2n-1-t), which the two
// phases share: the statement grammar is not needed, and must not break anything.
func.func @reversed(%a: memref<9x9xi32>, %b: memref<9x2xi32>, %c: memref<9x2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %two_n = arith.muli %n, %c2 : index
  %last = arith.subi %two_n, %c1 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %t = %c0 to %two_n step %c1 {
      %p = arith.cmpi ult, %t, %n : index
      scf.if %p {
        %x = memref.load %a[%i, %t] : memref<9x9xi32>
        %y = memref.load %b[%t, %c0] : memref<9x2xi32>
        %z = memref.load %c[%i, %c0] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<9x2xi32>
      } else {
        %r = arith.subi %last, %t : index
        %x = memref.load %a[%i, %r] : memref<9x9xi32>
        %y = memref.load %b[%r, %c1] : memref<9x2xi32>
        %z = memref.load %c[%i, %c1] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c1] : memref<9x2xi32>
      }
    }
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


/// A backend that never decides: an unknown is never a proof.
struct UnknownBackend : SmtBackend {
  SmtResponse check(const std::string &, const SmtLimits &, bool) override {
    SmtResponse r;
    r.answer = SmtAnswer::Unknown;
    r.detail = "stubbed";
    return r;
  }
  std::string version() const override { return "unknown-stub"; }
};
/// A backend that claims a model it cannot show: sat with nothing in it.
struct SatBackend : SmtBackend {
  SmtResponse check(const std::string &, const SmtLimits &, bool) override {
    SmtResponse r;
    r.answer = SmtAnswer::Sat;
    return r;
  }
  std::string version() const override { return "sat-stub"; }
};

/// Phase 2: the slice, the loop specification against the program, the closed
/// forms against the specification, and what the three answers may be called.
static void testSliceSpecification() {
  Module m(programs);
  auto evalForm = [](const SemanticProgram &p, const LoopSpecification &spec, Term form,
                     uint64_t iteration, const std::vector<APInt> &parameters,
                     const std::vector<APInt> &entry) -> std::optional<APInt> {
    // The closed form's leaves: m, the entries; here no fixed variable is read.
    EntryState e;
    e.parameters = parameters;
    Configuration c;
    c.state.assign(p.state.size() + 1 + spec.carried.size(), std::nullopt);
    c.shadow.assign(c.state.size(), false);
    c.state[spec.iteration->index] = APInt(spec.iteration->sort.width, iteration);
    for (unsigned k = 0; k < spec.entry.size(); ++k)
      c.state[spec.entry[k]->index] = entry[k];
    return evaluateTerm(p, e, c, form);
  };

  // gemm: three subscripts, each a loop variable; only the k loop is innermost.
  {
    SemanticProgram p = m.import("gemm", 8);
    std::string why;
    auto slices = sliceSubscripts(p, SliceLimits(), why);
    CHECK(slices.has_value() && slices->size() == 3);
    if (!slices)
      return;
    unsigned innermost = 0, outer = 0;
    const IndexSlice *kSlice = nullptr;
    const LoopSpecification *kSpec = nullptr;
    for (const IndexSlice &slice : *slices) {
      CHECK(slice.reason.empty() && slice.uses.size() == 2);
      for (const LoopSpecification &spec : slice.loops) {
        if (spec.reason.empty()) {
          ++innermost;
          kSlice = &slice;
          kSpec = &spec;
        } else {
          CHECK(spec.reason == "the loop contains another loop" ||
                spec.reason == "the loop assigns no variable of the slice");
          ++outer;
        }
      }
    }
    // One innermost loop carries one slice variable: k.
    CHECK(innermost == 1 && outer == 8);
    if (!kSpec)
      return;
    CHECK(kSpec->innermost && kSpec->carried.size() == 1 && kSpec->paths.size() == 1);
    // Nested, but entered with k := c0 on every path, and c0 is a materialized
    // constant: phase 5 reads it as its definition, so the init is the constant 0
    // (phase 2 had left it symbolic).
    CHECK(kSpec->init[0] != nullptr && kSpec->init[0]->op == TermOp::Const && kSpec->init[0]->value.isZero());
    CHECK(!kSpec->defined.empty());   // The step constant is read as its definition.
    // The specification against the program: every arrival at the header obeys c
    // and τ, and the subscript equals every recorded index.
    ParameterScope scope = range(p, 1, 3);
    uint64_t events = 0;
    for (int64_t n = 1; n <= 3; ++n) {
      EnumerationLimits limits;
      auto states = eventStates(p, {APInt(8, n)}, 0, limits);
      CHECK(states.has_value());
      if (!states)
        continue;
      LoopReplay replay = replayLoop(p, *kSlice, *kSpec, states->entry, RunLimits());
      CHECK(replay.status == RunStatus::Returned);
      CHECK(replay.stepMismatches == 0 && replay.conditionMismatches == 0 && replay.undefined == 0);
      CHECK(replay.subscriptMismatches == 0);
      CHECK(replay.subscriptMatches == 2 * states->run.events.size()); // A's column and B's row.
      CHECK(replay.traces.size() == (size_t)(3 * n)); // One activation per (i, j).
      events += states->run.events.size();
      // A forged specification is caught: a step off by one, a negated condition.
      LoopSpecification forged = *kSpec;
      forged.step[0] = p.terms->binary(TermOp::Add, forged.step[0], p.terms->bv(APInt(8, 1)));
      LoopReplay off = replayLoop(p, *kSlice, forged, states->entry, RunLimits());
      CHECK(off.stepMismatches == off.iterations && off.iterations > 0);
      LoopSpecification negated = *kSpec;
      negated.condition = p.terms->logicalNot(negated.condition);
      LoopReplay wrong = replayLoop(p, *kSlice, negated, states->entry, RunLimits());
      CHECK(wrong.conditionMismatches > 0);
    }
    CHECK(events > 0);
    // Closed forms against (F.2): the right one, two wrong ones, and the answers
    // that are not proofs.
    TermContext &terms = *p.terms;
    unsigned k = kSpec->carried[0];
    // k = m is right (the init is 0). entry + m reads the entry leaf, which the
    // known init leaves free: refuted at init with the entry in the model. 2m is
    // refuted at the step.
    Term right = kSpec->iteration;
    Term withEntry = terms.binary(TermOp::Add, kSpec->entry[0], kSpec->iteration);
    Term doubled = terms.binary(TermOp::Mul, kSpec->iteration, terms.bv(APInt(8, 2)));
    SmtLimits limits;
    auto checked = checkClosedForms(p, *kSpec, {{k, right}, {k, withEntry}, {k, doubled}}, scope,
                                    limits, z3Backend);
    CHECK(checked.size() == 3);
    if (checked.size() == 3) {
      CHECK(checked[0].status == ClosedFormStatus::Proved);
      CHECK(checked[0].records.size() == 2 && checked[0].records[0].id == "CF-init" &&
            checked[0].records[1].id == "CF-step" && checked[0].records[1].discharged);
      CHECK(checked[0].reason.find("width 8") != std::string::npos);
      CHECK(checked[1].status == ClosedFormStatus::Refuted && checked[1].reason.find("init") == 0);
      bool entryInModel = false;
      for (const auto &[name, value] : checked[1].model)
        entryInModel = entryInModel || name == "entry_s" + std::to_string(k);
      CHECK(entryInModel);
      CHECK(checked[2].status == ClosedFormStatus::Refuted && checked[2].reason.find("step") == 0);
      bool mInModel = false;
      for (const auto &[name, value] : checked[2].model)
        mInModel = mInModel || name == "m";
      CHECK(mInModel);
      // The refuting model, run through the specification, disagrees with the form.
      if (mInModel) {
        uint64_t mv = 0;
        std::vector<APInt> entry{APInt(8, 0)}, parameters{APInt(8, 2)};
        std::vector<APInt> fixedValues;
        for (unsigned v : kSpec->fixed)
          fixedValues.push_back(APInt::getZero(p.state[v].sort.width));
        for (const auto &[name, value] : checked[2].model) {
          if (name == "m") mv = value.getZExtValue();
          if (name == "entry_s" + std::to_string(k)) entry[0] = value;
          if (name == "p0") parameters[0] = value;
          for (unsigned f = 0; f < kSpec->fixed.size(); ++f)
            if (name == "s" + std::to_string(kSpec->fixed[f])) fixedValues[f] = value;
        }
        auto trace = runSpecification(p, *kSpec, parameters, entry, fixedValues, mv + 1);
        CHECK(trace.has_value());
        if (trace && trace->states.size() > mv + 1) {
          auto predicted = evalForm(p, *kSpec, doubled, mv + 1, parameters, entry);
          CHECK(predicted && *predicted != trace->states[mv + 1][0]);
          auto rightValue = evalForm(p, *kSpec, right, mv + 1, parameters, entry);
          CHECK(rightValue && *rightValue == trace->states[mv + 1][0]);
        }
      }
    }
    UnknownBackend unknown;
    auto undecided = checkClosedForms(p, *kSpec, {{k, right}}, scope, limits, unknown);
    CHECK(undecided.size() == 1 && undecided[0].status == ClosedFormStatus::Unknown);
    CHECK(undecided.size() == 1 && undecided[0].records.size() == 2 &&
          !undecided[0].records[0].discharged && undecided[0].records[0].evidence == "stubbed");
    SatBackend sat;
    auto claimed = checkClosedForms(p, *kSpec, {{k, right}}, scope, limits, sat);
    CHECK(claimed.size() == 1 && claimed[0].status == ClosedFormStatus::Refuted &&
          claimed[0].model.empty());
    // A form for a variable the loop does not carry, and one of the wrong sort.
    auto other = checkClosedForms(p, *kSpec, {{k + 100, right}}, scope, limits, z3Backend);
    CHECK(other.size() == 1 && other[0].status == ClosedFormStatus::Unknown);

    // The synthesizer finds and proves k = m from the sampled traces (the init is
    // the constant 0, so no entry leaf is offered).
    ClosedFormOptions options;
    options.scope = scope;
    std::vector<std::vector<APInt>> points{{APInt(8, 1)}, {APInt(8, 2)}, {APInt(8, 3)}};
    SliceSynthesisResult result = synthesizeClosedForms(p, points, options, z3Backend);
    CHECK(result.reason.empty() && result.slices.size() == 3);
    unsigned proved = 0;
    for (const SliceSynthesis &slice : result.slices)
      for (const LoopSynthesis &loop : slice.loops)
        for (const SynthesizedClosedForm &form : loop.forms) {
          CHECK(form.status == ClosedFormStatus::Proved);
          CHECK(form.rounds >= 1);
          CHECK(form.text == "v0 := m ; return v0");
          CHECK(loop.stepMismatches == 0 && loop.subscriptMismatches == 0 && loop.points == 3);
          ++proved;
        }
    CHECK(proved == 1);
    std::string json = closedFormsJSON(p, result);
    CHECK(json.find("\"status\": \"PROVED\"") != std::string::npos);
    CHECK(json.find("\"step_mismatches\": 0") != std::string::npos);
    SynthesisResult none;
    CHECK(toJSON(p, none, false, "closed_forms", json).find("\"closed_forms\": {") != std::string::npos);
    CHECK(toJSON(p, none, false).find("closed_forms") == std::string::npos);
    CHECK(toJSON(p, none, false, "closed_forms", "{not json").find("unparsable") != std::string::npos);
  }

  // parity: Listing E.5 as a loop at depth 0, so the init is the constant 0 and
  // the closed form is the thesis's 5m + 2(m mod 2), here as a branch at width 8.
  {
    SemanticProgram p = m.import("parity", 8);
    ParameterScope scope = range(p, 2, 6);
    ClosedFormOptions options;
    options.scope = scope;
    std::vector<std::vector<APInt>> points;
    for (int64_t n = 2; n <= 6; ++n)
      points.push_back({APInt(8, n)});
    SliceSynthesisResult result = synthesizeClosedForms(p, points, options, z3Backend);
    CHECK(result.reason.empty());
    const LoopSynthesis *loop = nullptr;
    for (const SliceSynthesis &slice : result.slices)
      for (const LoopSynthesis &l : slice.loops)
        if (l.spec.reason.empty() && l.spec.carried.size() == 2)
          loop = &l;
    CHECK(loop != nullptr);
    if (!loop)
      return;
    CHECK(loop->stepMismatches == 0 && loop->conditionMismatches == 0 && loop->subscriptMismatches == 0);
    CHECK(loop->subscriptMatches == 40 && loop->points == 5 && loop->activations == 5);
    for (const SynthesizedClosedForm &form : loop->forms) {
      CHECK(form.status == ClosedFormStatus::Proved);
      CHECK(form.rounds == 1); // The samples alone determine both forms.
    }
    // Which variable is the column: the one whose form has the branch.
    const SynthesizedClosedForm *column = nullptr;
    for (const SynthesizedClosedForm &form : loop->forms)
      if (form.text.find("if ") == 0)
        column = &form;
    CHECK(column != nullptr);
    if (column) {
      CHECK(column->text == "if (m mod 2) = 0 then (v0 := (5 * m)) else (v0 := (2 + (5 * m))) ; return v0");
      uint64_t expected[] = {0, 7, 10, 17, 20, 27, 30};
      for (uint64_t mv = 0; mv < 7; ++mv) {
        auto value = evalForm(p, loop->spec, column->term, mv, {APInt(8, 6)},
                              {APInt(8, 0), APInt(8, 0)});
        CHECK(value && value->getZExtValue() == expected[mv]);
      }
      // 5m + 2(m mod 2) over the integers wraps at 8 bits from m = 51: the proof is
      // at the width, where the program wraps the same way; it says nothing about Z.
      auto wrapped = evalForm(p, loop->spec, column->term, 52, {APInt(8, 6)}, {APInt(8, 0), APInt(8, 0)});
      CHECK(wrapped && wrapped->getZExtValue() == (5 * 52) % 256);
    }
    // The other form is the counter: m.
    for (const SynthesizedClosedForm &form : loop->forms)
      if (&form != column)
        CHECK(form.text == "v0 := m ; return v0");
    // Under a backend that never decides, nothing is proved and the record says so.
    UnknownBackend unknown;
    SliceSynthesisResult undecided = synthesizeClosedForms(p, points, options, unknown);
    for (const SliceSynthesis &slice : undecided.slices)
      for (const LoopSynthesis &l : slice.loops)
        for (const SynthesizedClosedForm &form : l.forms) {
          CHECK(form.status == ClosedFormStatus::Unknown);
          CHECK(form.status != ClosedFormStatus::Proved);
        }
    std::string json = closedFormsJSON(p, undecided);
    CHECK(json.find("PROVED") == std::string::npos && json.find("UNKNOWN") != std::string::npos);
  }

  // two_phase and rotated: the counter t of the inner loop, entry + m, proved; the
  // slice of the rotated subscript names t and n.
  for (const char *name : {"two_phase", "rotated"}) {
    SemanticProgram p = m.import(name, 8);
    ParameterScope scope = range(p, 2, 3);
    ClosedFormOptions options;
    options.scope = scope;
    std::vector<std::vector<APInt>> points{{APInt(8, 2)}, {APInt(8, 3)}};
    SliceSynthesisResult result = synthesizeClosedForms(p, points, options, z3Backend);
    unsigned proved = 0, specs = 0;
    for (const SliceSynthesis &slice : result.slices)
      for (const LoopSynthesis &loop : slice.loops) {
        if (!loop.spec.reason.empty())
          continue;
        ++specs;
        CHECK(loop.stepMismatches == 0 && loop.conditionMismatches == 0 && loop.subscriptMismatches == 0);
        CHECK(loop.spec.paths.size() == 2); // The two phases are two paths of one iteration.
        for (const SynthesizedClosedForm &form : loop.forms)
          proved += form.status == ClosedFormStatus::Proved;
      }
    CHECK(specs == 2 && proved == 2);
  }
}


/// Phase 3: while programs for a carried variable with no loop-free form, judged at
/// the finite tier by fuel-bounded execution; a cut run is Unknown and no status
/// speaks of termination.
static void testWhilePrograms() {
  Module m(programs);
  // factorial: a = (t + 1)!, Listing G.1 of the thesis. No loop-free form within the
  // grammar; the while program r := 0 ; a := 1 ; while r < m do (a := a * (r + 2) ;
  // r := r + 1) is found from the sampled steps and equals every sampled iteration.
  SemanticProgram p = m.import("factorial", 16);
  ParameterScope scope = range(p, 1, 5);
  ClosedFormOptions options;
  options.scope = scope;
  std::vector<std::vector<APInt>> points;
  for (int64_t n = 1; n <= 5; ++n)
    points.push_back({APInt(16, n)});
  SliceSynthesisResult result = synthesizeClosedForms(p, points, options, z3Backend);
  const LoopSynthesis *loop = nullptr;
  for (const SliceSynthesis &slice : result.slices)
    for (const LoopSynthesis &l : slice.loops)
      if (l.spec.reason.empty() && l.spec.carried.size() == 2)
        loop = &l;
  CHECK(loop != nullptr);
  if (!loop)
    return;
  CHECK(loop->stepMismatches == 0 && loop->subscriptMismatches == 0 && loop->points == 5);
  const SynthesizedClosedForm *fact = nullptr, *counter = nullptr;
  for (const SynthesizedClosedForm &form : loop->forms)
    (form.kind == "while" ? fact : counter) = &form;
  CHECK(fact && counter);
  if (!fact || !counter)
    return;
  // Phase 4: the program of the fixed-counter template passes W-shape, W-init,
  // W-step and W-term, and the form of t it substitutes is proved, so it is PROVED.
  CHECK(fact->status == ClosedFormStatus::Proved);
  CHECK(fact->symbolicRounds == 1);
  CHECK(fact->reason.find("shape, init, step and termination discharged") != std::string::npos);
  CHECK(fact->reason.find("fixed-counter template") != std::string::npos);
  CHECK(fact->records.size() == 4 && fact->records[0].id == "W-shape" && fact->records[0].expected == "structural" &&
        fact->records[1].id == "W-init" && fact->records[2].id == "W-step" && fact->records[3].id == "W-term");
  for (const ObligationRecord &record : fact->records)
    CHECK(record.discharged);
  CHECK(fact->program.has_value() && !fact->program->loopFree());
  CHECK(fact->text.find("while v0 < m do") != std::string::npos);
  CHECK(fact->rounds >= 2 && fact->counterexampleTraces >= 1); // CEGIS from the first point's seeds.
  CHECK(fact->fuelCuts == 0);
  // The program computes (m + 1)! at 16 bits on inputs the traces never had.
  if (fact->program) {
    uint64_t expected = 1;
    for (uint64_t mv = 0; mv <= 7; ++mv) {
      std::vector<APInt> inputs;
      for (Term leaf : fact->program->inputs)
        inputs.push_back(leaf == loop->spec.iteration ? APInt(16, mv) : APInt::getZero(leaf->sort.width));
      IndexRun run = runIndexProgram(*fact->program, inputs);
      CHECK(run.status == IndexRunStatus::Returned && run.value.getZExtValue() == expected);
      expected *= mv + 2;
    }
    // 9! does not fit 16 bits: the program wraps as the machine does. 8! = 40320 fits.
    std::vector<APInt> inputs;
    for (Term leaf : fact->program->inputs)
      inputs.push_back(leaf == loop->spec.iteration ? APInt(16, 8) : APInt::getZero(leaf->sort.width));
    IndexRun run = runIndexProgram(*fact->program, inputs);
    CHECK(run.status == IndexRunStatus::Returned && run.value.getZExtValue() == (362880 % 65536));
  }
  // The counter t: its loop-free form m is proved with a read as a free leaf (the
  // obligation holds for every value of a, so for the actual one).
  CHECK(counter->kind == "loop-free" && counter->status == ClosedFormStatus::Proved);
  CHECK(counter->text == "v0 := m ; return v0");
  // JSON names the kind, the status and the symbolic round.
  std::string json = closedFormsJSON(p, result);
  CHECK(json.find("\"kind\": \"while\"") != std::string::npos);
  CHECK(json.find("\"status\": \"PROVED\"") != std::string::npos);
  CHECK(json.find("\"symbolic_rounds\": 1") != std::string::npos);
  CHECK(json.find("W-term") != std::string::npos);
  // Not proved means not proved: a stub that never decides leaves the while
  // program SAMPLE_CONSISTENT with the solver's word in the reason; a stub that
  // says sat without a model refutes nothing and proves nothing.
  {
    UnknownBackend undecided;
    SliceSynthesisResult u = synthesizeClosedForms(p, points, options, undecided);
    unsigned whiles = 0;
    for (const SliceSynthesis &slice : u.slices)
      for (const LoopSynthesis &l : slice.loops)
        for (const SynthesizedClosedForm &form : l.forms) {
          CHECK(form.status != ClosedFormStatus::Proved);
          if (form.kind == "while") {
            ++whiles;
            CHECK(form.status == ClosedFormStatus::SampleConsistent);
            CHECK(form.reason.find("not a proof") != std::string::npos);
            CHECK(form.reason.find("symbolic: the solver did not decide") != std::string::npos);
            CHECK(form.symbolicRounds == 1);
          }
        }
    CHECK(whiles == 1);
    SatBackend liar;
    SliceSynthesisResult sat = synthesizeClosedForms(p, points, options, liar);
    for (const SliceSynthesis &slice : sat.slices)
      for (const LoopSynthesis &l : slice.loops)
        for (const SynthesizedClosedForm &form : l.forms)
          CHECK(form.status != ClosedFormStatus::Proved);
  }

  // The finite-tier check itself: a wrong program is refuted at the first sampled
  // iteration it misses, with that sample as the model; a run cut by the fuel is
  // Unknown and not a refutation.
  {
    const LoopSpecification &spec = loop->spec;
    unsigned k = fact->variable == spec.carried[0] ? 0 : 1;
    std::vector<LoopTrace> traces;
    EnumerationLimits limits;
    for (const auto &point : points) {
      auto states = eventStates(p, point, 0, limits);
      CHECK(states.has_value());
      if (!states)
        continue;
      LoopReplay replay = replayLoop(p, result.slices[1].slice, spec, states->entry, RunLimits());
      for (LoopTrace &trace : replay.traces)
        traces.push_back(std::move(trace));
    }
    CHECK(traces.size() == 5);
    CheckedClosedForm good = checkProgramOnTraces(p, spec, k, *fact->program, traces, 1u << 12);
    CHECK(good.status == ClosedFormStatus::SampleConsistent);
    IndexProgram doubling;
    for (Term leaf : fact->program->inputs)
      doubling.input(leaf);
    unsigned r = doubling.local(16), v = doubling.local(16);
    unsigned mIn = 0;
    for (unsigned n = 0; n < doubling.inputs.size(); ++n)
      if (doubling.inputs[n] == spec.iteration)
        mIn = n;
    doubling.body = doubling.seq(
        doubling.seq(doubling.assign(r, doubling.constant(APInt(16, 0))), doubling.assign(v, doubling.constant(APInt(16, 1)))),
        doubling.whileDo(doubling.ult(doubling.localExpr(r), doubling.inputExpr(mIn)),
                         doubling.seq(doubling.assign(v, doubling.mul(doubling.localExpr(v), doubling.constant(APInt(16, 2)))),
                                      doubling.assign(r, doubling.add(doubling.localExpr(r), doubling.constant(APInt(16, 1)))))));
    doubling.result = doubling.localExpr(v);
    CheckedClosedForm wrong = checkProgramOnTraces(p, spec, k, doubling, traces, 1u << 12);
    CHECK(wrong.status == ClosedFormStatus::Refuted);
    bool mAtTwo = false;
    for (const auto &[name, value] : wrong.model)
      mAtTwo = mAtTwo || (name == "m" && value.getZExtValue() == 2); // 2! = 2 = 2^1 agrees; 3! = 6 != 4.
    CHECK(mAtTwo);
    CheckedClosedForm cut = checkProgramOnTraces(p, spec, k, *fact->program, traces, 3);
    CHECK(cut.status == ClosedFormStatus::Unknown && cut.reason.find("fuel") != std::string::npos);
    CHECK(cut.reason.find("no claim about termination") != std::string::npos);
    // Phase 4, the W obligations themselves. The doubling program has the shape and
    // the init; its step has a model (a free, r = m), which is reported as Unknown with
    // the model, not as a refutation, until a run replays it. The template's
    // termination obligation holds for any body.
    {
      SmtLimits limits;
      ClosedForm tForm{spec.carried[1 - k], counter->term, nullptr};
      std::vector<CheckedClosedForm> w =
          checkClosedForms(p, spec, {tForm, ClosedForm{spec.carried[k], nullptr, &doubling}}, scope, limits, z3Backend);
      CHECK(w.size() == 2 && w[1].status == ClosedFormStatus::Unknown && !w[1].model.empty());
      CHECK(w[1].reason.find("free") != std::string::npos && w[1].reason.find("neither a proof nor a refutation") != std::string::npos);
      CHECK(w[1].records.size() == 4 && w[1].records[0].discharged && w[1].records[1].discharged &&
            !w[1].records[2].discharged && w[1].records[3].id == "W-term" && w[1].records[3].discharged);
      CHECK(llvm::is_contained(w[1].freeLeaves, spec.carried[k]));
      // The right program, alone: its step reads t, whose form is not given, so t is
      // free as well, and the step still holds (a * (2 + m) against a * (t + 2) needs
      // t = m): not proved, Unknown with a model.
      std::vector<CheckedClosedForm> alone =
          checkClosedForms(p, spec, {ClosedForm{spec.carried[k], nullptr, &*fact->program}}, scope, limits, z3Backend);
      CHECK(alone.size() == 1 && alone[0].status != ClosedFormStatus::Proved);
      // With t's form: proved, and it depends on t.
      std::vector<CheckedClosedForm> with =
          checkClosedForms(p, spec, {tForm, ClosedForm{spec.carried[k], nullptr, &*fact->program}}, scope, limits, z3Backend);
      CHECK(with.size() == 2 && with[1].status == ClosedFormStatus::Proved && with[1].discharged);
      CHECK(with[1].dependsOn == std::vector<unsigned>{spec.carried[1 - k]});
      // With a wrong form of t substituted (t = 2m), a's own obligations fail or a's
      // proof is demoted: never PROVED through an unproved dependence.
      Term wrongT = p.terms->binary(TermOp::Mul, spec.iteration, p.terms->bv(APInt(16, 2)));
      std::vector<CheckedClosedForm> demoted = checkClosedForms(
          p, spec, {ClosedForm{spec.carried[1 - k], wrongT, nullptr}, ClosedForm{spec.carried[k], nullptr, &*fact->program}},
          scope, limits, z3Backend);
      CHECK(demoted.size() == 2 && demoted[0].status != ClosedFormStatus::Proved &&
            demoted[1].status != ClosedFormStatus::Proved);
      // Simultaneous induction: a's obligations hold with t's form substituted, but a
      // backend that cannot decide t's obligations leaves t unproved, and a is then
      // demoted: discharged, not PROVED.
      {
        struct SelectiveBackend : SmtBackend {
          SmtBackend &inner;
          std::string undecided;
          SelectiveBackend(SmtBackend &inner, std::string undecided) : inner(inner), undecided(std::move(undecided)) {}
          SmtResponse check(const std::string &script, const SmtLimits &limits, bool wantModel) override {
            if (script.find(undecided) != std::string::npos) {
              SmtResponse r;
              r.answer = SmtAnswer::Unknown;
              r.detail = "selectively undecided";
              return r;
            }
            return inner.check(script, limits, wantModel);
          }
          std::string version() const override { return "selective"; }
        } selective(z3Backend, "variable s" + std::to_string(spec.carried[1 - k]) + "\n");
        std::vector<CheckedClosedForm> demotedByT = checkClosedForms(
            p, spec, {tForm, ClosedForm{spec.carried[k], nullptr, &*fact->program}}, scope, limits, selective);
        CHECK(demotedByT.size() == 2 && demotedByT[0].status == ClosedFormStatus::Unknown);
        CHECK(demotedByT[1].discharged && demotedByT[1].status == ClosedFormStatus::Unknown);
        CHECK(demotedByT[1].reason.find("obligations discharged, but the substituted form of s") != std::string::npos);
      }
      // Outside the template: a body that reads the bound m is refused at W-shape.
      IndexProgram readsBound = *fact->program;
      {
        std::string why;
        auto shape = matchCounterTemplate(readsBound, why);
        CHECK(shape.has_value());
        if (shape) {
          unsigned mIn2 = shape->bound;
          // v := v + m inside the body.
          unsigned bump = readsBound.assign(shape->value, readsBound.add(readsBound.localExpr(shape->value),
                                                                          readsBound.inputExpr(mIn2)));
          unsigned loopStmt = readsBound.stmts[readsBound.body].b;
          unsigned step = readsBound.stmts[loopStmt].a;
          readsBound.stmts[step].a = readsBound.seq(readsBound.stmts[step].a, bump);
          std::string again;
          CHECK(!matchCounterTemplate(readsBound, again).has_value());
          CHECK(again.find("reads the bound") != std::string::npos);
          std::vector<CheckedClosedForm> refused = checkClosedForms(
              p, spec, {tForm, ClosedForm{spec.carried[k], nullptr, &readsBound}}, scope, limits, z3Backend);
          CHECK(refused.size() == 2 && refused[1].status == ClosedFormStatus::Unknown);
          CHECK(refused[1].reason.find("not of a supported template") != std::string::npos);
          CHECK(refused[1].records.size() == 1 && !refused[1].records[0].discharged);
        }
      }
      // The coupling lemma on this program, as test evidence: the run on m + 1 is the
      // run on m followed by one body step, for m = 0..6.
      for (uint64_t mv = 0; mv <= 6; ++mv) {
        std::vector<APInt> inputs, inputsNext;
        for (Term leaf : fact->program->inputs) {
          inputs.push_back(leaf == spec.iteration ? APInt(16, mv) : APInt::getZero(leaf->sort.width));
          inputsNext.push_back(leaf == spec.iteration ? APInt(16, mv + 1) : APInt::getZero(leaf->sort.width));
        }
        IndexRun at = runIndexProgram(*fact->program, inputs), next = runIndexProgram(*fact->program, inputsNext);
        CHECK(at.status == IndexRunStatus::Returned && next.status == IndexRunStatus::Returned);
        CHECK(next.value == at.value * APInt(16, 2 + mv));
      }
    }
    // A fuel too small for the search: no while program is consistent or proved;
    // the loop-free form of t, which no fuel touches, is proved as before.
    ClosedFormOptions starved = options;
    starved.programFuel = 3;
    SliceSynthesisResult cutResult = synthesizeClosedForms(p, points, starved, z3Backend);
    for (const SliceSynthesis &slice : cutResult.slices)
      for (const LoopSynthesis &l : slice.loops)
        for (const SynthesizedClosedForm &form : l.forms) {
          CHECK(form.status != ClosedFormStatus::SampleConsistent);
          if (form.variable == fact->variable) {
            CHECK(form.status != ClosedFormStatus::Proved);
            if (form.kind == "while")
              CHECK(form.fuelCuts > 0 || form.status == ClosedFormStatus::Unknown);
          }
        }
    // Off: no while program is tried; a has no loop-free form and stays Unknown.
    ClosedFormOptions off = options;
    off.whilePrograms = false;
    SliceSynthesisResult plain = synthesizeClosedForms(p, points, off, z3Backend);
    for (const SliceSynthesis &slice : plain.slices)
      for (const LoopSynthesis &l : slice.loops)
        for (const SynthesizedClosedForm &form : l.forms) {
          CHECK(form.kind == "loop-free");
          if (form.variable == fact->variable)
            CHECK(form.status == ClosedFormStatus::Unknown && form.term == nullptr);
        }
    // Symbolic proving off: the while program stays at the finite tier.
    ClosedFormOptions finiteOnly = options;
    finiteOnly.proveWhile = false;
    SliceSynthesisResult finite = synthesizeClosedForms(p, points, finiteOnly, z3Backend);
    for (const SliceSynthesis &slice : finite.slices)
      for (const LoopSynthesis &l : slice.loops)
        for (const SynthesizedClosedForm &form : l.forms)
          if (form.kind == "while") {
            CHECK(form.status == ClosedFormStatus::SampleConsistent && form.symbolicRounds == 0);
            CHECK(form.reason.find("not a proof") != std::string::npos);
          }
  }

  // stepped: the reduction loop has no plain counter and the entry into it is the
  // constant 0 from the outer loop, so the init is known. Over a scope wide enough
  // for five iterations, the loop-free branch 5m + 2(m mod 2) is proved and no while
  // program is needed; over a narrow scope the samples underdetermine the form.
  {
    SemanticProgram q = m.import("stepped", 8);
    ParameterScope wide = range(q, 8, 40);
    ClosedFormOptions o;
    o.scope = wide;
    std::vector<std::vector<APInt>> pts;
    for (int64_t n = 8; n <= 40; n += 4)
      pts.push_back({APInt(8, n)});
    SliceSynthesisResult r = synthesizeClosedForms(q, pts, o, z3Backend);
    unsigned proved = 0;
    for (const SliceSynthesis &slice : r.slices)
      for (const LoopSynthesis &l : slice.loops) {
        if (!l.spec.reason.empty())
          continue;
        CHECK(l.spec.init.size() == 1 && l.spec.init[0] != nullptr);
        CHECK(l.stepMismatches == 0 && l.subscriptMismatches == 0);
        for (const SynthesizedClosedForm &form : l.forms) {
          CHECK(form.kind == "loop-free" && form.status == ClosedFormStatus::Proved);
          CHECK(form.text.find("mod 2") != std::string::npos);
          proved += form.status == ClosedFormStatus::Proved;
        }
      }
    CHECK(proved == 1);
    // A narrow scope, n in [8, 12]: three iterations at most, and the samples
    // underdetermine the form. The loop-free candidate's step obligation has a
    // model at an iteration the loop never reaches (m = 185 at n = 12); the
    // specification's own run from that model agrees with the candidate, so the
    // model refutes nothing and the candidate is UNKNOWN, neither PROVED nor REFUTED.
    ParameterScope narrow = range(q, 8, 12);
    ClosedFormOptions no;
    no.scope = narrow;
    std::vector<std::vector<APInt>> few;
    for (int64_t n = 8; n <= 12; ++n)
      few.push_back({APInt(8, n)});
    SliceSynthesisResult nr = synthesizeClosedForms(q, few, no, z3Backend);
    unsigned judged = 0;
    for (const SliceSynthesis &slice : nr.slices)
      for (const LoopSynthesis &l : slice.loops)
        for (const SynthesizedClosedForm &form : l.forms)
          if (l.spec.reason.empty()) {
            ++judged;
            CHECK(form.status != ClosedFormStatus::Proved);
            CHECK(form.status == ClosedFormStatus::Unknown);
            CHECK(form.reason.find("does not replay into a disagreement") != std::string::npos);
          }
    CHECK(judged == 1);
  }
}

/// Phase 5, step 1 (Appendix G.5): arrays in the grammar and the semantics. The
/// lattice program P_L of (G.4) is written by hand here, as the thesis wrote it,
/// and checked against the rules: it is a REFERENCE for the semantics, not a
/// synthesis result, and the record keeps the two apart.
static void testArrays() {
  TermContext terms;
  const unsigned w = 32;
  Term mIn = terms.param(0, Sort::bv(w)), nIn = terms.param(1, Sort::bv(w));
  auto run = [&](const IndexProgram &p, std::vector<APInt> inputs, std::vector<IndexTable> tables = {},
                 uint64_t fuel = 1u << 20) {
    IndexRunLimits limits;
    limits.fuel = fuel;
    return runIndexProgram(p, inputs, limits, tables);
  };
  // P_L: i := 0; while i < M do (dp[i][0] := 1; i := i + 1);
  //      j := 0; while j < N do (dp[0][j] := 1; j := j + 1);
  //      i := 1; while i < M do (j := 1; while j < N do (dp[i][j] := dp[i-1][j] + dp[i][j-1]; j := j + 1); i := i + 1);
  //      return dp[M-1][N-1]
  auto lattice = [&](bool boundaryLoops, bool badWrite, bool badRank) {
    IndexProgram p;
    unsigned M = p.input(mIn), N = p.input(nIn);
    unsigned i = p.local(w), j = p.local(w);
    unsigned dp = p.array(w, w, {p.inputExpr(M), p.inputExpr(N)});
    auto k = [&](uint64_t v) { return p.constant(APInt(w, v)); };
    unsigned one = k(1);
    unsigned rows = p.whileDo(p.ult(p.localExpr(i), p.inputExpr(M)),
                              p.seq(p.write(dp, {p.localExpr(i), k(0)}, one),
                                    p.assign(i, p.add(p.localExpr(i), k(1)))));
    unsigned cols = p.whileDo(p.ult(p.localExpr(j), p.inputExpr(N)),
                              p.seq(p.write(dp, {k(0), p.localExpr(j)}, one),
                                    p.assign(j, p.add(p.localExpr(j), k(1)))));
    unsigned cell = p.add(p.read(dp, {p.sub(p.localExpr(i), k(1)), p.localExpr(j)}),
                          p.read(dp, {p.localExpr(i), p.sub(p.localExpr(j), k(1))}));
    unsigned inner = p.whileDo(p.ult(p.localExpr(j), p.inputExpr(N)),
                               p.seq(p.write(dp, {p.localExpr(i), p.localExpr(j)}, cell),
                                     p.assign(j, p.add(p.localExpr(j), k(1)))));
    unsigned outer = p.whileDo(p.ult(p.localExpr(i), p.inputExpr(M)),
                               p.seq(p.seq(p.assign(j, k(1)), inner), p.assign(i, p.add(p.localExpr(i), k(1)))));
    unsigned nest = p.seq(p.assign(i, k(1)), outer);
    unsigned body = nest;
    if (boundaryLoops)
      body = p.seq(p.seq(p.seq(p.assign(i, k(0)), rows), p.seq(p.assign(j, k(0)), cols)), nest);
    if (badWrite) // dp[M][0] := 1, index M outside [0, M).
      body = p.seq(body, p.write(dp, {p.inputExpr(M), k(0)}, one));
    if (badRank) // dp[0] with one index on an array of rank 2: refused by the builder.
      p.read(dp, {k(0)});
    p.body = body;
    p.result = p.read(dp, {p.sub(p.inputExpr(M), k(1)), p.sub(p.inputExpr(N), k(1))});
    return p;
  };
  IndexProgram pl = lattice(true, false, false);
  CHECK(pl.usesArrays() && !pl.loopFree());
  CHECK(!indexProgramTerm(terms, pl).has_value());
  // Table: binom(M+N-2, M-1) for 1 <= M, N <= 8; every element (i, j) holds binom(i+j, i);
  // M*N defined elements; M + N + (M-1)(N-1) writes, (0, 0) written twice.
  auto binom = [](uint64_t n, uint64_t k) {
    uint64_t r = 1;
    for (uint64_t t = 1; t <= k; ++t)
      r = r * (n - k + t) / t;
    return r;
  };
  unsigned checkedCells = 0;
  for (uint64_t M = 1; M <= 8; ++M)
    for (uint64_t N = 1; N <= 8; ++N) {
      IndexRun r = run(pl, {APInt(w, M), APInt(w, N)});
      CHECK(r.status == IndexRunStatus::Returned);
      if (r.status != IndexRunStatus::Returned)
        continue;
      CHECK(r.value.getZExtValue() == binom(M + N - 2, M - 1));
      std::vector<uint64_t> shape{M, N};
      CHECK(r.finalArrays.size() == 1 && r.finalArrays[0].extents == shape);
      CHECK(r.finalArrays[0].elements.size() == M * N);
      CHECK(r.writes == M + N + (M - 1) * (N - 1));
      for (const auto &[k, v] : r.finalArrays[0].elements) {
        CHECK(v.getZExtValue() == binom(k[0] + k[1], k[0]));
        ++checkedCells;
      }
    }
  CHECK(checkedCells == 1296); // Sum of M*N over 1..8 x 1..8 = 36 * 36.
  // Machine integers: binom(i+j, i) fits unsigned 32 bits up to i + j = 34; at
  // M = 18, N = 19 the top element is binom(35, 17), which does not, and the run
  // holds it modulo 2^32 (the thesis's int bound, i + j <= 33, is the signed one).
  {
    IndexRun r = run(pl, {APInt(w, 18), APInt(w, 19)});
    CHECK(r.status == IndexRunStatus::Returned);
    uint64_t exact = binom(35, 17);
    CHECK(exact > 0xFFFFFFFFull && r.value.getZExtValue() == (exact & 0xFFFFFFFFull));
  }
  // No rule: without the boundary loops the nest reads dp[0][1], an element without
  // a value, at M = N = 2; dp[M][0] := 1 writes outside the extent; one index on a
  // rank-2 array is refused before any run.
  {
    IndexRun r = run(lattice(false, false, false), {APInt(w, 2), APInt(w, 2)});
    CHECK(r.status == IndexRunStatus::NoRule);
    CHECK(r.reason.find("read of T0(0, 1), an element without a value") != std::string::npos);
    IndexRun o = run(lattice(true, true, false), {APInt(w, 3), APInt(w, 2)});
    CHECK(o.status == IndexRunStatus::NoRule);
    CHECK(o.reason.find("write of T0 at index 3 in dimension 0, outside the extent 3") != std::string::npos);
    CHECK_THROWS(lattice(true, false, true));
    CHECK(std::string(stringifyIndexRunStatus(IndexRunStatus::NoRule)) == "NO_RULE");
    // Extent 0: an empty table; the return reads dp[-1][-1] = (2^32-1, ...) outside it.
    IndexRun z = run(pl, {APInt(w, 0), APInt(w, 1)});
    CHECK(z.status == IndexRunStatus::NoRule && z.reason.find("outside the extent 0") != std::string::npos);
    // Fuel still cuts: no value, no claim.
    IndexRun cut = run(pl, {APInt(w, 8), APInt(w, 8)}, {}, 20);
    CHECK(cut.status == IndexRunStatus::OutOfFuel);
  }
  // Text names the arrays and their extents.
  {
    std::string text = formatIndexProgram(pl, [&](Term t) { return t == mIn ? std::string("M") : std::string("N"); });
    CHECK(text.rfind("array T0[M][N] ; ", 0) == 0);
    CHECK(text.find("T0[v0][0] := 1") != std::string::npos);
    CHECK(text.find("T0[v0][v1] := (T0[(v0 - 1)][v1] + T0[v0][(v1 - 1)])") != std::string::npos);
    CHECK(text.find("return T0[(M - 1)][(N - 1)]") != std::string::npos);
  }
  // Input arrays (the a1 case of F.6): the value read at the counter; the table's
  // extents must match the declaration; an element the table lacks has no rule.
  {
    IndexProgram p;
    unsigned len = p.input(mIn), m = p.input(nIn);
    unsigned a = p.array(w, w, {p.inputExpr(len)}, /*input=*/true);
    p.body = p.skip();
    p.result = p.read(a, {p.inputExpr(m)});
    IndexTable table;
    table.extents = {4};
    for (uint64_t k = 0; k < 4; ++k)
      table.elements[{k}] = APInt(w, 10 * k + 1);
    IndexRun r = run(p, {APInt(w, 4), APInt(w, 2)}, {table});
    CHECK(r.status == IndexRunStatus::Returned && r.value.getZExtValue() == 21);
    IndexRun o = run(p, {APInt(w, 4), APInt(w, 4)}, {table});
    CHECK(o.status == IndexRunStatus::NoRule && o.reason.find("outside the extent 4") != std::string::npos);
    IndexRun bad = run(p, {APInt(w, 5), APInt(w, 2)}, {table});
    CHECK(bad.status == IndexRunStatus::NoRule && bad.reason.find("extents differ") != std::string::npos);
    table.elements.erase({2});
    IndexRun hole = run(p, {APInt(w, 4), APInt(w, 2)}, {table});
    CHECK(hole.status == IndexRunStatus::NoRule && hole.reason.find("without a value") != std::string::npos);
    IndexRun none = run(p, {APInt(w, 4), APInt(w, 2)});
    CHECK(none.status == IndexRunStatus::NoRule && none.reason.find("has no table") != std::string::npos);
    CHECK(formatIndexProgram(p).rfind("input array T0[x0] ; ", 0) == 0);
    // An extent over a local is refused; a program with arrays is not the counter template.
    IndexProgram q;
    q.input(mIn);
    unsigned v = q.local(w);
    CHECK_THROWS(q.array(w, w, {q.localExpr(v)}));
    std::string why;
    CHECK(!matchCounterTemplate(pl, why).has_value() && why.find("array") != std::string::npos);
  }
  // Frame and overwrite: a second write replaces the element; the other array and
  // the scalars are untouched; the written set is exact.
  {
    IndexProgram p;
    unsigned n = p.input(mIn);
    unsigned v = p.local(w);
    unsigned a = p.array(w, w, {p.inputExpr(n)}), b = p.array(w, w, {p.inputExpr(n)});
    auto k = [&](uint64_t x) { return p.constant(APInt(w, x)); };
    p.body = p.seq(p.seq(p.write(a, {k(1)}, k(5)), p.write(b, {k(1)}, k(7))),
                   p.seq(p.write(a, {k(1)}, k(9)), p.assign(v, p.add(p.read(a, {k(1)}), p.read(b, {k(1)})))));
    p.result = p.localExpr(v);
    IndexRun r = run(p, {APInt(w, 3)});
    CHECK(r.status == IndexRunStatus::Returned && r.value.getZExtValue() == 16);
    CHECK(r.finalArrays[0].elements.size() == 1 && r.finalArrays[1].elements.size() == 1);
    CHECK(r.finalArrays[0].elements.at({1}).getZExtValue() == 9 && r.finalArrays[1].elements.at({1}).getZExtValue() == 7);
    CHECK(r.writes == 3);
  }
  // The history table template (phase 5), T of extent m + 2: T[0] := 0 ; T[1] := 1 ;
  // r := 2 ; while r < m + 1 do (T[r] := T[r-1] + T[r-2] + 1 ; r := r + 1) ; return T[m].
  {
    IndexProgram p;
    unsigned m = p.input(mIn);
    unsigned r = p.local(w);
    auto k = [&](uint64_t x) { return p.constant(APInt(w, x)); };
    unsigned T = p.array(w, w, {p.add(p.inputExpr(m), k(2))}); // Extent m + d, d = 2.
    unsigned E = p.add(p.add(p.read(T, {p.sub(p.localExpr(r), k(1))}), p.read(T, {p.sub(p.localExpr(r), k(2))})), k(1));
    p.body = p.seq(p.seq(p.seq(p.write(T, {k(0)}, k(0)), p.write(T, {k(1)}, k(1))), p.assign(r, k(2))),
                   p.whileDo(p.ult(p.localExpr(r), p.add(p.inputExpr(m), k(1))),
                             p.seq(p.write(T, {p.localExpr(r)}, E), p.assign(r, p.add(p.localExpr(r), k(1))))));
    p.result = p.read(T, {p.inputExpr(m)});
    std::string why;
    auto t = matchTableTemplate(p, why);
    CHECK(t.has_value());
    if (t) {
      CHECK(t->depth() == 2 && t->counter == r && t->bound == m && t->value == E);
    }
    uint64_t expected[] = {0, 1, 2, 4, 7, 12, 20, 33, 54};
    for (uint64_t mv = 0; mv < 9; ++mv) {
      IndexRun run1 = run(p, {APInt(w, mv)});
      CHECK(run1.status == IndexRunStatus::Returned && run1.value.getZExtValue() == expected[mv]);
      if (run1.status == IndexRunStatus::Returned)
        CHECK(run1.finalArrays[0].elements.size() == std::max<uint64_t>(mv + 1, 2)); // The inits, then T[2..m].
    }
    // At m = 2^32 - 1 the bound m + 1 wraps to 0: the loop does not run; the extent
    // m + 2 wraps to 1, so the init write T[1] has no rule. No nontermination.
    IndexRun top = run(p, {APInt::getAllOnes(w)});
    CHECK(top.status == IndexRunStatus::NoRule && top.reason.find("write of T0 at index 1") != std::string::npos);
    // Not the template: a read T[r - 3] with depth 2; a return of T[m - 1]; a second array.
    IndexProgram q = p;
    q.exprs[E].a = q.read(T, {q.sub(q.localExpr(r), k(3))});
    CHECK(!matchTableTemplate(q, why).has_value() && why.find("T[r - j]") != std::string::npos);
    IndexProgram q2 = p;
    q2.result = q2.read(T, {q2.sub(q2.inputExpr(m), q2.constant(APInt(w, 1)))});
    CHECK(!matchTableTemplate(q2, why).has_value() && why.find("return T[m]") != std::string::npos);
    IndexProgram q3 = p;
    q3.array(w, w, {q3.inputExpr(m)});
    CHECK(!matchTableTemplate(q3, why).has_value() && why.find("exactly one array") != std::string::npos);
    CHECK(!matchTableTemplate(pl, why).has_value());
    // An extent other than m + d (here m + 1 with d = 2) is not the template either.
    IndexProgram q4 = p;
    q4.exprs[q4.exprs[q4.arrays[0].extents[0]].b].value = APInt(w, 1);
    CHECK(!matchTableTemplate(q4, why).has_value() && why.find("extent") != std::string::npos);
  }
}

/// Phase 5, step 2: synthesis of a table program (not a hand-written one) for a
/// subscript held as a two-term recurrence, its proof by the TT obligations, and
/// the controls that keep SAMPLE_CONSISTENT, REFUTED and UNKNOWN apart.
static void testTables() {
  Module m(programs);
  SemanticProgram p = m.import("history", 8);
  ParameterScope scope = range(p, 2, 9);
  ClosedFormOptions options;
  options.scope = scope;
  std::vector<std::vector<APInt>> points;
  for (int64_t n = 2; n <= 9; ++n)
    points.push_back({APInt(8, n)});
  SliceSynthesisResult result = synthesizeClosedForms(p, points, options, z3Backend);
  const LoopSynthesis *loop = nullptr;
  for (const SliceSynthesis &slice : result.slices)
    for (const LoopSynthesis &l : slice.loops)
      if (l.spec.reason.empty() && l.spec.carried.size() == 3)
        loop = &l;
  CHECK(loop != nullptr);
  if (!loop)
    return;
  CHECK(loop->stepMismatches == 0 && loop->subscriptMismatches == 0 && loop->points == 8);
  // k = m loop-free; f a table PROVED; g a table at the finite tier only.
  const SynthesizedClosedForm *fForm = nullptr, *gForm = nullptr, *kForm = nullptr;
  for (const SynthesizedClosedForm &form : loop->forms) {
    if (form.kind == "loop-free")
      kForm = &form;
    else if (form.status == ClosedFormStatus::Proved)
      fForm = &form;
    else
      gForm = &form;
  }
  CHECK(kForm && fForm && gForm);
  if (!kForm || !fForm || !gForm)
    return;
  CHECK(kForm->status == ClosedFormStatus::Proved && kForm->text == "v0 := m ; return v0");
  CHECK(fForm->kind == "table" && fForm->program.has_value() && fForm->program->usesArrays());
  CHECK(fForm->text == "array T0[(m + 2)] ; T0[0] := 0 ; T0[1] := 1 ; v0 := 2 ; while v0 < (m + 1) do "
                       "(T0[v0] := (1 + (T0[(v0 - 1)] + T0[(v0 - 2)])) ; v0 := (v0 + 1)) ; return T0[m]");
  CHECK(fForm->history.size() == 1 && fForm->history[0] == gForm->variable);
  CHECK(fForm->symbolicRounds == 1 && fForm->rounds >= 1);
  CHECK(fForm->reason.find("shape, init, step, shift, bounds and termination discharged") != std::string::npos);
  CHECK(fForm->reason.find("for m < 2^8 - 2") != std::string::npos);
  CHECK(fForm->records.size() == 7);
  for (const ObligationRecord &record : fForm->records)
    CHECK(record.discharged);
  if (fForm->records.size() == 7) {
    CHECK(fForm->records[0].id == "TT-shape" && fForm->records[0].expected == "structural");
    CHECK(fForm->records[1].id == "TT-init0" && fForm->records[2].id == "TT-init1" && fForm->records[3].id == "TT-step" &&
          fForm->records[4].id == "TT-shift1" && fForm->records[5].id == "TT-bounds" && fForm->records[6].id == "TT-term");
  }
  // The synthesized program on values the traces never had: 0, 1, 2, 4, 7, 12, 20, 33,
  // 54, 88 (n <= 9 keeps the sampled ones below 64); at m = 2^8 - 2 the extent wraps
  // and the semantics has no rule, which is outside what was proved.
  {
    uint64_t expected[] = {0, 1, 2, 4, 7, 12, 20, 33, 54, 88, 143};
    for (uint64_t mv = 0; mv < 11; ++mv) {
      std::vector<APInt> inputs;
      for (Term leaf : fForm->program->inputs)
        inputs.push_back(leaf == loop->spec.iteration ? APInt(8, mv) : APInt::getZero(leaf->sort.width));
      IndexRun run = runIndexProgram(*fForm->program, inputs);
      CHECK(run.status == IndexRunStatus::Returned && run.value.getZExtValue() == (expected[mv] & 0xFF));
    }
    std::vector<APInt> inputs;
    for (Term leaf : fForm->program->inputs)
      inputs.push_back(leaf == loop->spec.iteration ? APInt(8, 254) : APInt::getZero(leaf->sort.width));
    CHECK(runIndexProgram(*fForm->program, inputs).status == IndexRunStatus::NoRule);
  }
  // g = f(m - 1): a table program fits every sample, but no carried variable trails
  // g, so the step cannot be stated over the loop's state: not proved, and said so.
  CHECK(gForm->kind == "table" && gForm->status == ClosedFormStatus::SampleConsistent);
  CHECK(gForm->reason.find("not a proof") != std::string::npos);
  CHECK(gForm->reason.find("no carried variable trails the chain") != std::string::npos);
  CHECK(gForm->history.empty());
  // Reference versus synthesis: the hand-written template of testArrays is the same
  // program text the search produced here, and that agreement is a test, not the
  // proof; the proof is the seven obligations above.
  // Controls.
  {
    // The TT obligations on a wrong body: T[r-1] + T[r-2] (no + 1) fits no sample
    // beyond m = 1 and is refuted at the finite tier before any solver call.
    IndexProgram wrong = *fForm->program;
    std::string why;
    auto shape = matchTableTemplate(wrong, why);
    CHECK(shape.has_value());
    if (shape) {
      const IndexProgram::Expr &e = wrong.exprs[shape->value];
      CHECK(e.kind == IndexExprKind::Add);
      // Replace E by its right operand (the sum of the two reads).
      unsigned sum = e.a;
      if (wrong.exprs[sum].kind != IndexExprKind::Add)
        sum = e.b;
      for (IndexProgram::Stmt &st : wrong.stmts)
        if (st.kind == IndexStmtKind::Write && st.expr == shape->value)
          st.expr = sum;
      std::vector<LoopTrace> traces;
      EnumerationLimits limits;
      for (const auto &point : points) {
        auto states = eventStates(p, point, 0, limits);
        if (!states)
          continue;
        LoopReplay replay = replayLoop(p, result.slices[1].slice, loop->spec, states->entry, RunLimits());
        for (LoopTrace &trace : replay.traces)
          traces.push_back(std::move(trace));
      }
      unsigned kf = std::find(loop->spec.carried.begin(), loop->spec.carried.end(), fForm->variable) - loop->spec.carried.begin();
      CheckedClosedForm finite = checkProgramOnTraces(p, loop->spec, kf, wrong, traces, 1u << 12);
      CHECK(finite.status == ClosedFormStatus::Refuted);
      // A table too small for its inits (extent m + 1 with d = 2) has no rule at m = 0:
      // a definite failure at a sample, refuted, not a cut run.
      IndexProgram narrow = *fForm->program;
      narrow.exprs[narrow.exprs[narrow.arrays[0].extents[0]].b].value = APInt(8, 1);
      CheckedClosedForm noRule = checkProgramOnTraces(p, loop->spec, kf, narrow, traces, 1u << 12);
      CHECK(noRule.status == ClosedFormStatus::Refuted && noRule.reason.find("no rule applies") != std::string::npos);
      // And symbolically: TT-step has a model over the free chain, which is Unknown
      // with the model, never a refutation by itself.
      SmtLimits smt;
      auto checked = checkClosedForms(p, loop->spec, {ClosedForm{fForm->variable, nullptr, &wrong, fForm->history}}, scope, smt, z3Backend);
      CHECK(checked.size() == 1 && checked[0].status == ClosedFormStatus::Unknown && !checked[0].model.empty());
      CHECK(checked[0].reason.find("TT-step has a model") != std::string::npos);
      // A wrong init (T[1] := 2) is refuted at TT-init1 outright: no free leaf there.
      IndexProgram wrongInit = *fForm->program;
      for (IndexProgram::Stmt &st : wrongInit.stmts)
        if (st.kind == IndexStmtKind::Write && wrongInit.exprs[st.indices[0]].kind == IndexExprKind::Const &&
            wrongInit.exprs[st.indices[0]].value == 1 && wrongInit.exprs[st.expr].kind == IndexExprKind::Const)
          st.expr = wrongInit.constant(APInt(8, 2));
      auto init = checkClosedForms(p, loop->spec, {ClosedForm{fForm->variable, nullptr, &wrongInit, fForm->history}}, scope, smt, z3Backend);
      CHECK(init.size() == 1 && init[0].status == ClosedFormStatus::Refuted && init[0].reason == "init1 has a counterexample");
      // A chain proposal that is wrong (k instead of g) fails TT-shift and is not proved.
      auto badChain = checkClosedForms(p, loop->spec, {ClosedForm{fForm->variable, nullptr, &*fForm->program, {kForm->variable}}}, scope, smt, z3Backend);
      CHECK(badChain.size() == 1 && badChain[0].status != ClosedFormStatus::Proved);
      // Not decided is not proved: the stub backend leaves the table SAMPLE_CONSISTENT.
      UnknownBackend undecided;
      SliceSynthesisResult u = synthesizeClosedForms(p, points, options, undecided);
      for (const SliceSynthesis &slice : u.slices)
        for (const LoopSynthesis &l : slice.loops)
          for (const SynthesizedClosedForm &form : l.forms) {
            CHECK(form.status != ClosedFormStatus::Proved);
            if (form.kind == "table")
              CHECK(form.status == ClosedFormStatus::SampleConsistent && form.reason.find("not a proof") != std::string::npos);
          }
      // Tables off: f and g stay without a program.
      ClosedFormOptions off = options;
      off.tablePrograms = false;
      SliceSynthesisResult plain = synthesizeClosedForms(p, points, off, z3Backend);
      for (const SliceSynthesis &slice : plain.slices)
        for (const LoopSynthesis &l : slice.loops)
          for (const SynthesizedClosedForm &form : l.forms)
            if (form.variable == fForm->variable || form.variable == gForm->variable) {
              CHECK(form.kind != "table" && form.status == ClosedFormStatus::Unknown);
              CHECK(!form.program.has_value());
            }
    }
  }
  // SemGuS text for the table program: constructor form round trip over the skeleton,
  // and the export carries every constructor the program uses with a rule each; it
  // has NOT been read by any external tool, and the header says so.
  {
    std::string text = semgusTerm(*fForm->program);
    CHECK(text.find("(write T0 ") != std::string::npos && text.find("(read T0 ") != std::string::npos);
    std::string why;
    auto back = parseSemgusTerm(text, *fForm->program, why);
    CHECK(back.has_value());
    if (back) {
      CHECK(semgusTerm(*back) == text);
      CHECK(formatIndexProgram(*back) == formatIndexProgram(*fForm->program));
      for (uint64_t mv = 0; mv < 6; ++mv) {
        std::vector<APInt> inputs;
        for (Term leaf : back->inputs)
          inputs.push_back(leaf == loop->spec.iteration ? APInt(8, mv) : APInt::getZero(leaf->sort.width));
        IndexRun a = runIndexProgram(*back, inputs), b = runIndexProgram(*fForm->program, inputs);
        CHECK(a.status == b.status && a.value == b.value);
      }
    }
    SemgusProblem q;
    q.width = 8;
    for (unsigned n = 0; n < fForm->program->inputs.size(); ++n)
      q.inputNames.push_back("x" + std::to_string(n));
    q.locals = 1;
    q.constants = {APInt(8, 0), APInt(8, 1), APInt(8, 2)};
    SemgusArray T;
    T.rank = 1;
    T.extents = {"(bvadd x0 (_ bv2 8))"};
    q.arrays.push_back(T);
    q.candidate = &*fForm->program;
    // The inductive specification (G.3) of f, as the two constraints, from the slice.
    auto leafName = [&](Term leaf) { return closedFormLeafName(p, loop->spec, leaf); };
    unsigned kf = std::find(loop->spec.carried.begin(), loop->spec.carried.end(), fForm->variable) - loop->spec.carried.begin();
    auto condition = semgusTermText(loop->spec.condition, leafName), step = semgusTermText(loop->spec.step[kf], leafName);
    CHECK(condition.has_value() && step.has_value());
    if (condition && step) {
      q.declaredVariables = {"m (_ BitVec 8)", "n (_ BitVec 8)"};
      q.constraints = {"(P.Sem f (_ bv0 8) (_ bv0 8))", "(=> (and (P.Sem f m n) " + *condition + ") (P.Sem f (bvadd m (_ bv1 8)) " + *step + "))"};
    }
    std::string exported = exportSemgusProblem(q);
    CHECK(exported.find("NOT read by any external SemGuS parser") != std::string::npos);
    CHECK(exported.find("($read_T0 E)") != std::string::npos && exported.find("($write_T0 E E)") != std::string::npos);
    CHECK(exported.find("(select W0 k1)") != std::string::npos && exported.find("(store T0 k1 n)") != std::string::npos);
    CHECK(exported.find("(bvult k1 (bvadd x0 (_ bv2 8)))") != std::string::npos);
    CHECK(exported.find("(declare-var m (_ BitVec 8))") != std::string::npos);
    CHECK(semgusExportMissing(*fForm->program, exported).empty());
    std::string wf;
    CHECK(semgusExportWellFormed(exported, wf));
    // A program the export's grammar does not have: a second array.
    IndexProgram two = *fForm->program;
    two.array(8, 8, {two.inputExpr(0)});
    two.result = two.read(1, {two.inputExpr(0)});
    auto missing = semgusExportMissing(two, exported);
    CHECK(missing.size() == 1 && missing[0].find("$read_T1") == 0);
  }
}

/// Phase 4 (I1): iteration counters as ghost state. The instrumented program runs
/// as the original on every point, the counters count, the search takes the
/// counter over the loop variable of equal kernel, and the entry definitions of
/// rotated give the identity two_n = 2n.
static void testCounters() {
  Module m(programs);
  SemanticProgram original = m.import("stepped", 8);
  SemanticProgram p = original;
  CounterInstrumentation added = addIterationCounters(p);
  CHECK(added.reason.empty() && added.counters.size() == 2);
  CHECK(p.state.size() == original.state.size() + 2 && p.iterationCounters.size() == 2);
  for (const LoopCounter &c : added.counters) {
    CHECK(p.state[c.state].kind == StateVar::IterationCounter && p.state[c.state].location == c.header);
    CHECK(p.state[c.state].loopCarried && p.state[c.state].sort == Sort::bv(8));
    CHECK(llvm::is_contained(c.body, c.header));
  }
  // Nothing reads a counter.
  for (const Transition &t : p.transitions) {
    for (const auto &[id, term] : t.assignments)
      if (!llvm::is_contained(p.iterationCounters, id))
        for (unsigned r : stateSupport(term))
          CHECK(!llvm::is_contained(p.iterationCounters, r));
    for (unsigned r : stateSupport(t.guard))
      CHECK(!llvm::is_contained(p.iterationCounters, r));
  }
  // Twice: refused.
  SemanticProgram again = p;
  CHECK(!addIterationCounters(again).reason.empty() && again.state.size() == p.state.size());
  // Replay: same transitions, events and returns at every point; the counters count
  // the header arrivals, which the replay recomputes from the transitions taken.
  uint64_t arrivals = 0;
  for (int64_t n = 8; n <= 40; n += 3) {
    EnumerationLimits limits;
    auto states = eventStates(original, {APInt(8, n)}, 0, limits);
    CHECK(states.has_value());
    if (!states)
      continue;
    CounterReplay replay = compareRuns(original, p, added, states->entry, RunLimits());
    CHECK(replay.equal);
    CHECK(replay.events == states->run.events.size());
    arrivals += replay.headerArrivals;
    // The inner counter at each event equals the iteration index of the inner loop,
    // read back from the instrumented run's state.
    auto instrumented = eventStates(p, {APInt(8, n)}, 0, limits);
    CHECK(instrumented.has_value());
    if (!instrumented)
      continue;
    unsigned inner = added.counters.back().state, i = 1; // i is state 1 (the inner block argument).
    for (size_t e = 0; e < instrumented->run.events.size(); ++e) {
      const auto &state = instrumented->states[e];
      CHECK(state[inner].has_value() && state[i].has_value());
      if (!state[inner] || !state[i])
        continue;
      uint64_t mv = state[inner]->getZExtValue();
      uint64_t expected = 5 * mv + 2 * (mv % 2); // The proved closed form of i.
      CHECK(state[i]->getZExtValue() == expected);
    }
  }
  CHECK(arrivals > 0);
  // A forged instrumentation (a counter that is bumped from outside its loop) is
  // caught by the replay.
  {
    SemanticProgram forged = original;
    CounterInstrumentation wrong = addIterationCounters(forged);
    unsigned inner = wrong.counters.back().state;
    TermContext &terms = *forged.terms;
    for (Transition &t : forged.transitions)
      for (auto &[id, term] : t.assignments)
        if (id == inner && term->op == TermOp::Const)
          term = terms.bv(APInt(8, 1)); // Enter at 1 instead of 0.
    EnumerationLimits limits;
    auto states = eventStates(original, {APInt(8, 9)}, 0, limits);
    CHECK(states.has_value());
    if (states) {
      CounterReplay replay = compareRuns(original, forged, wrong, states->entry, RunLimits());
      CHECK(!replay.equal && replay.reason.find("does not count") != std::string::npos);
    }
  }
  // The search: the inner counter has the kernel of i and takes its place as K.
  {
    ParameterScope scope = range(p, 8, 40);
    auto proof = certify(original, scope);
    CHECK(proof.has_value());
    if (proof) {
      SynthesisOptions options = synthesisOptions(scope, 0);
      options.synthesis.maxCandidates = 8;
      SynthesisResult plain = synthesizeProgram(original, &*proof, options, z3Backend);
      SynthesisResult counted = synthesizeProgram(p, &*proof, options, z3Backend);
      CHECK(plain.groups.size() == 1 && counted.groups.size() == 1);
      Term inner = p.terms->state(added.counters.back().state, Sort::bv(8));
      Term i = p.terms->state(1, Sort::bv(8));
      bool plainK = false, countedK = false, countedI = false;
      for (const VerifiedCandidate &c : plain.groups[0].candidates)
        plainK = plainK || llvm::is_contained(c.family.axes[AxisK], i);
      for (const VerifiedCandidate &c : counted.groups[0].candidates) {
        countedK = countedK || llvm::is_contained(c.family.axes[AxisK], inner);
        for (const auto &axis : c.family.axes)
          countedI = countedI || llvm::is_contained(axis, i);
        CHECK(c.finite.status() == FamilyStatus::Verified);
        // The symbolic coordinate tier still has no bound for the counter: the number
        // of iterations is not a parameter plus a shift. Not proved, not refuted.
        CHECK(c.symbolic.tier() != EvidenceTier::SymbolicExact);
      }
      CHECK(plainK && countedK && !countedI);
    }
  }
  // Entry definitions (I2): rotated's 2n bound, computed once before the loops, and
  // its materialized constants; nothing a loop assigns.
  {
    SemanticProgram r = m.import("rotated", 8);
    std::string why;
    auto defined = entryDefinedState(r, SliceLimits(), why);
    CHECK(why.empty() && !defined.empty());
    bool twoN = false;
    for (const auto &[id, term] : defined) {
      CHECK(stateSupport(term).empty());
      if (term->op == TermOp::Mul)
        twoN = true;
      for (const Transition &t : r.transitions)
        for (const auto &[assigned, value] : t.assignments)
          if (assigned == id)
            CHECK(t.from == r.entryLocation); // Only the entry block defines it.
    }
    CHECK(twoN);
  }
}

/// Full at some point, as the driver's preference reads it.
static bool fullSomewhereForTest(const VerifiedCandidate &candidate) {
  for (const PointFacts &point : candidate.finite.points())
    if (point.report.structurallyFull)
      return true;
  return false;
}

/// Appendix G.2, one check per rule, at bit-vector widths; the lowering against
/// the interpreter; the SemGuS-style text against itself.
static void testIndexPrograms() {
  TermContext terms;
  const unsigned w = 32;
  Term tIn = terms.param(0, Sort::bv(w));
  auto run = [&](const IndexProgram &p, std::vector<APInt> inputs, uint64_t fuel = 1u << 16) {
    IndexRunLimits limits;
    limits.fuel = fuel;
    return runIndexProgram(p, inputs, limits);
  };
  auto value = [&](const IndexProgram &p, int64_t t) {
    IndexRun r = run(p, {APInt(w, t)});
    CHECK(r.status == IndexRunStatus::Returned);
    return r.status == IndexRunStatus::Returned ? r.value.getZExtValue() : UINT64_MAX;
  };

  // skip ; return x: locals start at 0, the input is read, nothing else moves.
  {
    IndexProgram p;
    unsigned x = p.input(tIn);
    unsigned v = p.local(w);
    p.body = p.skip();
    p.result = p.add(p.inputExpr(x), p.localExpr(v));
    CHECK(value(p, 7) == 7);
    IndexRun r = run(p, {APInt(w, 7)});
    CHECK(r.finalLocals.size() == 1 && r.finalLocals[0].isZero());
    CHECK(formatIndexProgram(p) == "skip ; return (x0 + v0)");
  }
  // v := e ; sequence ; both arms of if ; negation, products, div and mod, wrap.
  {
    IndexProgram p;
    unsigned x = p.input(tIn);
    unsigned v = p.local(w), u = p.local(w);
    unsigned c3 = p.constant(APInt(w, 3));
    unsigned s1 = p.assign(v, p.mul(p.inputExpr(x), c3));                   // v = 3x
    unsigned s2 = p.assign(u, p.sub(p.localExpr(v), p.inputExpr(x)));       // u = 2x
    unsigned cond = p.ult(p.localExpr(u), p.constant(APInt(w, 10)));        // 2x < 10
    unsigned s3 = p.ifThenElse(cond, p.assign(v, p.modConst(p.localExpr(u), APInt(w, 3))),
                               p.assign(v, p.divConst(p.localExpr(u), APInt(w, 3))));
    p.body = p.seq(p.seq(s1, s2), s3);
    p.result = p.neg(p.localExpr(v));
    CHECK(value(p, 4) == (uint64_t)(uint32_t)(-(8 % 3)));   // then arm: 8 mod 3 = 2
    CHECK(value(p, 6) == (uint64_t)(uint32_t)(-(12 / 3)));  // else arm: 12 div 3 = 4
    // Wrapping is the value semantics: 3 * 0x60000000 wraps at 32 bits.
    IndexRun r = run(p, {APInt(w, 0x60000000)});
    CHECK(r.status == IndexRunStatus::Returned && r.finalLocals[1] == APInt(w, 0xC0000000u));
    auto lowered = indexProgramTerm(terms, p);
    CHECK(lowered.has_value());
    SemanticProgram empty;
    for (int64_t t : {0, 4, 5, 6, 100}) {
      EntryState entry;
      entry.parameters = {APInt(w, t)};
      auto evaluated = evaluateTerm(empty, entry, Configuration(), *lowered);
      CHECK(evaluated && evaluated->getZExtValue() == value(p, t));
    }
  }
  // Listing G.1: r := 0; a := 1; while r < t do (a := a * (r + 2); r := r + 1); return a.
  // Table G.1 at 32 bits, then the wrap that Section G.4 locates at t = 12.
  {
    IndexProgram p;
    unsigned x = p.input(tIn);
    unsigned r = p.local(w), a = p.local(w);
    unsigned init = p.seq(p.assign(r, p.constant(APInt(w, 0))), p.assign(a, p.constant(APInt(w, 1))));
    unsigned body = p.seq(p.assign(a, p.mul(p.localExpr(a), p.add(p.localExpr(r), p.constant(APInt(w, 2))))),
                          p.assign(r, p.add(p.localExpr(r), p.constant(APInt(w, 1)))));
    p.body = p.seq(init, p.whileDo(p.ult(p.localExpr(r), p.inputExpr(x)), body));
    p.result = p.localExpr(a);
    CHECK(!p.loopFree() && !indexProgramTerm(terms, p).has_value());
    uint64_t expected[] = {1, 2, 6, 24, 120};
    for (unsigned t = 0; t < 5; ++t)
      CHECK(value(p, t) == expected[t]);
    IndexRun four = run(p, {APInt(w, 4)});
    CHECK(four.status == IndexRunStatus::Returned && four.finalLocals[0] == APInt(w, 4));
    // 12! fits: the bit-vector run equals the integer; 13! does not, and the run
    // holds 13! mod 2^32, which the integer reading does not. Neither is a claim
    // about the thesis's K(t): the adaptation compares at the width, nothing else.
    uint64_t factorial = 1;
    for (uint64_t k = 2; k <= 12; ++k)
      factorial *= k;
    CHECK(value(p, 11) == factorial);
    factorial *= 13;
    CHECK(value(p, 12) == (factorial & 0xFFFFFFFFu) && value(p, 12) != factorial);
    // Fuel: a cut run has no value and says nothing about termination.
    IndexRun cut = run(p, {APInt(w, 1000)}, 50);
    CHECK(cut.status == IndexRunStatus::OutOfFuel && cut.steps == 50);
    CHECK(std::string(stringifyIndexRunStatus(cut.status)) == "OUT_OF_FUEL");
    IndexProgram forever;
    forever.input(tIn);
    unsigned z = forever.local(w);
    forever.body = forever.whileDo(forever.ule(forever.localExpr(z), forever.localExpr(z)), forever.skip());
    forever.result = forever.localExpr(z);
    CHECK(run(forever, {APInt(w, 0)}, 1000).status == IndexRunStatus::OutOfFuel);
    CHECK(p.size() > 10 && forever.size() == 6); // while, skip; ule; three reads of v0
  }
  // Ill-sorted nodes are refused as TermContext refuses them.
  {
    IndexProgram p;
    unsigned x = p.input(tIn);
    unsigned narrow = p.constant(APInt(8, 1));
    CHECK_THROWS(p.add(p.inputExpr(x), narrow));
    CHECK_THROWS(p.divConst(p.inputExpr(x), APInt(w, 0)));
    CHECK_THROWS(p.modConst(p.inputExpr(x), APInt(8, 2)));
    CHECK_THROWS(p.assign(p.local(8), p.inputExpr(x)));
    CHECK_THROWS(p.input(terms.boolean(true)));
    CHECK_THROWS(runIndexProgram(p, {APInt(8, 1)}));
  }
  // The lowering agrees with the interpreter on random loop-free programs.
  {
    uint64_t seed = 12345;
    auto next = [&] { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed >> 33; };
    Term y = terms.param(1, Sort::bv(8)), z = terms.param(2, Sort::bv(8));
    unsigned agreements = 0;
    for (unsigned trial = 0; trial < 300; ++trial) {
      IndexProgram p;
      unsigned x0 = p.input(y), x1 = p.input(z);
      unsigned v0 = p.local(8), v1 = p.local(8);
      std::function<unsigned(unsigned)> expr = [&](unsigned depth) -> unsigned {
        switch (depth == 0 ? next() % 3 : next() % 9) {
        case 0: return p.inputExpr(next() % 2 ? x0 : x1);
        case 1: return p.localExpr(next() % 2 ? v0 : v1);
        case 2: return p.constant(APInt(8, next() % 7));
        case 3: return p.add(expr(depth - 1), expr(depth - 1));
        case 4: return p.sub(expr(depth - 1), expr(depth - 1));
        case 5: return p.neg(expr(depth - 1));
        case 6: return p.mul(expr(depth - 1), expr(depth - 1));
        case 7: return p.divConst(expr(depth - 1), APInt(8, 1 + next() % 5));
        default: return p.modConst(expr(depth - 1), APInt(8, 1 + next() % 5));
        }
      };
      std::function<unsigned(unsigned)> cond = [&](unsigned depth) -> unsigned {
        switch (depth == 0 ? next() % 3 : next() % 6) {
        case 0: return p.eq(expr(1), expr(1));
        case 1: return p.ult(expr(1), expr(1));
        case 2: return p.ule(expr(1), expr(1));
        case 3: return p.lnot(cond(depth - 1));
        case 4: return p.land(cond(depth - 1), cond(depth - 1));
        default: return p.lor(cond(depth - 1), cond(depth - 1));
        }
      };
      std::function<unsigned(unsigned)> stmt = [&](unsigned depth) -> unsigned {
        switch (depth == 0 ? next() % 2 : next() % 4) {
        case 0: return p.skip();
        case 1: return p.assign(next() % 2 ? v0 : v1, expr(2));
        case 2: return p.seq(stmt(depth - 1), stmt(depth - 1));
        default: return p.ifThenElse(cond(1), stmt(depth - 1), stmt(depth - 1));
        }
      };
      p.body = stmt(3);
      p.result = expr(2);
      auto lowered = indexProgramTerm(terms, p);
      CHECK(lowered.has_value());
      if (!lowered)
        continue;
      SemanticProgram empty;
      for (unsigned sample = 0; sample < 8; ++sample) {
        APInt a(8, next() % 256), b(8, next() % 256);
        EntryState entry;
        entry.parameters = {APInt(w, 0), a, b};
        auto evaluated = evaluateTerm(empty, entry, Configuration(), *lowered);
        IndexRun r = run(p, {a, b});
        bool same = evaluated && r.status == IndexRunStatus::Returned && *evaluated == r.value;
        CHECK(same);
        agreements += same;
      }
      // Constructor form round trip: the parsed program runs the same.
      std::string why;
      auto parsed = parseSemgusTerm(semgusTerm(p), p.inputs, p.localWidths, why);
      CHECK(parsed.has_value());
      if (parsed) {
        CHECK(semgusTerm(*parsed) == semgusTerm(p));
        APInt a(8, next() % 256), b(8, next() % 256);
        IndexRun r1 = run(p, {a, b}), r2 = run(*parsed, {a, b});
        CHECK(r1.status == r2.status && r1.value == r2.value);
      }
    }
    CHECK(agreements == 300 * 8);
  }
  // The SemGuS-style text: well formed by its own check, labelled as unchecked by
  // any external tool, and the reader refuses what it does not know.
  {
    IndexProgram p;
    unsigned x = p.input(tIn);
    unsigned v = p.local(w);
    p.body = p.ifThenElse(p.eq(p.inputExpr(x), p.constant(APInt(w, 0))),
                          p.assign(v, p.add(p.inputExpr(x), p.constant(APInt(w, 1)))),
                          p.assign(v, p.add(p.inputExpr(x), p.constant(APInt(w, 2)))));
    p.result = p.localExpr(v);
    CHECK(formatIndexProgram(p) == "if x0 = 0 then (v0 := (x0 + 1)) else (v0 := (x0 + 2)) ; return v0");
    CHECK(value(p, 0) == 1 && value(p, 5) == 7);
    SemgusProblem q;
    q.width = w;
    q.inputNames = {"t"};
    q.constants = {APInt(w, 0), APInt(w, 1), APInt(w, 2)};
    q.divisors = {APInt(w, 2)};
    q.examples = {{{APInt(w, 0)}, APInt(w, 1)}, {{APInt(w, 5)}, APInt(w, 7)}};
    q.candidate = &p;
    std::string text = exportSemgusProblem(q), why;
    CHECK(semgusExportWellFormed(text, why));
    CHECK(text.find("NOT read by any external SemGuS parser") != std::string::npos);
    CHECK(text.find("(declare-term-types") != std::string::npos);
    CHECK(text.find("(define-funs-rec") != std::string::npos);
    CHECK(text.find("($while B S)") != std::string::npos);
    CHECK(text.find("(synth-fun f () P)") != std::string::npos);
    CHECK(text.find("(constraint (P.Sem f (_ bv5 32) (_ bv7 32)))") != std::string::npos);
    CHECK(text.find(semgusTerm(p)) != std::string::npos);
    CHECK(!semgusExportWellFormed(text + "(check-sat)", why) && why.find("check-sat") != std::string::npos);
    CHECK(!semgusExportWellFormed("(constraint (f", why));
    CHECK(!semgusExportWellFormed("(constraint true)", why) && why == "no check-synth");
    for (const char *bad : {"(return skip (shl x0 x0))", "(return skip x9)", "(return (assign x0 v0) v0)",
                            "(return skip (div x0 (_ bv0 32)))", "(return skip (add x0 (_ bv1 8)))",
                            "(return skip", "(return skip v0) v0", "(return (while (eq x0 x0)) v0)"})
      CHECK(!parseSemgusTerm(bad, p.inputs, p.localWidths, why).has_value() && !why.empty());
  }
}

/// The statement productions in the search: off by default and then absent, on by
/// option, and then every candidate still goes through the same checkers.
static void testStatementGrammar() {
  for (unsigned level = 0; level <= GrammarSpec::MaxLevel; ++level)
    CHECK(!GrammarSpec::atLevel(level).statements);
  Module m(programs);
  SemanticProgram p = m.import("rotated", 8);
  ParameterScope scope = range(p, 2, 3);
  auto proof = certify(p, scope);
  CHECK(proof.has_value());
  if (!proof)
    return;

  // The bank: no program without the option; with it, the branch on t < n with the
  // two phases' subscripts as arms, lowered to an ite, text kept with the atom.
  DomainEnumeration enumeration = liveDomains(p, *proof, scope);
  CHECK(enumeration.gate == GateStatus::Open);
  auto domains = domainsOf(enumeration);
  GrammarSpec plain = GrammarSpec::atLevel(0);
  AtomBank plainBank = buildAtoms(p, enumeration.groups[0], domains, plain);
  CHECK(plainBank.programs == 0);
  for (const Atom &atom : plainBank.atoms)
    CHECK(atom.program.empty());
  GrammarSpec withStatements = plain;
  withStatements.statements = true;
  withStatements.maxAtoms = 400;
  withStatements.maxOperands = 4000;
  AtomBank bank = buildAtoms(p, enumeration.groups[0], domains, withStatements);
  CHECK(bank.programs > 0 && bank.atoms.size() > plainBank.atoms.size());
  bool branchOnPhase = false;
  for (const Atom &atom : bank.atoms) {
    CHECK(atom.program.empty() == (atom.term->op != TermOp::Ite));
    if (atom.term->op == TermOp::Ite && atom.program.find("if ") == 0 &&
        atom.program.find(" then (v0 := ") != std::string::npos &&
        atom.program.find(" ; return v0") != std::string::npos)
      branchOnPhase = true;
    if (getenv("GG_DEBUG_ATOMS") && !atom.program.empty())
      fprintf(stderr, "atom size %u: %s\n", atom.size, atom.program.c_str());
  }
  CHECK(branchOnPhase);
  // Every program atom means what its program means, at every event.
  for (const Atom &atom : bank.atoms) {
    if (atom.program.empty())
      continue;
    std::string why;
    for (unsigned d = 0; d < domains.size(); ++d) {
      EventEvaluator evaluator(p, *domains[d]);
      for (size_t e = 0; e < evaluator.size(); ++e) {
        auto v = evaluator.value(atom.term, e);
        CHECK(v && v->getZExtValue() == atom.values[d][e]);
      }
    }
  }

  // The search. Without the option the expression grammar has no full family for
  // this program within its levels; with it, the checkers accept one whose K is
  // the branch, at the finite tier, and the record names the program.
  SynthesisOptions off = synthesisOptions(scope, 3);
  SynthesisResult without = synthesizeProgram(p, &*proof, off, z3Backend);
  CHECK(without.groups.size() == 1);
  bool fullWithout = false;
  for (const VerifiedCandidate &c : without.groups[0].candidates)
    fullWithout = fullWithout || fullSomewhereForTest(c);
  CHECK(!fullWithout);
  SynthesisOptions on = off;
  on.statementGrammar = true;
  SynthesisResult with = synthesizeProgram(p, &*proof, on, z3Backend);
  CHECK(with.groups.size() == 1);
  bool fullWith = false, branchK = false;
  for (const VerifiedCandidate &c : with.groups[0].candidates) {
    CHECK(c.finite.status() == FamilyStatus::Verified);
    CHECK(c.finite.tier() == EvidenceTier::ScopeEnumerated);
    if (!fullSomewhereForTest(c))
      continue;
    fullWith = true;
    CHECK(c.family.origin == "selector search over statement programs");
    CHECK(c.family.programs[AxisK].size() == c.family.axes[AxisK].size());
    for (unsigned n = 0; n < c.family.axes[AxisK].size(); ++n)
      if (c.family.axes[AxisK][n]->op == TermOp::Ite && !c.family.programs[AxisK][n].empty())
        branchK = true;
    if (getenv("GG_DEBUG_ATOMS"))
      for (unsigned axis = 0; axis < 4; ++axis)
        for (const std::string &text : c.family.programs[axis])
          fprintf(stderr, "axis %u program: %s\n", axis, text.c_str());
  }
  CHECK(fullWith && branchK);
  std::string json = toJSON(p, with, false);
  CHECK(json.find("\"index_programs\"") != std::string::npos);
  CHECK(json.find("if ") != std::string::npos);
  CHECK(toJSON(p, without, false).find("index_programs") == std::string::npos);

  // The same checkers judge the lowered term: a family with the program's term
  // in K passes checkFamilyFinite, and one with the arms swapped at one phase fails.
  for (const VerifiedCandidate &c : with.groups[0].candidates) {
    if (!fullSomewhereForTest(c))
      continue;
    CheckedFamily again = checkFamilyFinite(p, with.enumeration, 0, c.family);
    CHECK(again.status() == FamilyStatus::Verified);
    CoordinateFamily forged = c.family;
    for (Term &term : forged.axes[AxisK])
      if (term->op == TermOp::Ite)
        term = p.terms->ite(term->args[0], term->args[2], term->args[1]);
    CheckedFamily forgedCheck = checkFamilyFinite(p, with.enumeration, 0, forged);
    CHECK(forgedCheck.status() != FamilyStatus::Verified);
    break;
  }

  // Two shifted phases: the expression grammar needs level 2 (t div n, t mod n), the
  // statement grammar has the branch at level 0, and the family it returns reaches
  // the symbolic tier through the unchanged checker.
  SemanticProgram two = m.import("two_phase", 8);
  ParameterScope twoScope = range(two, 2, 3);
  auto twoProof = certify(two, twoScope);
  CHECK(twoProof.has_value());
  if (twoProof) {
    SynthesisResult plainRun = synthesizeProgram(two, &*twoProof, off, z3Backend);
    SynthesisOptions twoOn = off;
    twoOn.statementGrammar = true;
    SynthesisResult stmtRun = synthesizeProgram(two, &*twoProof, twoOn, z3Backend);
    CHECK(plainRun.groups.size() == 1 && stmtRun.groups.size() == 1);
    CHECK(plainRun.groups[0].status == RecoveryStatus::CandidateFound && plainRun.groups[0].level == 2);
    CHECK(stmtRun.groups[0].status == RecoveryStatus::CandidateFound && stmtRun.groups[0].level == 0);
    bool exact = false;
    for (const VerifiedCandidate &c : stmtRun.groups[0].candidates) {
      CHECK(c.finite.status() == FamilyStatus::Verified);
      if (c.symbolic.tier() == EvidenceTier::SymbolicExact &&
          c.symbolic.status() == FamilyStatus::Verified && fullSomewhereForTest(c)) {
        exact = true;
        CHECK(c.family.origin == "selector search over statement programs");
        bool kProgram = false;
        for (const std::string &text : c.family.programs[AxisK])
          kProgram = kProgram || text.find("if ") == 0;
        CHECK(kProgram);
      }
    }
    CHECK(exact);
  }

  // A program where the expression grammar already has a coordinate: the option
  // adds atoms and changes no verdict.
  SemanticProgram rev = m.import("reversed", 8);
  ParameterScope revScope = range(rev, 2, 3);
  auto revProof = certify(rev, revScope);
  CHECK(revProof.has_value());
  if (revProof) {
    SynthesisResult a = synthesizeProgram(rev, &*revProof, off, z3Backend);
    SynthesisOptions revOn = off;
    revOn.statementGrammar = true;
    SynthesisResult b = synthesizeProgram(rev, &*revProof, revOn, z3Backend);
    CHECK(a.groups.size() == 1 && b.groups.size() == 1);
    CHECK(a.groups[0].status == RecoveryStatus::CandidateFound);
    CHECK(b.groups[0].status == RecoveryStatus::CandidateFound);
    for (const VerifiedCandidate &c : b.groups[0].candidates)
      CHECK(c.finite.status() == FamilyStatus::Verified);
  }
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

int main(int argc, char **argv) {
  mlir::MLIRContext local;
  configure(local);
  context = &local;
  // An argument names the only tests to run, for a quick turn; CTest passes none.
  if (argc > 1) {
    for (int n = 1; n < argc; ++n) {
      std::string name = argv[n];
      if (name == "index") testIndexPrograms();
      else if (name == "statements") testStatementGrammar();
      else if (name == "slices") testSliceSpecification();
      else if (name == "while") testWhilePrograms();
      else if (name == "counters") testCounters();
      else if (name == "arrays") testArrays();
      else if (name == "tables") testTables();
      else if (name == "grammar") testGrammar();
      else if (name == "search") testSearch();
      else if (name == "clauses") testClauses();
      else { fprintf(stderr, "unknown test %s\n", argv[n]); return 2; }
    }
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
  }
  testGrammar();
  testIndexPrograms();
  testStatementGrammar();
  testSliceSpecification();
  testWhilePrograms();
  testCounters();
  testArrays();
  testTables();
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
