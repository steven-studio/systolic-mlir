//===- ReductionSemantics.cpp ---------------------------------------------===//
#include "GeneralizedGemm/ReductionSemantics.h"

#include "GeneralizedGemm/FiniteDomain.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/bit.h"

#include <functional>
#include <random>

using llvm::APInt;

namespace mlir::generalized_gemm {

namespace {

/// Whether `term` loads from `root`, at any depth.
bool loadsFrom(Term term, unsigned root) {
  llvm::DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term n) {
    if (!seen.insert(n).second)
      return false;
    if (n->op == TermOp::Load) {
      Term memory = n->args[0];
      while (memory->op == TermOp::Store)
        memory = memory->args[0];
      if (memory->sort.root == root)
        return true;
    }
    return llvm::any_of(n->args, visit);
  };
  return visit(term);
}

std::optional<EntryState> randomEntry(const SemanticProgram &program,
                                      const std::vector<APInt> &parameters, uint64_t maxCells,
                                      std::mt19937_64 &random) {
  EntryState out;
  out.parameters = parameters;
  for (const MemoryRoot &root : program.roots) {
    RootData data;
    uint64_t cells = 1;
    for (unsigned d = 0; d < root.extents.size(); ++d) {
      int64_t extent = root.extents[d];
      if (extent < 0) {
        if (parameters[root.dimensionParameter[d]].isNegative())
          return std::nullopt;
        extent = parameters[root.dimensionParameter[d]].getZExtValue();
      }
      data.shape.push_back(extent);
      if (extent && cells > maxCells / extent)
        return std::nullopt;
      cells *= extent;
    }
    for (uint64_t c = 0; c < cells; ++c) {
      if (root.origin != MemoryRoot::FunctionArgument) {
        data.cells.emplace_back(std::nullopt);
        continue;
      }
      unsigned width = root.sort.width;
      if (!root.sort.elementIsFloat) {
        // Large values make an overflow likely, small ones a long sum without one.
        uint64_t raw = random() % 3 ? random() : random() % 8;
        data.cells.emplace_back(APInt(width, raw, /*isSigned=*/false, /*implicitTrunc=*/true));
        continue;
      }
      static const double pool[] = {0.0, -0.0, 1.0, -1.0, 0.5, 3.0, 1e16, -1e16, 1e-3,
                                    9007199254740992.0};
      double value = pool[random() % std::size(pool)];
      if (width == 32)
        data.cells.emplace_back(APInt(32, (uint64_t)llvm::bit_cast<uint32_t>((float)value)));
      else
        data.cells.emplace_back(APInt(64, llvm::bit_cast<uint64_t>(value)));
    }
    out.memory.push_back(std::move(data));
  }
  return out;
}

} // namespace

bool integerReorderConditions(const SemanticProgram &program, const SiteGroup &group,
                              const CheckedComputation &computation, std::string &why) {
  if (computation.status() != ComputationStatus::Verified) {
    why = "the computation is not verified";
    return false;
  }
  if (computation.fold() != FoldForm::ModularInteger) {
    why = "a float fold";
    return false;
  }
  for (const SiteContract &contract : computation.contracts())
    if (contract.reductionFlags & (FlagNSW | FlagNUW)) {
      why = "the reduction carries nsw or nuw, so a partial sum can stop the run";
      return false;
    }
  for (const ResidualEffect &effect : computation.frame())
    if (effect.observesOutput) {
      why = "another effect reads the output, so it can see a partial sum";
      return false;
    }
  // A partial sum can reach a stop or a guard through a load of the output, or
  // through any state that holds a value from memory. Either may decide the run.
  std::vector<bool> independent = memoryIndependentState(program);
  auto throughState = [&](Term term) {
    llvm::DenseSet<Term> seen;
    std::function<bool(Term)> visit = [&](Term n) {
      if (!seen.insert(n).second)
        return false;
      if ((n->op == TermOp::State || n->op == TermOp::Shadow) && !independent[n->index])
        return true;
      return llvm::any_of(n->args, visit);
    };
    return visit(term);
  };
  for (const Transition &t : program.transitions)
    for (const SideCondition &condition : t.sideConditions)
      if (loadsFrom(condition.violated, *group.output) || throughState(condition.violated)) {
        why = "a stop reads the output or memory-derived state, so a partial sum can decide it";
        return false;
      }
  for (const Transition &t : program.transitions)
    if (loadsFrom(t.guard, *group.output) || throughState(t.guard)) {
      why = "control reads the output or memory-derived state";
      return false;
    }
  return true;
}

