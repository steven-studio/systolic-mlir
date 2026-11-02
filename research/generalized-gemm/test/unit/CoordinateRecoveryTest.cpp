// G3a: tables, proposer, checker and the gates of the enumerator. The oracles in
// test/fuzz recompute everything independently; these tests pin the named cases.
#include "GeneralizedGemm/CoordinateAnalysis.h"
#include "GeneralizedGemm/TerminationAnalysis.h"
#include "GeneralizedGemm/Z3Backend.h"

#include "TestSupport.h"

#include <functional>
#include <set>

using namespace mlir::generalized_gemm;
using llvm::APInt;
using V = TerminationVerdict;

static mlir::MLIRContext *context;
static Z3Backend backend;

//===----------------------------------------------------------------------===//
// Tables
//===----------------------------------------------------------------------===//

static APInt n(int64_t value) { return APInt(64, (uint64_t)value, true); }
static IndexTuple tuple(std::initializer_list<int64_t> values) {
  IndexTuple out;
  for (int64_t value : values)
    out.push_back(n(value));
  return out;
}

using Row = std::vector<int64_t>;
using View = std::function<IndexTuple(const Row &)>;

/// One event per row, in row order, with x = the row.
static AccessTable table(const std::vector<Row> &rows, View h, View f, View g) {
  AccessTable t;
  t.coordinatesUniform = true;
  for (unsigned c = 0; c < rows[0].size(); ++c)
    t.coordinateNames.push_back(c);
  uint64_t step = 0;
  for (const Row &row : rows) {
    IndexTuple x;
    for (int64_t value : row)
      x.push_back(n(value));
    IndexTuple out = h(row);
    t.add(0, step++, x, out, out, f(row), g(row));
  }
  return t;
}

static std::vector<Row> box(std::vector<int64_t> extents) {
  std::vector<Row> rows{{}};
  for (int64_t extent : extents) {
    std::vector<Row> next;
    for (const Row &row : rows)
      for (int64_t v = 0; v < extent; ++v) {
        Row longer = row;
        longer.push_back(v);
        next.push_back(longer);
      }
    rows = std::move(next);
  }
  return rows;
}

static CandidateReport canonical(const AccessTable &t) {
  CanonicalPartitions p;
  viewKernels(t, p.kerH, p.kerF, p.kerG);
  p = joinKernels(p.kerH, p.kerF, p.kerG);
  return inspectCandidate(t, canonicalCandidate(t, p));
}

static bool hasKind(const CandidateReport &r, CounterexampleKind kind) {
  for (const Counterexample &c : r.counterexamples)
    if (c.kind == kind)
      return true;
  return false;
}

static View at(std::vector<unsigned> columns) {
  return [columns](const Row &row) {
    IndexTuple out;
    for (unsigned c : columns)
      out.push_back(n(row[c]));
    return out;
  };
}

// Views of the standard form over rows (i, j, k).
static View ij = at({0, 1}), ik = at({0, 2}), kj = at({2, 1});

static void testTables() {
  TupleTable tuples;
  CHECK(tuples.intern(tuple({1, 2})) == 0);
  CHECK(tuples.intern(tuple({2, 1})) == 1);
  CHECK(tuples.intern(tuple({1, 2})) == 0);
  // Width is identity. A concatenation does not collide either.
  CHECK(tuples.intern({APInt(8, 1)}) != tuples.intern({APInt(16, 1)}));
  CHECK(tuples.intern(tuple({})) != tuples.intern(tuple({0})));

  CHECK(normalizePartition({7, 7, 3, 9, 3}) == Partition({0, 0, 1, 2, 1}));

  // Repeats stay events, and T indexes them.
  AccessTable t;
  IndexTuple x = tuple({1});
  t.add(0, 5, x, x, x, x, x);
  t.add(0, 9, x, x, x, x, x);
  t.add(1, 9, x, x, x, x, x); // Another site: another iteration point.
  CHECK(t.events().size() == 3);
  CHECK(t.triples().size() == 1 && t.multiplicity()[0] == 3);
  CHECK(t.pointCount() == 2);
  CHECK(t.events()[0].step == 5 && t.events()[2].site == 1);
  CHECK(t.events()[0].point == t.events()[1].point);
  CHECK(t.events()[0].point != t.events()[2].point);

  // h, f and g of one triple are compared by value, per position.
  AccessTable s;
  s.add(0, 0, x, tuple({1}), tuple({1}), tuple({2}), tuple({3}));
  s.add(0, 1, x, tuple({2}), tuple({2}), tuple({1}), tuple({3}));
  CHECK(s.triples().size() == 2);
}

static void testJoins() {
  CanonicalPartitions p = joinKernels({0, 0, 1, 1, 2}, {0, 1, 1, 2, 2}, {0, 1, 2, 3, 4});
  // A chain through two partitions closes transitively.
  CHECK(p.rhoI == Partition({0, 0, 0, 0, 0}));
  CHECK(p.rhoJ == Partition({0, 0, 1, 1, 2}));
  CHECK(p.rhoK == Partition({0, 1, 1, 2, 2}));
  CHECK(p.rhoP == Partition({0, 0, 0, 0, 0}));
  CanonicalPartitions none = joinKernels({0, 1}, {0, 1}, {0, 1});
  CHECK(none.rhoP == Partition({0, 1}));
  CHECK(none.meetH && none.meetF && none.meetG);
  // Only the last join uses all three.
  CanonicalPartitions three = joinKernels({0, 0, 1}, {0, 1, 2}, {0, 1, 1});
  CHECK(three.rhoI == Partition({0, 0, 1}));
  CHECK(three.rhoJ == Partition({0, 0, 0}));
  CHECK(three.rhoK == Partition({0, 1, 1}));
  CHECK(three.rhoP == Partition({0, 0, 0}));
}

//===----------------------------------------------------------------------===//
// Canonical cases: the facts are reported independently
//===----------------------------------------------------------------------===//

