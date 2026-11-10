#!/usr/bin/env python3
"""G4a mutation test. Each mutant weakens one thing the checker relies on, and the
unmodified tree must pass first. Every result is one of:

  KILLED          a test failed by its own assertion. Nothing else is a kill.
  SURVIVED        every test ran to the end and passed.
  EQUIVALENT      cannot be killed: listed below, with the reason and the evidence.
  TIMEOUT         a test ran out of time and no other test killed it.
  RESOURCE_LIMIT  a test ran out of memory and no other test killed it.
  HARNESS_ERROR   a stale pattern, a mutant that does not build, or a crash that no
                  assertion explains.

Only KILLED and EQUIVALENT let a run pass. Each test runs in its own process group under
a wall-clock limit that kills the whole group, and under an address-space limit that its
descendants inherit. There is at most one worker by default and never more than two.

Results are appended to --results as they finish, with the hash of the tree they are
about. A run skips mutants that file already holds for this tree, and refuses a file
about another tree, so an interrupted run loses nothing and mixes nothing. A scratch
directory holding a snapshot of another tree is refused too.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import resource
import shutil
import signal
import subprocess
import sys

C = "ComputationChecker.cpp"
H = "HerbrandReplay.cpp"
A = "ComputationAnalysis.cpp"
Y = "SymbolicDomain.cpp"
T = "SiteSets.cpp"  # G5b-M4-0: the group gate and root separation, shared.

MUTANTS = [
    # The finite route.
    ("mismatch accepted", C, "    if (replay.outcome == ReplayOutcome::Equal) {\n", "    if (replay.outcome == ReplayOutcome::Equal || replay.outcome == ReplayOutcome::Mismatch) {\n"),
    ("unobservable accepted", C, "    if (replay.outcome == ReplayOutcome::Equal) {\n", "    if (replay.outcome == ReplayOutcome::Equal || replay.outcome == ReplayOutcome::Unobservable) {\n"),
    ("limit accepted", C, "    if (replay.outcome == ReplayOutcome::Equal) {\n", "    if (replay.outcome == ReplayOutcome::Equal || replay.outcome == ReplayOutcome::Limit) {\n"),
    ("mismatch refutes alone", C, "      auto difference = replayDifference(program, group, *entry, run, replay.prefix);\n", "      std::optional<std::string> difference = std::string(\"nodes differ\");\n"),
    ("no witness search", C, "for (unsigned n = 0; n < options.limits.witnessFills; ++n) {", "for (unsigned n = 0; n < 0u; ++n) {"),
    ("stopped point ignored", C, "    if (!point.reason.empty()) {\n      complete = false;\n      continue;\n    }\n", "    if (!point.reason.empty())\n      continue;\n"),
    # Re-anchored in G5b-M3: G5a replaced `!enumeration.sampled` with the scope check.
    ("sample called a scope", C, "bool complete = enumerationCoversScope(program, enumeration, options.scope), stopsSeen = false;", "bool complete = true, stopsSeen = false;"),
    ("open point called a scope", C, "    complete = false;\n    if (open.empty())\n", "    if (open.empty())\n"),
    ("no events accepted", C, "      if (replay.facts.events)\n        facts.push_back(std::move(replay.facts));\n", "      facts.push_back(std::move(replay.facts));\n"),
    ("closed gate checked", C, "  if (enumeration.gate != GateStatus::Open)\n    return CheckedComputation::unknown(\"the domain gate is closed: \" + enumeration.reason);\n", ""),
    ("every run regardless of stops", C, "TerminationVerdict::ProvenTerminating &&\n                                   !stopsSeen);", "TerminationVerdict::ProvenTerminating);"),
    # Separation.
    ("any two roots are apart", T, "  } else if (authorized.count(out.name)) {\n", "  } else if (true) {\n"),
    ("noalias is enough", T, "  } else if (authorized.count(out.name)) {\n", "  } else if (authorized.count(out.name) || a.restrictArgument || b.restrictArgument) {\n"),
    ("fresh hides as an assumption", T, "    out.assumption = false;\n", ""),
    ("output not separated", C, "written, accessed{*group.output, group.first, group.second};\n", "written, accessed{group.first, group.second};\n"),
    ("other reads not separated", C, "      accessed.insert(effect.root);\n", ""),
    ("only the output's writes", C, "      if (effect.kind == MemoryEffect::Write)\n        written.insert(effect.root);\n", "      if (effect.kind == MemoryEffect::Write && effect.root == *group.output)\n        written.insert(effect.root);\n"),
    ("separations dropped", C, "  ComputationAuthority::contracts(out, std::move(contracts), foldOf(program, group));\n  ComputationAuthority::separations(out, std::move(separations));\n", "  ComputationAuthority::contracts(out, std::move(contracts), foldOf(program, group));\n"),
    ("symbolic separations dropped", C, "    ComputationAuthority::records(out, std::move(proof.records));\n    ComputationAuthority::separations(out, std::move(separations));\n", "    ComputationAuthority::records(out, std::move(proof.records));\n"),
    # Gates.
    ("part of a group", T, "    if (same != (listed.count(site.id) != 0)) {\n", "    if (false) {\n"),
    ("unsupported group", T, "  if (!group.unsupported.empty()) {\n    why = group.unsupported;\n    return false;\n  }\n", ""),
    ("oblivious unchecked", C, "  if (!program.controlOblivious || !program.addressOblivious) {\n    why = \"the trace depends on memory contents\";\n    return false;\n  }\n", ""),
    ("zero limit accepted", C, "  if (solver.timeoutMilliseconds && solver.resourceLimit)\n", "  if (true)\n"),
    ("either limit enough", C, "  if (solver.timeoutMilliseconds && solver.resourceLimit)\n", "  if (solver.timeoutMilliseconds || solver.resourceLimit)\n"),
    ("certificate not needed", C, "  if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating &&\n      outcome.verdict.kind() != TerminationVerdict::ProvenTerminatingIfDefined) {\n", "  if (false) {\n"),
    ("fresh root admitted", C, "program.roots[root].origin == MemoryRoot::FunctionArgument))\n      return false;\n", "true))\n      return false;\n"),
    # Obligations.
    ("E0 skipped", C, "    if (!structural(\"E0\", \"block \" + std::to_string(t.from), same))\n      return false;\n", ""),
    ("E1 skipped", C, "  if (!structural(\"E1\", where, storeOk))\n    return false;\n", "  if (!block || site.effectIndex >= block->effects.size())\n    return false;\n"),
    ("E2 group skipped", C, "owned.insert({site.location, site.effectIndex}).second))", "(owned.insert({site.location, site.effectIndex}), true)))"),
    ("E2 other sites skipped", C, "!owned.count({site.location, site.effectIndex})))", "true))"),
    ("R1 stored value", C, "store.value == site.updateValue && updateShape(site).ok", "updateShape(site).ok"),
    ("R1 shape", C, "store.value == site.updateValue && updateShape(site).ok", "store.value == site.updateValue"),
    ("flags of the update", C, "update->op != add || update->aux != site.reductionFlags ||\n      update->args.size() != 2", "update->op != add ||\n      update->args.size() != 2"),
    ("flags of the product", C, "product->op == mul && product->aux == site.multiplyFlags &&", "product->op == mul &&"),
    ("OP1 skipped", C, "  if (!structural(\"OP1\", where, readA && readB))\n    return false;\n", ""),
    ("load of another memory", C, "    if (terms.load(memoryBefore(program, block, access.root, e), access.index) == value)\n      return e;\n", "    return e;\n"),
    ("W1r skipped", C, "  if (!structural(\"W1r\", where, readC && site.outputRead->root == site.output->root &&\n                                    *readC < site.effectIndex))\n    return false;\n", ""),
    ("W1 query skipped", C, "      if (!require(\"W1\", where, terms.logicalAnd(a.premise, differs)))\n        return false;\n", "      (void)differs;\n"),
    ("W2 identical index passes", C, "    if (!structural(\"W2i\", where, !sameTerms(between.index, site.output->index)))\n      return false;\n", ""),
    ("W2 query skipped", C, "      if (!require(\"W2\", where, same))\n        return false;\n", "      (void)same;\n"),
    ("OP2 own writes skipped", C, "for (const std::vector<Access> *writes : {&ownWrites, &otherWrites})", "for (const std::vector<Access> *writes : {&otherWrites})"),
    ("OP2 other writes skipped", C, "for (const std::vector<Access> *writes : {&ownWrites, &otherWrites})", "for (const std::vector<Access> *writes : {&ownWrites})"),
    ("F1 skipped", C, "      if (!apart(\"F1\", \"write at \" + std::to_string(write.location) + \", output at \" +\n                           std::to_string(own.location),\n                 write, own))\n        return false;\n", "      (void)write, (void)own;\n"),
    ("operand a unchecked", C, "    operands.push_back({site.location, 0, site.a.root, site.a.index});\n", ""),
    ("operand b unchecked", C, "    operands.push_back({site.location, 0, site.b.root, site.b.index});\n", ""),
    ("F2 skipped", C, "  if (!returnsAreFree())\n    return false;\n\n  // NV", "  // NV"),
    ("F2 ignores a load", C, "    if (term->op == TermOp::Load || term->op == TermOp::MemVar)\n      return false;\n", ""),
    ("F2 ignores state", C, "    if ((term->op == TermOp::State || term->op == TermOp::Shadow) && !independent[term->index])\n      return false;\n", ""),
    ("pair limit ignored", C, "  if (pairs > options.limits.maxPairs) {", "  if (false) {"),
    ("query limit ignored", C, "  if (queries >= options.limits.maxQueries) {", "  if (false) {"),
    ("unknown discharges", C, "  SmtAnswer answer = ask(id, where, formula);\n  if (answer == SmtAnswer::Unsat)\n    return true;\n", "  SmtAnswer answer = ask(id, where, formula);\n  if (answer != SmtAnswer::Sat)\n    return true;\n"),
    ("sat discharges", C, "  SmtAnswer answer = ask(id, where, formula);\n  if (answer == SmtAnswer::Unsat)\n    return true;\n", "  SmtAnswer answer = ask(id, where, formula);\n  if (answer != SmtAnswer::Unknown)\n    return true;\n"),
    ("copies share a state", C, "secondCopy(program, atPathEnd(program, b.path, second[n]))));\n  return all;", "atPathEnd(program, b.path, second[n])));\n  return all;"),
    ("every run regardless of memory stops", C, "TerminationVerdict::ProvenTerminating &&\n                                         stops.empty());", "TerminationVerdict::ProvenTerminating);"),
    ("frame loses writes", C, "      if (ownEffects.count({t.from, e}))\n        continue;\n", "      if (effect.kind == MemoryEffect::Write || ownEffects.count({t.from, e}))\n        continue;\n"),
    ("frame loses reads", C, "      if (ownEffects.count({t.from, e}))\n        continue;\n", "      if (effect.kind == MemoryEffect::Read || ownEffects.count({t.from, e}))\n        continue;\n"),
    ("frame merges equal loads", C, "      if (auto read = readOf(program, *block, value, access))\n        ownEffects.insert({site.location, *read});\n", "      for (unsigned e = 0; e < block->effects.size(); ++e)\n        if (block->effects[e].kind == MemoryEffect::Read && block->effects[e].value == value)\n          ownEffects.insert({site.location, e});\n      (void)access;\n"),
    ("stops through state missed", C, "      if (dependsOnMemory(condition.violated, independent) || containsFloat(condition.violated))\n", "      if (readsMemory(condition.violated) || containsFloat(condition.violated))\n"),
    ("vacuous claim passes", C, "  return structural(\"NV\", \"a completing run with an event\", hasEventfulPoint());\n", "  return true;\n"),
    ("a run with no event counts", C, "    if (replay.outcome == ReplayOutcome::Equal && replay.facts.events)\n      return true;\n", "    if (replay.outcome == ReplayOutcome::Equal)\n      return true;\n"),
    ("a stopped point counts", C, "    if (replay.outcome == ReplayOutcome::Equal && replay.facts.events)\n      return true;\n", "    if (replay.facts.events)\n      return true;\n"),
    ("unbound finite claim", C, "      finiteClaimOfGroup.binding() == binding ? finiteClaimOfGroup : unbound;\n", "      finiteClaimOfGroup;\n"),
    ("unbound symbolic claim", C, "      symbolicClaimOfGroup.binding() == binding ? symbolicClaimOfGroup : unbound;\n", "      symbolicClaimOfGroup;\n"),
    ("binding ignores the scope", C, "  for (const ParameterRange &range : options.scope.ranges)\n    out += std::to_string(range.parameter)", "  for (const ParameterRange &range : llvm::ArrayRef<ParameterRange>())\n    out += std::to_string(range.parameter)"),
    ("binding ignores authorizations", C, "  for (const std::string &name : options.authorized)\n    out += \"|\" + name;\n", ""),
    ("short key passes", C, "              if (!kept)\n                return ViewInjectivity::NotProven;\n              list.push_back(kept);\n", "              if (kept)\n                list.push_back(kept);\n"),
    ("frame hides observers", C, "        residual.observesOutput = observed.count(effect.root) != 0;\n", ""),
    ("frame hides value source", C, "        residual.valueReadsMemory = dependsOnMemory(effect.value, independent);\n", "        residual.valueReadsMemory = false;\n"),
    # Witnesses.
    ("difference ignores operands", C, "    if (event.aValue != *a0 || event.bValue != *b0)\n      note(name + \" reads an operand that is not its initial value\");\n", ""),
    ("difference ignores events", C, "    if (event.updateValue != next)\n      note(name + \" writes a value other than the replay's\");\n", ""),
    ("difference ignores final cells", C, "    if (!final || *final != value)\n      note(\"a final cell differs from the replay\");\n", "    (void)final, (void)value;\n"),
    ("difference ignores the prefix", C, "    else if (started != prefix.end())\n      current = started->second;\n", ""),
    ("difference of a stopped run", C, "  if (run.status != RunStatus::Returned)\n    return std::nullopt;\n", ""),
    # Candidates.
    ("candidate on a lower coordinate tier", C, "  if (coordinateSymbolicOk && symbolic.tier() == ComputationTier::Symbolic &&\n", "  if (symbolic.tier() == ComputationTier::Symbolic &&\n"),
    ("candidate on a lower computation tier", C, "  if (coordinateSymbolicOk && symbolic.tier() == ComputationTier::Symbolic &&\n      symbolic.status() == ComputationStatus::Verified) {", "  if (coordinateSymbolicOk) {"),
    ("refuted group passes", C, "  if (finite.status() == ComputationStatus::Refuted ||\n      symbolic.status() == ComputationStatus::Refuted) {", "  if (false) {"),
    ("finite candidate unchecked", C, "             (coordinateFiniteOk || coordinateSymbolicOk)) {", "             true) {"),
    ("no shared points needed", C, "      if (coordinateSymbolicOk || covered.count(key(facts.parameters)))\n", ""),
    ("point called a scope", C, "    bool whole = finite.tier() == ComputationTier::ScopeEnumerated &&\n                 (coordinateSymbolicOk ||\n                  out.coordinateFinite.tier() == EvidenceTier::ScopeEnumerated);\n", "    bool whole = true;\n"),
    # The replay.
    ("operands not compared", H, "    if (*a != *a0 || *b != *b0)\n      mismatch(\"an operand read is not the initial value of its cell\");\n", ""),
    ("events not compared", H, "    if (next != *written)\n      mismatch(\"event \" + std::to_string(out.facts.events) +\n               \" writes a value that is not the replay's next value\");\n", ""),
    ("final cells not compared", H, "    if (memory[cell.first][cell.second] != replay[cell] && pending.empty())\n      pending = \"a cell's final value is not the replay's\";\n", ""),
    ("C0 not compared", H, "  if (site.outputRead->root != site.output->root || *cellRead != *cellH)\n    mismatch(\"the update reads a cell it does not write\");\n", ""),
    ("first mismatch stops the run", H, "    if (pending.empty())\n      pending = std::move(why);\n", "    pending = std::move(why);\n    throw LimitReached{pending};\n"),
    ("constant may follow an event", H, "  if (folds.count(cell))\n    return;\n", ""),
    ("a value starts a fold", H, "  if (table.isConstant(value)) {\n    replay[cell] = value;\n", "  if (true) {\n    replay[cell] = value;\n"),
    ("replay kept past a value", H, "    replay.erase(cell);\n    out.prefix.erase(cell);\n", ""),
    ("prefix after the block", H, "      foreignWrite({effect.root, *at}, *written);\n      continue;\n", "      continue;\n"),
    ("flags not in a node", H, "std::vector<uint64_t> key{3, (uint64_t)op, aux, (uint64_t)sort.kind, sort.width};", "std::vector<uint64_t> key{3, (uint64_t)op, 0, (uint64_t)sort.kind, sort.width};"),
    ("width not in a node", H, "std::vector<uint64_t> key{3, (uint64_t)op, aux, (uint64_t)sort.kind, sort.width};", "std::vector<uint64_t> key{3, (uint64_t)op, aux, (uint64_t)sort.kind, 0};"),
    ("operands sorted", H, "    key.insert(key.end(), args.begin(), args.end());\n", "    std::vector<NodeId> sorted(args.begin(), args.end());\n    std::sort(sorted.begin(), sorted.end());\n    key.insert(key.end(), sorted.begin(), sorted.end());\n"),
    ("constant width lost", H, "std::vector<uint64_t> key{1, (uint64_t)isFloat, value.getBitWidth()};", "std::vector<uint64_t> key{1, (uint64_t)isFloat, 0};"),
    ("wide constants truncated", H, "    key.insert(key.end(), value.getRawData(), value.getRawData() + value.getNumWords());\n", "    key.push_back(value.getRawData()[0]);\n"),
    ("effect kind unchecked", H, "      if (site.effectIndex >= t->effects.size() ||\n          t->effects[site.effectIndex].kind != MemoryEffect::Write) {", "      if (false) {"),
    ("guard from memory taken", H, "    if (!table.isConstant(*guard)) {\n      finish(ReplayOutcome::Refused, \"a guard depends on memory\");\n      return false;\n    }\n    if (table[*guard].value.isOne()) {", "    if (!table.isConstant(*guard) || table[*guard].value.isOne()) {"),
    ("float parameter is a value", H, "    return table.leaf(FloatParameter, t->index, 0);\n", "    return constant(parameters[t->index]);\n"),
    ("uninitialized is a leaf", H, "    if (root.origin == MemoryRoot::FunctionArgument)\n      for (uint64_t c = 0; c < cells; ++c)\n        contents[c] = table.leaf(InitialCell, root.id, c);\n", "    for (uint64_t c = 0; c < cells; ++c)\n      contents[c] = table.leaf(InitialCell, root.id, c);\n"),
    ("store chain skipped", H, "    if (same)\n      return value(chain->args[1]);\n", "    (void)same;\n"),
    ("return from memory passes", H, "    if (!table.isConstant(*node)) {\n      finish(ReplayOutcome::Unobservable, \"a return value depends on memory\");\n      return false;\n    }\n", ""),
    ("certain stop passes", H, "      if (table[*violated].value.isOne()) {\n        finish(ReplayOutcome::NoCompletingRun, condition.reason);\n        return false;\n      }\n", ""),
    ("conditional stops unreported", H, "      out.conditionalStops.insert({condition.kind, condition.reason});\n", ""),
    ("step limit ignored", H, "    if (out.facts.steps >= limits.maxSteps) {", "    if (false) {"),
    ("node limit ignored", H, "    if (nodes.size() > limit)\n      throw LimitReached{\"the node limit\"};\n", ""),
    ("cell limit ignored", H, "      if (extent && cells > limits.maxCells / extent) {", "      if (false) {"),
    ("multiplicity not counted", H, "++folds[cell]);\n  ++out.facts.events;", "folds[cell] = 1);\n  ++out.facts.events;"),
    # The pipeline and the reach.
    ("a lead is not run", A, "    if (group.finite.status() != ComputationStatus::Refuted && group.symbolic.failingPoint()) {", "    if (false) {"),
    ("a lead replaces a finite claim", A, "      if (lead.status() == ComputationStatus::Refuted)\n        group.finite = std::move(lead);\n", "      group.finite = std::move(lead);\n"),
    ("locations not reached", Y, "  for (unsigned location : locations)\n    targets.push_back({location, location});\n", ""),
    # G5b-M3: claims over sets of groups.
    ("set of part of a group", T, "    if (!wholeGroup(program, group, why))\n      return false;\n", ""),
    ("set lists a group twice", T, "    if (!triples.insert({*group.output, group.first, group.second}).second) {\n      why = \"a group is listed twice\";\n      return false;\n    }\n", "    triples.insert({*group.output, group.first, group.second});\n"),
    ("set lists a site twice", T, "      if (united.listedOf[site] != -1) {\n        why = \"a site is listed twice\";\n        return false;\n      }\n", ""),
    ("set of more than 64", T, "  if (listed.size() > 64) {", "  if (false) {"),
    ("set outputs unrecorded", C, "    set.outputs.insert(*group->output);\n", ""),
    ("set replayed as a group", C, "        set ? replayGroupSet(program, group, set->groupOf, set->groups.size(), point.parameters,", "        false ? replayGroupSet(program, group, set->groupOf, set->groups.size(), point.parameters,"),
    ("set points unobserved", C, "    if (set)\n      set->observe(point.parameters, replay);\n", ""),
    ("order violation unseen", C, "    if (!replay.listedOrder && !orderBroken)\n      orderBroken = parameters;\n", ""),
    ("shared cell unseen", C, "    if (replay.sharedCells && !cellShared)\n      cellShared = parameters;\n", ""),
    ("mismatch points listed", C, "    if (replay.outcome == ReplayOutcome::Equal && replay.facts.events)\n      points.push_back(", "    if (replay.facts.events)\n      points.push_back("),
    ("finite fact refutation dropped", C, "  if (broken)\n    return refutedFact(*broken, refuted);\n", ""),
    ("finite fact without a claim", C, "  if (claim.status() == ComputationStatus::Verified)\n    return provenFact(claim.tier(), proven);", "  if (true)\n    return provenFact(claim.tier(), proven);"),
    ("fold of the first site", C, "  bool modular = llvm::all_of(group.sites, [&](unsigned id) {", "  bool modular = llvm::any_of(group.sites, [&](unsigned id) {"),
    ("frame observes the first output", C, "  std::set<unsigned> observed = outputs ? *outputs : std::set<unsigned>{*group.output};", "  std::set<unsigned> observed = std::set<unsigned>{*group.output};"),
    ("F3 for no listed group", C, "    for (const SiteGroup *listed : ordered)\n      roots.insert(roots.end(), {*listed->output, listed->first, listed->second});\n", ""),
    ("F3 in the listed order", C, "    std::sort(ordered.begin(), ordered.end(), [](const SiteGroup *x, const SiteGroup *y) {\n      return x->sites.front() < y->sites.front();\n    });\n", ""),
    ("union in the listed order", T, "  std::sort(united.sites.begin(), united.sites.end());\n", ""),
    ("set facts before the claim", C, "    if (set && passed) {\n      try {", "    if (set) {\n      try {"),
    ("shared query ignored", C, "        if (answer != SmtAnswer::Unsat)\n          return unknownFact(std::string(\"SH at sites \")", "        if (false)\n          return unknownFact(std::string(\"SH at sites \")"),
    ("shared pairs within a group", C, "      if (set->groupOf[x] >= set->groupOf[y] || first.output->root != second.output->root)", "      if (set->groupOf[x] > set->groupOf[y] || first.output->root != second.output->root)"),
    ("shared pair limit ignored", C, "  if (count > options.limits.maxPairs)\n    return unknownFact(\"more arrival pairs than the limit\");\n", ""),
    ("order walk skipped", C, "      if (!walkToNextEvents(program, locations, options.limits.maxUnfolding, quiet, why,", "      if (false && !walkToNextEvents(program, locations, options.limits.maxUnfolding, quiet, why,"),
    ("order backward step admitted", C, "        if (next->second >= here)\n          return true;\n", "        return true;\n"),
    ("two groups in a block admitted", C, "    if (!added && slot->second != set->groupOf[id])\n      return unknownFact(\"two listed groups have sites in one block\");\n", ""),
    ("fact queries recorded", C, "  if (keep)\n    records.push_back(std::move(record));\n", "  records.push_back(std::move(record));\n"),
    ("set limits unchecked", C, "  if (!limitsAreEffective(options, why))\n    return CheckedComputation::unknown(why);\n  SetTrace set;", "  SetTrace set;"),
    ("set trace not fixed", C, "  SetTrace set;\n  auto united = unite(program, listed, set, why);\n  if (!united || !traceIsFixed(program, why))", "  SetTrace set;\n  auto united = unite(program, listed, set, why);\n  if (!united)"),
    ("set binding of the first group", C, "  for (const SiteGroup *group : listed) {\n    out += std::to_string(group->output", "  for (const SiteGroup *group : listed.take_front(1)) {\n    out += std::to_string(group->output"),
    ("set events uncounted", H, "    ++out.groupEvents[listed];\n", ""),
    ("set cells unmarked", H, "    cellGroups[cell] |= uint64_t(1) << listed;\n", ""),
    ("set order never broken", H, "    if (listed < latest)\n      out.listedOrder = false;\n", ""),
    ("one writer is shared", H, "    out.sharedCells += llvm::popcount(groups) > 1;", "    out.sharedCells += llvm::popcount(groups) > 0;"),
    ("a set lead is not run", A, "    if (set.finite.status() != ComputationStatus::Refuted && set.symbolic.failingPoint()) {", "    if (false) {"),
    ("a set lead replaces a finite claim", A, "      if (lead.status() == ComputationStatus::Refuted)\n        set.finite = std::move(lead);\n", "      set.finite = std::move(lead);\n"),
]


EQUIVALENT = [
    ("output not separated", "`accessed` starts with the group's roots and then gains the root "
     "of every effect, and each root of a group is the root of an effect: its site's loads and "
     "store. So the initializer adds nothing, and is kept as a guard for a record that lies, "
     "which E1 and OP1 refuse. Evidence: the last block of testGaps."),
    ("operands not compared", "the stored value of every imported site is its update (R1, and "
     "the last block of testGapsTwo checks it on the catalogue), so the stored node is "
     "apply(add, acc, apply(mul, a, b)) over the operands the program read. Nodes are compared "
     "by key, so a stored value equal to the replay's next value forces a = a0 and b = b0: the "
     "event comparison already decides what this one would."),
    ("width not in a node", "every comparison has a replay node on one side, built only of "
     "leaves (a root and cell, whose sort is fixed), constants (keyed with their width) and "
     "add or mul of replay nodes. By induction a program node equal to one without its width "
     "has the same op, flags, kind and children, and add and mul take their width from their "
     "children, so it is equal with its width. No comparison changes."),
    ("finite candidate unchecked", "points join only through coordinateFinite.points(), and "
     "checkFamilyFinite fills those only on its Verified path, or every point when the "
     "coordinates are symbolic. With neither, the list is empty and the result Unknown either "
     "way. Evidence: the closed-enumeration case of testGapsTwo."),
    ("short key passes", "injectivity is examined only when the coordinates are Verified "
     "symbolically, and checkFamilySymbolic adopts every term of the family first, by the same "
     "adoptProofTerm, refusing the family otherwise. So no term reaching this point fails to "
     "adopt. Evidence: the unadoptable family of testGapsTwo."),
    ("set lists a site twice", "unreachable since G5b-M4-0. The shared gate refuses a site listed "
     "twice within one group, in the same words, before the union is built. A site listed in two "
     "groups gives both groups the one root triple of that site, so the second is refused first "
     "as a group listed twice. The check stays as a defence. Evidence: the `a site twice` case of "
     "testGroupSets, now refused by the gate, and the duplicate cases of the identity section."),
]

MEMORY_LIMIT = 6 << 30   # Per process, inherited by its descendants.
COMPILE_LIMIT = 8 << 30  # A compile peaks at 1.6 GB and a link stays below this.
TEST_SECONDS = 1500
BUILD_SECONDS = 1200


class Finished:
    def __init__(self, returncode, stdout, stderr):
        self.returncode, self.stdout, self.stderr = returncode, stdout, stderr


def run(command, cwd=None, timeout=TEST_SECONDS, memory=MEMORY_LIMIT):
    """Its own process group, so a timeout reaches every descendant."""
    def limit():
        resource.setrlimit(resource.RLIMIT_AS, (memory, memory))

    process = subprocess.Popen([str(c) for c in command], cwd=cwd, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True, start_new_session=True,
                               preexec_fn=limit)
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        process.communicate()
        raise
    return Finished(process.returncode, stdout, stderr)


def verdict_of(label, done):
    """killed, resource, crash or passed, for one finished test."""
    text = done.stderr + done.stdout
    if ("bad_alloc" in text or "Cannot allocate" in text or "out of memory" in text or
            "failed to allocate" in text or done.returncode == -signal.SIGKILL):
        return "resource"
    if done.returncode == 0:
        return "passed"
    if done.returncode == 1:
        # A test's own assertion, in the words of that test.
        if label in ("programs", "groups"):
            own = ("driver failed" not in done.stderr and "Traceback" not in done.stderr and
                   done.stderr.strip())
        else:
            own = "FAIL " in done.stderr
        if own:
            return "killed"
    return "crash"


def contents(root, parts):
    found = {}
    for part in parts:
        for path in (root / part).rglob("*"):
            if path.is_file() and "__pycache__" not in path.parts:
                found[path.relative_to(root)] = hashlib.sha256(path.read_bytes()).hexdigest()
    return found


class Worker:
    def __init__(self, index, scratch, root, args):
        self.lib = scratch / f"lib{index}"
        self.build = scratch / f"build{index}"
        self.root, self.args = root, args
        if not self.lib.exists():
            shutil.copytree(root / "lib" / "GeneralizedGemm", self.lib)
        configured = run(["cmake", "-S", root / "research" / "generalized-gemm", "-B", self.build,
                          "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", f"-DGG_LIB_DIR={self.lib}",
                          f"-DLLVM_DIR={args.llvm_dir}", f"-DMLIR_DIR={args.mlir_dir}"])
        if configured.returncode:
            raise RuntimeError(f"cmake failed:\n{configured.stderr[-1500:]}")

    def restore(self):
        original = self.root / "lib" / "GeneralizedGemm"
        for f in original.iterdir():
            copy = self.lib / f.name
            if copy.read_text() != f.read_text():
                shutil.copy(f, copy)

    def compile(self):
        return run(["ninja", "-C", self.build, "-j2", "generalized-gemm-computation-test",
                    "generalized-gemm-computation-report"], timeout=BUILD_SECONDS,
                   memory=COMPILE_LIMIT)

    def commands(self):
        fuzz = self.root / "research" / "generalized-gemm" / "test" / "fuzz"
        test = self.build / "generalized-gemm-computation-test"
        report = self.build / "generalized-gemm-computation-report"
        out = [(name, [test, name]) for name in
               ("sets", "gaps", "gaps2", "tampered", "separation", "limits", "float", "reach", "claims",
                "candidates", "exhaustive")]
        out.append(("programs", [sys.executable, fuzz / "computation_oracle.py", "--driver", report,
                                 "--programs", "16", "--fills", "24", "--seed", "41", "--jobs", "2"]))
        out.append(("groups", [sys.executable, fuzz / "multigroup_oracle.py", "--driver", report,
                               "--programs", "4", "--fills", "6", "--seed", "41", "--jobs", "2"]))
        return out

    def trial(self, name, filename, old, new):
        self.restore()
        path = self.lib / filename
        text = path.read_text()
        if text.count(old) != 1:
            return "HARNESS_ERROR", "stale pattern" if old not in text else "ambiguous pattern"
        path.write_text(text.replace(old, new, 1))
        try:
            built = self.compile()
        except subprocess.TimeoutExpired:
            return "HARNESS_ERROR", "the build timed out"
        if built.returncode:
            return "HARNESS_ERROR", "does not build: " + built.stdout[-300:]
        notes = []
        for label, command in self.commands():
            try:
                outcome = verdict_of(label, run(command))
            except subprocess.TimeoutExpired:
                outcome = "timeout"
            if outcome == "killed":
                return "KILLED", label
            if outcome != "passed":
                notes.append((outcome, label))
        for outcome, name_of in (("resource", "RESOURCE_LIMIT"), ("timeout", "TIMEOUT"),
                                 ("crash", "HARNESS_ERROR")):
            found = [label for kind, label in notes if kind == outcome]
            if found:
                return name_of, outcome + " in " + ",".join(found)
        return "SURVIVED", ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, help="the repository root")
    parser.add_argument("--scratch", required=True)
    parser.add_argument("--llvm-dir", required=True)
    parser.add_argument("--mlir-dir", required=True)
    parser.add_argument("--only", default="", help="comma-separated mutant names")
    parser.add_argument("--workers", type=int, default=1)
    parser.add_argument("--results", required=True, help="a JSON-lines file, appended to")
    args = parser.parse_args()
    if args.workers > 2:
        print("at most two workers: compiles take 1.6 GB each and other jobs share this machine")
        return 2

    # Names may hold a comma: a semicolon then separates them.
    names = [n for n in args.only.split(";" if ";" in args.only else ",") if n]
    unknown = sorted(set(names) - {m[0] for m in MUTANTS})
    if unknown:
        print(f"no such mutants: {unknown}")
        return 2
    if len({m[0] for m in MUTANTS}) != len(MUTANTS):
        print("two mutants share a name")
        return 2
    equivalent = {e[0] for e in EQUIVALENT}
    if not equivalent <= {m[0] for m in MUTANTS}:
        print("an equivalent mutant is not in the list")
        return 2
    chosen = [m for m in MUTANTS if (not names or m[0] in names) and m[0] not in equivalent]

    scratch = Path(args.scratch).resolve()
    scratch.mkdir(exist_ok=True)
    snapshot = scratch / "tree"
    source = Path(args.source).resolve()
    parts = ["include", "lib", "research"] + [
        f"test/{p.name}" for p in (source / "test").glob("polynomial*")]
    if not snapshot.exists():
        snapshot.mkdir()
        for part in parts:
            (snapshot / part).parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(source / part, snapshot / part)
    elif contents(source, parts) != contents(snapshot, parts):
        print(f"{snapshot} is not a copy of {source}: use a new --scratch")
        return 2
    tree = hashlib.sha256(json.dumps(sorted((str(k), v) for k, v in contents(source, parts).items())).encode()).hexdigest()

    results = Path(args.results)
    done = {}
    if results.exists():
        for line in results.read_text().splitlines():
            row = json.loads(line)
            if row["tree"] != tree:
                print(f"{results} is about another tree")
                return 2
            done[row["mutant"]] = row
    chosen = [m for m in chosen if m[0] not in done]
    print(f"tree {tree}: {len(done)} already recorded, {len(chosen)} to run", flush=True)
    for name, reason in EQUIVALENT:
        if name not in done:
            row = {"mutant": name, "tree": tree, "result": "EQUIVALENT", "detail": reason}
            done[name] = row
            with results.open("a") as out:
                out.write(json.dumps(row) + "\n")
    workers = [Worker(i, scratch, snapshot, args) for i in range(args.workers)]

    # A kill only means something if the unmodified tree passes the same tests.
    first = workers[0]
    first.restore()
    built = first.compile()
    if built.returncode:
        print(f"BASELINE NO-BUILD\n{built.stdout[-1500:]}")
        return 2
    for label, command in first.commands():
        result = run(command)
        if verdict_of(label, result) != "passed":
            print(f"BASELINE FAILED: {label}\n{result.stdout[-1500:]}\n{result.stderr[-1500:]}")
            return 2
    print("baseline passes", flush=True)

    queue = list(chosen)

    def serve(worker):
        while True:
            try:
                name, filename, old, new = queue.pop(0)
            except IndexError:
                return
            result, detail = worker.trial(name, filename, old, new)
            row = {"mutant": name, "tree": tree, "result": result, "detail": detail}
            with results.open("a") as out:
                out.write(json.dumps(row) + "\n")
                out.flush()
                os.fsync(out.fileno())
            done[name] = row
            print(f"{result}: {name} ({detail})", flush=True)

    with ThreadPoolExecutor(len(workers)) as pool:
        list(pool.map(serve, workers))
    counts = {}
    for row in done.values():
        counts[row["result"]] = counts.get(row["result"], 0) + 1
    bad = sorted(n for n, r in done.items() if r["result"] not in ("KILLED", "EQUIVALENT"))
    print(f"{len(done)} of {len(MUTANTS)} recorded; {counts}; open: {bad}")
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
