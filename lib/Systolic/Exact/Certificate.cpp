//===- Certificate.cpp - Witnesses and infeasibility proofs ---------------===//

#include "Systolic/Exact/Certificate.h"

namespace systolic::exact {

ProofStep ProofStep::input(RowRef row) {
  ProofStep step;
  step.rule = Rule::Input;
  step.row = row;
  return step;
}

ProofStep ProofStep::combine(std::vector<ProofTerm> terms) {
  ProofStep step;
  step.rule = Rule::Combine;
  step.terms = std::move(terms);
  return step;
}

ProofStep ProofStep::tighten(std::size_t source) {
  ProofStep step;
  step.rule = Rule::Tighten;
  step.source = source;
  return step;
}

std::string canonicalText(const InfeasibilityProof &proof) {
  std::string text = "exact-proof 1\n";
  for (std::size_t i = 0; i < proof.steps.size(); ++i) {
    const ProofStep &step = proof.steps[i];
    text += std::to_string(i);
    switch (step.rule) {
    case ProofStep::Rule::Input:
      text += " input ";
      text += step.row.kind == RowRef::Kind::Inequality ? "ge"
              : step.row.kind == RowRef::Kind::Equality ? "eq"
                                                        : "cong";
      text += ' ';
      text += std::to_string(step.row.index);
      text += step.row.negated ? " neg" : " pos";
      break;
    case ProofStep::Rule::Combine:
      text += " combine";
      for (const ProofTerm &term : step.terms) {
        text += ' ';
        text += toDecimal(term.multiplier);
        text += '*';
        text += std::to_string(term.step);
      }
      break;
    case ProofStep::Rule::Tighten:
      text += " tighten ";
      text += std::to_string(step.source);
      break;
    }
    text += '\n';
  }
  return text;
}

} // namespace systolic::exact