static void testRecovery() {
  auto rows = box({3, 4, 2});
  CandidateReport r = canonical(table(rows, ij, ik, kj));
  CHECK(r.decompositionValid && r.genuine && r.uniqueUpToRelabelling);
  CHECK(r.cartesian == CartesianForm::Box && r.missingPoints == "0");
  CHECK(r.multiplicity == MultiplicityForm::Unit);
  CHECK(r.injectiveH && r.injectiveF && r.injectiveG && !r.batchesShareViews);
  CHECK(r.shape == ShapeClass::GEMM);
  // Unequal axes stay unequal.
  CHECK(r.batches.size() == 1 && r.batches[0].i == 3 && r.batches[0].j == 4 &&
        r.batches[0].k == 2 && r.events == 24);
  CHECK(r.eligibility == Eligibility::ForLaterVerification);
  CHECK(r.counterexamples.empty());
  // The labels are readable as loop variables.
  CHECK(r.support.has_value());
  CHECK((*r.support)[0].empty());
  CHECK((*r.support)[1] == std::vector<unsigned>{0});
  CHECK((*r.support)[2] == std::vector<unsigned>{1});
  CHECK((*r.support)[3] == std::vector<unsigned>{2});

  // Transposes need no special case: labels do not name a side.
  CHECK(canonical(table(rows, at({1, 0}), at({2, 0}), kj)).genuine);

  // Batched, with tiles of different sizes.
  std::vector<Row> batched;
  for (Row row : box({2, 3, 2}))
    batched.push_back({0, row[0], row[1], row[2]});
  for (Row row : box({3, 2, 4}))
    batched.push_back({1, row[0], row[1], row[2]});
  CandidateReport b = canonical(table(batched, at({0, 1, 2}), at({0, 1, 3}), at({0, 3, 2})));
  CHECK(b.genuine && b.batches.size() == 2);
  CHECK(b.batches[0].i == 2 && b.batches[0].k == 2 && b.batches[1].i == 3 && b.batches[1].k == 4);
  CHECK(b.distinctP == 2);

  // Multidimensional coordinates: I = (i1, i2), K = (k1, k2).
  CandidateReport m = canonical(table(box({2, 3, 2, 2, 2}), at({0, 1, 2}), at({0, 1, 3, 4}), at({3, 4, 2})));
  CHECK(m.genuine && m.batches[0].i == 6 && m.batches[0].j == 2 && m.batches[0].k == 4);
  CHECK((*m.support)[1] == std::vector<unsigned>({0, 1}));
  CHECK((*m.support)[3] == std::vector<unsigned>({3, 4}));

  // Linearized and nonlinear views: injective on this domain is all that counts.
  CHECK(canonical(table(rows,
                        [](const Row &q) { return tuple({4 * q[0] + q[1]}); }, ik, kj))
            .genuine);
  CHECK(canonical(table(rows,
                        [](const Row &q) { return tuple({q[0] * q[0] * q[0], 7 * q[1] * q[1]}); },
                        [](const Row &q) { return tuple({q[0] * q[0] * q[0] + 1, q[2]}); },
                        [](const Row &q) { return tuple({q[2], 7 * q[1] * q[1]}); }))
            .genuine);
  // A piecewise access is one function of the data.
  CHECK(canonical(table(rows, ij,
                        [](const Row &q) {
                          return q[0] < 2 ? tuple({q[0], q[2]}) : tuple({q[2] + 50, q[0]});
                        },
                        kj))
            .genuine);
  // An operand shared over p folds into J: a real single GEMM, and p is in J.
  CandidateReport shared = canonical(table(box({2, 3, 2, 2}), at({0, 1, 2}), at({1, 3}), at({0, 3, 2})));
  CHECK(shared.genuine && shared.batches[0].j == 4 && shared.batches[0].i == 3);
}

static void testDegenerateShapes() {
  auto shapeOf = [](const AccessTable &t) { return canonical(t); };
  auto valid = [](const CandidateReport &r, ShapeClass shape) {
    CHECK(r.decompositionValid);
    CHECK(r.shape == shape);
    CHECK(!r.strictNonDegenerate && !r.genuine);
    // Not genuine, and still carried forward.
    CHECK(r.eligibility == Eligibility::ForLaterVerification);
  };
  // Element-wise: valid, each tile 1 x 1 x 1.
  CandidateReport hadamard = shapeOf(table(box({5}), at({0}), at({0}), at({0})));
  valid(hadamard, ShapeClass::ElementWise);
  CHECK(hadamard.batches.size() == 5);
  // One point is a scalar, not element-wise.
  valid(shapeOf(table(box({1}), at({0}), at({0}), at({0}))), ShapeClass::Scalar);
  // Dot: one output cell.
  valid(shapeOf(table(box({5}), [](const Row &) { return tuple({0}); }, at({0}), at({0}))),
        ShapeClass::Dot);
  // GEMV.
  valid(shapeOf(table(box({3, 4}), at({0}), at({0, 1}), at({1}))), ShapeClass::GEMV);
  // K = 1 outer product: the view of K is constant.
  valid(shapeOf(table(box({3, 4}), at({0, 1}), at({0}), at({1}))), ShapeClass::Outer);
  // One axis.
  valid(shapeOf(table(box({3}), at({0}), at({0}), [](const Row &) { return tuple({0}); })),
        ShapeClass::ScaledVector);
  // Batches of different class.
  std::vector<Row> mixed{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}};
  CandidateReport m = canonical(table(mixed, at({0, 1}), at({0, 2}), at({0, 2})));
  CHECK(m.shape == ShapeClass::Mixed && !m.genuine);
  // One good tile does not make a run of 1 x 1 x 1 tiles genuine.
  std::vector<Row> partly;
  for (Row row : box({2, 2, 2}))
    partly.push_back({0, row[0], row[1], row[2]});
  partly.push_back({1, 0, 0, 0});
  CandidateReport strict = canonical(table(partly, at({0, 1, 2}), at({0, 1, 3}), at({0, 3, 2})));
  CHECK(strict.decompositionValid && strict.cartesian == CartesianForm::Box);
  CHECK(!strict.strictNonDegenerate && !strict.genuine);
}

static void testPartialDomains() {
  // Triangular: the joins recover the axes, and the box does not hold. No
  // dependence implies Cartesian structure, and nothing here pretends it does.
  std::vector<Row> triangle;
  for (Row q : box({4, 4, 3}))
    if (q[1] <= q[0])
      triangle.push_back(q);
  CandidateReport t = canonical(table(triangle, ij, ik, kj));
  CHECK(t.decompositionValid && t.cartesian == CartesianForm::Partial);
  CHECK(t.batches[0].i == 4 && t.batches[0].j == 4 && t.batches[0].k == 3);
  CHECK(t.missingPoints == "18" && !t.genuine && !t.uniqueUpToRelabelling);
  CHECK(t.strictNonDegenerate); // The axes are long enough. It is not a box.
  CHECK(t.eligibility == Eligibility::ForLaterVerification);
  CHECK(hasKind(t, CounterexampleKind::MissingPoint));

  // One point missing: the first absent point, in label order.
  std::vector<Row> holed;
  for (Row q : box({3, 4, 2}))
    if (q != Row{1, 1, 1})
      holed.push_back(q);
  CandidateReport h = canonical(table(holed, ij, ik, kj));
  CHECK(h.missingPoints == "1" && h.cartesian == CartesianForm::Partial);
  CHECK(h.counterexamples.size() == 1);
  CHECK(h.counterexamples[0].kind == CounterexampleKind::MissingPoint);

  // Disconnected slices over-split: a diagonal is three separate tiles.
  std::vector<Row> diagonal{{0, 0, 0}, {0, 1, 1}, {0, 2, 2}};
  CandidateReport d = canonical(table(diagonal, ij, ik, kj));
  CHECK(d.shape == ShapeClass::ElementWise && d.batches.size() == 3);

  // A band: K is k + j, on a partial domain.
  CandidateReport band = canonical(table(box({3, 3, 3}), ij,
                                         [](const Row &q) { return tuple({q[0], q[2] + q[1]}); }, kj));
  CHECK(band.decompositionValid && band.cartesian == CartesianForm::Partial);
  CHECK(band.batches[0].k == 5);
}

