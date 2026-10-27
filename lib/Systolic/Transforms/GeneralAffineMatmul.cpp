//===- GeneralAffineMatmul.cpp - Exact subspaces and finite domains
//--------===//
#include "Systolic/GeneralAffineMatmul.h"
#include "mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/Dialect/Affine/IR/AffineValueMap.h"
#include "mlir/IR/IntegerSet.h"
#include "mlir/IR/Matchers.h"
#include "llvm/Support/CheckedArithmetic.h"
#include <functional>
#include <limits>
#include <map>
#include <set>

using namespace mlir;
using namespace mlir::systolic;
using lattice::IntMatrix;
using lattice::IntVector;
namespace {
static std::optional<unsigned> rankOf(const IntMatrix &m, unsigned n) {
  auto e = lattice::columnEchelon(m, n);
  if (!e)
    return std::nullopt;
  return e->rank;
}
static IntMatrix sumOf(IntMatrix a, const IntMatrix &b) {
  a.append(b.begin(), b.end());
  return a;
}
static std::optional<IntMatrix> basis(const IntMatrix &m, unsigned n) {
  IntMatrix result;
  for (const auto &row : m) {
    auto p = lattice::primitive(row);
    if (!p)
      return std::nullopt;
    IntMatrix trial = result;
    trial.push_back(*p);
    auto r = rankOf(trial, n);
    if (!r)
      return std::nullopt;
    if (*r > result.size())
      result.push_back(*p);
  }
  return result;
}
/// X^T a = Y^T b. An integer kernel spans all rational solutions after
/// clearing denominators; multiplying by X yields the actual intersection,
/// not just its dimension. Check Grassmann independently on the result.
static std::optional<IntMatrix> intersection(const IntMatrix &x,
                                             const IntMatrix &y, unsigned n) {
  IntMatrix equations(n, IntVector(x.size() + y.size(), 0));
  for (unsigned d = 0; d < n; ++d) {
    for (unsigned j = 0; j < x.size(); ++j)
      equations[d][j] = x[j][d];
    for (unsigned j = 0; j < y.size(); ++j) {
      auto c = llvm::checkedSub(int64_t(0), y[j][d]);
      if (!c)
        return std::nullopt;
      equations[d][x.size() + j] = *c;
    }
  }
  auto kernel = lattice::columnEchelon(equations, x.size() + y.size());
  if (!kernel)
    return std::nullopt;
  IntMatrix images;
  for (const auto &v : kernel->kernel) {
    IntVector row(n, 0);
    for (unsigned d = 0; d < n; ++d)
      for (unsigned j = 0; j < x.size(); ++j) {
        auto t = llvm::checkedMul(v[j], x[j][d]);
        auto u = t ? llvm::checkedAdd(row[d], *t) : std::nullopt;
        if (!u)
          return std::nullopt;
        row[d] = *u;
      }
    images.push_back(row);
  }
  auto b = basis(images, n);
  auto xy = rankOf(sumOf(x, y), n);
  if (!b || !xy || b->size() != x.size() + y.size() - *xy)
    return std::nullopt;
  return b;
}
/// Representatives for X/(X intersect S); the quotient has unique dimension
/// but these representatives are not unique. No executable batch is inferred.
static std::optional<IntMatrix> complement(const IntMatrix &x,
                                           const IntMatrix &s, unsigned n) {
  auto span = basis(s, n);
  if (!span)
    return std::nullopt;
  IntMatrix result;
  for (const auto &row : x) {
    auto r = rankOf(sumOf(*span, IntMatrix{row}), n);
    if (!r)
      return std::nullopt;
    if (*r > span->size()) {
      span->push_back(row);
      result.push_back(row);
    }
  }
  return result;
}
static std::optional<OperandSubspaces>
subspaces(const std::array<IntMatrix, 3> &rows, unsigned n) {
  OperandSubspaces out;
  for (unsigned a = 0; a < 3; ++a) {
    auto b = basis(rows[a], n);
    if (!b)
      return std::nullopt;
    out.operand[a] = *b;
    out.ranks[a] = b->size();
  }
  const unsigned first[3] = {0, 0, 1}, second[3] = {1, 2, 2};
  for (unsigned p = 0; p < 3; ++p) {
    auto r = rankOf(sumOf(out.operand[first[p]], out.operand[second[p]]), n);
    auto b = intersection(out.operand[first[p]], out.operand[second[p]], n);
    if (!r || !b)
      return std::nullopt;
    out.ranks[3 + p] = *r;
    out.pair[p] = *b;
  }
  auto shared = intersection(out.pair[0], out.operand[2], n);
  auto all = rankOf(sumOf(sumOf(rows[0], rows[1]), rows[2]), n);
  if (!shared || !all)
    return std::nullopt;
  out.shared = *shared;
  out.ranks[6] = *all;
  for (unsigned a = 0; a < 3; ++a) {
    auto exclusive = complement(
        out.operand[a],
        sumOf(out.operand[(a + 1) % 3], out.operand[(a + 2) % 3]), n);
    if (!exclusive)
      return std::nullopt;
    out.exclusive[a] = *exclusive;
  }
  IntMatrix used = out.shared;
  for (unsigned r = 0; r < 3; ++r) {
    auto c = complement(out.pair[(r + 1) % 3], out.shared, n);
    if (!c)
      return std::nullopt;
    out.core[r] = *c;
    used = sumOf(used, *c);
  }
  auto rank = rankOf(used, n);
  if (!rank)
    return std::nullopt;
  out.independentCore = *rank == used.size();
  IntMatrix ambient(n, IntVector(n, 0));
  for (unsigned d = 0; d < n; ++d)
    ambient[d][d] = 1;
  auto rem = complement(ambient, used, n);
  auto cKernel = lattice::columnEchelon(rows[2], n);
  if (!rem || !cKernel)
    return std::nullopt;
  out.residual = *rem;
  out.outputKernel = cKernel->kernel;
  auto frame = basis(sumOf(used, *rem), n);
  if (!frame || frame->size() != n)
    return std::nullopt;
  out.coordinateFrame = *frame;
  auto allRows = sumOf(sumOf(rows[0], rows[1]), rows[2]);
  auto unused = complement(ambient, allRows, n);
  auto kernel = lattice::columnEchelon(allRows, n);
  if (!unused || !kernel)
    return std::nullopt;
  out.unusedComplement = *unused;
  out.unobservedKernel = kernel->kernel;
  return out;
}
static std::optional<int64_t> eval(AffineExpr e, ArrayRef<int64_t> x) {
  if (auto c = dyn_cast<AffineConstantExpr>(e))
    return c.getValue();
  if (auto d = dyn_cast<AffineDimExpr>(e))
    return x[d.getPosition()];
  auto b = dyn_cast<AffineBinaryOpExpr>(e);
  if (!b)
    return std::nullopt;
  auto a = eval(b.getLHS(), x), c = eval(b.getRHS(), x);
  if (!a || !c)
    return std::nullopt;
  switch (e.getKind()) {
  case AffineExprKind::Add:
    return llvm::checkedAdd(*a, *c);
  case AffineExprKind::Mul:
    return llvm::checkedMul(*a, *c);
  case AffineExprKind::FloorDiv:
  case AffineExprKind::CeilDiv:
  case AffineExprKind::Mod: {
    if (*c <= 0)
      return std::nullopt;
    int64_t q = *a / *c, r = *a % *c;
    if (e.getKind() == AffineExprKind::CeilDiv)
      return q + (r > 0);
    if (e.getKind() == AffineExprKind::FloorDiv)
      return q - (r < 0);
    return r < 0 ? r + *c : r;
  }
  default:
    return std::nullopt;
  }
}
static std::optional<AffineMap> onUniverse(AffineMap map, ValueRange operands,
                                           ArrayRef<Value> universe) {
  SmallVector<Value> values(operands);
  affine::fullyComposeAffineMapAndOperands(&map, &values);
  SmallVector<AffineExpr> dims, syms;
  for (auto [i, v] : llvm::enumerate(values)) {
    AffineExpr e;
    auto pos = llvm::find(universe, v);
    if (pos != universe.end())
      e = getAffineDimExpr(pos - universe.begin(), map.getContext());
    else {
      IntegerAttr attr;
      if (!matchPattern(v, m_Constant(&attr)))
        return std::nullopt;
      e = getAffineConstantExpr(attr.getInt(), map.getContext());
    }
    (i < map.getNumDims() ? dims : syms).push_back(e);
  }
  return simplifyAffineMap(
      map.replaceDimsAndSymbols(dims, syms, universe.size(), 0));
}
static std::optional<IntVector> linearRow(AffineExpr e, unsigned n) {
  if (isa<AffineConstantExpr>(e))
    return IntVector(n, 0);
  if (auto d = dyn_cast<AffineDimExpr>(e)) {
    IntVector r(n, 0);
    r[d.getPosition()] = 1;
    return r;
  }
  auto b = dyn_cast<AffineBinaryOpExpr>(e);
  if (!b)
    return std::nullopt;
  auto a = linearRow(b.getLHS(), n), c = linearRow(b.getRHS(), n);
  if (!a || !c)
    return std::nullopt;
  IntVector row(n, 0);
  if (e.getKind() == AffineExprKind::Add) {
    for (unsigned d = 0; d < n; ++d) {
      auto t = llvm::checkedAdd((*a)[d], (*c)[d]);
      if (!t)
        return std::nullopt;
      row[d] = *t;
    }
    return row;
  }
  if (e.getKind() != AffineExprKind::Mul)
    return std::nullopt;
  auto k = dyn_cast<AffineConstantExpr>(b.getRHS());
  if (!k)
    return std::nullopt;
  for (unsigned d = 0; d < n; ++d) {
    auto t = llvm::checkedMul((*a)[d], k.getValue());
    if (!t)
      return std::nullopt;
    row[d] = *t;
  }
  return row;
}
} // namespace

