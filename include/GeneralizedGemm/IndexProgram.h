//===- IndexProgram.h - Statement-level index programs ----------*- C++ -*-===//
//
// The statement grammar G_S of the thesis (Section 4.4, Appendix G.1), as a
// bit-vector adaptation: every variable has an exact width, arithmetic wraps, and
// division and remainder take a positive constant divisor, so no expression is
// undefined. A program reads its inputs, which are terms over the state and the
// parameters of a SemanticProgram, assigns locals that start at zero (the σ_t
// convention of Appendix G.2), and returns one expression.
//
//   P ::= S ; return E
//   S ::= skip | v := E | S ; S | if B then S else S | while B do S | a[E]...[E] := E
//   E ::= x | v | K | E + E | E - E | -E | E * E | E div C | E mod C | a[E]...[E]
//   B ::= E = E | E < E | E <= E | not B | B and B | B or B     (unsigned)
//
// Phase 5 (Appendix G.5): arrays. A program declares its arrays with a rank and
// one extent expression per dimension over the inputs; a read or a write of an
// array carries exactly its rank of indices (the builders refuse any other
// number, so a partially indexed array is no term). The state carries every
// array as a partial map from index tuples to values, defined exactly on the
// elements written (or given, for an input array). A read of an index outside
// [0, extent) in any dimension, or of an element without a value, has no rule:
// the run has no output (IndexRunStatus::NoRule), which is neither a value nor a
// statement about termination. A write changes that element and nothing else.
// Arrays are distinct objects: no two names alias, by construction.
//
// A program is a proposal and never evidence. Its meaning is the rule-by-rule
// interpreter below, one case per constrained Horn clause of Appendix G.2; the
// relation is a partial function, and a run that exhausts its fuel has no value,
// which is recorded as such and is no statement about termination. A loop-free
// program lowers to a Term of the same meaning, which is what every checker
// consumes; the lowering is tested against the interpreter and is not trusted
// otherwise.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_INDEXPROGRAM_H
#define GENERALIZEDGEMM_INDEXPROGRAM_H

#include "GeneralizedGemm/SemanticTerm.h"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

enum class IndexExprKind { Input, Local, Const, Add, Sub, Neg, Mul, DivConst, ModConst, Read };
enum class IndexCondKind { Eq, Ult, Ule, Not, And, Or };
enum class IndexStmtKind { Skip, Assign, Seq, If, While, Write };

/// The contents of one array: its extents and the elements that have a value.
struct IndexTable {
  std::vector<uint64_t> extents;
  std::map<std::vector<uint64_t>, llvm::APInt> elements;
};

/// Nodes are owned by the program and refer to each other by index; a child
/// always has a smaller index than its parent.
struct IndexProgram {
  struct Expr {
    IndexExprKind kind;
    unsigned width;
    unsigned index = 0;     // Input or local number; Read: the array.
    llvm::APInt value;      // Const: the value. DivConst, ModConst: the divisor.
    unsigned a = 0, b = 0;  // Child expressions.
    std::vector<unsigned> indices; // Read: one expression per dimension.
  };
  struct Cond {
    IndexCondKind kind;
    unsigned a = 0, b = 0;  // Eq, Ult, Ule: expressions. Not, And, Or: conditions.
  };
  struct Stmt {
    IndexStmtKind kind;
    unsigned local = 0, expr = 0, cond = 0; // Write: local is the array, expr the value.
    unsigned a = 0, b = 0;  // Child statements.
    std::vector<unsigned> indices; // Write: one expression per dimension.
  };
  /// An array of the program (Appendix G.5): element width, index width, one
  /// extent expression per dimension over the inputs and constants alone (a
  /// local in an extent is refused), evaluated once when a run starts. An input
  /// array takes its elements from the run's input tables; any other array
  /// starts with no element.
  struct Array {
    unsigned width = 0, indexWidth = 0;
    std::vector<unsigned> extents;
    bool input = false;
  };