static void testNonInjective() {
  // Each fails C1, and each has a true GEMM: Unknown is the honest answer.
  auto fails = [](const AccessTable &t) {
    CandidateReport r = canonical(t);
    CHECK(!r.decompositionValid);
    CHECK(!r.dependenceH);
    CHECK(r.eligibility == Eligibility::NotEligible);
    CHECK(r.openObligations.empty());
    CHECK(r.counterexamples.front().kind == CounterexampleKind::DependenceH);
  };
  auto rows = box({3, 4, 2});
  fails(table(rows, ij, at({2}), kj)); // A[k]: no I.
  fails(table(rows, ij, ik, at({2}))); // B[k]: no J.
  fails(table(box({2, 3, 2, 2}), at({0, 1, 2}), at({1, 3}), at({3, 2}))); // Both shared.
  fails(table(box({4, 4}), [](const Row &q) { return tuple({q[0] + q[1]}); }, at({0}), at({1})));

  // Theorem 5: two valid inequivalent coordinates for one non-injective program.
  auto program = table(box({3, 3, 2}), ij, ik, at({2}));
  auto labels = [&](std::function<uint32_t(const Row &)> j) {
    CoordinateCandidate c;
    for (const Row &q : box({3, 3, 2})) {
      c.p.push_back(0);
      c.i.push_back(q[0]);
      c.j.push_back(j(q));
      c.k.push_back(q[2]);
    }
    return inspectCandidate(program, c);
  };
  CandidateReport first = labels([](const Row &q) { return q[1]; });
  CandidateReport second = labels([](const Row &q) { return (q[1] + q[0]) % 3; });
  CHECK(first.decompositionValid && second.decompositionValid);
  CHECK(first.cartesian == CartesianForm::Box && second.cartesian == CartesianForm::Box);
  // B ignores J, so the candidate is valid and neither unique nor genuine.
  CHECK(!first.injectiveG && !first.uniqueUpToRelabelling && !first.genuine);
  CHECK(first.eligibility == Eligibility::ForLaterVerification);
}

static CoordinateCandidate labelled(const std::vector<Row> &rows,
                                    std::function<std::array<uint32_t, 4>(const Row &)> make) {
  CoordinateCandidate c;
  for (const Row &q : rows) {
    auto labels = make(q);
    c.p.push_back(labels[0]);
    c.i.push_back(labels[1]);
    c.j.push_back(labels[2]);
    c.k.push_back(labels[3]);
  }
  return c;
}

static void testTrivialCandidates() {
  auto rows = box({2, 3, 2});
  AccessTable t = table(rows, ij, ik, kj);
  uint32_t next = 0;
  auto id = [&](const Row &) { return next++; };

  // I = J = K = x: every dependence holds, and the box does not.
  next = 0;
  CandidateReport x = inspectCandidate(t, labelled(rows, [&](const Row &q) {
    uint32_t v = id(q);
    return std::array<uint32_t, 4>{0, v, v, v};
  }));
  CHECK(x.decompositionValid);
  CHECK(x.cartesian == CartesianForm::Partial && !x.genuine);
  // 12^3 - 12.
  CHECK(x.missingPoints == "1716");
  CHECK(!x.injectiveH && !x.injectiveF && !x.injectiveG);

  // P = x, the rest constant: a batch of 1 x 1 x 1 per event.
  next = 0;
  CandidateReport batches = inspectCandidate(t, labelled(rows, [&](const Row &q) {
    return std::array<uint32_t, 4>{id(q), 0, 0, 0};
  }));
  CHECK(batches.decompositionValid && batches.shape == ShapeClass::ElementWise);
  CHECK(!batches.genuine && !batches.injectiveH && batches.batchesShareViews);
  CHECK(batches.cartesian == CartesianForm::Box);

  // P = (i, j) with a dot per output cell: true, degenerate, and it shows.
  CandidateReport dots = inspectCandidate(t, labelled(rows, [](const Row &q) {
    return std::array<uint32_t, 4>{(uint32_t)(q[0] * 10 + q[1]), 0, 0, (uint32_t)q[2]};
  }));
  CHECK(dots.decompositionValid && dots.cartesian == CartesianForm::Box);
  CHECK(dots.shape == ShapeClass::Dot && !dots.genuine && !dots.injectiveF);
  CHECK(dots.batchesShareViews);

  // A hidden loop made a coordinate: F ignores it, and the multiset is kept.
  auto repeated = box({2, 2, 3});
  AccessTable hidden = table(repeated, at({0, 1}), at({0}), at({1}));
  CandidateReport canonicalHidden = canonical(hidden);
  CHECK(canonicalHidden.decompositionValid);
  CHECK(canonicalHidden.multiplicity == MultiplicityForm::Uniform);
  CHECK(canonicalHidden.multiplicityMax == 3 && canonicalHidden.shape == ShapeClass::Outer);
  CHECK(canonicalHidden.triples == 4 && canonicalHidden.events == 12);
  CandidateReport named = inspectCandidate(hidden, labelled(repeated, [](const Row &q) {
    return std::array<uint32_t, 4>{0, (uint32_t)q[0], (uint32_t)q[1], (uint32_t)q[2]};
  }));
  CHECK(named.decompositionValid && named.multiplicity == MultiplicityForm::Unit);
  CHECK(!named.injectiveF && !named.genuine);
  CHECK(named.tripleMultiplicityMax == 3);
}

