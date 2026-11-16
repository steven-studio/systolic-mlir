//===- ReductionSemantics.cpp ---------------------------------------------===//
#include "GeneralizedGemm/ReductionSemantics.h"

#include "GeneralizedGemm/FiniteDomain.h"
#include "GeneralizedGemm/SiteSets.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/bit.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <random>
#include <set>

using llvm::APInt;
using llvm::ArrayRef;

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

bool integerReorderConditionsOver(const SemanticProgram &program, ArrayRef<unsigned> outputs,
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
  // A partial sum can reach a stop or a guard through a load of an output root, or
  // through any state that holds a value from memory. Either may decide the run.
  std::vector<bool> independent = memoryIndependentState(program);
  auto loadsOutput = [&](Term term) {
    return llvm::any_of(outputs, [&](unsigned root) { return loadsFrom(term, root); });
  };
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
      if (loadsOutput(condition.violated) || throughState(condition.violated)) {
        why = "a stop reads the output or memory-derived state, so a partial sum can decide it";
        return false;
      }
  for (const Transition &t : program.transitions)
    if (loadsOutput(t.guard) || throughState(t.guard)) {
      why = "control reads the output or memory-derived state";
      return false;
    }
  return true;
}

bool integerReorderConditions(const SemanticProgram &program, const SiteGroup &group,
                              const CheckedComputation &computation, std::string &why) {
  unsigned root = *group.output;
  return integerReorderConditionsOver(program, root, computation, why);
}

std::optional<SiteGroup> unitedSiteGroup(const SemanticProgram &program, ArrayRef<const SiteGroup *> listed,
                                         std::vector<int> &groupOfSite, std::string &why) {
  GroupUnion gate;
  if (!uniteGroups(program, listed, gate, why))
    return std::nullopt;
  groupOfSite = std::move(gate.listedOf);
  SiteGroup united;
  united.sites = std::move(gate.sites);
  united.output = listed.front()->output;
  united.first = listed.front()->first;
  united.second = listed.front()->second;
  return united;
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
    // A NaN anywhere stops a run, so the forward fold is finite or infinite. A reversed
    // fold that is NaN would stop that run: the runs that complete differ, which refutes
    // as a changed value does, and is named (G5b-M4b-2b-1).
    if (isFloat && forward != backward) {
      // The model's formats, as evalFloatBinary chooses them: binary32 at 32 bits, else binary64.
      llvm::APFloat reversed(backward.getBitWidth() == 32 ? llvm::APFloat::IEEEsingle() : llvm::APFloat::IEEEdouble(),
                             backward);
      detail = "reversing " + cellName +
               (reversed.isNaN() ? " gives NaN, so that run would stop" : " changes its final bits");
      return true;
    }
  }
  return false;
}