std::optional<TensorContractionSpaces>
mlir::systolic::analyzeTensorContractionSpaces(
    const std::array<IntMatrix, 3> &accessMatrices,
    const std::array<IntVector, 3> &offsets,
    unsigned sourceRank) {

  TensorContractionSpaces out;

  IntMatrix allRows;

  // Step 1: Analyze each operand independently.
  for (unsigned a = 0; a < 3; ++a) {
    const auto &matrix = accessMatrices[a];

    if (matrix.size() != offsets[a].size())
      return std::nullopt;

    for (const auto &row : matrix)
      if (row.size() != sourceRank)
        return std::nullopt;

    auto rowBasis = basis(matrix, sourceRank);
    auto echelon = lattice::columnEchelon(matrix, sourceRank);

    if (!rowBasis || !echelon)
      return std::nullopt;

    if (rowBasis->size() != echelon->rank)
      return std::nullopt;

    if (echelon->kernel.size() + echelon->rank != sourceRank)
      return std::nullopt;

    auto &operand = out.operands[a];

    operand.accessMatrix = matrix;
    operand.offsets = offsets[a];
    operand.rowBasis = *rowBasis;
    operand.kernel = echelon->kernel;
    operand.rank = echelon->rank;

    allRows = sumOf(allRows, matrix);
  }

  // Step 2: Compute exact pairwise intersections.
  constexpr unsigned first[3] = {0, 0, 1};
  constexpr unsigned second[3] = {1, 2, 2};

  for (unsigned p = 0; p < 3; ++p) {
    const auto &x = out.operands[first[p]].rowBasis;
    const auto &y = out.operands[second[p]].rowBasis;

    auto xy = intersection(x, y, sourceRank);
    auto sumRank = rankOf(sumOf(x, y), sourceRank);

    if (!xy || !sumRank)
      return std::nullopt;

    // Grassmann dimension identity.
    if (xy->size() + *sumRank != x.size() + y.size())
      return std::nullopt;

    out.pairIntersections[p] = *xy;
  }

  // Step 3: Triple intersection.
  //
  // (A intersect B) intersect C
  auto abc = intersection(
      out.pairIntersections[0],
      out.operands[2].rowBasis,
      sourceRank);

  if (!abc)
    return std::nullopt;

  out.tripleIntersection = *abc;

  // Step 4: Combined observable row space.
  auto combinedBasis = basis(allRows, sourceRank);
  auto combinedEchelon =
      lattice::columnEchelon(allRows, sourceRank);

  if (!combinedBasis || !combinedEchelon)
    return std::nullopt;

  if (combinedBasis->size() != combinedEchelon->rank)
    return std::nullopt;

  if (combinedEchelon->kernel.size() +
          combinedEchelon->rank != sourceRank)
    return std::nullopt;

  out.combinedSpace = *combinedBasis;
  out.combinedKernel = combinedEchelon->kernel;
  out.combinedRank = combinedEchelon->rank;

  return out;
}


