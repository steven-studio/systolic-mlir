//===- CoordinateModel.h - Exact finite-domain access data ------*- C++ -*-===//
//
// G3a data, with no reference to MLIR or to a solver. See
// docs/GENERALIZED_GEMM_G3A.md for the theorems.
//
// An AccessTable is one ordered event sequence E with its access tuples. T is
// the set of distinct triples (h, f, g), and a triple's multiplicity is how many
// events carry it. Events are never merged: order, repeats and site identity
// stay available, and T is an index over E.
//
// The interning here is memoization. It saves work and is evidence of nothing.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COORDINATEMODEL_H
#define GENERALIZEDGEMM_COORDINATEMODEL_H

#include "GeneralizedGemm/ProofStatus.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

using IndexTuple = std::vector<llvm::APInt>;

/// Dense ids for index tuples, in first-seen order. Width is part of a tuple's
/// identity, so equal bits at different widths stay different. At most 64 bits.
class TupleTable {
public:
  uint32_t intern(llvm::ArrayRef<llvm::APInt> tuple);
  const IndexTuple &operator[](uint32_t id) const { return tuples[id]; }
  uint32_t size() const { return tuples.size(); }

private:
  std::vector<IndexTuple> tuples;
  std::map<std::vector<uint64_t>, uint32_t> ids;
};

/// One dynamic MAC, as views into the table.
struct AccessEvent {
  uint32_t site = 0;
  uint64_t step = 0;
  uint32_t point = 0; // Iteration point: site plus the coordinates x.
  uint32_t triple = 0;
  uint32_t h = 0, hRead = 0, f = 0, g = 0;
};

struct AccessTriple {
  uint32_t h = 0, f = 0, g = 0;
};

class AccessTable {
public:
  void add(uint32_t site, uint64_t step, llvm::ArrayRef<llvm::APInt> x,
           llvm::ArrayRef<llvm::APInt> h, llvm::ArrayRef<llvm::APInt> hRead,
           llvm::ArrayRef<llvm::APInt> f, llvm::ArrayRef<llvm::APInt> g);

  const std::vector<AccessEvent> &events() const { return eventList; }
  const std::vector<AccessTriple> &triples() const { return tripleList; }
  /// Events per triple, parallel to triples().
  const std::vector<uint64_t> &multiplicity() const { return counts; }
  const IndexTuple &view(uint32_t id) const { return viewTable[id]; }
  /// The iteration point: its site first, then x.
  const IndexTuple &point(uint32_t id) const { return pointTable[id]; }
  uint32_t pointCount() const { return pointTable.size(); }

  /// The state variable behind each entry of x, when one list serves all sites.
  std::vector<unsigned> coordinateNames;
  bool coordinatesUniform = false;

private:
  std::vector<AccessEvent> eventList;
  std::vector<AccessTriple> tripleList;
  std::vector<uint64_t> counts;
  TupleTable viewTable, pointTable;
  std::map<std::array<uint32_t, 3>, uint32_t> tripleIds;
};

/// A partition as block labels, numbered by first occurrence, so two partitions
/// are equal exactly when their vectors are.
using Partition = std::vector<uint32_t>;
Partition normalizePartition(llvm::ArrayRef<uint32_t> labels);

/// Coordinates as labels per event. A label may stand for a tuple: nothing here
/// assumes a coordinate is one-dimensional. Untrusted: see CandidateChecker.h.
struct CoordinateCandidate {
  std::vector<uint32_t> p, i, j, k;
  std::string origin; // Who proposed it. Not evidence.
};

enum class CounterexampleKind {
  OutputReadDiffers, // C0: hRead != h on one event, `second`.
  NotAFunctionOfPoint,
  DependenceH, // Equal (P,I,J) with different h.
  DependenceF, // Equal (P,I,K) with different f.
  DependenceG, // Equal (P,K,J) with different g.
  ViewNotInjectiveH, // Equal h with different (P,I,J).
  ViewNotInjectiveF,
  ViewNotInjectiveG,
  MissingPoint,        // `labels` is absent from the image of (P,I,J,K).
  MultiplicityDiffers, // `first` and `second` have different multiplicities.
};
const char *stringifyCounterexampleKind(CounterexampleKind kind);

/// Exact, and the same on every run: the earliest `second` in event order, with
/// the first event that disagrees with it.
struct Counterexample {
  CounterexampleKind kind = CounterexampleKind::MissingPoint;
  uint64_t first = 0, second = 0; // Event positions.
  std::array<uint32_t, 4> labels{}; // (P, I, J, K), for MissingPoint.
};