  std::vector<Term> inputs;          // Read only. Width from the term.
  std::vector<unsigned> localWidths; // Every local starts at zero.
  std::vector<Array> arrays;
  std::vector<Expr> exprs;
  std::vector<Cond> conds;
  std::vector<Stmt> stmts;
  unsigned body = 0;   // The statement S of P.
  unsigned result = 0; // The expression E of return E.

  // Builders. Each checks widths and throws TermError on an ill-sorted node, as
  // TermContext does. Divisors must be nonzero.
  unsigned input(Term term);
  unsigned local(unsigned width);
  unsigned constant(const llvm::APInt &value);
  unsigned inputExpr(unsigned input);
  unsigned localExpr(unsigned local);
  unsigned add(unsigned a, unsigned b);
  unsigned sub(unsigned a, unsigned b);
  unsigned neg(unsigned a);
  unsigned mul(unsigned a, unsigned b);
  unsigned divConst(unsigned a, const llvm::APInt &divisor);
  unsigned modConst(unsigned a, const llvm::APInt &divisor);
  unsigned eq(unsigned a, unsigned b);
  unsigned ult(unsigned a, unsigned b);
  unsigned ule(unsigned a, unsigned b);
  unsigned lnot(unsigned a);
  unsigned land(unsigned a, unsigned b);
  unsigned lor(unsigned a, unsigned b);
  unsigned skip();
  unsigned assign(unsigned local, unsigned expr);
  unsigned seq(unsigned a, unsigned b);
  unsigned ifThenElse(unsigned cond, unsigned a, unsigned b);
  unsigned whileDo(unsigned cond, unsigned a);
  // Phase 5. Declares an array; `extents` are expressions over inputs and
  // constants, one per dimension (the rank). A read or a write takes exactly
  // rank indices of the array's index width, or is refused.
  unsigned array(unsigned width, unsigned indexWidth, llvm::ArrayRef<unsigned> extents,
                 bool input = false);
  unsigned read(unsigned array, llvm::ArrayRef<unsigned> indices);
  unsigned write(unsigned array, llvm::ArrayRef<unsigned> indices, unsigned value);

  unsigned exprWidth(unsigned expr) const { return exprs.at(expr).width; }
  /// Whether `body` has no while statement.
  bool loopFree() const;
  /// Whether any reachable node reads or writes an array.
  bool usesArrays() const;
  /// Statements, conditions and expressions reachable from body and result.
  unsigned size() const;
};

struct IndexRunLimits {
  uint64_t fuel = 1u << 16; // Statements executed, loop tests included.
};

/// NoRule (phase 5): no rule of G.5 applies at some step: an index outside the
/// extents, a read of an element without a value, an input table of the wrong
/// shape. The run has no output; `reason` says which. Like OutOfFuel it is not a
/// value; unlike OutOfFuel it is definite: the same inputs give the same stop.
enum class IndexRunStatus { Returned, OutOfFuel, NoRule };
const char *stringifyIndexRunStatus(IndexRunStatus status);

struct IndexRun {
  IndexRunStatus status = IndexRunStatus::OutOfFuel;
  llvm::APInt value;       // Only when Returned.
  uint64_t steps = 0;
  std::vector<llvm::APInt> finalLocals; // Only when Returned.
  std::vector<IndexTable> finalArrays;  // Only when Returned: the exposed final state (G.5).
  std::string reason;                   // NoRule: which rule failed, with the indices.
  uint64_t writes = 0;                  // Array writes performed.
};

/// The rules of Appendix G.2 and G.5 at bit-vector widths. `inputs` is parallel
/// to program.inputs; `inputTables`, when given, is parallel to program.arrays
/// and supplies the elements of each input array (ignored for the others).
/// OutOfFuel means the run was cut: no value, and nothing is claimed about
/// whether the program terminates. NoRule means the semantics has no output.
IndexRun runIndexProgram(const IndexProgram &program, llvm::ArrayRef<llvm::APInt> inputs,
                         const IndexRunLimits &limits = {},
                         llvm::ArrayRef<IndexTable> inputTables = {});

