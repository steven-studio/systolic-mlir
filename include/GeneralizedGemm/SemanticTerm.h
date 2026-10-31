//===- SemanticTerm.h - Exact bitvector term language -----------*- C++ -*-===//
//
// An immutable, hash-consed term DAG. Integers are bitvectors of an explicit
// width (invariant I1): there is no mathematical integer sort anywhere in this
// language. Wrapping is the value semantics of every integer node; nsw, nuw,
// exact and nneg are preserved on the node and their poison conditions are
// separate terms (invariant I2).
//
// Division, remainder and shifts are total, with the SMT-LIB definitions, so
// the concrete evaluator and the relational encoding agree on every input. The
// undefined cases of the source operations are side conditions in the program
// model, never hidden in these definitions.
//
// Booleans are BV1, as `i1` is in MLIR. Mem is the sort of one root's contents.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SEMANTICTERM_H
#define GENERALIZEDGEMM_SEMANTICTERM_H

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct Sort {
  enum Kind { BV, Float, Mem };
  Kind kind = BV;
  // BV: the bit width. Float: total bits, 32 or 64. Mem: the element's bits.
  unsigned width = 0;
  // Mem only.
  unsigned root = 0, rank = 0, indexWidth = 0;
  bool elementIsFloat = false;

  static Sort bv(unsigned width);
  static Sort flt(unsigned width);
  static Sort mem(unsigned root, unsigned rank, unsigned indexWidth,
                  Sort element);
  Sort element() const;
  bool operator==(const Sort &o) const {
    return kind == o.kind && width == o.width && root == o.root &&
           rank == o.rank && indexWidth == o.indexWidth &&
           elementIsFloat == o.elementIsFloat;
  }
  bool operator!=(const Sort &o) const { return !(*this == o); }
  std::string str() const;
};

enum class TermOp {
  Const, FConst,
  Param, State, Shadow, MemVar,
  Add, Sub, Mul, SDiv, UDiv, SRem, URem, Shl, LShr, AShr,
  And, Or, Xor, Not,
  ICmp, Ite, ZExt, SExt, Trunc,
  Load, Store,
  FAdd, FMul, FIsNaN, FIsInf, FIsZero,
};

enum class Predicate { Eq, Ne, Slt, Sle, Sgt, Sge, Ult, Ule, Ugt, Uge };

/// Flags preserved on integer nodes.
enum IntegerFlags : unsigned { FlagNSW = 1, FlagNUW = 2, FlagExact = 4,
                               FlagNNeg = 8 };
/// Flags preserved on floating-point nodes.
enum FloatFlags : unsigned { FlagReassoc = 1, FlagNNaN = 2, FlagNInf = 4,
                             FlagNSZ = 8, FlagARCFP = 16, FlagContract = 32,
                             FlagAFN = 64 };

struct TermNode;
using Term = const TermNode *;

struct TermNode {
  TermOp op;
  Sort sort;
  unsigned id;        // Creation order; children always have smaller ids.
  unsigned aux;       // Flags, or the Predicate of an ICmp.
  unsigned index;     // Param/State/Shadow number.
  llvm::APInt value;  // Const/FConst bit pattern.
  llvm::SmallVector<Term, 3> args;
};

/// Thrown by the builders for an ill-sorted term, and when the term limit is
/// exhausted. The importer turns it into a fail-closed result.
struct TermError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

class TermContext {
public:
  explicit TermContext(unsigned maxTerms = 1u << 20) : limit(maxTerms) {}

  Term bv(const llvm::APInt &value);
  Term boolean(bool value);
  Term fconst(const llvm::APInt &bits);
  Term param(unsigned id, Sort sort);
  Term state(unsigned id, Sort sort);
  Term shadow(unsigned id);
  Term memVar(Sort sort);

  Term binary(TermOp op, Term a, Term b, unsigned flags = 0);
  Term logicalAnd(Term a, Term b);
  Term logicalOr(Term a, Term b);
  Term logicalNot(Term a);
  Term icmp(Predicate predicate, Term a, Term b);
  Term ite(Term condition, Term a, Term b);
  Term zext(Term a, unsigned width);
  Term sext(Term a, unsigned width);
  Term trunc(Term a, unsigned width);
  Term load(Term memory, llvm::ArrayRef<Term> index);
  Term store(Term memory, llvm::ArrayRef<Term> index, Term value);
  Term fbinary(TermOp op, Term a, Term b, unsigned flags = 0);
  Term fclass(TermOp op, Term a);

  /// `node` with new children. The sort-checked builders run again.
  Term rebuild(Term node, llvm::ArrayRef<Term> args);

  /// Replaces leaves bottom-up; `leaf` returns nullptr to keep a leaf.
  Term substitute(Term root, const std::function<Term(Term)> &leaf);

  size_t size() const { return nodes.size(); }

private:
  Term intern(TermOp op, Sort sort, unsigned aux, unsigned index,
              const llvm::APInt &value, llvm::ArrayRef<Term> args);
  unsigned limit;
  std::vector<std::unique_ptr<TermNode>> nodes;
  llvm::StringMap<Term> table;
};

/// Deterministic text of a node in terms of its children's ids. No addresses.
std::string describeNode(Term term);
/// Deterministic expanded text, for small terms and tests.
std::string formatTerm(Term term, unsigned depthLimit = 16);
bool containsOp(Term term, TermOp op);
/// Ids of the State leaves of `term`, sorted, each once.
llvm::SmallVector<unsigned> stateSupport(Term term);

/// The SMT-LIB total definitions of the integer operations. Shared by every
/// evaluator so the value semantics has one definition.
llvm::APInt evalIntegerBinary(TermOp op, const llvm::APInt &a, const llvm::APInt &b);
bool evalPredicate(Predicate predicate, const llvm::APInt &a,
                   const llvm::APInt &b);
/// IEEE-754 binary32/binary64, round to nearest ties to even, as bit patterns.
llvm::APInt evalFloatBinary(TermOp op, const llvm::APInt &a, const llvm::APInt &b);
bool evalFloatClass(TermOp op, const llvm::APInt &a);

} // namespace mlir::generalized_gemm
#endif
