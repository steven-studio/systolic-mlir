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

ProofStep ProofStep::harden(std::size_t source) {
  ProofStep step;
  step.rule = Rule::Harden;
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
      text += step.row.kind == RowRef::Kind::Inequality         ? "ge"
              : step.row.kind == RowRef::Kind::StrictInequality ? "gt"
              : step.row.kind == RowRef::Kind::Equality         ? "eq"
              : step.row.kind == RowRef::Kind::Assumption       ? "case"
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
    case ProofStep::Rule::Harden:
      text += " harden ";
      text += std::to_string(step.source);
      break;
    }
    text += '\n';
  }
  return text;
}

std::string canonicalText(const BranchProof &proof) {
  std::string text = "exact-branch-proof 1\n";
  for (std::size_t i = 0; i < proof.nodes.size(); ++i) {
    const BranchNode &node = proof.nodes[i];
    text += "node ";
    text += std::to_string(i);
    if (node.kind == BranchNode::Kind::Leaf) {
      text += " leaf\n";
      text += canonicalText(node.leaf);
      continue;
    }
    text += " split";
    for (const Int &c : node.form) {
      text += ' ';
      text += toDecimal(c);
    }
    text += " ; ";
    text += toDecimal(node.bound);
    text += " low ";
    text += std::to_string(node.low);
    text += " high ";
    text += std::to_string(node.high);
    text += '\n';
  }
  return text;
}

} // namespace systolic::exact