std::optional<TensorSpaceDependencies>
mlir::systolic::analyzeTensorSpaceDependencies(
    const TensorContractionSpaces &spaces, unsigned sourceRank) {

  TensorSpaceDependencies out;

  // The three row bases are independent within their own operand spaces.
  std::array<IntMatrix, 3> bases;

  for (unsigned a = 0; a < 3; ++a) {
    bases[a] = spaces.operands[a].rowBasis;

    for (const auto &row : bases[a])
      if (row.size() != sourceRank)
        return std::nullopt;

    auto r = rankOf(bases[a], sourceRank);
    if (!r || *r != bases[a].size())
      return std::nullopt;

    if (bases[a].size() > std::numeric_limits<unsigned>::max() -
                              out.coefficientOffsets[a])
      return std::nullopt;

    out.coefficientOffsets[a + 1] =
        out.coefficientOffsets[a] + bases[a].size();
  }

  const unsigned total = out.coefficientOffsets[3];

  // Build the linear map Phi:
  //
  //   Phi(alpha, beta, gamma)
  //       = A^T alpha + B^T beta + C^T gamma
  //
  // Matrix shape: sourceRank x total.
  IntMatrix phi(sourceRank, IntVector(total, 0));

  for (unsigned a = 0; a < 3; ++a)
    for (unsigned r = 0; r < bases[a].size(); ++r)
      for (unsigned d = 0; d < sourceRank; ++d)
        phi[d][out.coefficientOffsets[a] + r] = bases[a][r][d];

  auto full = lattice::columnEchelon(phi, total);
  if (!full)
    return std::nullopt;

  auto fullBasis = basis(full->kernel, total);
  if (!fullBasis)
    return std::nullopt;

  auto sumRank = rankOf(sumOf(sumOf(bases[0], bases[1]), bases[2]),
                        sourceRank);

  if (!sumRank || *sumRank > total)
    return std::nullopt;

  // Rank-nullity certificate.
  if (fullBasis->size() != total - *sumRank)
    return std::nullopt;

  out.allRelations = *fullBasis;
  out.relationRank = fullBasis->size();

  const unsigned first[3] = {0, 0, 1};
  const unsigned second[3] = {1, 2, 2};

  IntMatrix pairRows;

  // Compute the kernel of each pair's basis transpose and embed the
  // resulting relations into the full [A | B | C] coordinate system.
  for (unsigned p = 0; p < 3; ++p) {
    const unsigned a = first[p];
    const unsigned b = second[p];

    const unsigned ra = bases[a].size();
    const unsigned rb = bases[b].size();

    if (ra > std::numeric_limits<unsigned>::max() - rb)
      return std::nullopt;

    const unsigned pairTotal = ra + rb;

    IntMatrix pairPhi(sourceRank, IntVector(pairTotal, 0));

    for (unsigned r = 0; r < ra; ++r)
      for (unsigned d = 0; d < sourceRank; ++d)
        pairPhi[d][r] = bases[a][r][d];

    for (unsigned r = 0; r < rb; ++r)
      for (unsigned d = 0; d < sourceRank; ++d)
        pairPhi[d][ra + r] = bases[b][r][d];

    auto pairKernel = lattice::columnEchelon(pairPhi, pairTotal);
    if (!pairKernel)
      return std::nullopt;

    auto pairBasis = basis(pairKernel->kernel, pairTotal);
    if (!pairBasis)
      return std::nullopt;

    auto pairRank = rankOf(sumOf(bases[a], bases[b]), sourceRank);
    if (!pairRank || *pairRank > pairTotal)
      return std::nullopt;

    if (pairBasis->size() != pairTotal - *pairRank)
      return std::nullopt;

    for (const auto &relation : *pairBasis) {
      IntVector embedded(total, 0);

      for (unsigned r = 0; r < ra; ++r)
        embedded[out.coefficientOffsets[a] + r] = relation[r];

      for (unsigned r = 0; r < rb; ++r)
        embedded[out.coefficientOffsets[b] + r] = relation[ra + r];

      out.pairRelations[p].push_back(embedded);
      pairRows.push_back(embedded);
    }
  }

  auto generated = basis(pairRows, total);
  if (!generated)
    return std::nullopt;

  out.pairGeneratedRelations = *generated;
  out.pairGeneratedRank = generated->size();

  // Pairwise-generated relations must belong to ker(Phi).
  for (const auto &row : *generated) {
    auto combinedRank = rankOf(
        sumOf(out.allRelations, IntMatrix{row}), total);

    if (!combinedRank || *combinedRank != out.relationRank)
      return std::nullopt;
  }

  // Pick representatives for:
  //
  //   ker(Phi) / span(pairwise-supported relations)
  //
  // The complement is relative, not canonical.
  auto essential = complement(out.allRelations, *generated, total);
  if (!essential)
    return std::nullopt;

  if (out.pairGeneratedRank > out.relationRank)
    return std::nullopt;

  if (essential->size() !=
      out.relationRank - out.pairGeneratedRank)
    return std::nullopt;

  out.essentialThreeWayRelations = *essential;
  out.essentialThreeWayRank = essential->size();

  // Verify Phi * relation = 0 using checked integer arithmetic.
  // A failure here indicates an invalid relation or an arithmetic
  // representation failure, not a mathematical non-contraction.
  auto annihilates = [&](const IntVector &relation) -> bool {
    if (relation.size() != total)
      return false;

    for (unsigned d = 0; d < sourceRank; ++d) {
      int64_t sum = 0;

      for (unsigned c = 0; c < total; ++c) {
        auto product = llvm::checkedMul(phi[d][c], relation[c]);
        if (!product)
          return false;

        auto next = llvm::checkedAdd(sum, *product);
        if (!next)
          return false;

        sum = *next;
      }

      if (sum != 0)
        return false;
    }

    return true;
  };

  for (const auto &relation : out.allRelations)
    if (!annihilates(relation))
      return std::nullopt;

  for (const auto &relation : out.pairGeneratedRelations)
    if (!annihilates(relation))
      return std::nullopt;

  for (const auto &relation : out.essentialThreeWayRelations)
    if (!annihilates(relation))
      return std::nullopt;

  // Verify that the chosen quotient representatives give a direct
  // sum decomposition of the full rational relation space.
  //
  // ker(Phi) = R_pair (+) R_essential
  //
  // The decomposition is exact over Q, not a certificate of an
  // integer lattice direct sum.
  auto essentialRank =
      rankOf(out.essentialThreeWayRelations, total);

  auto reconstructedRank = rankOf(
      sumOf(out.pairGeneratedRelations,
            out.essentialThreeWayRelations),
      total);

  if (!essentialRank || !reconstructedRank)
    return std::nullopt;

  if (*essentialRank != out.essentialThreeWayRank)
    return std::nullopt;

  if (*reconstructedRank != out.relationRank)
    return std::nullopt;

  if (out.pairGeneratedRank + *essentialRank != out.relationRank)
    return std::nullopt;

  return out;
}