namespace {

/// The search both witness functions run: the fixed fill sequence at `parameters`, each
/// fill judged by reversalDiffers against the replay's prefix constants. A fill that shows
/// no difference is no evidence; the detail says so.
Fact searchReversal(const SemanticProgram &program, const SiteGroup &group, const ReplayResult &replay,
                    const std::vector<APInt> &parameters, unsigned fills, uint64_t maxCells) {
  Fact out;
  out.detail = "no memory found at which a reversal differs";
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

} // namespace

Fact reorderWitness(const SemanticProgram &program, const SiteGroup &group,
                    const std::vector<APInt> &parameters, unsigned fills, uint64_t maxCells) {
  ReplayResult replay = replayGroup(program, group, parameters, HerbrandLimits());
  return searchReversal(program, group, replay, parameters, fills, maxCells);
}

Fact reorderWitnessOver(const SemanticProgram &program, ArrayRef<const SiteGroup *> listed,
                        const std::vector<APInt> &parameters, unsigned fills, uint64_t maxCells) {
  std::vector<int> groupOfSite;
  std::string why;
  std::optional<SiteGroup> united = unitedSiteGroup(program, listed, groupOfSite, why);
  if (!united) {
    Fact out;
    out.detail = why;
    return out;
  }
  ReplayResult replay =
      replayGroupSet(program, *united, groupOfSite, listed.size(), parameters, HerbrandLimits());
  return searchReversal(program, *united, replay, parameters, fills, maxCells);
}

// ---- G5b-M4b-2b-2: designed memories.

namespace {

/// The memory that is zero in every cell an entry sets, in the shapes randomEntry gives.
std::optional<EntryState> zeroEntry(const SemanticProgram &program, const std::vector<APInt> &parameters,
                                    uint64_t maxCells) {
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
    if (root.origin == MemoryRoot::FunctionArgument)
      data.cells.assign(cells, APInt(root.sort.width, 0));
    else
      data.cells.assign(cells, std::nullopt);
    out.memory.push_back(std::move(data));
  }
  return out;
}

/// A value of the model's float format at `width`, as bits: binary32 at 32 bits, else
/// binary64, as evalFloatBinary reads them.
APInt floatBits(unsigned width, double value) {
  if (width == 32)
    return APInt(32, (uint64_t)llvm::bit_cast<uint32_t>((float)value));
  return APInt(64, llvm::bit_cast<uint64_t>(value));
}

/// What a designed memory puts in the target cell and makes of its first and its last
/// contribution, each as bits at a root's width.
struct DesignedPattern {
  std::function<APInt(unsigned)> start, first, last, one;
};

std::vector<DesignedPattern> designedPatterns(bool isFloat, unsigned foldWidth) {
  if (!isFloat) {
    auto maximum = [](unsigned w) { return APInt::getSignedMaxValue(w); };
    auto minimum = [](unsigned w) { return APInt::getSignedMinValue(w); };
    auto plus = [](unsigned w) { return APInt(w, 1); };
    auto minus = [](unsigned w) { return APInt::getAllOnes(w); };
    // INT_MAX - 1 + 1 completes and INT_MAX + 1 overflows; INT_MIN + 1 - 1 completes and
    // INT_MIN - 1 overflows.
    return {{maximum, minus, plus, plus}, {minimum, plus, minus, plus}};
  }
  // 2^p is the first power of two whose successor the format cannot hold: 2^p + 1 rounds
  // back to 2^p (the tie goes to the even significand), so (2^p + 1) - 2^p is 0 and
  // (2^p - 2^p) + 1 is 1. The largest finite value added to itself is +infinity, which
  // stays through a finite addend, and less itself is 0.
  double big = foldWidth == 32 ? 16777216.0 : 9007199254740992.0;
  double largest = foldWidth == 32 ? (double)std::numeric_limits<float>::max() : std::numeric_limits<double>::max();
  auto value = [](double v) { return [v](unsigned w) { return floatBits(w, v); }; };
  return {{value(big), value(1.0), value(-big), value(1.0)},
          {value(largest), value(largest), value(-largest), value(1.0)}};
}

} // namespace