static void testCounterexamples() {
  auto rows = box({2, 2, 2});
  AccessTable t = table(rows, ij, ik, kj);
  auto constant = labelled(rows, [](const Row &) { return std::array<uint32_t, 4>{0, 0, 0, 0}; });
  CandidateReport r = inspectCandidate(t, constant);
  CHECK(!r.decompositionValid && !r.dependenceH && !r.dependenceF && !r.dependenceG);
  CHECK(r.counterexamples.size() >= 3);
  // Earliest disagreement: event 2 is the first with a different h from event 0,
  // and event 1 the first with a different f.
  CHECK(r.counterexamples[0].kind == CounterexampleKind::DependenceH);
  CHECK(r.counterexamples[0].first == 0 && r.counterexamples[0].second == 2);
  CHECK(r.counterexamples[1].kind == CounterexampleKind::DependenceF);
  CHECK(r.counterexamples[1].first == 0 && r.counterexamples[1].second == 1);
  CHECK(r.counterexamples[2].kind == CounterexampleKind::DependenceG);
  CHECK(r.counterexamples[2].second == 1);

  // Deterministic: the same input gives the same list.
  CandidateReport again = inspectCandidate(t, constant);
  CHECK(again.counterexamples.size() == r.counterexamples.size());

  // Each dependence alone.
  auto rowsOnly = [&](unsigned broken) {
    return labelled(rows, [broken](const Row &q) {
      std::array<uint32_t, 4> labels{0, (uint32_t)q[0], (uint32_t)q[1], (uint32_t)q[2]};
      if (broken < 4)
        labels[broken] = 0;
      return labels;
    });
  };
  CHECK(inspectCandidate(t, rowsOnly(9)).genuine);
  CandidateReport noI = inspectCandidate(t, rowsOnly(1));
  CHECK(!noI.dependenceH && !noI.dependenceF && noI.dependenceG);
  CandidateReport noJ = inspectCandidate(t, rowsOnly(2));
  CHECK(!noJ.dependenceH && noJ.dependenceF && !noJ.dependenceG);
  CandidateReport noK = inspectCandidate(t, rowsOnly(3));
  CHECK(noK.dependenceH && !noK.dependenceF && !noK.dependenceG);

  // Injectivity counterexamples name the views.
  AccessTable sameH = table(rows, at({0}), ik, kj);
  auto extra = rowsOnly(9);
  CandidateReport lost = inspectCandidate(sameH, extra);
  CHECK(lost.decompositionValid && !lost.injectiveH && lost.injectiveF && lost.injectiveG);
  CHECK(hasKind(lost, CounterexampleKind::ViewNotInjectiveH));
  AccessTable sameF = table(rows, ij, at({0}), kj);
  CHECK(hasKind(inspectCandidate(sameF, extra), CounterexampleKind::ViewNotInjectiveF));
  AccessTable sameG = table(rows, ij, ik, at({1}));
  CHECK(hasKind(inspectCandidate(sameG, extra), CounterexampleKind::ViewNotInjectiveG));

  // A coordinate must be a function of the iteration point.
  std::vector<Row> twice = {{0, 0, 0}, {0, 0, 0}};
  AccessTable duplicate = table(twice, ij, ik, kj);
  CandidateReport split = inspectCandidate(duplicate, labelled(twice, [n = 0u](const Row &) mutable {
    return std::array<uint32_t, 4>{0, 0, 0, n++};
  }));
  CHECK(!split.functionOfPoint && !split.decompositionValid);
  CHECK(split.counterexamples.front().kind == CounterexampleKind::NotAFunctionOfPoint);
  CandidateReport same = canonical(duplicate);
  CHECK(same.decompositionValid && same.multiplicity == MultiplicityForm::Uniform);
  CHECK(same.duplicateIterationPoints == 1 && same.multiplicityMax == 2);

  // Nonuniform multiplicity, with the two events that show it.
  std::vector<Row> uneven = {{0, 0, 0}, {0, 0, 0}, {1, 0, 0}};
  CandidateReport u = canonical(table(uneven, ij, ik, kj));
  CHECK(u.multiplicity == MultiplicityForm::Nonuniform);
  CHECK(u.multiplicityMin == 1 && u.multiplicityMax == 2 && !u.genuine);
  CHECK(hasKind(u, CounterexampleKind::MultiplicityDiffers));

  // C0: a read of another cell.
  AccessTable read;
  read.coordinatesUniform = true;
  read.add(0, 0, tuple({0}), tuple({0}), tuple({0}), tuple({0}), tuple({0}));
  read.add(0, 1, tuple({1}), tuple({1}), tuple({9}), tuple({1}), tuple({1}));
  CandidateReport c0 = canonical(read);
  CHECK(!c0.outputReadMatches && !c0.decompositionValid);
  CHECK(c0.counterexamples.front().kind == CounterexampleKind::OutputReadDiffers);
  CHECK(c0.counterexamples.front().second == 1);

  // A box that would be genuine, but for one read: nothing may overrule validity.
  AccessTable nearly;
  nearly.coordinatesUniform = true;
  uint64_t step = 0;
  for (const Row &q : rows) {
    IndexTuple x = at({0, 1, 2})(q), out = ij(q);
    nearly.add(0, step++, x, out, step == 3 ? tuple({0, 0}) : out, ik(q), kj(q));
  }
  CandidateReport almost = canonical(nearly);
  CHECK(!almost.decompositionValid && almost.strictNonDegenerate);
  CHECK(almost.cartesian == CartesianForm::Box && almost.injectiveH);
  CHECK(!almost.genuine && !almost.uniqueUpToRelabelling);
  CHECK(almost.eligibility == Eligibility::NotEligible);

  // Malformed input is refused without a fact.
  CoordinateCandidate empty;
  CHECK(!inspectCandidate(t, empty).malformed.empty());
  CHECK(!inspectCandidate(AccessTable(), empty).malformed.empty());
  auto shortCandidate = rowsOnly(9);
  shortCandidate.k.pop_back();
  CHECK(!inspectCandidate(t, shortCandidate).malformed.empty());
}

//===----------------------------------------------------------------------===//
// Gates
//===----------------------------------------------------------------------===//

struct Module {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  explicit Module(const std::string &text) : module(parse(*context, text)) {
    if (!module) {
      fprintf(stderr, "cannot parse:\n%s\n", text.c_str());
      exit(2);
    }
  }
  SemanticProgram import(const std::string &name, bool structured = false) {
    ImportOptions options;
    options.indexBitWidth = 8;
    options.lowerStructured = structured;
    return importFunction(module->lookupSymbol<mlir::func::FuncOp>(name), options);
  }
};

static ParameterScope range(const SemanticProgram &p, unsigned parameter, int64_t low,
                            int64_t high) {
  ParameterScope scope;
  scope.allParameters = false;
  unsigned w = p.parameters[parameter].sort.width;
  scope.ranges.push_back({parameter, APInt(w, low, true), APInt(w, high, true)});
  return scope;
}

static TerminationOptions terminationOptions(ParameterScope scope) {
  TerminationOptions options;
  options.scope = std::move(scope);
  options.limits.replayFuel = 1u << 12;
  return options;
}

static EnumerationOptions enumerationOptions(ParameterScope scope) {
  EnumerationOptions options;
  options.scope = std::move(scope);
  return options;
}

static CoordinateResult recover(const SemanticProgram &p, ParameterScope scope = {}) {
  TerminationResult proof = analyzeTermination(p, backend, terminationOptions(scope));
  return recoverCoordinates(p, proof.certificate ? &*proof.certificate : nullptr,
                            enumerationOptions(scope), backend);
}