FailureOr<GeneralAffineMatmul>
mlir::systolic::analyzeGeneralAffineMatmul(Operation *anchor, std::string &why,
                                           unsigned budget) {
  auto fail = [&](StringRef msg) -> FailureOr<GeneralAffineMatmul> {
    why = msg.str();
    return failure();
  };
  auto root = dyn_cast_or_null<affine::AffineForOp>(anchor);
  if (!root)
    return fail("not an affine loop");
  // Include every enclosing affine loop, even across affine.if and even
  // when none of its counters occurs directly in an access expression.
  for (Operation *p = root->getParentOp(); p; p = p->getParentOp())
    if (auto loop = dyn_cast<affine::AffineForOp>(p))
      root = loop;
  GeneralAffineMatmul out;
  out.anchor = root;
  SmallVector<affine::AffineIfOp> guards;
  Block *body = root.getBody();
  out.loops.push_back(root);
  // Every chain level must contain exactly one structural operation, or
  // exactly the terminal MAC. No stores/calls/initializers are discarded.
  for (;;) {
    SmallVector<Operation *> ops;
    for (auto &op : *body)
      if (!isa<affine::AffineYieldOp>(op))
        ops.push_back(&op);
    if (ops.size() == 1) {
      if (auto loop = dyn_cast<affine::AffineForOp>(ops[0])) {
        out.loops.push_back(loop);
        body = loop.getBody();
        continue;
      }
      if (auto g = dyn_cast<affine::AffineIfOp>(ops[0])) {
        if (g.hasElse())
          return fail("an else branch needs a disjoint union certificate");
        guards.push_back(g);
        body = g.getThenBlock();
        continue;
      }
    }
    break;
  }
  SmallVector<affine::AffineLoadOp> loads;
  for (auto &op : *body) {
    if (auto l = dyn_cast<affine::AffineLoadOp>(op))
      loads.push_back(l);
    else if (auto s = dyn_cast<affine::AffineStoreOp>(op)) {
      if (out.store)
        return fail("more than one store");
      out.store = s;
    } else if (auto m = dyn_cast<arith::MulFOp>(op)) {
      if (out.multiply)
        return fail("more than one product");
      out.multiply = m;
    } else if (auto a = dyn_cast<arith::AddFOp>(op)) {
      if (out.add)
        return fail("more than one sum");
      out.add = a;
    } else if (!isa<affine::AffineYieldOp>(op))
      return fail("body is not one pure MAC");
  }
  if (loads.size() != 3 || !out.store || !out.multiply || !out.add ||
      !out.add.getType().isF32() ||
      out.store.getValue() != out.add.getResult() ||
      !out.multiply->hasOneUse() || !out.add->hasOneUse())
    return fail("not one f32 accumulating MAC");
  Value old;
  if (out.add.getLhs() == out.multiply.getResult())
    old = out.add.getRhs();
  else if (out.add.getRhs() == out.multiply.getResult())
    old = out.add.getLhs();
  else
    return fail("sum does not use the product");
  out.loads = {out.multiply.getLhs().getDefiningOp<affine::AffineLoadOp>(),
               out.multiply.getRhs().getDefiningOp<affine::AffineLoadOp>(),
               old.getDefiningOp<affine::AffineLoadOp>()};
  for (auto l : out.loads)
    if (!l || !llvm::is_contained(loads, l) || !l->hasOneUse())
      return fail("MAC does not have three private loads");
  if (out.loads[0] == out.loads[1] || out.loads[0] == out.loads[2] ||
      out.loads[1] == out.loads[2] ||
      out.loads[2].getMemRef() != out.store.getMemRef())
    return fail("ambiguous accumulation");
  SmallVector<Value> ivs;
  for (auto loop : out.loops) {
    if (loop.getNumResults() || loop.getStepAsInt() != 1)
      return fail("general path requires unit-step loops without iter_args");
    ivs.push_back(loop.getInductionVar());
  }
  // The source iteration dimension is a mathematical input parameter.
  // Do not impose an arbitrary upper bound on the number of IVs.
  if (ivs.empty())
    return fail("empty source iteration universe");

  // This budget limits the current finite execution decomposition,
  // not the mathematical validity of the contraction.
  if (budget == 0)
    return fail("finite execution decomposition has zero point budget");
  std::array<IntMatrix, 3> rows;
  for (unsigned a = 0; a < 3; ++a) {
    auto map =
        onUniverse(out.loads[a].getMap(), out.loads[a].getIndices(), ivs);
    if (!map)
      return fail("access has a symbolic or external value");
    out.access[a] = *map;
    for (auto e : map->getResults()) {
      auto row = linearRow(e, ivs.size());
      if (!row)
        return fail("access is not a checked integer linear form");
      rows[a].push_back(*row);
      auto offset = eval(e, SmallVector<int64_t>(ivs.size(), 0));
      if (!offset)
        return fail("affine offset overflows checked arithmetic");
      out.accessOffsets[a].push_back(*offset);
    }
  }
  auto storeMap = onUniverse(out.store.getMap(), out.store.getIndices(), ivs);
  if (!storeMap || *storeMap != out.access[2])
    return fail("load and store output addresses differ");
  auto spaces = subspaces(rows, ivs.size());
  if (!spaces)
    return fail("exact subspace construction overflowed");
  out.spaces = *spaces;
  // The output kernel really fixes every C linear form. The dimension is
  // checked rather than inferred from variable-reader membership.
  if (out.spaces.outputKernel.size() + out.spaces.ranks[2] != ivs.size())
    return fail("output kernel dimension certificate failed");
  SmallVector<Operation *> band;
  for (auto l : out.loops)
    band.push_back(l);
  for (auto g : guards)
    band.push_back(g);
  auto domain = std::make_shared<affine::FlatAffineValueConstraints>();
  if (failed(affine::getIndexSet(band, domain.get())) ||
      domain->getNumSymbolVars() != 0)
    return fail("source domain has unresolved parameters");
  // Keep the relation space in precisely the lexical SSA order.
  if (domain->getNumDimVars() != ivs.size())
    return fail("domain universe differs from loop universe");
  for (unsigned d = 0; d < ivs.size(); ++d)
    if (domain->getValue(d) != ivs[d])
      return fail("domain SSA order differs from source order");
  out.sourceDomain = domain;
  SmallVector<AffineMap> lower, upper;
  for (auto loop : out.loops) {
    auto lb =
        onUniverse(loop.getLowerBoundMap(), loop.getLowerBoundOperands(), ivs);
    auto ub =
        onUniverse(loop.getUpperBoundMap(), loop.getUpperBoundOperands(), ivs);
    if (!lb || !ub)
      return fail("loop bounds are not closed over the source universe");
    lower.push_back(*lb);
    upper.push_back(*ub);
  }
  SmallVector<AffineMap> conditions;
  for (auto g : guards) {
    auto set = g.getIntegerSet();
    auto map = onUniverse(AffineMap::get(set.getNumDims(), set.getNumSymbols(),
                                         set.getConstraints(), g.getContext()),
                          g.getOperands(), ivs);
    if (!map)
      return fail("guard has an external parameter");
    conditions.push_back(*map);
  }
  std::map<SmallVector<int64_t>, unsigned> outputs;
  std::map<SmallVector<int64_t>, unsigned> physicalOutputs;
  // Shared/I/J quotient representatives all belong to C's row space.
  // They drive the logical output partition key, while actual C addresses
  // independently check that no write dependence crosses those partitions.
  IntMatrix outputCoordinates =
      sumOf(out.spaces.shared, sumOf(out.spaces.core[0], out.spaces.core[1]));
  auto dot = [&](ArrayRef<int64_t> row,
                 ArrayRef<int64_t> x) -> std::optional<int64_t> {
    int64_t total = 0;
    for (unsigned d = 0; d < x.size(); ++d) {
      auto t = llvm::checkedMul(row[d], x[d]);
      auto s = t ? llvm::checkedAdd(total, *t) : std::nullopt;
      if (!s)
        return std::nullopt;
      total = *s;
    }
    return total;
  };
  std::set<SmallVector<int64_t>> unique;
  SmallVector<int64_t> point(ivs.size(), 0);
  uint64_t work = 0;
  std::function<bool(unsigned)> enumerate = [&](unsigned depth) {
    if (++work > uint64_t(budget) * 16u)
      return false;
    if (depth < ivs.size()) {
      std::optional<int64_t> lo, hi;
      for (auto e : lower[depth].getResults()) {
        auto v = eval(e, point);
        if (!v)
          return false;
        lo = lo ? std::max(*lo, *v) : *v;
      }
      for (auto e : upper[depth].getResults()) {
        auto v = eval(e, point);
        if (!v)
          return false;
        hi = hi ? std::min(*hi, *v) : *v;
      }
      if (!lo || !hi)
        return false;
      // Do not use stale deeper IVs when evaluating bounds. Affine verifier
      // already ensures each bound only reads enclosing IVs.
      for (int64_t x = *lo; x < *hi;) {
        point[depth] = x;
        if (!enumerate(depth + 1))
          return false;
        auto next = llvm::checkedAdd(x, int64_t(1));
        if (!next)
          return false;
        x = *next;
      }
      return true;
    }
    for (unsigned g = 0; g < guards.size(); ++g)
      for (auto [d, e] : llvm::enumerate(conditions[g].getResults())) {
        auto v = eval(e, point);
        if (!v)
          return false;
        if (guards[g].getIntegerSet().isEq(d) ? *v != 0 : *v < 0)
          return true;
      }
    if (out.points.size() >= budget || !unique.insert(point).second)
      return false;
    GeneralAffinePoint p;
    p.source = point;
    for (const auto &row : out.spaces.coordinateFrame) {
      auto y = dot(row, point);
      if (!y)
        return false;
      p.logical.push_back(*y);
    }
    for (unsigned a = 0; a < 3; ++a) {
      auto ty = cast<MemRefType>(out.loads[a].getMemRef().getType());
      if (!ty.getLayout().isIdentity())
        return false;
      for (auto [d, e] : llvm::enumerate(out.access[a].getResults())) {
        auto v = eval(e, point);
        if (!v || *v < 0 || (!ty.isDynamicDim(d) && *v >= ty.getDimSize(d)))
          return false;
        p.address[a].push_back(*v);
      }
    }
    SmallVector<int64_t> key;
    for (const auto &row : outputCoordinates) {
      auto y = dot(row, point);
      if (!y)
        return false;
      key.push_back(*y);
    }
    key.append(p.address[2].begin(), p.address[2].end());
    auto [it, inserted] = outputs.emplace(key, out.fibers.size());
    auto [physical, firstWrite] =
        physicalOutputs.emplace(p.address[2], it->second);
    if (!firstWrite && physical->second != it->second)
      return false;
    if (inserted)
      out.fibers.emplace_back();
    out.fibers[it->second].push_back(out.points.size());
    out.points.push_back(std::move(p));
    return true;
  };
  if (!enumerate(0))
    return fail(
        "finite execution decomposition failed: point/work budget, "
        "bounds, memory layout, alias partition, or checked arithmetic; "
        "this does not establish mathematical invalidity");
  if (out.points.empty())
    return fail("empty source domain");
  // An independent Presburger equality checks the constructed cover against
  // loop/guard constraints. Singleton pieces retain integer congruences and
  // source witnesses; nothing is replaced by a rational bounding polytope.
  using namespace presburger;
  auto pointSpace = PresburgerSpace::getSetSpace(ivs.size());
  auto cover = PresburgerSet::getEmpty(pointSpace);
  auto jobSpace = PresburgerSpace::getRelationSpace(ivs.size(), 2);
  auto relation = std::make_shared<PresburgerRelation>(
      PresburgerRelation::getEmpty(jobSpace));
  auto logicalSpace = PresburgerSpace::getRelationSpace(ivs.size(), ivs.size());
  auto logical = std::make_shared<PresburgerRelation>(
      PresburgerRelation::getEmpty(logicalSpace));
  for (auto [f, fiber] : llvm::enumerate(out.fibers))
    for (auto [t, p] : llvm::enumerate(fiber)) {
      IntegerPolyhedron singleton(pointSpace);
      IntegerRelation entry(jobSpace);
      SmallVector<int64_t> coords(out.points[p].source);
      coords.push_back(f);
      coords.push_back(t);
      for (unsigned d = 0; d < coords.size(); ++d) {
        SmallVector<int64_t> row(coords.size() + 1, 0);
        row[d] = 1;
        auto neg = llvm::checkedSub(int64_t(0), coords[d]);
        if (!neg)
          return fail("point certificate offset overflow");
        row.back() = *neg;
        entry.addEquality(row);
        if (d < ivs.size()) {
          SmallVector<int64_t> eq(ivs.size() + 1, 0);
          eq[d] = 1;
          eq.back() = *neg;
          singleton.addEquality(eq);
        }
      }
      IntegerRelation logicalEntry(logicalSpace);
      for (unsigned d = 0; d < 2 * ivs.size(); ++d) {
        int64_t value = d < ivs.size() ? out.points[p].source[d]
                                       : out.points[p].logical[d - ivs.size()];
        auto neg = llvm::checkedSub(int64_t(0), value);
        if (!neg)
          return fail("logical certificate offset overflow");
        SmallVector<int64_t> row(2 * ivs.size() + 1, 0);
        row[d] = 1;
        row.back() = *neg;
        logicalEntry.addEquality(row);
      }
      cover.unionInPlace(singleton);
      relation->unionInPlace(entry);
      logical->unionInPlace(logicalEntry);
    }
  PresburgerSet source(*domain);
  if (!cover.isEqual(source))
    return fail("Presburger exact source cover verification failed");
  out.sourceToJobs = relation;
  out.sourceToLogical = logical;
  return out;
}

