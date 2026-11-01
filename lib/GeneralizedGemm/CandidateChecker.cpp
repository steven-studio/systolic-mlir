#include "GeneralizedGemm/CandidateChecker.h"

#include "llvm/ADT/StringExtras.h"

#include <algorithm>
#include <map>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

struct CandidateAuthority {
  static CheckedCandidate make(RecoveryStatus status, std::string reason,
                               const AccessDomain &domain, CandidateReport report) {
    CheckedCandidate result(status, std::move(reason));
    result.programFingerprint = domain.fingerprint();
    result.facts = std::move(report);
    return result;
  }
};

CheckedCandidate CheckedCandidate::unknown(std::string reason) {
  return CheckedCandidate(RecoveryStatus::Unknown, std::move(reason));
}

CheckedCandidate CheckedCandidate::unsupported(std::string reason) {
  return CheckedCandidate(RecoveryStatus::Unsupported, std::move(reason));
}

namespace {

/// Axis products can exceed 64 bits: three 32-bit cardinalities fit in 96.
APInt wide(uint64_t value) { return APInt(128, value); }

std::string decimal(const APInt &value) {
  return toString(value, 10, /*Signed=*/false);
}

using Labels = std::array<uint32_t, 4>; // P, I, J, K.

struct Batch {
  std::set<uint32_t> i, j, k;
  uint64_t points = 0, events = 0;
};

ShapeClass classify(uint64_t i, uint64_t j, uint64_t k) {
  if (i == 1 && j == 1 && k == 1)
    return ShapeClass::Scalar;
  if (k == 1)
    return i >= 2 && j >= 2 ? ShapeClass::Outer : ShapeClass::ScaledVector;
  if (i >= 2 && j >= 2)
    return ShapeClass::GEMM;
  return i == 1 && j == 1 ? ShapeClass::Dot : ShapeClass::GEMV;
}

class Inspection {
public:
  Inspection(const AccessTable &table, const CoordinateCandidate &candidate)
      : table(table), events(table.events()), c(candidate) {}

  CandidateReport run();

private:
  Labels labels(uint64_t e) const {
    return {c.p[e], c.i[e], c.j[e], c.k[e]};
  }
  void add(CounterexampleKind kind, uint64_t first, uint64_t second) {
    Counterexample found;
    found.kind = kind;
    found.first = first;
    found.second = second;
    out.counterexamples.push_back(found);
  }

  template <typename Key, typename Value>
  bool functional(Key key, Value value, CounterexampleKind kind);
  void checkValidity();
  void checkInjectivity();
  void measure();
  void classifyShape();
  void findSupport();
  void classifyResult();