/// A term with the program's meaning at every input, for a loop-free program
/// without arrays; nullopt otherwise. Assignments substitute, a branch becomes
/// ite per local it changes, and a sequence composes.
std::optional<Term> indexProgramTerm(TermContext &terms, const IndexProgram &program);

/// Phase 5: what a lowering reads an array element as. Given the array and the
/// index terms, the term standing for that element, or nullptr to refuse.
using ReadLowering = std::function<Term(unsigned array, llvm::ArrayRef<Term> indices)>;

/// Phase 4. The value of local `local` after the loop-free statement `stmt` runs
/// from an environment that gives every local a term (`locals`, parallel to
/// localWidths); nullopt when the statement has a while, writes an array, or
/// reads one without a `reads` lowering. Same rules as above.
std::optional<Term> indexStatementTerm(TermContext &terms, const IndexProgram &program,
                                       unsigned stmt, llvm::ArrayRef<Term> locals,
                                       unsigned local, const ReadLowering &reads = {});

/// The term of expression `expr` in the environment `locals`; nullptr when it
/// reads an array and `reads` refuses or is absent.
Term indexExprTerm(TermContext &terms, const IndexProgram &program, unsigned expr,
                   llvm::ArrayRef<Term> locals, const ReadLowering &reads = {});

/// Phase 4. The one while shape this implementation searches and proves, the
/// fixed-counter template (Appendix G.1's P_K is one):
///
///   r := 0 ; v := init ; while r < m do (S ; r := r + 1) ; return v
///
/// with r and v distinct locals, init over inputs alone, m an input, and S a
/// loop-free statement that assigns v and no other local and does not read m.
/// Those conditions are what the coupling lemma of the record needs (the first
/// m iterations of the run on m + 1 are the run on m). This is NOT the general
/// while of G.1: a program outside the template is matched by nothing here. A
/// program that reads or writes an array is not this template either (see
/// TableTemplate).
struct CounterTemplate {
  unsigned counter = 0, value = 0; // Locals r and v.
  unsigned init = 0;               // Expression.
  unsigned bound = 0;              // Input index of m.
  unsigned body = 0;               // Statement S.
};
std::optional<CounterTemplate> matchCounterTemplate(const IndexProgram &program, std::string &why);

/// Phase 5. The one table shape this implementation searches and proves, the
/// history table template, for one array T of rank 1 with extent m + d:
///
///   T[0] := k_0 ; ... ; T[d-1] := k_{d-1} ;
///   r := d ; while r < m + 1 do (T[r] := E ; r := r + 1) ; return T[m]
///
/// with d ≥ 1 init writes of expressions over inputs alone, m an input (the
/// iteration count), and E an expression over r, the inputs, constants and the
/// reads T[r - 1], ..., T[r - d] (no other array read, no local but r). The
/// loop writes T[d], ..., T[m] in order; every read is of an element written
/// before. This is NOT the general array grammar of G.5: a second array, a rank
/// above one, a write at another index, a nest, or a read elsewhere is neither
/// searched for nor proved, and the matcher says so.
struct TableTemplate {
  unsigned array = 0;              // T.
  unsigned counter = 0;            // Local r.
  unsigned bound = 0;              // Input index of m.
  std::vector<unsigned> inits;     // Expressions k_0, ..., k_{d-1}.
  unsigned value = 0;              // Expression E.
  unsigned depth() const { return inits.size(); }
};
std::optional<TableTemplate> matchTableTemplate(const IndexProgram &program, std::string &why);

/// Deterministic text in the grammar above: locals v0, v1, ...; inputs x0, x1,
/// ... or the text `inputName` gives for the input term.
std::string formatIndexProgram(const IndexProgram &program,
                               const std::function<std::string(Term)> &inputName = {});

} // namespace mlir::generalized_gemm
#endif