bool reversalDiffers(const SemanticProgram &program, const SiteGroup &group,
                     const EntryState &entry, const std::map<CellKey, APInt> &prefix,
                     std::string &detail) {
  RunResult run = runConcrete(program, entry);
  if (run.status != RunStatus::Returned)
    return false;
  std::set<unsigned> members(group.sites.begin(), group.sites.end());
  // A reversal is a statement about one operation; sites that differ have none.
  for (unsigned id : group.sites) {
    const MacSite &site = program.sites[id], &first = program.sites[group.sites.front()];
    if (site.reduction != first.reduction || site.reductionFlags != first.reductionFlags ||
        site.multiplyFlags != first.multiplyFlags)
      return false;
  }
  auto offsetIn = [&](unsigned root, llvm::ArrayRef<APInt> key) {
    uint64_t at = 0;
    for (unsigned i = 0; i < key.size(); ++i)
      at = at * entry.memory[root].shape[i] + key[i].getZExtValue();
    return at;
  };
  // The contributions of each cell in order, and how each update combines them.
  struct Cell {
    APInt start;
    std::vector<APInt> products;
    const MacSite *site = nullptr;
  };
  std::map<CellKey, Cell> cells;
  for (const MacEvent &event : run.events) {
    if (!members.count(event.site))
      continue;
    const MacSite &site = program.sites[event.site];
    CellKey key{site.output->root, offsetIn(site.output->root, event.h)};
    auto [slot, added] = cells.try_emplace(key);
    if (added) {
      auto started = prefix.find(key);
      const auto &initial = entry.memory[key.first].cells[key.second];
      if (started != prefix.end())
        slot->second.start = started->second;
      else if (initial)
        slot->second.start = *initial;
      else
        return false;
      slot->second.site = &site;
    }
    bool isFloat = site.reduction == ReductionOp::FAdd;
    slot->second.products.push_back(isFloat ? evalFloatBinary(TermOp::FMul, event.aValue, event.bValue)
                                            : evalIntegerBinary(TermOp::Mul, event.aValue, event.bValue));
  }
  for (auto &[key, cell] : cells) {
    if (cell.products.size() < 2)
      continue;
    const MacSite &site = *cell.site;
    bool isFloat = site.reduction == ReductionOp::FAdd;
    auto fold = [&](bool reverse, bool &overflow) {
      APInt value = cell.start;
      overflow = false;
      for (size_t n = 0; n < cell.products.size(); ++n) {
        const APInt &p = cell.products[reverse ? cell.products.size() - 1 - n : n];
        if (isFloat) {
          value = evalFloatBinary(TermOp::FAdd, value, p);
          continue;
        }
        bool o = false;
        if (site.reductionFlags & FlagNSW) {
          (void)value.sadd_ov(p, o);
          overflow = overflow || o;
        }
        if (site.reductionFlags & FlagNUW) {
          (void)value.uadd_ov(p, o);
          overflow = overflow || o;
        }
        value = value + p;
      }
      return value;
    };
    bool forwardOverflow = false, backwardOverflow = false;
    APInt forward = fold(false, forwardOverflow), backward = fold(true, backwardOverflow);
    if (forwardOverflow)
      continue; // The run completed, so this cannot be; a mismatch says nothing here.
    std::string cellName = "cell " + std::to_string(key.second) + " of root " + std::to_string(key.first);
    if (!isFloat && backwardOverflow) {
      detail = "reversing " + cellName + " overflows a flagged addition";
      return true;
    }
    // A NaN anywhere stops a run, so only finite and infinite values reach here.
    if (isFloat && forward != backward) {
      detail = "reversing " + cellName + " changes its final bits";
      return true;
    }
  }
  return false;
}

Fact reorderWitness(const SemanticProgram &program, const SiteGroup &group,
                    const std::vector<APInt> &parameters, unsigned fills, uint64_t maxCells) {
  Fact out;
  out.detail = "no memory found at which a reversal differs";
  ReplayResult replay = replayGroup(program, group, parameters, HerbrandLimits());
  if (replay.outcome != ReplayOutcome::Equal) {
    out.detail = "the replay at this point is not equal";
    return out;
  }
  std::mt19937_64 random(20261104);
  for (unsigned n = 0; n < fills; ++n) {
    auto entry = randomEntry(program, parameters, maxCells, random);
    if (!entry)
      return out;
    std::string detail;
    if (reversalDiffers(program, group, *entry, replay.prefix, detail)) {
      out.status = FactStatus::Refuted;
      out.detail = detail;
      out.point = parameters;
      out.memory = std::move(*entry);
      return out;
    }
  }
  return out;
}

} // namespace mlir::generalized_gemm
