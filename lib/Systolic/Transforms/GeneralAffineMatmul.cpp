//===- GeneralAffineMatmul.cpp - Exact subspaces and finite domains
//--------===//
#include "Systolic/GeneralAffineMatmul.h"
#include "mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/Dialect/Affine/IR/AffineValueMap.h"
#include "mlir/IR/IntegerSet.h"
#include "mlir/IR/Matchers.h"
#include "llvm/Support/CheckedArithmetic.h"
#include <functional>
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
/// Pairwise intersections alone need not form a distributive decomposition.
/// Work in the primal annihilators instead: I must be invisible to B, J to A,
/// and K to C. At fixed partition coordinates the three restrictions must be
/// full rank-2 GEMM views. Completing these directions gives a rational frame,
/// not a license to execute its partition axes independently. The exact source
/// relation below still retains every integer witness and write dependence.
static void recoverParametricFrame(const std::array<IntMatrix, 3> &rows,
                                   unsigned n, OperandSubspaces &out) {
  if (n < 3 || out.ranks[0] != 2 || out.ranks[1] != 2 || out.ranks[2] != 2)
    return;
  std::array<IntMatrix, 3> kernel;
  const unsigned invisible[3] = {1, 0, 2};
  for (unsigned r = 0; r < 3; ++r) {
    auto e = lattice::columnEchelon(rows[invisible[r]], n);
    if (!e)
      return;
    kernel[r] = e->kernel;
    // Sums avoid mistaking a union of two annihilator hyperplanes for a
    // complete obstruction. This is a bounded candidate search, not a
    // completeness claim; every accepted frame is checked exactly.
    unsigned count = kernel[r].size();
    for (unsigned a = 0; a < count && kernel[r].size() < 16; ++a)
      for (unsigned b = a + 1; b < count && kernel[r].size() < 16; ++b) {
        IntVector v(n, 0);
        bool valid = true;
        for (unsigned d = 0; d < n; ++d) {
          auto sum = llvm::checkedAdd(kernel[r][a][d], kernel[r][b][d]);
          if (!sum) {
            valid = false;
            break;
          }
          v[d] = *sum;
        }
        if (valid)
          kernel[r].push_back(v);
      }
  }
  auto restrict = [&](const IntMatrix &m,
                      const IntMatrix &directions) -> std::optional<IntMatrix> {
    IntMatrix result(m.size(), IntVector(directions.size(), 0));
    for (unsigned a = 0; a < m.size(); ++a)
      for (unsigned b = 0; b < directions.size(); ++b)
        for (unsigned d = 0; d < n; ++d) {
          auto p = llvm::checkedMul(m[a][d], directions[b][d]);
          auto q = p ? llvm::checkedAdd(result[a][b], *p) : std::nullopt;
          if (!q)
            return std::nullopt;
          result[a][b] = *q;
        }
    return result;
  };
  unsigned attempts = 0;
  for (const auto &i : kernel[0])
    for (const auto &j : kernel[1])
      for (const auto &k : kernel[2]) {
        if (++attempts > 128)
          return;
        IntMatrix directions{i, j, k};
        std::array<IntMatrix, 3> projected;
        bool valid = true;
        for (unsigned a = 0; a < 3; ++a) {
          auto p = restrict(rows[a], directions);
          if (!p || rankOf(*p, 3) != std::optional<unsigned>(2)) {
            valid = false;
            break;
          }
          projected[a] = *p;
          unsigned absent = a == 0 ? 1 : a == 1 ? 0 : 2;
          for (const auto &row : *p)
            valid &= row[absent] == 0;
        }
        if (!valid || rankOf(directions, n) != std::optional<unsigned>(3))
          continue;
        IntMatrix ambient(n, IntVector(n, 0));
        for (unsigned d = 0; d < n; ++d)
          ambient[d][d] = 1;
        auto partitions = complement(ambient, directions, n);
        if (!partitions || partitions->size() + 3 != n)
          continue;
        IntMatrix columns = sumOf(*partitions, directions);
        IntMatrix sourceFrame(n, IntVector(n, 0));
        for (unsigned a = 0; a < n; ++a)
          for (unsigned b = 0; b < n; ++b)
            sourceFrame[a][b] = columns[b][a];
        IntMatrix numeratorRows;
        SmallVector<int64_t> denominators;
        for (unsigned d = 0; d < n; ++d) {
          auto inverse = lattice::rowTimesInverse(ambient[d], sourceFrame);
          if (!inverse) {
            valid = false;
            break;
          }
          numeratorRows.push_back(inverse->numerators);
          denominators.push_back(inverse->denominator);
        }
        std::array<IntMatrix, 3> physical;
        for (unsigned a = 0; a < 3 && valid; ++a) {
          auto m = restrict(rows[a], columns);
          if (!m)
            valid = false;
          else
            physical[a] = *m;
        }
        if (!valid)
          continue;
        out.parametricSourceFrame = sourceFrame;
        out.parametricOperandMaps = physical;
        out.coordinateFrame = numeratorRows;
        out.frameDenominators = denominators;
        out.hasParametricCore = true;
        return;
      }
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
  if (!out.independentCore || out.core[0].size() != 1 ||
      out.core[1].size() != 1 || out.core[2].size() != 1)
    recoverParametricFrame(rows, n, out);
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
  if (ivs.empty() || ivs.size() > 8 || budget == 0)
    return fail("source universe or point budget exceeded");
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
    return fail("finite exact decomposition budget, bounds, layout, or checked "
                "arithmetic failed");
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
  if (s.hasParametricCore) {
    os << "; parametric partition dims=" << s.coordinateFrame.size() - 3;
    matrix("source-frame-V", s.parametricSourceFrame);
    os << "; inverse denominators=[";
    llvm::interleaveComma(s.frameDenominators, os);
    os << "]";
    matrix("A-in-frame", s.parametricOperandMaps[0]);
    matrix("B-in-frame", s.parametricOperandMaps[1]);
    matrix("C-in-frame", s.parametricOperandMaps[2]);
  }
  os << "; independent core=" << (s.independentCore ? "yes" : "no");
}