static const char *programs = R"(
func.func @gemm(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %j] : memref<8x8xi32>
        %z = memref.load %c[%i, %j] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @floats(%a: memref<2x2xf32>, %b: memref<2x2xf32>, %c: memref<2x2xf32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<2x2xf32>
        %y = memref.load %b[%k, %j] : memref<2x2xf32>
        %z = memref.load %c[%i, %j] : memref<2x2xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m : f32
        memref.store %s, %c[%i, %j] : memref<2x2xf32>
      }
    }
  }
  return
}
func.func @no_site(%n: index) {
  return
}
func.func @address_reads_memory(%a: memref<4xindex>, %b: memref<4xi32>, %c: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %c4 step %c1 {
    %t = memref.load %a[%i] : memref<4xindex>
    %x = memref.load %b[%t] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
func.func @control_reads_memory(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  scf.for %i = %c0 to %c4 step %c1 {
    %t = memref.load %a[%i] : memref<4xi32>
    %p = arith.cmpi ne, %t, %zero : i32
    scf.if %p {
      %x = memref.load %a[%i] : memref<4xi32>
      %y = memref.load %b[%i] : memref<4xi32>
      %z = memref.load %c[%i] : memref<4xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i] : memref<4xi32>
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
func.func @direct_and_scalar(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %d: memref<1xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  %r = scf.for %i = %c0 to %c4 step %c1 iter_args(%acc = %zero) -> (i32) {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
    %t = arith.addi %acc, %m : i32
    scf.yield %t : i32
  }
  memref.store %r, %d[%c0] : memref<1xi32>
  return
}
func.func @even_only(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: i8) {
  %z = arith.constant 0 : i8
  %two = arith.constant 2 : i8
  %c0 = arith.constant 0 : index
  cf.br ^h(%z : i8)
^h(%i: i8):
  %g = arith.cmpi ne, %i, %n : i8
  cf.cond_br %g, ^b, ^x
^b:
  %x = memref.load %a[%c0] : memref<8xi32>
  %y = memref.load %b[%c0] : memref<8xi32>
  %v = memref.load %c[%c0] : memref<8xi32>
  %m = arith.muli %x, %y : i32
  %s = arith.addi %v, %m : i32
  memref.store %s, %c[%c0] : memref<8xi32>
  %u = arith.addi %i, %two : i8
  cf.br ^h(%u : i8)
^x:
  return
}
func.func @dynamic(%a: memref<?xi32>, %b: memref<?xi32>, %c: memref<?xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<?xi32>
    %y = memref.load %b[%i] : memref<?xi32>
    %z = memref.load %c[%i] : memref<?xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<?xi32>
  }
  return
}
func.func @out_of_bounds(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %n: index) {
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
func.func @two_sites(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %p = arith.cmpi eq, %i, %c0 : index
        scf.if %p {
          %x = memref.load %a[%i, %k] : memref<4x4xi32>
          %y = memref.load %b[%k, %j] : memref<4x4xi32>
          %z = memref.load %c[%i, %j] : memref<4x4xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<4x4xi32>
        } else {
          %t = arith.addi %k, %c2 : index
          %x = memref.load %a[%t, %i] : memref<4x4xi32>
          %y = memref.load %b[%k, %j] : memref<4x4xi32>
          %z = memref.load %c[%i, %j] : memref<4x4xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<4x4xi32>
        }
      }
    }
  }
  return
}
func.func @two_groups(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %d: memref<2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<2xi32>
    %y = memref.load %b[%i] : memref<2xi32>
    %z = memref.load %c[%i] : memref<2xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<2xi32>
  }
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<2xi32>
    %y = memref.load %b[%i] : memref<2xi32>
    %z = memref.load %d[%i] : memref<2xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %d[%i] : memref<2xi32>
  }
  return
}
func.func @other_first(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %d: memref<2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<2xi32>
    %y = memref.load %b[%i] : memref<2xi32>
    %z = memref.load %c[%i] : memref<2xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<2xi32>
    %x2 = memref.load %d[%i] : memref<2xi32>
    %y2 = memref.load %b[%i] : memref<2xi32>
    %z2 = memref.load %c[%i] : memref<2xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%i] : memref<2xi32>
  }
  return
}
func.func @other_second(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %d: memref<2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<2xi32>
    %y = memref.load %b[%i] : memref<2xi32>
    %z = memref.load %c[%i] : memref<2xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<2xi32>
    %x2 = memref.load %a[%i] : memref<2xi32>
    %y2 = memref.load %d[%i] : memref<2xi32>
    %z2 = memref.load %c[%i] : memref<2xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%i] : memref<2xi32>
  }
  return
}
func.func @equal_h_kernels(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %d: memref<2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<2xi32>
    %y = memref.load %b[%i] : memref<2xi32>
    %z = memref.load %c[%i] : memref<2xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<2xi32>
    %x2 = memref.load %a[%c0] : memref<2xi32>
    %y2 = memref.load %b[%i] : memref<2xi32>
    %z2 = memref.load %d[%i] : memref<2xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %d[%i] : memref<2xi32>
  }
  return
}
func.func @wrong_read(%a: memref<2x2xi32>, %b: memref<2x2xi32>, %c: memref<2x2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      %x = memref.load %a[%i, %j] : memref<2x2xi32>
      %y = memref.load %b[%i, %j] : memref<2x2xi32>
      %z = memref.load %c[%j, %i] : memref<2x2xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %j] : memref<2x2xi32>
    }
  }
  return
}
)";

static const std::string &foundReason() {
  static const std::string text = "the accesses decompose through this candidate";
  return text;
}

