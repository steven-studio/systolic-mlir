#include "GeneralizedGemm/CoordinateModel.h"

#include <cassert>

using namespace llvm;

namespace mlir::generalized_gemm {

uint32_t TupleTable::intern(ArrayRef<APInt> tuple) {
  std::vector<uint64_t> key;
  key.reserve(tuple.size() * 2);
  for (const APInt &value : tuple) {
    assert(value.getBitWidth() <= 64 && "G1 refuses wider integers");
    key.push_back(value.getBitWidth());
    key.push_back(value.getZExtValue());
  }
  auto [slot, added] = ids.try_emplace(std::move(key), (uint32_t)tuples.size());
  if (added)
    tuples.emplace_back(tuple.begin(), tuple.end());
  return slot->second;
}

void AccessTable::add(uint32_t site, uint64_t step, ArrayRef<APInt> x,
                      ArrayRef<APInt> h, ArrayRef<APInt> hRead, ArrayRef<APInt> f,
                      ArrayRef<APInt> g) {
  AccessEvent event;
  event.site = site;
  event.step = step;
  IndexTuple key{APInt(32, site)};
  key.insert(key.end(), x.begin(), x.end());
  event.point = pointTable.intern(key);
  event.h = viewTable.intern(h);
  event.hRead = viewTable.intern(hRead);
  event.f = viewTable.intern(f);
  event.g = viewTable.intern(g);
  auto [slot, added] = tripleIds.try_emplace({event.h, event.f, event.g},
                                             (uint32_t)tripleList.size());
  if (added) {
    tripleList.push_back({event.h, event.f, event.g});
    counts.push_back(0);
  }
  event.triple = slot->second;
  ++counts[event.triple];
  eventList.push_back(event);
}

Partition normalizePartition(ArrayRef<uint32_t> labels) {
  std::map<uint32_t, uint32_t> renamed;
  Partition out;
  out.reserve(labels.size());
  for (uint32_t label : labels)
    out.push_back(renamed.try_emplace(label, (uint32_t)renamed.size()).first->second);
  return out;
}

const char *stringifyCounterexampleKind(CounterexampleKind kind) {
  switch (kind) {
  case CounterexampleKind::OutputReadDiffers: return "output_read_differs";
  case CounterexampleKind::NotAFunctionOfPoint: return "not_a_function_of_point";
  case CounterexampleKind::DependenceH: return "dependence_h";
  case CounterexampleKind::DependenceF: return "dependence_f";
  case CounterexampleKind::DependenceG: return "dependence_g";
  case CounterexampleKind::ViewNotInjectiveH: return "view_not_injective_h";
  case CounterexampleKind::ViewNotInjectiveF: return "view_not_injective_f";
  case CounterexampleKind::ViewNotInjectiveG: return "view_not_injective_g";
  case CounterexampleKind::MissingPoint: return "missing_point";
  case CounterexampleKind::MultiplicityDiffers: return "multiplicity_differs";
  }
  return "unknown";
}

const char *stringifyRecoveryStatus(RecoveryStatus status) {
  switch (status) {
  case RecoveryStatus::CandidateFound: return "CANDIDATE_FOUND";
  case RecoveryStatus::CandidateRefuted: return "CANDIDATE_REFUTED";
  case RecoveryStatus::Unsupported: return "UNSUPPORTED";
  case RecoveryStatus::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

const char *stringifyCartesianForm(CartesianForm form) {
  return form == CartesianForm::Box ? "box" : "partial";
}

const char *stringifyMultiplicityForm(MultiplicityForm form) {
  switch (form) {
  case MultiplicityForm::Unit: return "unit";
  case MultiplicityForm::Uniform: return "uniform";
  case MultiplicityForm::Nonuniform: return "nonuniform";
  }
  return "nonuniform";
}

const char *stringifyShapeClass(ShapeClass shape) {
  switch (shape) {
  case ShapeClass::ElementWise: return "element_wise";
  case ShapeClass::Scalar: return "scalar";
  case ShapeClass::ScaledVector: return "scaled_vector";
  case ShapeClass::Outer: return "outer";
  case ShapeClass::Dot: return "dot";
  case ShapeClass::GEMV: return "gemv";
  case ShapeClass::GEMM: return "gemm";
  case ShapeClass::Mixed: return "mixed";
  }
  return "mixed";
}

const char *stringifyEligibility(Eligibility eligibility) {
  return eligibility == Eligibility::ForLaterVerification ? "for_later_verification"
                                                          : "not_eligible";
}

} // namespace mlir::generalized_gemm