enum class RecoveryStatus { CandidateFound, CandidateRefuted, Unsupported, Unknown };
const char *stringifyRecoveryStatus(RecoveryStatus status);

enum class CartesianForm { Box, Partial };
const char *stringifyCartesianForm(CartesianForm form);

enum class MultiplicityForm { Unit, Uniform, Nonuniform };
const char *stringifyMultiplicityForm(MultiplicityForm form);

/// A description of the batches, and never a judgement of validity.
enum class ShapeClass {
  ElementWise, // More than one batch, each 1 x 1 x 1.
  Scalar,      // One point.
  ScaledVector, Outer, Dot, GEMV, GEMM,
  Mixed, // The batches differ in class.
};
const char *stringifyShapeClass(ShapeClass shape);

/// Whether later phases may examine the candidate. There is deliberately no
/// value that authorizes a transformation: no G3a result can.
enum class Eligibility { NotEligible, ForLaterVerification };
const char *stringifyEligibility(Eligibility eligibility);

struct BatchShape {
  uint32_t batch = 0;
  uint64_t i = 0, j = 0, k = 0; // Axis cardinalities in this batch.
  uint64_t points = 0, events = 0;
  std::string missing; // Decimal: the product can exceed 64 bits.
};

/// Independent facts about one candidate. No field is derived from `genuine`,
/// and a false `genuine` does not make a candidate invalid.
struct CandidateReport {
  std::string malformed; // Nonempty: nothing below was computed.

  // Decomposition validity.
  bool outputReadMatches = false, functionOfPoint = false;
  bool dependenceH = false, dependenceF = false, dependenceG = false;
  bool decompositionValid = false;

  // Cartesian domain structure. Never inferred from the dependences.
  CartesianForm cartesian = CartesianForm::Partial;
  std::string missingPoints = "0";

  // Reduction multiplicity: events per value of (P,I,J,K), and per triple.
  MultiplicityForm multiplicity = MultiplicityForm::Nonuniform;
  uint64_t multiplicityMin = 0, multiplicityMax = 0;
  uint64_t tripleMultiplicityMin = 0, tripleMultiplicityMax = 0;
  uint64_t duplicateIterationPoints = 0; // Events beyond the first per point.

  // Shape and cardinalities.
  uint64_t events = 0, triples = 0, points = 0;
  uint64_t distinctP = 0, distinctI = 0, distinctJ = 0, distinctK = 0;
  std::vector<BatchShape> batches;
  ShapeClass shape = ShapeClass::Mixed;
  // The x entries, as positions, that determine a coordinate. Given P for I, J
  // and K. Empty when P alone suffices. Absent when not computed.
  std::optional<std::array<std::vector<unsigned>, 4>> support;

  // Operand-view injectivity.
  bool injectiveH = false, injectiveF = false, injectiveG = false;
  bool batchesShareViews = false; // A view value occurs in two batches.

  // Strict non-degeneracy: every batch has at least two points on each axis.
  bool strictNonDegenerate = false;
  // Valid, a box, unit multiplicity, injective views, strictly non-degenerate.
  bool genuine = false;
  // Valid, a box, unit multiplicity and strictly non-degenerate, with no demand
  // on the views. A view may ignore an axis: `genuine` is false for that, and
  // the decomposition is no less valid. Not printed by the G3a report.
  bool structurallyFull = false;
  // Theorem 2 applies: within injective-view boxes this is the only answer.
  bool uniqueUpToRelabelling = false;

  // Candidate transformation eligibility.
  Eligibility eligibility = Eligibility::NotEligible;
  std::vector<std::string> openObligations;

  // Invalidating ones first, then the informative ones.
  std::vector<Counterexample> counterexamples;
};

struct DomainAuthority; // Defined only where domains are enumerated.

/// One parameter point and one site group, exactly enumerated. Only the
/// enumerator constructs one, after the G1 and G2a gates.
class AccessDomain {
public:
  const std::string &fingerprint() const { return programFingerprint; }
  const std::vector<llvm::APInt> &parameters() const { return point; }
  const DomainEvidence &evidence() const { return domainEvidence; }
  const AccessTable &table() const { return accesses; }

private:
  friend struct DomainAuthority;
  AccessDomain(std::string fingerprint, std::vector<llvm::APInt> point,
               DomainEvidence evidence, AccessTable table)
      : programFingerprint(std::move(fingerprint)), point(std::move(point)),
        domainEvidence(std::move(evidence)), accesses(std::move(table)) {}
  std::string programFingerprint;
  std::vector<llvm::APInt> point;
  DomainEvidence domainEvidence;
  AccessTable accesses;
};

} // namespace mlir::generalized_gemm
#endif