static void testGates() {
  Module m(programs);

  // A candidate in hand, from a real domain.
  SemanticProgram gemm = m.import("gemm", true);
  CHECK(gemm.isSupported());
  ParameterScope one = range(gemm, 0, 3, 3);
  CoordinateResult r = recover(gemm, one);
  CHECK(r.enumeration.gate == GateStatus::Open);
  CHECK(r.enumeration.termination.kind() == V::ProvenTerminating);
  CHECK(r.points.size() == 1 && r.points[0].groups.size() == 1);
  const CheckedCandidate &found = r.points[0].groups[0].result;
  CHECK(found.kind() == RecoveryStatus::CandidateFound);
  CHECK(found.reason() == foundReason());
  CHECK(found.report().genuine);
  CHECK(found.fingerprint() == gemm.fingerprint);
  // A candidate is never a proof.
  CHECK(found.equivalence().kind() == ProofStatus::Kind::Unknown);
  // The evidence is for the point run, and only that point.
  const AccessDomain &domain = *r.enumeration.points[0].domains[0];
  CHECK(domain.evidence().form == DomainForm::ExactEnumerated);
  CHECK(admitsClaim(domain.evidence(), DomainClaim::Coverage).admitted);
  CHECK(scopeContains(domain.evidence(), domain.parameters()));
  CHECK(!scopeContains(domain.evidence(), {APInt(8, 2)}));

  // The checker refutes a given candidate, and only that one.
  CoordinateCandidate wrong = *r.points[0].groups[0].candidate;
  std::fill(wrong.i.begin(), wrong.i.end(), 0u);
  CheckedCandidate refuted = checkCandidate(domain, wrong);
  CHECK(refuted.kind() == RecoveryStatus::CandidateRefuted);
  CHECK(refuted.reason().find("dependence_h") != std::string::npos);
  CHECK(refuted.equivalence().kind() == ProofStatus::Kind::Unknown);
  wrong.i.clear();
  CHECK(checkCandidate(domain, wrong).kind() == RecoveryStatus::Unknown);

  // Anyone can say they do not know. Nothing else is open to them.
  CHECK(CheckedCandidate::unknown("x").kind() == RecoveryStatus::Unknown);
  CHECK(CheckedCandidate::unsupported("x").kind() == RecoveryStatus::Unsupported);

  // A float nest proves only "if defined", and D is still exact for completing
  // runs. The stops stay reported.
  SemanticProgram floats = m.import("floats", true);
  CoordinateResult f = recover(floats);
  CHECK(f.enumeration.gate == GateStatus::Open);
  CHECK(f.enumeration.termination.kind() == V::ProvenTerminatingIfDefined);
  CHECK(f.enumeration.stops[(unsigned)SideKind::Underspecified].status != StopStatus::Unreachable);
  CHECK(f.points[0].groups[0].result.report().genuine);

  // No certificate, no enumeration: a run that would return is not enough.
  EnumerationOptions options = enumerationOptions(one);
  CoordinateResult none = recoverCoordinates(gemm, nullptr, options, backend);
  CHECK(none.enumeration.gate == GateStatus::Unknown);
  CHECK(none.enumeration.points.empty() && none.points.empty());

  TerminationResult proof = analyzeTermination(gemm, backend, terminationOptions(one));
  CHECK(proof.certificate.has_value());
  // A certificate of another program, a forged one, and an empty one.
  TerminationCertificate other = *proof.certificate;
  other.fingerprint = "other";
  CHECK(enumerateDomain(gemm, &other, options, backend).gate == GateStatus::Unknown);
  TerminationCertificate blank;
  blank.fingerprint = gemm.fingerprint;
  CHECK(enumerateDomain(gemm, &blank, options, backend).gate == GateStatus::Unknown);
  TerminationCertificate noLevels = *proof.certificate;
  noLevels.levels.clear();
  DomainEnumeration refused = enumerateDomain(gemm, &noLevels, options, backend);
  CHECK(refused.gate == GateStatus::Unknown && refused.points.empty());
  CHECK(refused.termination.kind() == V::Unknown);
  // A verdict is re-derived: a solver that answers nothing leaves nothing.
  struct Silent : SmtBackend {
    SmtResponse check(const std::string &, const SmtLimits &, bool) override {
      SmtResponse response;
      response.detail = "no answer";
      return response;
    }
    std::string version() const override { return "silent"; }
  } silent;
  CHECK(enumerateDomain(gemm, &*proof.certificate, options, silent).gate == GateStatus::Unknown);

  // A proof for a scope is no proof for a wider one, even at the points it had.
  SemanticProgram even = m.import("even_only");
  auto small = [&](std::vector<int64_t> values) {
    ParameterScope scope;
    scope.allParameters = false;
    unsigned w = even.parameters[0].sort.width;
    scope.ranges.push_back({0, APInt(w, values[0], true), APInt(w, values[1], true)});
    return scope;
  };
  // The rank needs an even distance, which the scope below gives.
  ParameterScope proved = small({0, 0}), wider = small({0, 3});
  TerminationResult narrow = analyzeTermination(even, backend, terminationOptions(proved));
  CHECK(narrow.certificate.has_value());
  EnumerationOptions wideOptions = enumerationOptions(wider);
  if (narrow.certificate) {
    DomainEnumeration wide = enumerateDomain(even, &*narrow.certificate, wideOptions, backend);
    CHECK(wide.gate == GateStatus::Unknown);
    CHECK(wide.points.empty());
    CHECK(wide.reason.find("not accepted") != std::string::npos);
  }

  // The scope must be finite within the limit. A limit gives no sample.
  ParameterScope all;
  EnumerationOptions everything = enumerationOptions(all);
  everything.limits.maxPoints = 16;
  DomainEnumeration tooMany = enumerateDomain(gemm, &*proof.certificate, everything, backend);
  CHECK(tooMany.gate == GateStatus::Unknown && tooMany.points.empty());
  CHECK(tooMany.reason.find("limit") != std::string::npos);
  // Exactly at the limit is inside it.
  EnumerationOptions four = enumerationOptions(range(gemm, 0, 0, 3));
  four.limits.maxPoints = 4;
  TerminationResult fourProof = analyzeTermination(gemm, backend, terminationOptions(four.scope));
  CHECK(enumerateDomain(gemm, &*fourProof.certificate, four, backend).points.size() == 4);
  four.limits.maxPoints = 3;
  CHECK(enumerateDomain(gemm, &*fourProof.certificate, four, backend).points.empty());

  // Unsupported forms.
  auto gate = [&](const char *name, bool structured = true) {
    SemanticProgram p = m.import(name, structured);
    return recover(p);
  };
  CHECK(gate("no_site").enumeration.gate == GateStatus::Unsupported);
  CoordinateResult address = gate("address_reads_memory");
  CHECK(address.enumeration.gate == GateStatus::Unsupported);
  CHECK(address.enumeration.reason.find("memory contents") != std::string::npos);
  CoordinateResult control = gate("control_reads_memory");
  CHECK(control.enumeration.gate == GateStatus::Unsupported);
  CHECK(control.enumeration.reason.find("memory contents") != std::string::npos);
  CHECK(control.enumeration.groups.size() == 1); // It got past the site gate.
  SemanticProgram plain = m.import("gemm"); // Nested regions, not lowered.
  CHECK(!plain.isSupported());
  CHECK(recoverCoordinates(plain, nullptr, enumerationOptions({}), backend).enumeration.gate ==
        GateStatus::Unsupported);

  // A scalar accumulator has no output view, and none is invented.
  SemanticProgram scalar = m.import("accumulator", true);
  CoordinateResult s = recover(scalar);
  CHECK(s.enumeration.gate == GateStatus::Unsupported);
  CHECK(s.enumeration.reason.find("scalar accumulator") != std::string::npos);
  CHECK(s.enumeration.groups.size() == 1);
  CHECK(!s.enumeration.groups[0].output.has_value());
  CHECK(s.points.empty());

  // Beside a direct site it stays unsupported, and the direct site proceeds.
  SemanticProgram both = m.import("direct_and_scalar", true);
  CHECK(both.sites.size() == 2 && !both.assumptions.empty());
  TerminationOptions separated = terminationOptions({});
  EnumerationOptions enumerated = enumerationOptions({});
  for (const ProofStatus &assumption : both.assumptions) {
    separated.authorized.insert(assumption.detail());
    enumerated.authorized.insert(assumption.detail());
  }
  // This proof reads memory, so it rests on separation, which must be authorized
  // here as well: the proposer's authorization does not carry over.
  TerminationResult bothProof = analyzeTermination(both, backend, separated);
  if (bothProof.certificate)
    CHECK(enumerateDomain(both, &*bothProof.certificate, enumerationOptions({}), backend).gate ==
          GateStatus::Unknown);
  CHECK(bothProof.certificate.has_value());
  if (bothProof.certificate) {
    CoordinateResult mixed = recoverCoordinates(both, &*bothProof.certificate, enumerated, backend);
    CHECK(mixed.enumeration.gate == GateStatus::Open);
    CHECK(mixed.enumeration.groups.size() == 2);
    CHECK(!mixed.enumeration.assumptionsUsed.empty());
    CHECK(mixed.points.size() == 1 && mixed.points[0].groups.size() == 2);
    for (const GroupRecovery &group : mixed.points[0].groups) {
      bool direct = mixed.enumeration.groups[group.group].output.has_value();
      CHECK(group.result.kind() ==
            (direct ? RecoveryStatus::CandidateFound : RecoveryStatus::Unsupported));
      CHECK(mixed.enumeration.points[0].domains[group.group].has_value() == direct);
      if (!direct) {
        CHECK(group.result.reason().find("scalar accumulator") != std::string::npos);
        CHECK(!group.candidate.has_value());
      }
    }
    // The direct table holds the direct events only.
    for (const auto &domain : mixed.enumeration.points[0].domains)
      if (domain)
        CHECK(domain->table().events().size() == 4);
  }
}