void mlir::systolic::describeOperandSubspaces(const OperandSubspaces &s,
                                              llvm::raw_ostream &os) {
  os << "ranks A/B/C/AB/AC/BC/ABC = ";
  llvm::interleaveComma(s.ranks, os);
  os << "; intersections AB/AC/BC/shared = " << s.pair[0].size() << ","
     << s.pair[1].size() << "," << s.pair[2].size() << "," << s.shared.size();
  auto matrix = [&](StringRef name, const IntMatrix &m) {
    os << "; " << name << "=[";
    llvm::interleave(
        m, os,
        [&](const IntVector &r) {
          os << "[";
          llvm::interleaveComma(r, os);
          os << "]";
        },
        ",");
    os << "]";
  };
  matrix("AB", s.pair[0]);
  matrix("AC", s.pair[1]);
  matrix("BC", s.pair[2]);
  matrix("shared", s.shared);
  matrix("I", s.core[0]);
  matrix("J", s.core[1]);
  matrix("K", s.core[2]);
  matrix("residual", s.residual);
  matrix("C-kernel", s.outputKernel);
  matrix("integer-coordinate-frame", s.coordinateFrame);
  matrix("A-only-quotient", s.exclusive[0]);
  matrix("B-only-quotient", s.exclusive[1]);
  matrix("C-only-quotient", s.exclusive[2]);
  matrix("unused-dual-complement", s.unusedComplement);
  matrix("unobserved-source-kernel", s.unobservedKernel);
  os << "; independent core=" << (s.independentCore ? "yes" : "no");
}
