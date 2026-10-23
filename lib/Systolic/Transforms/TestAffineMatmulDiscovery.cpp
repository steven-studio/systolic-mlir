#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/AffineMatmulPartitioning.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicTilePlan.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include "mlir/Analysis/Presburger/PresburgerRelation.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"

#include <random>

using namespace mlir;
using namespace mlir::systolic;

namespace {

static void printLogicalExtent(StringRef name,
                               const LogicalExtent &extent) {
  llvm::errs() << "  " << name << " extent: ";

  switch (extent.kind) {
  case LogicalExtent::Kind::Unknown:
    llvm::errs() << "unknown";
    break;

  case LogicalExtent::Kind::Static:
    llvm::errs() << "static " << extent.staticValue;
    break;

  case LogicalExtent::Kind::Symbolic:
    llvm::errs() << "symbolic " << extent.symbolicValue;
    break;
  }

  llvm::errs() << "\n";
}

struct TestAffineMatmulDiscoveryPass
    : public PassWrapper<TestAffineMatmulDiscoveryPass,
                         OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      TestAffineMatmulDiscoveryPass)

  StringRef getArgument() const final {
    return "test-affine-matmul-discovery";
  }

  StringRef getDescription() const final {
    return "Test discovery of candidate affine matmul loop nests";
  }

  TestAffineMatmulDiscoveryPass() = default;
  TestAffineMatmulDiscoveryPass(const TestAffineMatmulDiscoveryPass &other)
      : PassWrapper(other) {}

  // Regions to hand partitionLogicalMatmul directly, six integers each:
  // lower I, J, K, then extent I, J, K. Exercises the exactness proof on
  // partitions no plan would produce (overlaps, gaps, out of the domain).
  ListOption<int64_t> regions{
      *this, "regions",
      llvm::cl::desc("Regions to verify as a partition: lower I,J,K, "
                     "extent I,J,K per region")};

  // Deliberately wrong logical mappings for every verified GEMM: the
  // bijection certificate must refuse each (a test that it is not vacuous,
  // docs/AFFINE_GEMM_MAPPING.md section 19.13.4).
  Option<bool> perturbCertificate{
      *this, "perturb-certificate",
      llvm::cl::desc("After a verified logical bijection, check it again on "
                     "wrong mappings (M + 1, M - 1, I's scale doubled, loop "
                     "0's origin + 1, I's row of T zero), each of which the "
                     "certificate must refuse"),
      llvm::cl::init(false)};

  Option<bool> geometry{
      *this, "geometry",
      llvm::cl::desc("Print the geometric screening of every candidate "
                     "(docs/AFFINE_GEMM_MAPPING.md section 19: necessary "
                     "conditions only, not a decision)"),
      llvm::cl::init(false)};

  Option<bool> reductionBatches{
      *this, "reduction-batches",
      llvm::cl::desc("Recognize with reduction-batch partitioning "
                     "(docs/AFFINE_GEMM_MAPPING.md section 23; on by default, "
                     "as in --affine-matmul-to-systolic)"),
      llvm::cl::init(true)};

  Option<bool> r8{
      *this, "r8",
      llvm::cl::desc("Print the R8 structure of every candidate: dot "
                     "products and ranks on the counters as written, the "
                     "loop classes and Rule E (docs/AFFINE_GEMM_MAPPING.md "
                     "section 22: a diagnostic, not a decision)"),
      llvm::cl::init(false)};

  // The exact set inclusion of docs section 19.14 on built-in sets -- the
  // LLVM 24 counterexample among them -- and on random sets, each answer
  // checked point by point (containsPoint) over a box that holds both sets.
  Option<bool> exactSets{
      *this, "exact-sets",
      llvm::cl::desc("Run the exact set-inclusion self test (docs section "
                     "19.14) instead of the discovery"),
      llvm::cl::init(false)};
  Option<unsigned> exactSetsRandom{
      *this, "exact-sets-random",
      llvm::cl::desc("With exact-sets: also this many random pairs of sets"),
      llvm::cl::init(0)};
  Option<unsigned> exactSetsSeed{
      *this, "exact-sets-seed",
      llvm::cl::desc("With exact-sets-random: the generator's seed"),
      llvm::cl::init(1)};

  // The tiling legality (docs section 26.4) is decided as
  // --affine-matmul-to-systolic decides it with the same option.
  Option<bool> assumeNoAlias{
      *this, "assume-no-alias",
      llvm::cl::desc("Decide the tiling legality (docs section 26.4) as "
                     "--affine-matmul-to-systolic does with this option"),
      llvm::cl::init(false)};

  /// Whether a bound of the perfect band from `anchor` reads the counter of a
  /// loop around it: the band is then a GEMM at each value of that loop (the
  /// point band of a tiling), not the GEMM of the whole nest.
  static bool readsEnclosingCounter(Operation *anchor) {
    auto top = dyn_cast<affine::AffineForOp>(anchor);
    if (!top)
      return false;
    SmallVector<affine::AffineForOp> band;
    affine::getPerfectlyNestedLoops(band, top);
    for (affine::AffineForOp loop : band)
      for (ValueRange operands : {ValueRange(loop.getLowerBoundOperands()),
                                  ValueRange(loop.getUpperBoundOperands())})
        for (Value operand : operands)
          if (affine::AffineForOp owner =
                  affine::getForInductionVarOwner(operand))
            if (owner->isProperAncestor(anchor))
              return true;
    return false;
  }

  static void printVector(const lattice::IntVector &v) {
    llvm::errs() << "[";
    llvm::interleaveComma(v, llvm::errs());
    llvm::errs() << "]";
  }

  static void printMatrix(const lattice::IntMatrix &m) {
    llvm::errs() << "[";
    llvm::interleaveComma(m, llvm::errs(),
                          [](const lattice::IntVector &v) { printVector(v); });
    llvm::errs() << "]";
  }

  static void printGeometry(AffineMatmulCandidate candidate) {
    std::string why;
    FailureOr<AffineGeometryScreening> g =
        screenAffineMatmulGeometry(candidate, &why);
    llvm::errs() << "geometry screening (necessary conditions only, not a "
                    "decision):\n";
    if (failed(g)) {
      llvm::errs() << "  not screened: " << why << "\n";
      return;
    }
    static const char *const kOperand[3] = {"A", "B", "C"};
    for (auto [s, loop] : llvm::enumerate(g->loops)) {
      llvm::errs() << "  loop " << s << ": trip " << loop.trip;
      if (loop.trip < 2) {
        llvm::errs() << ", one iteration (a constant)\n";
        continue;
      }
      llvm::errs() << ", readers";
      for (unsigned x = 0; x < 3; ++x)
        if (loop.readers & (1u << x))
          llvm::errs() << " " << kOperand[x];
      llvm::errs() << " -> ";
      switch (loop.role) {
      case 'I':
        llvm::errs() << "I";
        break;
      case 'J':
        llvm::errs() << "J";
        break;
      case 'K':
        llvm::errs() << "K (reduction)";
        break;
      case 'P':
        llvm::errs() << "batch";
        break;
      case '1':
        llvm::errs() << "one reader (a broadcast)";
        break;
      default:
        llvm::errs() << "no reader";
        break;
      }
      llvm::errs() << "\n";
    }
    for (unsigned x = 0; x < 3; ++x) {
      llvm::errs() << "  " << kOperand[x] << " ordinal rows: ";
      printMatrix(g->ordinal[x]);
      llvm::errs() << "\n";
    }
    llvm::errs() << "  rank of C: " << g->cRankFull << " (role space "
                 << g->cRankRoles << "); rank of all subscripts: "
                 << g->allRankFull << " (role space " << g->allRankRoles
                 << ")\n";
    llvm::errs() << "  C kernel: ";
    printMatrix(g->cKernel);
    llvm::errs() << "\n  reduction loops:";
    if (g->reductionLoops.empty())
      llvm::errs() << " none";
    for (unsigned s : g->reductionLoops)
      llvm::errs() << " " << s;
    llvm::errs() << "\n";
    for (const lattice::IntVector &v : g->cKernelBeyondReduction) {
      llvm::errs() << "  C kernel direction ";
      printVector(v);
      llvm::errs() << " joins two points of the box and is not a reduction: "
                      "two iterations write one C element\n";
    }
    auto printConditions =
        [](StringRef space,
           const std::optional<AffineGeometryScreening::SixVectorConditions>
               &c) {
          if (!c)
            return;
          auto yn = [](bool b) { return b ? "yes" : "no"; };
          llvm::errs() << "  six-vector conditions (" << space
                       << "): A " << yn(c->a) << ", B " << yn(c->b)
                       << ", C " << yn(c->c);
          if (c->reductionDirection) {
            llvm::errs() << " (d = ";
            printVector(*c->reductionDirection);
            llvm::errs() << ")";
          }
          llvm::errs() << ", D " << yn(c->d1) << " " << yn(c->d2) << " "
                       << yn(c->d3) << "\n";
          // The task algorithm of 2026-10-18, line by line (section 19.10).
          llvm::errs() << "  task algorithm (" << space
                       << "): rank(Ic,Jc)=2 " << yn(c->a)
                       << ", rank(Ia,Ka)=2 " << yn(c->d1)
                       << ", rank(Kb,Jb)=2 " << yn(c->d2)
                       << ", rank(six)=3 " << yn(c->b)
                       << ", rank(Ia,Ka,Kb,Jb)=3 " << yn(c->e)
                       << ", A varies along null(Ic,Jc) " << yn(c->aVaries)
                       << ", B varies along null(Ic,Jc) " << yn(c->bVaries)
                       << ", swap(Ia,Ka) " << yn(c->swapA)
                       << ", swap(Kb,Jb) " << yn(c->swapB) << " -> "
                       << (c->passes ? "passes" : "error") << "\n";
        };
    printConditions("full space", g->conditionsFull);
    printConditions("role space", g->conditionsRoles);
  }

  // The loop tiling the nest writes (docs section 26, S5.1): recorded, not a
  // decision.
  static void printLoopTiling(const LoopTilingStructure &tiling) {
    using Role = LoopTilingStructure::LoopRole;
    using Boundary = LoopTilingStructure::Boundary;
    static const char *const kCoordinate[] = {"I", "J", "K"};
    static const char *const kSize[] = {"TM", "TN", "TK"};
    static const char *const kDimension[] = {"M", "N", "K"};
    auto name = [](int d) -> StringRef { return d >= 0 ? kCoordinate[d] : "?"; };
    llvm::errs() << "  loop tiling (as written; recorded, not a decision): "
                 << tiling.tilings.size() << " tiled coordinate(s)\n";
    llvm::errs() << "    loop order:";
    for (auto [s, role] : llvm::enumerate(tiling.roles)) {
      const LoopTilingStructure::Tiling *of = nullptr;
      for (const LoopTilingStructure::Tiling &one : tiling.tilings)
        if (one.tileLoop == s || one.pointLoop == s)
          of = &one;
      const int coordinate = s < tiling.loopCoordinate.size()
                                 ? tiling.loopCoordinate[s]
                                 : -1;
      llvm::errs() << " " << s << "=";
      if (role == Role::Tile)
        llvm::errs() << "tile(" << name(of->coordinate) << ")";
      else if (role == Role::Point)
        llvm::errs() << "point(" << name(of->coordinate) << ")";
      else
        llvm::errs() << "untiled(" << name(coordinate) << ")";
    }
    llvm::errs() << "\n";
    SmallVector<StringRef, 3> tiled;
    bool reduction = false;
    for (const LoopTilingStructure::Tiling &one : tiling.tilings) {
      const int d = one.coordinate;
      llvm::errs() << "    " << name(d) << ": tile loop " << one.tileLoop
                   << ", point loop " << one.pointLoop << ", "
                   << (d >= 0 ? kSize[d] : "T?") << " = " << one.tileSize;
      if (one.form == LoopTilingStructure::Form::Counter)
        llvm::errs() << ", counter form (" << name(d) << " = "
                     << one.tileSize << " * loop " << one.tileLoop
                     << " + loop " << one.pointLoop << ")";
      else
        llvm::errs() << ", bounds form";
      llvm::errs() << ", boundary "
                   << (one.boundary == Boundary::Divisible   ? "divisible"
                       : one.boundary == Boundary::MinClipped ? "min-clipped"
                                                              : "unaligned")
                   << " (last tile " << one.lastTile << "), " << name(d)
                   << " in [" << one.lower << ", " << one.upper << ")\n";
      if (d == 0 || d == 1)
        tiled.push_back(kDimension[d]);
      if (d == 2)
        reduction = true;
    }
    llvm::sort(tiled);
    llvm::errs() << "    tiled dimensions: ";
    if (tiled.empty())
      llvm::errs() << "none";
    llvm::interleave(tiled, llvm::errs(), " ");
    llvm::errs() << "; reduction tiling: " << (reduction ? "K" : "none")
                 << "\n";
    for (const std::string &why : tiling.rejected)
      llvm::errs() << "    not a tiling: " << why << "\n";
    // The exact coverage (section 26.2).
    const LoopTilingStructure::Coverage &coverage = tiling.coverage;
    if (!coverage.decided) {
      llvm::errs() << "    coverage: not decided (" << coverage.note << ")\n";
      return;
    }
    auto space = [&](llvm::raw_ostream &os) {
      for (auto [j, dim] : llvm::enumerate(coverage.dims)) {
        os << (j ? " x " : "");
        if (dim.coordinate >= 0)
          os << kCoordinate[dim.coordinate];
        else
          os << "loop " << dim.loop;
        os << " [" << dim.lower << ", " << dim.upper << ")";
        if (dim.step != 1)
          os << " step " << dim.step;
      }
    };
    if (coverage.exact) {
      llvm::errs() << "    coverage (exact, Presburger): every point of ";
      space(llvm::errs());
      llvm::errs() << " is run by exactly one iteration, none outside it\n";
      // M x N x K when each coordinate is one dimension of step 1.
      int64_t extent[3] = {0, 0, 0};
      unsigned count[3] = {0, 0, 0};
      bool dense = true;
      for (const LoopTilingStructure::Coverage::Dim &dim : coverage.dims) {
        if (dim.coordinate < 0 || dim.step != 1) {
          dense = false;
          continue;
        }
        extent[dim.coordinate] = dim.upper - dim.lower;
        ++count[dim.coordinate];
      }
      if (dense && count[0] == 1 && count[1] == 1 && count[2] == 1)
        llvm::errs() << "    logical iteration space: M x N x K = "
                     << extent[0] << " x " << extent[1] << " x " << extent[2]
                     << "\n";
    } else {
      llvm::errs() << "    coverage (exact, Presburger): NOT an exact cover "
                      "of ";
      space(llvm::errs());
      llvm::errs() << "\n";
      if (coverage.overlap)
        llvm::errs() << "      overlap: " << *coverage.overlap << "\n";
      if (coverage.missing)
        llvm::errs() << "      missing: " << *coverage.missing << "\n";
      if (coverage.outside)
        llvm::errs() << "      outside: " << *coverage.outside << "\n";
    }
    for (const LoopTilingStructure::Tiling &one : tiling.tilings)
      if (!one.boundaryProof.empty())
        llvm::errs() << "    boundary " << name(one.coordinate) << ": "
                     << one.boundaryProof << "\n";
  }

  static void printR8(AffineMatmulCandidate candidate,
                      ArrayRef<DomainNormalization> normalized) {
    llvm::errs() << "R8 structure (diagnostic, not a decision):\n";
    // R8 is defined on the counters as written; a normalized nest's
    // counters are x' (x = U x'), another basis.
    if (countersRewritten(normalized, candidate.anchor)) {
      llvm::errs() << "  not analysed: the domain normalization rewrote "
                      "the counters of this nest (x = U x'), they are not "
                      "the counters as written\n";
      return;
    }
    std::string why;
    FailureOr<AffineGeometryScreening> g =
        screenAffineMatmulGeometry(candidate, &why);
    if (failed(g)) {
      llvm::errs() << "  not analysed: " << why << "\n";
      return;
    }
    SmallVector<int64_t> trips;
    for (const AffineGeometryScreening::Loop &loop : g->loops)
      trips.push_back(loop.trip);
    std::optional<AffineR8Structure> r = computeR8Structure(g->written, trips);
    if (!r) {
      llvm::errs() << "  not analysed: overflow\n";
      return;
    }
    // A and B are the product's first and second factor, C the accumulator
    // (not the memrefs' names).
    static const char *const kOperand[3] = {"A", "B", "C"};
    for (unsigned x = 0; x < 3; ++x) {
      llvm::errs() << "  " << kOperand[x] << " rows: ";
      printMatrix(g->written[x]);
      llvm::errs() << "\n";
    }
    for (auto [s, readers] : llvm::enumerate(r->readers)) {
      llvm::errs() << "  loop " << s << ":";
      if (trips[s] < 2) {
        llvm::errs() << " one iteration (a constant)\n";
        continue;
      }
      if (readers == 0) {
        llvm::errs() << " used by no access\n";
        continue;
      }
      llvm::errs() << " readers";
      for (unsigned x = 0; x < 3; ++x)
        if (readers & (1u << x))
          llvm::errs() << " " << kOperand[x];
      llvm::errs() << " -> ";
      switch (readers) {
      case 0b111:
        llvm::errs() << "OB (batch)";
        break;
      case 0b101:
        llvm::errs() << "M or OB";
        break;
      case 0b110:
        llvm::errs() << "N or OB";
        break;
      case 0b100:
        llvm::errs() << "OB (C only)";
        break;
      case 0b011:
        llvm::errs() << "K or RB (contraction block)";
        break;
      case 0b001:
      case 0b010:
        llvm::errs() << "RB (one factor only)";
        break;
      default:
        break;
      }
      llvm::errs() << "\n";
    }
    llvm::errs() << "  reduction loops Z:";
    if (r->reductionLoops.empty())
      llvm::errs() << " none";
    for (unsigned s : r->reductionLoops)
      llvm::errs() << " " << s;
    llvm::errs() << "; shared loops S:";
    if (r->contractionLoops.empty())
      llvm::errs() << " none";
    for (unsigned s : r->contractionLoops)
      llvm::errs() << " " << s;
    llvm::errs() << "; groups:";
    for (const SmallVector<unsigned> &group : r->contractionGroups) {
      llvm::errs() << " {";
      llvm::interleave(group, llvm::errs(), " ");
      llvm::errs() << "}";
    }
    llvm::errs() << "\n";
    llvm::errs() << "  on Z: rank A " << r->rankFirst << ", rank B "
                 << r->rankSecond << ", rank [A; B] " << r->rankStacked
                 << " (row spaces meet in dimension "
                 << r->rankFirst + r->rankSecond - r->rankStacked
                 << "), coupling rank(A B^T) " << r->coupling << "\n";
    llvm::errs() << "  batch block coupling: " << r->batchCoupling << "\n";
    llvm::errs() << "  literal, loops of two or more iterations: rank(A B^T) " << r->literalAB
                 << ", rank(A C^T) " << r->literalAC << ", rank(B C^T) "
                 << r->literalBC << "; nonzero A.B pairs " << r->pairsAB
                 << " (not a dimension)\n";
    if (r->literalAB != r->coupling + r->batchCoupling)
      llvm::errs() << "  literal and restricted readings differ: dot "
                      "products of different loop classes cancel or add "
                      "(section 22.4)\n";
    llvm::errs() << "  operand ranks: A " << r->operandRank[0] << ", B "
                 << r->operandRank[1] << ", C " << r->operandRank[2];
    for (unsigned rank : {1u, 0u}) {
      bool first = true;
      for (unsigned x = 0; x < 3; ++x) {
        if (r->operandRank[x] != rank)
          continue;
        if (first)
          llvm::errs() << (rank == 1 ? "; one-dimensional (Phase 2):"
                                     : "; constant (rank 0):");
        llvm::errs() << " " << kOperand[x];
        first = false;
      }
    }
    llvm::errs() << "\n";
    auto yn = [](bool b) { return b ? "yes" : "no"; };
    llvm::errs() << "  intersect(A,B) (some a.b != 0 on Z): "
                 << yn(r->intersect)
                 << "; shared loop variables (some sum |a_t||b_t| > 0): "
                 << yn(r->sharedLoops) << "\n";
  }

  // Membership of a point of the variables, the locals searched exactly
  // (containsPointNoLocal: an integer feasibility test, conjunctive).
  static bool member(const presburger::IntegerRelation &set,
                     ArrayRef<llvm::DynamicAPInt> point) {
    return set.containsPointNoLocal(point).has_value();
  }

  // Brute force over the box [-R, R]^v: a ⊆ b iff every point of a in the
  // box is in b. The sets used here lie inside the box.
  static bool pointwiseSubset(const presburger::IntegerRelation &a,
                              const presburger::IntegerRelation &b,
                              unsigned vars, int64_t radius) {
    SmallVector<int64_t> point(vars, -radius);
    while (true) {
      SmallVector<llvm::DynamicAPInt> p;
      for (int64_t x : point)
        p.push_back(llvm::DynamicAPInt(x));
      if (member(a, p) && !member(b, p))
        return false;
      unsigned i = 0;
      while (i < vars && point[i] == radius)
        point[i++] = -radius;
      if (i == vars)
        return true;
      ++point[i];
    }
  }

  static const char *answer(std::optional<bool> r) {
    return !r ? "undecided" : *r ? "yes" : "no";
  }

  void runExactSetSelfTest() {
    using presburger::IntegerPolyhedron;
    using presburger::PresburgerSet;
    using presburger::PresburgerSpace;
    unsigned failures = 0;
    auto report = [&](StringRef name, const IntegerPolyhedron &a,
                      const IntegerPolyhedron &b, int64_t radius) {
      SmallVector<llvm::DynamicAPInt> witness;
      std::optional<bool> exact = exactIsSubset(a, b, &witness);
      const bool library = PresburgerSet(a).isSubsetOf(PresburgerSet(b));
      const unsigned vars = a.getNumDimVars();
      const bool truth = pointwiseSubset(a, b, vars, radius);
      llvm::errs() << "exact-sets " << name << ": exact " << answer(exact)
                   << ", by points " << (truth ? "yes" : "no")
                   << ", library " << (library ? "yes" : "no");
      if (exact && !*exact) {
        const bool inA = member(a, witness);
        const bool inB = member(b, witness);
        llvm::errs() << ", witness (";
        llvm::interleaveComma(witness, llvm::errs(),
                              [](const llvm::DynamicAPInt &x) {
                                llvm::errs() << int64_t(x);
                              });
        llvm::errs() << ") in a: " << (inA ? "yes" : "no")
                     << ", in b: " << (inB ? "yes" : "no");
        if (!inA || inB)
          ++failures;
      }
      if (exact && *exact != truth)
        ++failures;
      llvm::errs() << "\n";
    };

    // The LLVM 24 counterexample (docs section 19.14): Y = { (y0, y1) :
    // exists a, b, q1, q2 : a = 3 q1, 0 <= a <= 13, y1 = a, b = 4 q2,
    // 0 <= b <= 7, b <= y0 <= b + 3 } -- two equality-pinned locals -- and
    // B = [0, 7] x [0, 14]. (0, 1) is in B and not in Y.
    IntegerPolyhedron y(PresburgerSpace::getSetSpace(2, 0, 4));
    y.addEquality(SmallVector<int64_t>{0, 0, 1, 0, -3, 0, 0});
    y.addEquality(SmallVector<int64_t>{0, 0, 0, 1, 0, -4, 0});
    y.addInequality(SmallVector<int64_t>{0, 0, 1, 0, 0, 0, 0});
    y.addInequality(SmallVector<int64_t>{0, 0, -1, 0, 0, 0, 13});
    y.addInequality(SmallVector<int64_t>{0, 0, 0, 1, 0, 0, 0});
    y.addInequality(SmallVector<int64_t>{0, 0, 0, -1, 0, 0, 7});
    y.addEquality(SmallVector<int64_t>{0, 1, -1, 0, 0, 0, 0});
    y.addInequality(SmallVector<int64_t>{1, 0, 0, -1, 0, 0, 0});
    y.addInequality(SmallVector<int64_t>{-1, 0, 0, 1, 0, 0, 3});
    IntegerPolyhedron box(PresburgerSpace::getSetSpace(2, 0, 0));
    box.addInequality(SmallVector<int64_t>{1, 0, 0});
    box.addInequality(SmallVector<int64_t>{-1, 0, 7});
    box.addInequality(SmallVector<int64_t>{0, 1, 0});
    box.addInequality(SmallVector<int64_t>{0, -1, 14});
    report("box in two-stepped", box, y, 16);
    report("two-stepped in box", y, box, 16);
    // One stepped dimension (the library is right here).
    IntegerPolyhedron one(PresburgerSpace::getSetSpace(2, 0, 2));
    one.addEquality(SmallVector<int64_t>{0, 0, 1, -3, 0});
    one.addInequality(SmallVector<int64_t>{0, 0, 1, 0, 0});
    one.addInequality(SmallVector<int64_t>{0, 0, -1, 0, 13});
    one.addEquality(SmallVector<int64_t>{0, 1, -1, 0, 0});
    one.addInequality(SmallVector<int64_t>{1, 0, 0, 0, 0});
    one.addInequality(SmallVector<int64_t>{-1, 0, 0, 0, 7});
    report("box in one-stepped", box, one, 16);
    // Y written without the hidden window: y1 = 3 q, 0 <= y1 <= 13,
    // 0 <= y0 <= 7 -- the same set (both inclusions yes).
    IntegerPolyhedron same(PresburgerSpace::getSetSpace(2, 0, 1));
    same.addEquality(SmallVector<int64_t>{0, 1, -3, 0});
    same.addInequality(SmallVector<int64_t>{0, 1, 0, 0});
    same.addInequality(SmallVector<int64_t>{0, -1, 0, 13});
    same.addInequality(SmallVector<int64_t>{1, 0, 0, 0});
    same.addInequality(SmallVector<int64_t>{-1, 0, 0, 7});
    report("two-stepped in its plain form", y, same, 16);
    report("plain form in two-stepped", same, y, 16);
    // A floor division kept as a local: { x in [0, 9] : x floordiv 2 <= 2 }
    // = [0, 5].
    IntegerPolyhedron division(PresburgerSpace::getSetSpace(1, 0, 1));
    division.addInequality(SmallVector<int64_t>{1, -2, 0});      // x - 2q >= 0
    division.addInequality(SmallVector<int64_t>{-1, 2, 1});      // 2q + 1 - x >= 0
    division.addInequality(SmallVector<int64_t>{1, 0, 0});
    division.addInequality(SmallVector<int64_t>{-1, 0, 9});
    division.addInequality(SmallVector<int64_t>{0, -1, 2});      // q <= 2
    IntegerPolyhedron upTo5(PresburgerSpace::getSetSpace(1, 0, 0));
    upTo5.addInequality(SmallVector<int64_t>{1, 0});
    upTo5.addInequality(SmallVector<int64_t>{-1, 5});
    report("interval in floor-division set", upTo5, division, 12);
    report("floor-division set in interval", division, upTo5, 12);
    // An existential local that is neither pinned nor a division:
    // { x : exists t : 2t <= x <= 2t + 5, 0 <= t <= 3 }: undecided.
    IntegerPolyhedron window(PresburgerSpace::getSetSpace(1, 0, 1));
    window.addInequality(SmallVector<int64_t>{1, -2, 0});
    window.addInequality(SmallVector<int64_t>{-1, 2, 5});
    window.addInequality(SmallVector<int64_t>{0, 1, 0});
    window.addInequality(SmallVector<int64_t>{0, -1, 3});
    report("interval in existential window", upTo5, window, 12);

    // Random pairs: two dims in [-6, 6]; per set, up to three stepped
    // constraints (x_i = c + s q, or a pinned combination of both dims) and
    // random bounds, with locals pinned or floor divisions.
    std::mt19937 rng(exactSetsSeed);
    auto pick = [&](int lo, int hi) {
      return std::uniform_int_distribution<int>(lo, hi)(rng);
    };
    unsigned agree = 0, undecided = 0, libraryWrong = 0;
    for (unsigned n = 0; n < exactSetsRandom; ++n) {
      IntegerPolyhedron sets[2] = {
          IntegerPolyhedron(PresburgerSpace::getSetSpace(2, 0, 0)),
          IntegerPolyhedron(PresburgerSpace::getSetSpace(2, 0, 0))};
      for (IntegerPolyhedron &s : sets) {
        for (unsigned d = 0; d < 2; ++d) { // a box inside [-6, 6]
          int lo = pick(-6, 3), hi = pick(lo, 6);
          SmallVector<int64_t> ge(3 + s.getNumLocalVars(), 0),
              le(3 + s.getNumLocalVars(), 0);
          ge[d] = 1;
          ge.back() = -lo;
          le[d] = -1;
          le.back() = hi;
          s.addInequality(ge);
          s.addInequality(le);
        }
        const int locals = pick(0, 3);
        for (int k = 0; k < locals; ++k) {
          const unsigned q = s.appendVar(presburger::VarKind::Local);
          SmallVector<int64_t> row(s.getNumCols(), 0);
          row[0] = pick(-1, 1);
          row[1] = pick(-1, 1);
          if (row[0] == 0 && row[1] == 0)
            row[0] = 1;
          const int step = pick(2, 4);
          row.back() = -pick(0, step - 1);
          if (pick(0, 2) != 0) { // pinned: L - step q = 0
            row[q] = -step;
            s.addEquality(row);
          } else if (pick(0, 1) == 0) {
            // a hidden window (the counterexample's shape): w = step q,
            // w in [lo, hi], w <= x_d <= w + width -- w a second local.
            const unsigned w = s.appendVar(presburger::VarKind::Local);
            SmallVector<int64_t> pinW(s.getNumCols(), 0);
            pinW[w] = 1;
            pinW[q] = -step; // w - step q = 0
            s.addEquality(pinW);
            const unsigned d = pick(0, 1);
            const int width = pick(0, step);
            SmallVector<int64_t> atLeast(s.getNumCols(), 0),
                atMost(s.getNumCols(), 0), wLow(s.getNumCols(), 0),
                wHigh(s.getNumCols(), 0);
            atLeast[d] = 1; // x_d - w >= 0
            atLeast[w] = -1;
            atMost[d] = -1; // w + width - x_d >= 0
            atMost[w] = 1;
            atMost.back() = width;
            wLow[w] = 1; // w >= lo
            wLow.back() = -pick(-6, 0);
            wHigh[w] = -1; // w <= hi
            wHigh.back() = pick(0, 6);
            s.addInequality(atLeast);
            s.addInequality(atMost);
            s.addInequality(wLow);
            s.addInequality(wHigh);
          } else { // a floor division q = L floordiv step, bounded
            SmallVector<int64_t> lo = row, hi(s.getNumCols(), 0);
            lo[q] = -step;
            for (unsigned col = 0; col < s.getNumCols(); ++col)
              hi[col] = -lo[col];
            hi.back() += step - 1;
            s.addInequality(lo);
            s.addInequality(hi);
            SmallVector<int64_t> cap(s.getNumCols(), 0);
            cap[q] = pick(0, 1) ? 1 : -1;
            cap.back() = pick(-1, 2);
            s.addInequality(cap);
          }
        }
      }
      SmallVector<llvm::DynamicAPInt> witness;
      std::optional<bool> exact = exactIsSubset(sets[0], sets[1], &witness);
      const bool truth = pointwiseSubset(sets[0], sets[1], 2, 6);
      libraryWrong +=
          PresburgerSet(sets[0]).isSubsetOf(PresburgerSet(sets[1])) != truth;
      if (!exact) {
        ++undecided;
        continue;
      }
      bool good = *exact == truth;
      if (good && !*exact)
        good = member(sets[0], witness) && !member(sets[1], witness);
      if (good)
        ++agree;
      else
        ++failures;
    }
    if (exactSetsRandom)
      llvm::errs() << "exact-sets random: " << agree << " agree with the "
                   << "points, " << undecided << " undecided, "
                   << (exactSetsRandom - agree - undecided)
                   << " wrong, of " << exactSetsRandom
                   << " (the library's isSubsetOf wrong on " << libraryWrong
                   << ")\n";
    llvm::errs() << "exact-sets failures: " << failures << "\n";
  }

  void runOnOperation() override {
    if (exactSets) {
      runExactSetSelfTest();
      return;
    }
    // The domain normalization the lowering pass applies first (docs
    // sections 19.7, 19.12), reported line by line.
    // The loop tiling each nest writes, read before the normalization
    // below rewrites a band's bounds (docs section 26, S5.1).
    llvm::DenseMap<Operation *, LoopTilingStructure> writtenTiling;
    getOperation()->walk([&](affine::AffineForOp loop) {
      if (std::optional<LoopTilingStructure> tiling =
              analyzeWrittenLoopTiling(loop))
        writtenTiling.try_emplace(loop.getOperation(), std::move(*tiling));
    });
    // The untiling decisions of the lowering pass (sections 26.3, 26.4), on
    // the nests as written -- the IR it decides them on: the rewrites before
    // it act on bands with constant bounds only, which a tiled band is not.
    SmallVector<WrittenTilingDecision, 1> tilingDecisions =
        decideWrittenTilings(getOperation().getOperation(), assumeNoAlias);
    SmallVector<DomainNormalization, 1> normalized =
        normalizeParallelotopeDomains(getOperation().getOperation());
    if (!normalized.empty()) {
      llvm::errs() << "domain normalization: " << normalized.size()
                   << " nest(s)\n";
      for (const DomainNormalization &dn : normalized)
        describeDomainNormalization(dn, llvm::errs(), "  ");
    }

    SmallVector<AffineMatmulCandidate> candidates;

    collectAffineMatmulCandidates(
        getOperation().getOperation(), candidates);

    FailureOr<SystolicFleet> fleetOrFailure =
        collectSystolicFleet(getOperation());
    if (failed(fleetOrFailure)) {
      signalPassFailure();
      return;
    }
    const SystolicFleet &fleet = *fleetOrFailure;

    llvm::errs() << "candidate count: "
                 << candidates.size()
                 << "\n";

    SmallVector<LogicalMatmul, 1> logicalMatmuls;
    SmallVector<Operation *> failedOutermost;
    llvm::SmallPtrSet<Operation *, 4> claimed;

    for (AffineMatmulCandidate candidate : candidates) {
      if (geometry)
        printGeometry(candidate);
      if (r8)
        printR8(candidate, normalized);
      std::string notRecognized;
      GemmMappingClass notRecognizedClass = GemmMappingClass::ProvenInvalid;
      AffineMatmulRecognitionOptions recognition;
      recognition.countersAsWritten =
          !countersRewritten(normalized, candidate.anchor);
      recognition.reductionBatches =
          reductionBatches && recognition.countersAsWritten;
      auto result = recognizeLogicalMatmul(candidate, &notRecognized,
                                           &notRecognizedClass, recognition);

      if (failed(result)) {
        llvm::errs() << "recognition result: failure\n";
        llvm::errs() << "  reason: " << notRecognized << "\n";
        llvm::errs() << "  class: "
                     << stringifyGemmMappingClass(notRecognizedClass) << "\n";
        // A nest's outermost loop: its tiling is reported at the end unless
        // a GEMM recognized inside it claims it.
        if (!isa_and_nonnull<affine::AffineForOp>(
                candidate.anchor->getParentOp()))
          failedOutermost.push_back(candidate.anchor);
        continue;
      }

      bool duplicate = false;
      for (const LogicalMatmul &existing : logicalMatmuls) {
        if (existing.outputStore == result->outputStore) {
          duplicate = true;
          break;
        }
      }

      if (!duplicate) {
        // The tiling of the outermost band around the anchor that has one
        // (a Bounds tiling's GEMM may be recognized on its point band).
        Operation *owner = nullptr;
        for (Operation *op = candidate.anchor;
             isa_and_nonnull<affine::AffineForOp>(op); op = op->getParentOp())
          if (writtenTiling.count(op))
            owner = op;
        if (owner)
          result->tiling = writtenTiling.find(owner)->second;
        resolveLoopTiling(result->tiling, *result);
        if (result->tiling) {
          verifyLoopTilingCoverage(*result->tiling);
          claimed.insert(result->tiling->outermost);
        }
        logicalMatmuls.push_back(*result);
      }
    }

    llvm::errs() << "unique logical matmul count: "
                 << logicalMatmuls.size()
                 << "\n";

    for (const LogicalMatmul &result : logicalMatmuls) {
      llvm::errs() << "recognition result: success\n";
      // Recognition is not legality (section 26.4).
      if (readsEnclosingCounter(result.anchor))
        llvm::errs() << "  scope: the bounds of this band read the counter "
                        "of an enclosing loop: a GEMM at each value of it "
                        "(the point band of a tiling), not the GEMM of the "
                        "whole nest -- no mapping of the nest follows from "
                        "it (see its tiling legality, section 26.4)\n";

      llvm::errs() << "  row map: ";
      result.rowMap.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  column map: ";
      result.columnMap.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  reduction map: ";
      result.reductionMap.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  logical mapping:\n";
      for (unsigned row = 0;
           row < result.logicalMapping.transformation.size();
           ++row) {
        llvm::errs() << "    T[" << row << "] = [";
        const auto &coefficients =
            result.logicalMapping.transformation[row];

        for (unsigned column = 0;
             column < coefficients.size();
             ++column) {
          if (column != 0)
            llvm::errs() << " ";
          llvm::errs() << coefficients[column];
        }

        llvm::errs() << "]\n";
      }

      llvm::errs() << "    c = [";
      for (unsigned i = 0;
           i < result.logicalMapping.offset.size();
           ++i) {
        if (i != 0)
          llvm::errs() << " ";
        llvm::errs() << result.logicalMapping.offset[i];
      }
      llvm::errs() << "]\n";

      llvm::errs() << "    source origin: [";
      for (unsigned i = 0;
           i < result.logicalMapping.sourceOrigin.size();
           ++i) {
        if (i != 0)
          llvm::errs() << " ";
        llvm::errs() << result.logicalMapping.sourceOrigin[i];
      }
      llvm::errs() << "]\n";

      llvm::errs() << "    static source origin: "
                   << (result.logicalMapping.hasStaticSourceOrigin
                           ? "yes"
                           : "no")
                   << "\n";

      llvm::errs() << "    invertible: "
                   << (result.logicalMapping.isInvertible ? "yes" : "no")
                   << "\n";

      if (result.sourceDomain) {
        llvm::errs() << "  source iteration domain:\n";
        result.sourceDomain->dump();
      }

      // The task's exact mapping (section 19.12), reported, not decided.
      {
        const ExactMapping &em = result.exactMapping;
        llvm::errs() << "  exact mapping (task of 2026-10-17, over the counters of the nest as analysed): ";
        if (!em.defined) {
          llvm::errs() << em.note << "\n";
        } else {
          auto printRow = [](llvm::ArrayRef<int64_t> v) {
            llvm::errs() << "[";
            llvm::interleaveComma(v, llvm::errs());
            llvm::errs() << "]";
          };
          llvm::errs() << (em.degenerateK ? "2-D (K = 1: a degenerate reduction, "
                                            "the non-degenerate rule does not apply)"
                                          : "3-D")
                       << ", T = [";
          for (unsigned r = 0; r < em.T.size(); ++r) {
            if (r)
              llvm::errs() << ", ";
            printRow(em.T[r]);
          }
          llvm::errs() << "], d = ";
          printRow(em.d);
          if (em.singular) {
            llvm::errs() << ", det 0 (singular: rank(Ic, Jc, q) < " << em.T.size()
                         << ", the task's mapping has no inverse)\n";
          } else {
            llvm::errs() << ", det " << em.det
                         << (em.unimodular ? ", unimodular" : ", not unimodular")
                         << ", T diag(step) a signed permutation: "
                         << (em.boxImage ? "yes (the image of the box is a complete box)"
                                         : "no (the image is a sublattice, not a complete box in these coordinates)")
                         << "\n";
          }
          if (em.rowsSwapped)
            llvm::errs() << "    factor order: the recognizer takes the second "
                            "factor as the row operand (C = B' A')\n";
          if (!em.degenerateK) {
            llvm::errs() << "    v = ";
            if (em.v.empty()) {
              llvm::errs() << "none (rank(Ic, Jc) < 2 or overflow)\n";
            } else {
              printRow(em.v);
              llvm::errs() << "; Ia.v = " << em.deltas[0] << ", Ka.v = " << em.deltas[1]
                           << ", Kb.v = " << em.deltas[2] << ", Jb.v = " << em.deltas[3]
                           << "; A varies along v: " << (em.aVaries ? "yes" : "no")
                           << ", B varies along v: " << (em.bVaries ? "yes" : "no")
                           << "\n";
            }
          } else {
            llvm::errs() << "    no reduction direction: K has no loop\n";
          }
          llvm::errs() << "    rank(Cmat) " << em.rankC << ", rank(Amat) " << em.rankA
                       << ", rank(Bmat) " << em.rankB << "\n";
          auto printLambda = [&](const char *name,
                                 llvm::ArrayRef<lattice::RationalRow> rows,
                                 const char *restriction, bool holds) {
            llvm::errs() << "    lambda(" << name << ") = ";
            for (unsigned r = 0; r < rows.size(); ++r) {
              if (r)
                llvm::errs() << ", ";
              printRow(rows[r].numerators);
              llvm::errs() << "/" << rows[r].denominator;
            }
            if (rows.empty())
              llvm::errs() << "undefined";
            llvm::errs() << " -- " << restriction << ": " << (holds ? "yes" : "no")
                         << "\n";
          };
          printLambda("A", em.lambdaA, "A free of J", em.aFreeOfJ);
          printLambda("B", em.lambdaB, "B free of I", em.bFreeOfI);
        }
      }

      llvm::errs() << "  phiA: ";
      result.phiA.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  phiB: ";
      result.phiB.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  phiC: ";
      result.phiC.print(llvm::errs());
      llvm::errs() << "\n";

      // The run-time (symbolic) offsets of the accesses, if any.
      auto printSymbolic = [](const char *name, AffineMap m, ArrayRef<Value> syms) {
        llvm::errs() << "  symbolic offsets " << name << ": ";
        if (!m) {
          llvm::errs() << "none\n";
          return;
        }
        m.print(llvm::errs());
        llvm::errs() << " over " << syms.size() << " symbol(s)\n";
      };
      printSymbolic("A", result.symbolicA, result.symbolsA);
      printSymbolic("B", result.symbolicB, result.symbolsB);
      printSymbolic("C", result.symbolicC, result.symbolsC);

      // Each operand's own view of its coordinates: subscript = scale *
      // coordinate + offset (1 * I + 0 when the subscript is the coordinate).
      auto printView = [](StringRef operand, StringRef c0, StringRef c1,
                          const std::array<OperandCoordinateView, 2> &view) {
        llvm::errs() << "  view " << operand << ": [" << view[0].scale << "*"
                     << c0 << " + " << view[0].offset << ", " << view[1].scale
                     << "*" << c1 << " + " << view[1].offset << "]\n";
      };
      printView("A", "I", "K", result.viewA);
      printView("B", "K", "J", result.viewB);
      printView("C", "I", "J", result.viewC);
      // Where the roles sit in each memref, when not the canonical rank-2
      // (rows, columns): a transposed operand, a constant (extent-one)
      // dimension, or no dimension for a role of extent one.
      auto printLayout = [](StringRef operand, const OperandLayout &layout) {
        bool canonical = layout.roleDim[0] == 0 && layout.roleDim[1] == 1 &&
                         layout.constantIndex.size() == 2;
        if (canonical)
          return;
        if (layout.general) {
          // A skewed or merged view (docs section 19): every dimension's
          // index as constant + coefficients * the operand's two roles.
          static const char *const kRoles[3][2] = {
              {"I", "K"}, {"K", "J"}, {"I", "J"}};
          const unsigned which = operand == "A" ? 0 : operand == "B" ? 1 : 2;
          llvm::errs() << "  layout " << operand << ": general view";
          for (auto [dim, c] : llvm::enumerate(layout.roleCoefficients)) {
            llvm::errs() << (dim ? "; " : ": ") << "dimension " << dim
                         << " = " << layout.dimConstants[dim];
            for (unsigned r = 0; r < 2; ++r)
              if (c[r] != 0)
                llvm::errs() << " + " << c[r] << "*" << kRoles[which][r];
          }
          llvm::errs() << "\n";
          return;
        }
        llvm::errs() << "  layout " << operand << ": roles in dimensions ("
                     << layout.roleDim[0] << ", " << layout.roleDim[1] << ")"
                     << (layout.isTransposed() ? " transposed" : "");
        for (auto [dim, index] : llvm::enumerate(layout.constantIndex))
          if (index && !layout.hasBatchTerms(dim))
            llvm::errs() << ", dimension " << dim << " fixed at " << *index;
        llvm::errs() << "\n";
      };
      printLayout("A", result.layoutA);
      printLayout("B", result.layoutB);
      printLayout("C", result.layoutC);
      // What each dimension adds per batch value (ISSUE-007): a batch
      // dimension's index "= static + terms", a role dimension's view
      // "+= terms"; x_s is the counter of loop s.
      auto printBatchTerms = [](StringRef operand,
                                const OperandLayout &layout) {
        SmallVector<std::string> parts;
        for (unsigned dim = 0; dim < layout.batchCoefficients.size(); ++dim) {
          if (!layout.hasBatchTerms(dim))
            continue;
          std::string text;
          llvm::raw_string_ostream os(text);
          os << "dimension " << dim;
          const bool role =
              int(dim) == layout.roleDim[0] || int(dim) == layout.roleDim[1];
          if (role)
            os << " +=";
          else
            os << " = " << layout.constantIndex[dim].value_or(0) << " +";
          bool first = true;
          for (auto [s, coefficient] :
               llvm::enumerate(layout.batchCoefficients[dim])) {
            if (coefficient == 0)
              continue;
            os << (first ? " " : " + ") << coefficient << "*x" << s;
            first = false;
          }
          parts.push_back(os.str());
        }
        if (parts.empty())
          return;
        llvm::errs() << "  batch terms " << operand << ": ";
        llvm::interleave(parts, llvm::errs(), ", ");
        llvm::errs() << "\n";
      };
      printBatchTerms("A", result.layoutA);
      printBatchTerms("B", result.layoutB);
      printBatchTerms("C", result.layoutC);
      if (result.reductionMayReassociate)
        llvm::errs() << "  reduction: reassoc permitted\n";
      llvm::errs() << "  statement: "
                   << (result.accumulates
                           ? "C = C + A * B (accumulation)"
                           : "C = A * B (assignment: every element of C "
                             "written once, lowered as the K = 1 GEMM into "
                             "a zeroed C)")
                   << "\n";
      // Dimension-intersection partitioning (docs section 21): the loops
      // made batch loops beyond those read by all three operands.
      SmallVector<unsigned> partitionLoops;
      for (unsigned d : result.demotedLoops)
        if (!llvm::is_contained(result.reductionBatchLoops, d))
          partitionLoops.push_back(d);
      if (!partitionLoops.empty()) {
        llvm::errs() << "  partition: loop(s) ";
        llvm::interleave(partitionLoops, llvm::errs(), "+");
        llvm::errs() << " demoted to batch loops (each value is one GEMM on "
                        "its slab of C; the factor that does not read it is "
                        "shared; slabs of an accumulation may overlap, the "
                        "batches then run in program order, docs section "
                        "25)\n";
      }
      // Reduction-batch partitioning (docs section 23).
      if (!result.reductionBatchLoops.empty()) {
        llvm::errs() << "  reduction batch: loop(s) ";
        llvm::interleave(result.reductionBatchLoops, llvm::errs(), "+");
        llvm::errs() << " (not read by C: for each value a GEMM with offset "
                        "factor views, all accumulated into the same slab of "
                        "C in order)\n";
      }

      printLogicalExtent("row", result.rowExtent);
      printLogicalExtent("column", result.columnExtent);
      printLogicalExtent("reduction", result.reductionExtent);
      if (result.tiling)
        printLoopTiling(*result.tiling);

      std::string notDense;
      GemmMappingClass notDenseClass = GemmMappingClass::ProvenValid;
      auto dense = getDenseLogicalGemm(result, &notDense, &notDenseClass);
      if (succeeded(dense))
      {
        // Each coordinate's loops, outermost first ("0+3" for a split one).
        llvm::errs() << "  dense logical GEMM: yes, I/J/K on loops ";
        for (unsigned d = 0; d < 3; ++d) {
          SmallVector<unsigned, 2> loops(dense->sourceDims[d]);
          llvm::sort(loops);
          llvm::errs() << (d ? "/" : "");
          llvm::interleave(loops, llvm::errs(), "+");
        }
        // A batched GEMM: the batch loops, re-emitted around the tiles.
        if (dense->numBatchDims) {
          llvm::errs() << ", batched over loop(s) ";
          llvm::interleave(dense->batchLoops, llvm::errs(), "+");
          llvm::errs() << " (" << dense->batchTrips << " batch iteration(s))";
        }
        llvm::errs() << "\n";
        // The logical extents the partition and the lowering use (a role
        // with no loop has extent 1: K = 1 is the outer product, SQ-4).
        llvm::errs() << "  dense extents: M " << dense->extent[0] << ", N "
                     << dense->extent[1] << ", K " << dense->extent[2] << "\n";
        // The logical mapping as an exact bijection onto the box (docs
        // section 19.13): no logical iteration missing or duplicated.
        std::string notBijective;
        const bool bijective =
            succeeded(verifyLogicalBijection(result, *dense, &notBijective));
        if (bijective)
          llvm::errs() << "  logical bijection: verified (the iteration domain "
                          "maps one to one onto the box of the GEMM)\n";
        else
          llvm::errs() << "  logical bijection: not verified (" << notBijective
                       << ")\n";
        if (bijective && perturbCertificate) {
          auto check = [&](StringRef what, const LogicalMatmul &matmul,
                           const DenseLogicalGemm &gemm) {
            std::string why;
            llvm::errs() << "  perturbed mapping (" << what << "): ";
            if (succeeded(verifyLogicalBijection(matmul, gemm, &why)))
              llvm::errs() << "verified\n";
            else
              llvm::errs() << "not verified (" << why << ")\n";
          };
          DenseLogicalGemm wider = *dense;
          wider.extent[0] += 1;
          check("M + 1", result, wider);
          if (dense->extent[0] >= 2) {
            DenseLogicalGemm narrower = *dense;
            narrower.extent[0] -= 1;
            check("M - 1", result, narrower);
          }
          DenseLogicalGemm coarser = *dense;
          coarser.scale[0] *= 2;
          check("I's scale doubled", result, coarser);
          if (!result.normalization.sourceOffset.empty()) {
            LogicalMatmul shifted = result;
            shifted.normalization.sourceOffset[0] += 1;
            check("loop 0's origin + 1", shifted, *dense);
          }
          const unsigned rowI = dense->numBatchDims;
          if (rowI < result.logicalMapping.transformation.size()) {
            LogicalMatmul collapsed = result;
            for (int64_t &entry : collapsed.logicalMapping.transformation[rowI])
              entry = 0;
            check("I's row of T zero", collapsed, *dense);
          }
        }
      }
      else {
        llvm::errs() << "  dense logical GEMM: no (" << notDense << ")\n";
        // Which loop bound is not a constant, and why.
        if (!result.normalization.domain &&
            !result.normalizationDetail.empty())
          llvm::errs() << "  bounds: " << result.normalizationDetail << "\n";
        llvm::errs() << "  class: " << stringifyGemmMappingClass(notDenseClass)
                     << "\n";
      }

      auto partitions = partitionLogicalMatmul(result);
      if (failed(partitions)) {
        llvm::errs() << "  partitioning result: failure\n";
      } else {
        llvm::errs() << "  partition count: "
                     << partitions->size()
                     << "\n";

        for (unsigned i = 0; i < partitions->size(); ++i) {
          llvm::errs() << "  partition[" << i
                       << "] execution domain:\n";
          (*partitions)[i].iterationDomain.dump();
        }
      }

      if (!regions.empty() && succeeded(dense)) {
        if (regions.size() % 6 != 0) {
          getOperation().emitError("regions takes six integers per region");
          signalPassFailure();
          return;
        }
        SmallVector<LogicalBox> given;
        for (size_t r = 0; r < regions.size(); r += 6) {
          LogicalBox box;
          for (unsigned d = 0; d < 3; ++d) {
            box.lower[d] = regions[r + d];
            box.extent[d] = regions[r + 3 + d];
          }
          given.push_back(box);
        }
        std::string why;
        auto givenPartitions = partitionLogicalMatmul(result, given, &why);
        if (failed(givenPartitions))
          llvm::errs() << "  given partition: not exact (" << why << ")\n";
        else
          llvm::errs() << "  given partition: " << givenPartitions->size()
                       << " regions, exact\n";
      }

      // The partition the systolic fleet induces, when the module declares
      // one: the binary-search plan's tiles as regions, and whether they
      // were proven an exact partition of the iteration domain.
      if (fleet.resources.empty() || failed(dense))
        continue;
      auto plan = planSystolicMatmul(result.anchor, dense->extent[0],
                                     dense->extent[1], dense->extent[2],
                                     fleet, {});
      if (failed(plan))
        continue;
      SmallVector<LogicalBox> boxes;
      SmallVector<int64_t> boxDevice;
      for (const SystolicTileBlock &block : plan->blocks)
        for (const SystolicTileInvocation &inv : block.invocations) {
          LogicalBox box;
          box.lower = {block.row, block.column, inv.k0};
          box.extent = {block.m, block.n, inv.k};
          boxes.push_back(box);
          boxDevice.push_back(block.acceleratorId);
        }
      std::string why;
      auto fleetPartitions = partitionLogicalMatmul(result, boxes, &why);
      if (failed(fleetPartitions)) {
        llvm::errs() << "  fleet partition: not exact (" << why << ")\n";
        continue;
      }
      llvm::errs() << "  fleet partition: " << fleetPartitions->size()
                   << " regions, exact\n";
      for (auto [i, box] : llvm::enumerate(boxes))
        llvm::errs() << "    region[" << i << "] I[" << box.lower[0] << ", "
                     << box.lower[0] + box.extent[0] << ") J["
                     << box.lower[1] << ", " << box.lower[1] + box.extent[1]
                     << ") K[" << box.lower[2] << ", "
                     << box.lower[2] + box.extent[2] << ") on "
                     << fleet.symbols[boxDevice[i]] << "\n";
    }

    // The tilings of nests in which no GEMM was recognized (section 26).
    for (Operation *outermost : failedOutermost) {
      if (claimed.contains(outermost))
        continue;
      auto it = writtenTiling.find(outermost);
      if (it == writtenTiling.end())
        continue;
      llvm::errs() << "loop tiling of a nest with no recognized GEMM:\n";
      verifyLoopTilingCoverage(it->second);
      printLoopTiling(it->second);
    }

    // The tiling legality of each nest S5.3 decides (section 26.4), as the
    // lowering pass decides it: the same function, the same option.
    for (const WrittenTilingDecision &decision : tilingDecisions) {
      llvm::errs() << "tiling legality (section 26.4; as "
                      "--affine-matmul-to-systolic decides it, "
                      "assume-no-alias="
                   << (assumeNoAlias ? "true" : "false")
                   << "): " << describeTilingLegality(decision.legality)
                   << "\n";
      if (decision.legality.untiling != GemmMappingClass::ProvenValid)
        llvm::errs() << "  reason: " << decision.legality.why << "\n";
    }
  }
};

} // namespace

void mlir::systolic::registerTestAffineMatmulDiscoveryPass() {
  PassRegistration<TestAffineMatmulDiscoveryPass>();
}