static void testPoints() {
  Module m(programs);
  // Each point stands alone: a different shape, nothing extended over the scope.
  SemanticProgram gemm = m.import("gemm", true);
  CoordinateResult r = recover(gemm, range(gemm, 0, 0, 3));
  CHECK(r.points.size() == 4);
  // n = 0: no events. Nothing is claimed, and it is no failure.
  CHECK(r.points[0].groups[0].result.kind() == RecoveryStatus::Unknown);
  CHECK(r.points[0].groups[0].result.reason().find("no events") != std::string::npos);
  CHECK(!r.enumeration.points[0].domains[0].has_value());
  CHECK(r.points[1].groups[0].result.report().shape == ShapeClass::GEMV);
  CHECK(!r.points[1].groups[0].result.report().genuine);
  CHECK(r.points[1].groups[0].result.report().eligibility == Eligibility::ForLaterVerification);
  CHECK(r.points[2].groups[0].result.report().genuine);
  CHECK(r.points[3].groups[0].result.report().batches[0].i == 3);
  // The points have different tables, so nothing is reused.
  CHECK(r.memoizedJoins == 0);

  // A negative extent has no valid entry: it lies outside the domain.
  SemanticProgram dynamic = m.import("dynamic", true);
  CHECK(dynamic.parameters.size() == 4);
  // Scalar n first, then the three extents, each in its own range.
  ParameterScope scope;
  scope.allParameters = false;
  for (unsigned p = 0; p < 4; ++p)
    scope.ranges.push_back({p, APInt(8, (uint64_t)-1, true), APInt(8, 2, true)});
  // Without a proof of bounds this is still fine: a stop is a stop.
  CoordinateResult d = recover(dynamic, scope);
  CHECK(d.enumeration.gate == GateStatus::Open);
  // 4^4 = 256 points, of which 3^4 ... with n = -1 and three extents >= 0.
  uint64_t inside = 0;
  for (int64_t e1 = -1; e1 <= 2; ++e1)
    for (int64_t e2 = -1; e2 <= 2; ++e2)
      for (int64_t e3 = -1; e3 <= 2; ++e3)
        if (e1 >= 0 && e2 >= 0 && e3 >= 0)
          inside += 4;
  CHECK(d.enumeration.points.size() == inside);
  CHECK(d.enumeration.outsideDomain == 256 - inside);

  // A run that stops gives no domain at that point, and leaves the others.
  SemanticProgram bounds = m.import("out_of_bounds", true);
  CoordinateResult o = recover(bounds, range(bounds, 0, 1, 3));
  CHECK(o.enumeration.gate == GateStatus::Open);
  CHECK(o.enumeration.termination.kind() == V::ProvenTerminatingIfDefined);
  CHECK(o.points[0].groups[0].result.kind() == RecoveryStatus::CandidateFound);
  CHECK(o.points[1].groups[0].result.kind() == RecoveryStatus::CandidateFound);
  CHECK(o.enumeration.points[2].run == RunStatus::UndefinedBehavior);
  CHECK(!o.enumeration.points[2].domains[0].has_value());
  CHECK(o.points[2].groups[0].result.kind() == RecoveryStatus::Unknown);

  // Fuel and the event limit give nothing, never a partial table.
  TerminationResult proof = analyzeTermination(gemm, backend,
                                               terminationOptions(range(gemm, 0, 3, 3)));
  EnumerationOptions tight = enumerationOptions(range(gemm, 0, 3, 3));
  tight.limits.fuel = 8;
  DomainEnumeration fuel = enumerateDomain(gemm, &*proof.certificate, tight, backend);
  CHECK(fuel.gate == GateStatus::Open);
  CHECK(fuel.points[0].run == RunStatus::OutOfFuel && !fuel.points[0].domains[0]);
  CHECK(!fuel.points[0].reason.empty());
  EnumerationOptions fewEvents = enumerationOptions(range(gemm, 0, 3, 3));
  fewEvents.limits.maxEvents = 11;
  DomainEnumeration events = enumerateDomain(gemm, &*proof.certificate, fewEvents, backend);
  CHECK(events.points[0].run == RunStatus::Returned && !events.points[0].domains[0]);
  CHECK(events.points[0].reason.find("event limit") != std::string::npos);
  fewEvents.limits.maxEvents = 12; // Exactly the event count.
  CHECK(enumerateDomain(gemm, &*proof.certificate, fewEvents, backend).points[0].domains[0].has_value());
  EnumerationOptions fewCells = enumerationOptions(range(gemm, 0, 3, 3));
  fewCells.limits.maxCells = 16;
  DomainEnumeration cells = enumerateDomain(gemm, &*proof.certificate, fewCells, backend);
  CHECK(!cells.points[0].domains[0].has_value());
  // Refused before the memory is built, which is the point of the limit.
  CHECK(cells.points[0].reason.find("this point") != std::string::npos);

  // Two memory fills, one table: the access sequence ignores memory.
  EnumerationOptions zero = enumerationOptions(range(gemm, 0, 3, 3)), ones = zero;
  ones.referenceFill = 3;
  auto zeros = enumerateDomain(gemm, &*proof.certificate, zero, backend);
  auto threes = enumerateDomain(gemm, &*proof.certificate, ones, backend);
  const AccessTable &a = zeros.points[0].domains[0]->table();
  const AccessTable &b = threes.points[0].domains[0]->table();
  CHECK(a.events().size() == b.events().size());
  for (size_t e = 0; e < a.events().size(); ++e) {
    CHECK(a.events()[e].step == b.events()[e].step);
    CHECK(a.view(a.events()[e].h) == b.view(b.events()[e].h));
    CHECK(a.view(a.events()[e].f) == b.view(b.events()[e].f));
    CHECK(a.view(a.events()[e].g) == b.view(b.events()[e].g));
  }

  // Events keep execution order: k runs fastest in this nest.
  CHECK(a.events().size() == 12);
  CHECK(a.events()[0].step < a.events()[1].step);
  CHECK(a.view(a.events()[0].f) == IndexTuple({APInt(8, 0), APInt(8, 0)}));
  CHECK(a.view(a.events()[1].f) == IndexTuple({APInt(8, 0), APInt(8, 1)}));
  CHECK(a.view(a.events()[4].f) == IndexTuple({APInt(8, 1), APInt(8, 0)}));

  // Memoization changes no result: the two groups here have equal kernels.
  SemanticProgram two = m.import("two_groups", true);
  CoordinateResult memo = recover(two);
  CHECK(memo.points[0].groups.size() == 2);
  CHECK(memo.memoizedJoins == 1);
  CHECK(memo.points[0].groups[1].result.report().shape ==
        memo.points[0].groups[0].result.report().shape);
  CHECK(memo.points[0].groups[1].result.kind() == RecoveryStatus::CandidateFound);
}