std::vector<EntryState> designedReversalFills(const SemanticProgram &program, const SiteGroup &group,
                                              const std::map<CellKey, APInt> &prefix,
                                              const std::vector<APInt> &parameters, uint64_t maxCells) {
  std::vector<EntryState> out;
  if (group.sites.empty())
    return out;
  // One operation, as the judge asks: where the sites differ it reverses nothing.
  const MacSite &leader = program.sites[group.sites.front()];
  for (unsigned id : group.sites) {
    const MacSite &site = program.sites[id];
    if (!site.output || site.reduction != leader.reduction || site.reductionFlags != leader.reductionFlags ||
        site.multiplyFlags != leader.multiplyFlags)
      return out;
  }
  bool isFloat = leader.reduction == ReductionOp::FAdd;
  unsigned foldWidth = program.roots[leader.output->root].sort.width;
  // Without nsw no reversal of an integer fold overflows. With nuw as well both patterns
  // stop the program itself: -1 is the largest unsigned value.
  if (!isFloat && (!(leader.reductionFlags & FlagNSW) || (leader.reductionFlags & FlagNUW)))
    return out;
  std::optional<EntryState> zero = zeroEntry(program, parameters, maxCells);
  if (!zero)
    return out;
  // The trace at this point, from the zero memory: which cells each event of the sites
  // writes and reads. Where that run stops, nothing is designed.
  RunResult trace = runConcrete(program, *zero);
  if (trace.status != RunStatus::Returned)
    return out;
  auto cellOf = [&](unsigned root, llvm::ArrayRef<APInt> index) {
    uint64_t at = 0;
    for (unsigned i = 0; i < index.size(); ++i)
      at = at * zero->memory[root].shape[i] + index[i].getZExtValue();
    return CellKey{root, at};
  };
  struct Contribution {
    CellKey a, b;
  };
  std::set<unsigned> members(group.sites.begin(), group.sites.end());
  std::vector<CellKey> written; // In the order of their first events.
  std::map<CellKey, std::vector<Contribution>> contributions;
  for (const MacEvent &event : trace.events) {
    if (!members.count(event.site))
      continue;
    const MacSite &site = program.sites[event.site];
    CellKey key = cellOf(site.output->root, event.h);
    auto [slot, added] = contributions.try_emplace(key);
    if (added)
      written.push_back(key);
    slot->second.push_back({cellOf(site.a.root, event.f), cellOf(site.b.root, event.g)});
  }
  auto settable = [&](const CellKey &key) {
    return program.roots[key.first].origin == MemoryRoot::FunctionArgument;
  };
  auto widthOf = [&](const CellKey &key) { return program.roots[key.first].sort.width; };

  std::vector<std::map<CellKey, APInt>> listed;
  unsigned targets = 0;
  for (const CellKey &target : written) {
    const std::vector<Contribution> &folded = contributions[target];
    // The cell's start must be the entry's to choose: no constant stored before its first
    // event. (A cell no entry sets is refused below, with the operands.)
    if (folded.size() < 2 || prefix.count(target))
      continue;
    if (targets++ == designedTargets)
      break;
    for (const DesignedPattern &pattern : designedPatterns(isFloat, foldWidth))
      for (bool firstInSecondOperand : {true, false})
        for (bool lastInSecondOperand : {true, false}) {
          std::map<CellKey, APInt> cells;
          bool consistent = true;
          auto put = [&](const CellKey &key, const std::function<APInt(unsigned)> &value) {
            if (!settable(key)) {
              consistent = false;
              return;
            }
            APInt bits = value(widthOf(key));
            auto [slot, added] = cells.try_emplace(key, bits);
            consistent = consistent && (added || slot->second == bits);
          };
          put(target, pattern.start);
          const Contribution &first = folded.front(), &last = folded.back();
          put(firstInSecondOperand ? first.b : first.a, pattern.first);
          put(firstInSecondOperand ? first.a : first.b, pattern.one);
          put(lastInSecondOperand ? last.b : last.a, pattern.last);
          put(lastInSecondOperand ? last.a : last.b, pattern.one);
          if (!consistent || std::find(listed.begin(), listed.end(), cells) != listed.end())
            continue;
          listed.push_back(cells);
          EntryState entry = *zero;
          for (const auto &[key, bits] : cells)
            entry.memory[key.first].cells[key.second] = bits;
          out.push_back(std::move(entry));
        }
  }
  return out;
}

namespace {

/// The designed memories judged in order against the replay's prefix constants. One that
/// shows no difference is no evidence, as with the fill sequence.
Fact searchDesigned(const SemanticProgram &program, const SiteGroup &group, const ReplayResult &replay,
                    const std::vector<APInt> &parameters, unsigned fills, uint64_t maxCells) {
  Fact out;
  out.detail = "no designed memory at which a reversal differs";
  if (!fills || replay.outcome != ReplayOutcome::Equal)
    return out;
  for (EntryState &designed : designedReversalFills(program, group, replay.prefix, parameters, maxCells)) {
    std::string detail;
    if (!reversalDiffers(program, group, designed, replay.prefix, detail))
      continue;
    out.status = FactStatus::Refuted;
    out.detail = detail + ", at a designed memory";
    out.point = parameters;
    out.memory = std::move(designed);
    return out;
  }
  return out;
}

} // namespace

Fact designedWitness(const SemanticProgram &program, const SiteGroup &group,
                     const std::vector<APInt> &parameters, unsigned fills, uint64_t maxCells) {
  ReplayResult replay = replayGroup(program, group, parameters, HerbrandLimits());
  return searchDesigned(program, group, replay, parameters, fills, maxCells);
}

Fact designedWitnessOver(const SemanticProgram &program, ArrayRef<const SiteGroup *> listed,
                         const std::vector<APInt> &parameters, unsigned fills, uint64_t maxCells) {
  std::vector<int> groupOfSite;
  std::string why;
  std::optional<SiteGroup> united = unitedSiteGroup(program, listed, groupOfSite, why);
  if (!united) {
    Fact out;
    out.detail = why;
    return out;
  }
  ReplayResult replay =
      replayGroupSet(program, *united, groupOfSite, listed.size(), parameters, HerbrandLimits());
  return searchDesigned(program, *united, replay, parameters, fills, maxCells);
}

} // namespace mlir::generalized_gemm