  const AccessTable &table;
  const std::vector<AccessEvent> &events;
  const CoordinateCandidate &c;
  CandidateReport out;
  std::map<Labels, uint64_t> image; // Events per value of (P,I,J,K).
  std::map<uint32_t, Batch> perBatch;
};

/// Whether `value` is a function of `key` over the events. The first failure, in
/// event order, is the counterexample.
template <typename Key, typename Value>
bool Inspection::functional(Key key, Value value, CounterexampleKind kind) {
  using KeyType = decltype(key(uint64_t()));
  std::map<KeyType, uint64_t> first;
  for (uint64_t e = 0; e < events.size(); ++e) {
    auto [slot, added] = first.try_emplace(key(e), e);
    if (!added && value(slot->second) != value(e)) {
      add(kind, slot->second, e);
      return false;
    }
  }
  return true;
}

void Inspection::checkValidity() {
  out.outputReadMatches = true;
  for (uint64_t e = 0; e < events.size(); ++e)
    if (events[e].h != events[e].hRead) {
      out.outputReadMatches = false;
      add(CounterexampleKind::OutputReadDiffers, e, e);
      break;
    }
  out.functionOfPoint = functional(
      [&](uint64_t e) { return events[e].point; }, [&](uint64_t e) { return labels(e); },
      CounterexampleKind::NotAFunctionOfPoint);

  using Key = std::array<uint32_t, 3>;
  out.dependenceH = functional([&](uint64_t e) { return Key{c.p[e], c.i[e], c.j[e]}; },
                               [&](uint64_t e) { return events[e].h; },
                               CounterexampleKind::DependenceH);
  out.dependenceF = functional([&](uint64_t e) { return Key{c.p[e], c.i[e], c.k[e]}; },
                               [&](uint64_t e) { return events[e].f; },
                               CounterexampleKind::DependenceF);
  out.dependenceG = functional([&](uint64_t e) { return Key{c.p[e], c.k[e], c.j[e]}; },
                               [&](uint64_t e) { return events[e].g; },
                               CounterexampleKind::DependenceG);
  out.decompositionValid = out.outputReadMatches && out.functionOfPoint &&
                           out.dependenceH && out.dependenceF && out.dependenceG;
}

void Inspection::checkInjectivity() {
  using Value = std::array<uint32_t, 3>;
  out.injectiveH = functional([&](uint64_t e) { return events[e].h; },
                              [&](uint64_t e) { return Value{c.p[e], c.i[e], c.j[e]}; },
                              CounterexampleKind::ViewNotInjectiveH);
  out.injectiveF = functional([&](uint64_t e) { return events[e].f; },
                              [&](uint64_t e) { return Value{c.p[e], c.i[e], c.k[e]}; },
                              CounterexampleKind::ViewNotInjectiveF);
  out.injectiveG = functional([&](uint64_t e) { return events[e].g; },
                              [&](uint64_t e) { return Value{c.p[e], c.k[e], c.j[e]}; },
                              CounterexampleKind::ViewNotInjectiveG);
  // A batch that shares a view with another is not closed under the moves of
  // the accesses, so it is finer than the structure the accesses show.
  for (auto view : {&AccessEvent::h, &AccessEvent::f, &AccessEvent::g}) {
    std::map<uint32_t, uint32_t> batchOf;
    for (uint64_t e = 0; e < events.size(); ++e) {
      auto [slot, added] = batchOf.try_emplace(events[e].*view, c.p[e]);
      if (!added && slot->second != c.p[e])
        out.batchesShareViews = true;
    }
  }
}

void Inspection::measure() {
  out.events = events.size();
  out.triples = table.triples().size();
  out.duplicateIterationPoints = events.size() - table.pointCount();
  out.tripleMultiplicityMin = *std::min_element(table.multiplicity().begin(),
                                                table.multiplicity().end());
  out.tripleMultiplicityMax = *std::max_element(table.multiplicity().begin(),
                                                table.multiplicity().end());
  std::set<uint32_t> ps, is, js, ks;
  for (uint64_t e = 0; e < events.size(); ++e) {
    ++image[labels(e)];
    Batch &batch = perBatch[c.p[e]];
    batch.i.insert(c.i[e]);
    batch.j.insert(c.j[e]);
    batch.k.insert(c.k[e]);
    ++batch.events;
    ps.insert(c.p[e]);
    is.insert(c.i[e]);
    js.insert(c.j[e]);
    ks.insert(c.k[e]);
  }
  out.points = image.size();
  out.distinctP = ps.size();
  out.distinctI = is.size();
  out.distinctJ = js.size();
  out.distinctK = ks.size();
  for (const auto &[labelsOf, count] : image)
    ++perBatch[labelsOf[0]].points;

  // The image lies inside the product of the axes, so counting decides the box.
  // The first absent point is found within `points + 1` probes of the product.
  APInt totalMissing = wide(0);
  bool reported = false;
  for (const auto &[label, batch] : perBatch) {
    APInt product = wide(batch.i.size()) * wide(batch.j.size()) * wide(batch.k.size());
    APInt missing = product - wide(batch.points);
    totalMissing += missing;
    BatchShape shape;
    shape.batch = label;
    shape.i = batch.i.size();
    shape.j = batch.j.size();
    shape.k = batch.k.size();
    shape.points = batch.points;
    shape.events = batch.events;
    shape.missing = decimal(missing);
    out.batches.push_back(std::move(shape));
    if (missing.isZero() || reported)
      continue;
    reported = true;
    for (uint32_t i : batch.i)
      for (uint32_t j : batch.j)
        for (uint32_t k : batch.k)
          if (!image.count({label, i, j, k})) {
            Counterexample found;
            found.kind = CounterexampleKind::MissingPoint;
            found.labels = {label, i, j, k};
            out.counterexamples.push_back(found);
            goto probed;
          }
  probed:;
  }
  out.cartesian = totalMissing.isZero() ? CartesianForm::Box : CartesianForm::Partial;
  out.missingPoints = decimal(totalMissing);

  uint64_t low = UINT64_MAX, high = 0;
  for (const auto &[labelsOf, count] : image) {
    low = std::min(low, count);
    high = std::max(high, count);
  }
  out.multiplicityMin = low;
  out.multiplicityMax = high;
  out.multiplicity = high == 1      ? MultiplicityForm::Unit
                     : low == high ? MultiplicityForm::Uniform
                                   : MultiplicityForm::Nonuniform;
  if (low != high) {
    uint64_t reference = image[labels(0)];
    for (uint64_t e = 1; e < events.size(); ++e)
      if (image[labels(e)] != reference) {
        add(CounterexampleKind::MultiplicityDiffers, 0, e);
        break;
      }
  }
}

void Inspection::classifyShape() {
  bool strict = true, same = true;
  std::optional<ShapeClass> common;
  for (const BatchShape &batch : out.batches) {
    strict = strict && batch.i >= 2 && batch.j >= 2 && batch.k >= 2;
    ShapeClass shape = classify(batch.i, batch.j, batch.k);
    same = same && (!common || *common == shape);
    common = shape;
  }
  out.strictNonDegenerate = strict;
  if (!same)
    out.shape = ShapeClass::Mixed;
  else if (*common == ShapeClass::Scalar && out.batches.size() > 1)
    out.shape = ShapeClass::ElementWise;
  else
    out.shape = *common;
}

/// A description, for reading multidimensional labels: the fewest entries of x
/// that determine each coordinate, first by size and then in order.
void Inspection::findSupport() {
  if (!table.coordinatesUniform || !out.functionOfPoint)
    return;
  unsigned size = table.coordinateNames.size();
  constexpr unsigned MaxEntries = 12;
  constexpr uint64_t MaxWork = 1ull << 24;
  if (size > MaxEntries || ((uint64_t)table.pointCount() << size) > MaxWork)
    return;
  std::vector<uint32_t> masks(1u << size);
  for (uint32_t m = 0; m < masks.size(); ++m)
    masks[m] = m;
  std::stable_sort(masks.begin(), masks.end(), [](uint32_t a, uint32_t b) {
    return __builtin_popcount(a) < __builtin_popcount(b);
  });

  // One event per point: coordinates are functions of the point.
  std::vector<uint64_t> representative(table.pointCount(), UINT64_MAX);
  for (uint64_t e = 0; e < events.size(); ++e)
    if (representative[events[e].point] == UINT64_MAX)
      representative[events[e].point] = e;

  auto determined = [&](uint32_t mask, const std::vector<uint32_t> &target,
                        const std::vector<uint32_t> *given) {
    std::map<std::vector<uint64_t>, uint32_t> seen;
    for (uint64_t e : representative) {
      std::vector<uint64_t> key;
      key.push_back(given ? (*given)[e] : 0);
      const IndexTuple &point = table.point(events[e].point);
      for (unsigned entry = 0; entry < size; ++entry)
        if (mask >> entry & 1)
          key.push_back(point[entry + 1].getZExtValue());
      auto [slot, added] = seen.try_emplace(std::move(key), target[e]);
      if (!added && slot->second != target[e])
        return false;
    }
    return true;
  };
  // Nothing when x does not determine it, as when two sites share an x. An empty
  // list would say the coordinate needs no entry at all.
  auto smallest = [&](const std::vector<uint32_t> &target, const std::vector<uint32_t> *given)
      -> std::optional<std::vector<unsigned>> {
    for (uint32_t mask : masks)
      if (determined(mask, target, given)) {
        std::vector<unsigned> entries;
        for (unsigned entry = 0; entry < size; ++entry)
          if (mask >> entry & 1)
            entries.push_back(entry);
        return entries;
      }
    return std::nullopt;
  };
  auto p = smallest(c.p, nullptr), i = smallest(c.i, &c.p), j = smallest(c.j, &c.p),
       k = smallest(c.k, &c.p);
  if (p && i && j && k)
    out.support = std::array<std::vector<unsigned>, 4>{*p, *i, *j, *k};
}

void Inspection::classifyResult() {
  bool injective = out.injectiveH && out.injectiveF && out.injectiveG;
  bool box = out.cartesian == CartesianForm::Box;
  bool unit = out.multiplicity == MultiplicityForm::Unit;
  out.genuine = out.decompositionValid && box && unit && injective &&
                out.strictNonDegenerate;
  out.uniqueUpToRelabelling = out.decompositionValid && box && injective;
  if (!out.decompositionValid)
    return;
  // A degenerate shape is a fact to carry forward, and no reason to drop it.
  out.eligibility = Eligibility::ForLaterVerification;
  std::vector<std::string> &open = out.openObligations;
  open.push_back("equivalence: reduction order, floating point, aliasing, the "
                 "initial output and other writes are not examined");
  if (!box)
    open.push_back("the domain is not Cartesian: " + out.missingPoints +
                   " points are missing");
  if (!unit)
    open.push_back("the reduction multiplicity is not one");
  if (!out.injectiveH)
    open.push_back("the output view is not injective in (P,I,J)");
  if (!out.injectiveF)
    open.push_back("the first operand view is not injective in (P,I,K)");
  if (!out.injectiveG)
    open.push_back("the second operand view is not injective in (P,K,J)");
  if (out.batchesShareViews)
    open.push_back("two batches share a view value");
  if (!out.strictNonDegenerate)
    open.push_back(std::string("a degenerate shape: ") +
                   stringifyShapeClass(out.shape));
}

CandidateReport Inspection::run() {
  size_t size = events.size();
  if (!size) {
    out.malformed = "no events";
    return out;
  }
  if (c.p.size() != size || c.i.size() != size || c.j.size() != size ||
      c.k.size() != size) {
    out.malformed = "a coordinate does not label every event";
    return out;
  }
  checkValidity();
  checkInjectivity();
  measure();
  classifyShape();
  findSupport();
  classifyResult();
  return std::move(out);
}

} // namespace

CandidateReport inspectCandidate(const AccessTable &table,
                                 const CoordinateCandidate &candidate) {
  return Inspection(table, candidate).run();
}

CheckedCandidate checkCandidate(const AccessDomain &domain,
                                const CoordinateCandidate &candidate) {
  CandidateReport report = inspectCandidate(domain.table(), candidate);
  if (!report.malformed.empty())
    return CheckedCandidate::unknown(report.malformed);
  if (report.decompositionValid)
    return CandidateAuthority::make(RecoveryStatus::CandidateFound,
                                    "the accesses decompose through this candidate",
                                    domain, std::move(report));
  const Counterexample &failure = report.counterexamples.front();
  std::string reason = std::string(stringifyCounterexampleKind(failure.kind)) +
                       " at events " + std::to_string(failure.first) + " and " +
                       std::to_string(failure.second);
  return CandidateAuthority::make(RecoveryStatus::CandidateRefuted, std::move(reason),
                                  domain, std::move(report));
}

} // namespace mlir::generalized_gemm