static void testGroups() {
  Module m(programs);
  // Sites with one root triple share a group and keep their identity.
  SemanticProgram sites = m.import("two_sites", true);
  CHECK(sites.sites.size() == 2);
  CoordinateResult r = recover(sites);
  CHECK(r.enumeration.gate == GateStatus::Open);
  CHECK(r.enumeration.groups.size() == 1 && r.enumeration.groups[0].sites.size() == 2);
  const AccessTable &t = r.enumeration.points[0].domains[0]->table();
  std::set<uint32_t> seen;
  for (const AccessEvent &e : t.events())
    seen.insert(e.site);
  CHECK(seen.size() == 2 && t.events().size() == 8);
  CHECK(r.points[0].groups[0].result.kind() == RecoveryStatus::CandidateFound);
  CHECK(r.points[0].groups[0].result.report().genuine);

  // Different output roots stay apart, and no union is claimed.
  SemanticProgram two = m.import("two_groups", true);
  CoordinateResult g = recover(two);
  CHECK(g.enumeration.groups.size() == 2);
  CHECK(*g.enumeration.groups[0].output != *g.enumeration.groups[1].output);
  CHECK(g.enumeration.groups[0].first == g.enumeration.groups[1].first);
  CHECK(g.enumeration.points[0].domains[0]->table().events().size() == 2);
  CHECK(g.enumeration.points[0].domains[1]->table().events().size() == 2);

  // Each operand is part of the group's identity.
  for (const char *name : {"other_first", "other_second"}) {
    SemanticProgram p = m.import(name, true);
    CHECK(p.sites.size() == 2);
    CoordinateResult r = recover(p);
    CHECK(r.enumeration.gate == GateStatus::Open);
    CHECK(r.enumeration.groups.size() == 2);
    CHECK(*r.enumeration.groups[0].output == *r.enumeration.groups[1].output);
  }

  // Equal kernels of h are not equal tables: the memo needs all three views.
  SemanticProgram keys = m.import("equal_h_kernels", true);
  CoordinateResult k2 = recover(keys);
  CHECK(k2.enumeration.gate == GateStatus::Open && k2.points[0].groups.size() == 2);
  CHECK(k2.memoizedJoins == 0);
  CHECK(k2.points[0].groups[0].result.report().shape == ShapeClass::ElementWise);
  CHECK(k2.points[0].groups[1].result.report().shape == ShapeClass::ScaledVector);

  // C0 fails on an exact event: this is no += for any coordinates.
  SemanticProgram wrong = m.import("wrong_read", true);
  CoordinateResult w = recover(wrong);
  const GroupRecovery &bad = w.points[0].groups[0];
  CHECK(bad.result.kind() == RecoveryStatus::CandidateRefuted);
  CHECK(bad.result.reason().find("output_read_differs") != std::string::npos);
  CHECK(!bad.hints.has_value());

  // A read of another root is no reduction at any index, and C0 alone compares
  // indices. This program was accepted once.
  Module otherRoot(R"(
func.func @other(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %d: memref<2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<2xi32>
    %y = memref.load %b[%i] : memref<2xi32>
    %z = memref.load %d[%i] : memref<2xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<2xi32>
  }
  return
}
)");
  SemanticProgram other = otherRoot.import("other", true);
  CoordinateResult read = recover(other);
  CHECK(read.enumeration.gate == GateStatus::Unsupported);
  CHECK(read.enumeration.reason.find("root") != std::string::npos);
  CHECK(read.enumeration.groups.size() == 1);
  CHECK(read.points.empty());

  // A canonical failure is Unknown with hints: never a refutation of the program.
  Module failing(R"(
func.func @a_of_k(%a: memref<4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%k] : memref<4xi32>
        %y = memref.load %b[%k, %j] : memref<4x4xi32>
        %z = memref.load %c[%i, %j] : memref<4x4xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<4x4xi32>
      }
    }
  }
  return
}
)");
  SemanticProgram k = failing.import("a_of_k", true);
  CoordinateResult f = recover(k);
  const GroupRecovery &unknown = f.points[0].groups[0];
  CHECK(unknown.result.kind() == RecoveryStatus::Unknown);
  CHECK(unknown.result.reason().find("does not apply") != std::string::npos);
  CHECK(unknown.hints.has_value() && unknown.attempted.has_value());
  CHECK(!unknown.hints->meetH && unknown.hints->meetF && unknown.hints->meetG);
  CHECK(!unknown.candidate.has_value());
  CHECK(!unknown.attempted->dependenceH);

  // The JSON repeats that nothing here authorizes a transformation.
  std::string text = toJSON(k, f, /*events=*/true);
  CHECK(text.find("\"authorizes_lowering\": false") != std::string::npos);
  CHECK(text.find("\"status\": \"UNKNOWN\"") != std::string::npos);
  CHECK(text == toJSON(k, f, true));
  CHECK(toJSON(k, f, false).find("h_read") == std::string::npos);
}

int main() {
  mlir::MLIRContext local;
  configure(local);
  context = &local;
  testTables();
  testJoins();
  testRecovery();
  testDegenerateShapes();
  testPartialDomains();
  testNonInjective();
  testTrivialCandidates();
  testCounterexamples();
  testGates();
  testPoints();
  testGroups();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
