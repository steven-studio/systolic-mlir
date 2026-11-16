#!/usr/bin/env python3
"""G5a mutation test. Each mutant weakens one thing the checker relies on, and the
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

O = "GuardedOrbit.cpp"
D = "DomainChecker.cpp"
P = "DomainProposer.cpp"   # Phase 4 of the statement-level work: identities and sample-checked views.
N = "DomainAnalysis.cpp"   # Certificate strengthening, affinity, eligibility.
R = "ReductionSemantics.cpp"
V = "CoverageSimulation.cpp"
C = "ComputationChecker.cpp"
T = "SiteSets.cpp"                  # G5b-M4-0: groups and cells, one rule for G3b, G4a and G5a.
G = "SymbolicCandidateChecker.cpp"  # G3b: its refutation scope and finite gate (G5b-M4-0).
F = "FiniteDomain.cpp"              # The scope an enumeration records (G5b-M4-0).

MUTANTS = [
    # Theorem 12: D1 to D3.
    ("D1 skipped", O, "    return mustHold(\"D1\", \"the first event\", terms.logicalAnd(premise, bad));\n", "    (void)bad;\n    return true;\n"),
    ("D1 exit skipped", O, "      return mustHold(\"D1\", \"the entry reaches the exit\", terms.logicalAnd(premise, yInside));\n", "      return true;\n"),
    ("D2 skipped", O, "      return mustHold(\"D2\", \"the next event\", terms.logicalAnd(premise, bad));\n", "      (void)bad;\n      return true;\n"),
    ("D3 skipped", O, "        return mustHold(\"D3\", \"an event that is not the last reaches the exit\",\n                        terms.logicalAnd(premise, later));\n", "        return true;\n"),
    ("D1 earlier point ignored", O, "    Term bad = terms.logicalOr(terms.logicalNot(inGuardedSet(program, order, x)), earlier);\n", "    Term bad = terms.logicalNot(inGuardedSet(program, order, x));\n    (void)earlier;\n"),
    ("D2 between ignored", O, "                                                 between));\n", "                                                 terms.boolean(false)));\n    (void)between;\n"),
    ("D2 increase ignored", O, "terms.logicalOr(terms.logicalNot(guardedLess(program, order, b, x)),\n", "terms.logicalOr(terms.boolean(false),\n"),
    ("D2 increase ignored, segment", O, "terms.logicalOr(terms.logicalNot(guardedLess(program, order, b, x)), between));", "terms.logicalOr(terms.boolean(false), between));"),
    ("D2 membership ignored", O, "      Term bad = terms.logicalOr(terms.logicalNot(inGuardedSet(program, order, x)),\n                                 terms.logicalOr(", "      Term bad = terms.logicalOr(terms.boolean(false),\n                                 terms.logicalOr("),
    ("event assumed outside", O, "    std::vector<Term> b = values(arrival.path);\n    Term assumption = terms.logicalAnd(arrival.premise, inGuardedSet(program, order, b));\n", "    std::vector<Term> b = values(arrival.path);\n    Term assumption = arrival.premise;\n"),
    ("event assumed outside, segment", O, "    std::vector<Term> y = freeLevels(program, order);\n    Term assumption = terms.logicalAnd(arrival.premise, inGuardedSet(program, order, b));\n", "    std::vector<Term> y = freeLevels(program, order);\n    Term assumption = arrival.premise;\n"),
    ("entry-block site admitted", O, "  // The first event happens inside the entry block, where no walk starts.\n  if (std::find(locations.begin(), locations.end(), program.entryLocation) != locations.end()) {\n    why = \"a site in the entry block\";\n    return false;\n  }\n", ""),
    ("entry-block site admitted, segment", O, "  if (std::find(locations.begin(), locations.end(), program.entryLocation) != locations.end()) {\n    why = \"a site in the entry block\";\n    return false;\n  }\n\n  // D1', from the entry.\n", "\n  // D1', from the entry.\n"),
    ("first walk without scope", O, "begin, scope, firstLeaf))\n    return false;\n\n  // D2 and D3, from each state", "begin, terms.boolean(true), firstLeaf))\n    return false;\n\n  // D2 and D3, from each state"),
    ("first walk without scope, segment", O, "begin, scope, firstLeaf))\n    return false;\n\n  // D2 to D5, from each state", "begin, terms.boolean(true), firstLeaf))\n    return false;\n\n  // D2 to D5, from each state"),
    ("lower bound ignored", O, "    all = terms.logicalAnd(all, terms.icmp(le, bindLevels(program, level.lower, values), values[m]));\n", ""),
    ("upper bound ignored", O, "    all = terms.logicalAnd(all, terms.icmp(lt, values[m], bindLevels(program, level.upper, values)));\n", ""),
    ("guard ignored", O, "  Term all = bindLevels(program, order.guard, values);\n", "  Term all = terms.boolean(true);\n"),
    ("direction ignored", O, "  return level.descending ? terms.icmp(before, b, a) : terms.icmp(before, a, b);\n", "  return terms.icmp(before, a, b);\n"),
    ("signedness ignored", O, "  Predicate before = level.isSigned ? Predicate::Slt : Predicate::Ult;\n", "  Predicate before = Predicate::Ult;\n"),
    ("lex not prefix", O, "    same = terms.logicalAnd(same, terms.icmp(Predicate::Eq, a[m], b[m]));\n", ""),
    ("bound may read its own level", O, "    if (!levelTermOk(program, level.lower, sorts.size() - 1, sorts, why) ||\n        !levelTermOk(program, level.upper, sorts.size() - 1, sorts, why))", "    if (!levelTermOk(program, level.lower, 64, sorts, why) ||\n        !levelTermOk(program, level.upper, 64, sorts, why))"),
    ("bound may read state", O, "      if (!level || *level >= levels || sorts[*level] != n->sort) {\n        why = \"a bound or guard reads state, or a level it may not\";\n        return false;\n      }\n", "      if (level && (*level >= levels || sorts[*level] != n->sort)) {\n        why = \"a bound or guard reads state, or a level it may not\";\n        return false;\n      }\n"),
    ("level may read memory", O, "      if (!independent[id]) {\n        why = \"a level reads a value that depends on memory\";\n        return nullptr;\n      }\n", ""),
    ("coordinate need not be a level", O, "      if (found == levels.end()) {\n        why = \"a coordinate of the family is no level\";\n        return std::nullopt;\n      }\n      levels.erase(found);\n", "      if (found != levels.end())\n        levels.erase(found);\n"),
    ("enumeration ignores direction", O, "      APInt offset(low->getBitWidth(), level.descending ? count - 1 - n : n);\n", "      APInt offset(low->getBitWidth(), n);\n"),
    ("enumeration ignores the guard", O, "      if (keep->isOne())\n        out.push_back(current);\n", "      out.push_back(current);\n"),
    ("enumeration box limit ignored", O, "    if (span.getActiveBits() > 40 || visited + span.getZExtValue() > limit) {", "    if (span.getActiveBits() > 40) {"),
    ("concrete order ignores direction", O, "    return level.descending ? !before : before;\n", "    return before;\n"),
    ("concrete order ignores signedness", O, "    bool before = level.isSigned ? a[m].slt(b[m]) : a[m].ult(b[m]);\n", "    bool before = a[m].ult(b[m]);\n"),
    # Theorem 15 (G5b-M2): segmented orders.
    ("D1' empty before skipped", O, "    return empty(\"D1\", \"before the first event\", premise, 0, *s);\n", "    return true;\n"),
    ("D1' exit empties skipped", O, "      return empty(\"D1\", \"the entry reaches the exit\", premise, 0, m);\n", "      return true;\n"),
    ("D4 successor remains skipped", O, "      return mustHold(\"D4\", \"a successor remains at the hand-over \" + handover,\n                      terms.logicalAnd(premise, later)) &&\n", "      return true &&\n"),
    ("D4 between empty skipped", O, "             empty(\"D4\", \"between \" + handover, premise, *s + 1, *next) &&\n", ""),
    ("D4 first event skipped", O, "             mustHold(\"D4\", \"the first event after the hand-over \" + handover,\n                      terms.logicalAnd(premise, isFirst(*next, values(*next, path))));", "             true;"),
    ("D5 backward admitted", O, "      if (*next < *s)\n        return mustHold(\"D5\", \"an event of segment \" + std::to_string(*next) + \" after \" + here, premise);\n", "      if (*next < *s)\n        return true;\n"),
    ("D5 after last skipped", O, "               empty(\"D5\", \"after the last event of \" + here, premise, *s + 1, m);", "               true;"),
    ("D3 per segment skipped", O, "        return mustHold(\"D3\", \"an event that is not the last of \" + here + \" reaches the exit\",\n                        terms.logicalAnd(premise, later)) &&", "        return true &&"),
    ("segment order unchecked", D, "      if (rows[e].segment < rows[e - 1].segment)\n        return refute(", "      if (false)\n        return refute("),
    ("longer segment set admitted", D, "      if (mine.size() != sets[s].size())\n        return refute(", "      if (mine.size() > sets[s].size())\n        return refute("),
    ("segment elements unchecked", D, "        if (rows[mine[n]].lambda != sets[s][n])\n          return refute(", "        if (false)\n          return refute("),
    ("site in no segment admitted", D, "  if (seen.size() != listed.size()) {\n    why = std::string(\"a site of the \") + whole + \" is in no segment\";\n    return std::nullopt;\n  }\n", ""),
    ("site in two segments admitted", D, "      if (!seen.insert(site).second) {\n        why = \"a site in two segments\";\n        return std::nullopt;\n      }\n", "      seen.insert(site);\n"),
    ("site outside the group admitted", D, "      if (!listed.count(site)) {\n        why = std::string(\"a segment names a site outside the \") + whole;\n        return std::nullopt;\n      }\n", ""),
    ("family beside segments admitted", D, "  if (!familyIsEmpty(family)) {\n    why = \"a segmented request carries its own families\";\n    return std::nullopt;\n  }\n", ""),
    ("shared cells unchecked", D, "      if (segmentsHere.size() > 1 && sharedNone) {", "      if (false) {"),
    ("cross-segment T1 skipped", D, "        if (single && proof.ask(\"T1\", \"one contribution per cell, across segments\", sameCell) != SmtAnswer::Unsat)\n          single = false;\n", ""),
    ("symbolic shared cells unchecked", D, "        if (sharedNone && proof.ask(\"SH\", \"cells of two segments\", sameCell) != SmtAnswer::Unsat)\n          sharedNone = false;\n", ""),
    ("unvalued segment term admitted", D, "    if (!valued)\n      return CheckedDomain::unknown(\"the request is refused: \" + unvalued);\n", ""),
    ("segmented binding ignores segments", D, "  for (const Segment &segment : request.segments) {\n    out += \"|S\";", "  for (const Segment &segment : std::vector<Segment>()) {\n    out += \"|S\";"),
    ("segmented request on the flat route", D, "  CheckedDomain out = request.segments.empty() ? finiteClaim(program, enumeration, g, family, request, options)\n                                               : finiteSegmented(program, enumeration, g, family, request, options);", "  CheckedDomain out = finiteSegmented(program, enumeration, g, family, request, options);"),
    # The walker export.
    ("walker export skips", V, "  Walker walker(program, locations, depth, ask, why);\n  return walker.walk(path, assumption, leaf);\n", "  (void)program, (void)locations, (void)depth, (void)ask, (void)why, (void)path, (void)assumption, (void)leaf;\n  return true;\n"),
    # The finite route.
    ("order inversion passes", D, "      if (!concreteLess(order, lambdas[e - 1], lambdas[e]))\n        return refute(", "      if (false)\n        return refute("),
    ("count mismatch passes", D, "    if (lambdas.size() != set->size()) {", "    if (false) {"),
    ("element mismatch passes", D, "      if (lambdas[n] != (*set)[n])\n        return refute(", "      if (false)\n        return refute("),
    ("pointless points pass", D, "  if (!anyEvent)\n    return CheckedDomain::unknown(open.empty() ? \"no point has events\" : open);\n  ComputationTier tier = complete ? ComputationTier::ScopeEnumerated : ComputationTier::Point;\n  CheckedDomain out = DomainAuthority::make(\n      tier, ComputationStatus::Verified,\n      open.empty() ? \"each listed point enumerated exactly, in order\"", "  ComputationTier tier = complete ? ComputationTier::ScopeEnumerated : ComputationTier::Point;\n  CheckedDomain out = DomainAuthority::make(\n      tier, ComputationStatus::Verified,\n      open.empty() ? \"each listed point enumerated exactly, in order\""),
    ("pointless points pass, segment", D, "  if (!anyEvent)\n    return CheckedDomain::unknown(open.empty() ? \"no point has events\" : open);\n  ComputationTier tier = complete ? ComputationTier::ScopeEnumerated : ComputationTier::Point;\n  CheckedDomain out = DomainAuthority::make(\n      tier, ComputationStatus::Verified,\n      open.empty() ? \"each listed point enumerated exactly, segment by segment, in order\"", "  ComputationTier tier = complete ? ComputationTier::ScopeEnumerated : ComputationTier::Point;\n  CheckedDomain out = DomainAuthority::make(\n      tier, ComputationStatus::Verified,\n      open.empty() ? \"each listed point enumerated exactly, segment by segment, in order\""),
    ("stopped point counts", D, "    if (!point.reason.empty()) {\n      complete = false;\n      continue;\n    }\n    const std::vector<APInt> &v = point.parameters;\n    auto set = enumerateGuardedSet(", "    if (!point.reason.empty())\n      continue;\n    const std::vector<APInt> &v = point.parameters;\n    auto set = enumerateGuardedSet("),
    ("stopped point counts, segment", D, "    if (!point.reason.empty()) {\n      complete = false;\n      continue;\n    }\n    const std::vector<APInt> &v = point.parameters;\n    // The run again", "    if (!point.reason.empty())\n      continue;\n    const std::vector<APInt> &v = point.parameters;\n    // The run again"),
    ("scope not compared", D, "  std::vector<DomainPointFacts> facts;\n  bool complete = enumerationCoversScope(program, enumeration, options.scope);\n", "  std::vector<DomainPointFacts> facts;\n  bool complete = !enumeration.sampled;\n"),
    ("box multiplicity unchecked, segment", D, "          if (!mu || *mu != APInt(128, count))\n            return CheckedDomain::unknown(\"the multiplicity contradicts a segment's box: a defect\");", "          if (false)\n            return CheckedDomain::unknown(\"the multiplicity contradicts a segment's box: a defect\");"),
    ("scope not compared, segment", D, "  std::vector<SegmentFacts> segmentFacts(m);\n  bool complete = enumerationCoversScope(program, enumeration, options.scope);\n", "  std::vector<SegmentFacts> segmentFacts(m);\n  bool complete = !enumeration.sampled;\n"),
    ("box limit is a pass", D, "    if (!set) {\n      complete = false;\n      if (open.empty())\n        open = why;\n      continue;\n    }\n", "    if (!set)\n      continue;\n"),
    ("missing value passes", D, "      if (!valued) {\n        complete = false;\n", "      if (false) {\n        complete = false;\n"),
    ("key unchecked", D, "          if (!concreteLess(keyed, a, b) && keyHolds) {", "          if (false && keyHolds) {"),
    ("collision unchecked", D, "      if (outputsHere.size() > 1) {", "      if (false) {"),
    ("single unchecked", D, "      if (members.size() > 1 && single) {", "      if (false && single) {"),
    ("access unchecked", D, "          same = value && *value == views[view][e][n];\n", "          same = true;\n          (void)value;\n"),
    ("box multiplicity unchecked", D, "        if (!mu || *mu != APInt(128, count))\n          return CheckedDomain::unknown(\"the multiplicity contradicts the order's box: a defect\");", "        if (false)\n          return CheckedDomain::unknown(\"the multiplicity contradicts the order's box: a defect\");"),
    ("closed gate checked", D, "CheckedDomain finiteClaim(const SemanticProgram &program, const DomainEnumeration &enumeration,\n                          unsigned g, const CoordinateFamily &family, const DomainRequest &request,\n                          const DomainOptions &options) {\n  if (enumeration.gate != GateStatus::Open)\n    return CheckedDomain::unknown(\"the domain gate is closed: \" + enumeration.reason);\n", "CheckedDomain finiteClaim(const SemanticProgram &program, const DomainEnumeration &enumeration,\n                          unsigned g, const CoordinateFamily &family, const DomainRequest &request,\n                          const DomainOptions &options) {\n"),
    ("closed gate checked, segment", D, "                              const DomainOptions &options) {\n  if (enumeration.gate != GateStatus::Open)\n    return CheckedDomain::unknown(\"the domain gate is closed: \" + enumeration.reason);\n", "                              const DomainOptions &options) {\n"),
    # Gates.
    ("part of a group", T, "    if (same != (listed.count(site.id) != 0)) {\n      why = \"the group is not the sites of its roots\";", "    if (false) {\n      why = \"the group is not the sites of its roots\";"),
    ("oblivious unchecked", D, "  if (!program.controlOblivious || !program.addressOblivious) {\n    why = \"the trace depends on memory contents\";", "  if (false) {\n    why = \"the trace depends on memory contents\";"),
    ("zero limit accepted", D, "  if (!options.termination.solver.timeoutMilliseconds || !options.termination.solver.resourceLimit)\n    return CheckedDomain::unknown(\"a solver limit of zero is no limit\");\n  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))\n    return CheckedDomain::unknown(why);\n  auto structure = adoptRequest(", "  if (false)\n    return CheckedDomain::unknown(\"a solver limit of zero is no limit\");\n  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))\n    return CheckedDomain::unknown(why);\n  auto structure = adoptRequest("),
    ("zero limit accepted, segment", D, "  if (!options.termination.solver.timeoutMilliseconds || !options.termination.solver.resourceLimit)\n    return CheckedDomain::unknown(\"a solver limit of zero is no limit\");\n  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))\n    return CheckedDomain::unknown(why);\n  return symbolicSegmentedOver(", "  if (false)\n    return CheckedDomain::unknown(\"a solver limit of zero is no limit\");\n  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))\n    return CheckedDomain::unknown(why);\n  return symbolicSegmentedOver("),
    ("certificate not needed", D, "  if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating &&\n      outcome.verdict.kind() != TerminationVerdict::ProvenTerminatingIfDefined) {\n    why = \"the certificate was not accepted: \"", "  if (false) {\n    why = \"the certificate was not accepted: \""),
    ("shared block admitted", D, "      if (!places.insert(program.sites[id].location).second)\n        return CheckedDomain::unknown(\"two sites of the group share a block\");\n  }\n  Proof proof(", "      places.insert(program.sites[id].location);\n  }\n  Proof proof("),
    ("shared block admitted, segment", D, "      if (!places.insert(program.sites[id].location).second)\n        return CheckedDomain::unknown(std::string(\"two sites of the \") + whole + \" share a block\");\n  }\n  std::map<unsigned, unsigned> ofSite", "      places.insert(program.sites[id].location);\n  }\n  std::map<unsigned, unsigned> ofSite"),
    ("any certificate accepted", D, "  if (!acceptCertificate(program, certificate, options, backend, why))\n    return CheckedDomain::unknown(why);\n  const GuardedOrder &order = structure->order;", "  if (!certificate)\n    return CheckedDomain::unknown(why);\n  const GuardedOrder &order = structure->order;"),
    ("any certificate accepted, segment", D, "  if (!acceptCertificate(program, certificate, options, backend, why))\n    return CheckedDomain::unknown(why);\n  {\n    std::set<unsigned> places;", "  if (!certificate)\n    return CheckedDomain::unknown(why);\n  {\n    std::set<unsigned> places;"),
    ("key order unchecked", D, "    if (key[n] >= order.levels.size() || (n && key[n] <= key[n - 1]))", "    if (key[n] >= order.levels.size())"),
    ("access form unchecked", D, "      if (!isLevelTerm(program, out.order, term, out.order.levels.size(), why))\n        return std::nullopt;\n", "      (void)term;\n"),
    # Symbolic facts.
    ("K1 unchecked", D, "        SmtAnswer answer = proof.ask(\"K1\", \"the cell key\", bad);\n        if (answer != SmtAnswer::Unsat) {", "        SmtAnswer answer = SmtAnswer::Unsat;\n        (void)bad;\n        if (answer != SmtAnswer::Unsat) {"),
    ("C1 unchecked", D, "        if (proof.ask(\"C1\", \"one logical output per cell\", bad) != SmtAnswer::Unsat)", "        if (false && proof.ask(\"C1\", \"one logical output per cell\", bad) != SmtAnswer::Unsat)"),
    ("T1 unchecked", D, "        if (proof.ask(\"C1\", \"one logical output per cell\", bad) != SmtAnswer::Unsat)\n          injective = false;\n      }\n      if (single) {\n        Term bad = terms.logicalAnd(sameCell, terms.logicalNot(proof.equal(la, lb)));\n        if (proof.ask(\"T1\", \"one contribution per cell\", bad) != SmtAnswer::Unsat)", "        if (proof.ask(\"C1\", \"one logical output per cell\", bad) != SmtAnswer::Unsat)\n          injective = false;\n      }\n      if (single) {\n        Term bad = terms.logicalAnd(sameCell, terms.logicalNot(proof.equal(la, lb)));\n        if (false && proof.ask(\"T1\", \"one contribution per cell\", bad) != SmtAnswer::Unsat)"),
    ("T1 unchecked, same segment", D, "          injective[sa] = false;\n      }\n      if (single) {\n        Term bad = terms.logicalAnd(sameCell, terms.logicalNot(proof.equal(la, lb)));\n        if (proof.ask(\"T1\", \"one contribution per cell\", bad) != SmtAnswer::Unsat)", "          injective[sa] = false;\n      }\n      if (single) {\n        Term bad = terms.logicalAnd(sameCell, terms.logicalNot(proof.equal(la, lb)));\n        if (false && proof.ask(\"T1\", \"one contribution per cell\", bad) != SmtAnswer::Unsat)"),
    ("A1 unchecked", D, "        if (proof.ask(\"A1\", \"a closed form of a view\", program.terms->logicalAnd(a.premise, differs)) !=\n            SmtAnswer::Unsat) {", "        if (false) {"),
    ("second copy shares state", D, "      std::vector<Term> la = proof.values(order, a), lb = proof.second(proof.values(order, b));\n      TermContext &terms", "      std::vector<Term> la = proof.values(order, a), lb = proof.values(order, b);\n      TermContext &terms"),
    ("pair limit ignored", D, "    if (n * n > options.limits.maxPairs)\n      return unknownWith(\"more arrival pairs than the limit\");\n\n    // K1, C1, T1 over pairs", "\n    // K1, C1, T1 over pairs"),
    ("pair limit ignored, segment", D, "    if (n * n > options.limits.maxPairs)\n      return unknownWith(\"more arrival pairs than the limit\");\n\n    // Within a segment", "\n    // Within a segment"),
    ("query limit ignored", D, "    if (queries >= options.limits.maxQueries) {", "    if (false) {"),
    ("unknown discharges", D, "    entry.discharged = response.answer == SmtAnswer::Unsat;\n    if (response.answer == SmtAnswer::Unknown)\n      entry.evidence = response.detail;\n    if (response.answer == SmtAnswer::Sat && id != \"prune\" && !failing) {", "    entry.discharged = response.answer == SmtAnswer::Unsat;\n    if (response.answer == SmtAnswer::Unknown)\n      response.answer = SmtAnswer::Unsat;\n    if (response.answer == SmtAnswer::Sat && id != \"prune\" && !failing) {"),
    # Multiplicity.
    ("box guard may read repeats", D, "  if (reads(order.guard, true))\n    return nullptr;\n  // Each span", "  // Each span"),
    ("box bounds may read repeats", D, "    if (reads(level.lower, true) || reads(level.upper, true))\n      return nullptr;\n", ""),
    ("box width unchecked", D, "  if (bitsUsed >= 128)\n    return nullptr;\n", ""),
    ("logical box called constant", D, "    out.form = readsLogical ? DomainMultiplicity::LogicalBox : DomainMultiplicity::ConstantBox;", "    out.form = DomainMultiplicity::ConstantBox;"),
    ("mu read at no event", D, "        auto mu = evaluateLevelTerm(structure->mu, v, lambdas[firstEvent.at(point)]);", "        auto mu = evaluateLevelTerm(structure->mu, v, {});"),
    ("mu span not clamped", D, "    Term span = terms.ite(terms.icmp(Predicate::Slt, lo, hi), terms.binary(TermOp::Sub, hi, lo),\n                          terms.bv(APInt(128, 0)));", "    Term span = terms.binary(TermOp::Sub, hi, lo);"),
    ("mu span unsigned", D, "    Term lo = level.isSigned ? terms.sext(level.lower, 128) : terms.zext(level.lower, 128);\n    Term hi = level.isSigned ? terms.sext(level.upper, 128) : terms.zext(level.upper, 128);", "    Term lo = terms.zext(level.lower, 128);\n    Term hi = terms.zext(level.upper, 128);"),
    ("box ignores empty spans", D, "    Term span = terms.ite(terms.icmp(Predicate::Slt, lo, hi), terms.binary(TermOp::Sub, hi, lo),\n                          terms.bv(APInt(128, 0)));\n", "    Term span = terms.binary(TermOp::Sub, hi, lo);\n"),
    # Binding.
    ("binding ignores the request", D, "  for (const GuardedLevel &level : request.order.levels)\n    out += \"|\" + formatTerm(level.value)", "  for (const GuardedLevel &level : llvm::ArrayRef<GuardedLevel>())\n    out += \"|\" + formatTerm(level.value)"),
    ("binding ignores the family", D, "  for (const auto &axis : family.axes) {\n    out += \"|f\";", "  for (const auto &axis : std::array<std::vector<Term>, 0>()) {\n    out += \"|f\";"),
    ("binding ignores access", D, "  for (const auto &form : request.access) {\n    out += \"|a\";", "  for (const auto &form : std::array<std::vector<Term>, 0>()) {\n    out += \"|a\";"),
    ("binding ignores the scope", D, "  for (const ParameterRange &range : options.scope.ranges)\n    out += std::to_string(range.parameter) + \":\" + std::to_string(range.lower.getSExtValue()) +\n           \":\" + std::to_string(range.upper.getSExtValue()) + \",\";\n  for (const std::string &name : options.authorized)\n    out += \"|\" + name;\n  for (const GuardedLevel", "  for (const std::string &name : options.authorized)\n    out += \"|\" + name;\n  for (const GuardedLevel"),
    # Theorem 13.
    ("T13 ignores refutation", D, "  if (refuted) {\n    out.claim = ComputationStatus::Refuted;", "  if (false) {\n    out.claim = ComputationStatus::Refuted;"),
    ("T13 without G4a symbolic", D, "  } else if (out.compSymbolic.status() == ComputationStatus::Verified &&\n             out.domSymbolic.status() == ComputationStatus::Verified &&", "  } else if (out.domSymbolic.status() == ComputationStatus::Verified &&"),
    ("T13 without exact order", D, "             out.domSymbolic.exactOrder()) {", "             true) {"),
    ("T13 finite without G4a", D, "  } else if (out.compFinite.status() == ComputationStatus::Verified &&\n             out.domFinite.status() == ComputationStatus::Verified) {", "  } else if (out.domFinite.status() == ComputationStatus::Verified) {"),
    ("T13 points not shared", D, "      if (facts.events && computed.count(pointKey(facts.parameters)))\n", "      if (facts.events)\n"),
    ("T13 point called a scope", D, "      bool whole = out.compFinite.tier() == ComputationTier::ScopeEnumerated &&\n                   out.domFinite.tier() == ComputationTier::ScopeEnumerated;\n", "      bool whole = true;\n"),
    ("logical tier without G3", D, "    out.logical = coordinateSymbolicOk ? ComputationTier::Symbolic : ComputationTier::Proposed;\n", "    out.logical = ComputationTier::Symbolic;\n"),
    # Reorder facts.
    ("trivial from points at the scope", D, "    out.trivial = out.domFinite.singleContribution();\n", "  {\n    out.trivial = out.domFinite.singleContribution();\n    out.trivial.tier = (int)ComputationTier::Symbolic;\n  }\n"),
    ("float proven by algebra", D, "    out.floating = values.status() == ComputationStatus::Verified\n                           ? witness()\n                           : unknownFact(\"the computation is not verified\");\n", "    out.floating = proven(\"addition is associative\", ComputationTier::Symbolic);\n"),
    ("integer flags ignored", R, "    if (contract.reductionFlags & (FlagNSW | FlagNUW)) {", "    if (false) {"),
    ("integer observers ignored", R, "    if (effect.observesOutput) {", "    if (false) {"),
    ("integer stop loads ignored", R, "      if (loadsOutput(condition.violated) || throughState(condition.violated)) {", "      if (throughState(condition.violated)) {"),
    ("integer stop state ignored", R, "      if (loadsOutput(condition.violated) || throughState(condition.violated)) {", "      if (loadsOutput(condition.violated)) {"),
    # G5b-M4b-2a: the conditions over a union, the guard, the witness over the union.
    ("integer stop loads first root only", R, "    return llvm::any_of(outputs, [&](unsigned root) { return loadsFrom(term, root); });", "    return loadsFrom(term, outputs.front());"),
    ("integer guard loads ignored", R, "    if (loadsOutput(t.guard) || throughState(t.guard)) {", "    if (throughState(t.guard)) {"),
    ("set witness judges the first group", R, "  return searchReversal(program, *united, replay, parameters, fills, maxCells);", "  return searchReversal(program, *listed.front(), replay, parameters, fills, maxCells);"),
    ("witness memory dropped", R, "      out.memory = std::move(*entry);\n", ""),
    ("integer float fold admitted", R, "  if (computation.fold() != FoldForm::ModularInteger) {", "  if (false) {"),
    ("integer unverified admitted", R, "  if (computation.status() != ComputationStatus::Verified) {\n    why = \"the computation is not verified\";", "  if (false) {\n    why = \"the computation is not verified\";"),
    ("witness from a stopped run", R, "  RunResult run = runConcrete(program, entry);\n  if (run.status != RunStatus::Returned)\n    return false;\n", "  RunResult run = runConcrete(program, entry);\n"),
    ("witness without overflow", R, "    if (!isFloat && backwardOverflow) {", "    if (!isFloat) {"),
    ("witness float equal bits", R, "    if (isFloat && forward != backward) {", "    if (isFloat) {"),
    ("witness mixed sites", R, "    if (site.reduction != first.reduction || site.reductionFlags != first.reductionFlags ||\n        site.multiplyFlags != first.multiplyFlags)\n      return false;\n", ""),
    # G4a's scope check.
    ("scope sampled", C, "  if (enumeration.sampled || scope.allParameters)\n    return false;\n", ""),
    ("scope count ignored", C, "  return expected == APInt(128, seen.size() + enumeration.outsideDomain);\n", "  return true;\n"),
    ("scope point outside", C, "    if (!parametersInScope(scope, point.parameters))\n      return false;\n", ""),
    # G5b-M4-0: the group gate, one for G3b, G4a and G5a.
    ("site id unbounded", T, '  for (unsigned id : group.sites)\n    if (id >= program.sites.size()) {\n      why = "the group names a site the program does not have";\n      return false;\n    }\n', ""),
    ("a site listed twice admitted", T, '  if (listed.size() != group.sites.size()) {\n    why = "a site is listed twice";\n    return false;\n  }\n', ""),
    ("group without output admitted", T, '  if (!group.output) {\n    why = "the group has no output in memory";\n    return false;\n  }\n', ""),
    ("group without sites admitted", T, '  if (group.sites.empty()) {\n    why = "the group has no sites";\n    return false;\n  }\n', ""),
    ("unsupported group admitted", T, '  if (!group.unsupported.empty()) {\n    why = group.unsupported;\n    return false;\n  }\n', ""),
    ("a site twice before an id past", T, '  for (unsigned id : group.sites)\n    if (id >= program.sites.size()) {\n      why = "the group names a site the program does not have";\n      return false;\n    }\n  // The checkers walk the list, so a site in it twice would be walked twice. The words\n  // are the set route\'s, which refused this before any single group did.\n  std::set<unsigned> listed(group.sites.begin(), group.sites.end());\n  if (listed.size() != group.sites.size()) {\n    why = "a site is listed twice";\n    return false;\n  }\n', '  std::set<unsigned> listed(group.sites.begin(), group.sites.end());\n  if (listed.size() != group.sites.size()) {\n    why = "a site is listed twice";\n    return false;\n  }\n  for (unsigned id : group.sites)\n    if (id >= program.sites.size()) {\n      why = "the group names a site the program does not have";\n      return false;\n    }\n'),
    # Roots and cells.
    ("one root may alias itself", T, "  if (first == second)\n    return CellRelation::Same;\n", ""),
    ("roots that may alias are apart", T, "  return rootSeparation(program, authorized, first, second).apart ? CellRelation::Distinct\n                                                                  : CellRelation::MayAlias;\n", "  return CellRelation::Distinct;\n"),
    ("a fresh root may alias", T, "  if (a.origin == MemoryRoot::FreshAllocation || b.origin == MemoryRoot::FreshAllocation) {", "  if (false) {"),
    ("a reversed name separates", T, '  out.name = "roots " + std::to_string(low) + " and " + std::to_string(high) + " are separate";', '  out.name = "roots " + std::to_string(first) + " and " + std::to_string(second) + " are separate";'),
    ("an absent root is apart", T, "  if (first >= program.roots.size() || second >= program.roots.size())\n    return CellRelation::MayAlias;\n", "  if (first >= program.roots.size() || second >= program.roots.size())\n    return CellRelation::Distinct;\n"),
    ("cell indices ignored", T, "  for (unsigned n = 0; n < first.index.size(); ++n)\n    if (first.index[n] != second.index[n])\n      return CellRelation::Distinct;\n", ""),
    ("two ranks are two cells", T, "  if (first.index.size() != second.index.size())\n    return CellRelation::MayAlias;\n", "  if (first.index.size() != second.index.size())\n    return CellRelation::Distinct;\n"),
    ("cell key drops the root", T, "  std::vector<uint64_t> out{root};\n", "  std::vector<uint64_t> out;\n  (void)root;\n"),
    ("cell key drops the index", T, "    out.push_back(value.getZExtValue());\n", "    (void)value;\n"),
    ("apart cells may meet", T, "  case CellRelation::Distinct:\n    return terms.boolean(false);\n", "  case CellRelation::Distinct:\n    return premise;\n"),
    ("aliasing cells never meet", T, "  case CellRelation::MayAlias:\n    return premise;\n  }\n", "  case CellRelation::MayAlias:\n    return terms.boolean(false);\n  }\n"),
    ("one root, any index", T, "    return terms.logicalAnd(premise, equalIndices);\n", "    return premise;\n"),
    # G5a's cells.
    ("domain flat cells by index only", D, "      cells[cellKey(roots[e], views[0][e])].push_back(e);\n", "      cells[cellKey(0, views[0][e])].push_back(e);\n"),
    ("domain segment cells by index only", D, "      row.cell = cellKey(program.sites[event.site].output->root, event.h);\n", "      row.cell = cellKey(0, event.h);\n"),
    ("domain same cell ignores roots", D, "          rootRelation(program, options.authorized, first.output->root, second.output->root), both,\n", "          CellRelation::Same, both,\n"),
    ("domain same cell ignores roots, segment", D, "          terms, rootRelation(program, options.authorized, first.output->root, second.output->root), both,\n", "          terms, CellRelation::Same, both,\n"),
    ("domain cells may alias", D, "          rootRelation(program, options.authorized, first.output->root, second.output->root), both,\n", "          CellRelation::MayAlias, both,\n"),
    ("domain cells may alias, segment", D, "          terms, rootRelation(program, options.authorized, first.output->root, second.output->root), both,\n", "          terms, CellRelation::MayAlias, both,\n"),
    ("domain cells apart", D, "          rootRelation(program, options.authorized, first.output->root, second.output->root), both,\n", "          CellRelation::Distinct, both,\n"),
    ("domain cells apart, segment", D, "          terms, rootRelation(program, options.authorized, first.output->root, second.output->root), both,\n", "          terms, CellRelation::Distinct, both,\n"),
    # G3b: what a refutation refutes, and the finite route's gate.
    ("C0 refutes this family only", G, "c0 ? Refutes::EveryFamily : Refutes::ThisFamily", "Refutes::ThisFamily"),
    ("every counterexample refutes every family", G, "c0 ? Refutes::EveryFamily : Refutes::ThisFamily", "Refutes::EveryFamily"),
    ("undefined labels refute every family", G, "      return refuted(why, Refutes::ThisFamily);\n", "      return refuted(why, Refutes::EveryFamily);\n"),
    ("a refused family refutes nothing", G, "    refutes(out, Refutes::ThisFamily);\n", ""),
    ("G3a unknown refutes", G, '      return CheckedFamily::unknown("the G3a checker gives no verdict at a point: " + checked.reason());\n', "      return refuted(checked.reason(), Refutes::ThisFamily);\n"),
    ("finite group gate skipped", G, "  std::string why;\n  if (!wholeGroup(program, enumeration.groups[group], why))\n    return CheckedFamily::unknown(why);\n", "  std::string why;\n"),
    ("not a sample is the scope", G, "  bool complete = enumerationRanItsScope(enumeration);\n", "  bool complete = !enumeration.sampled;\n"),
    # The scope an enumeration ran.
    ("scope not recorded", F, "  out.scope = options.scope;\n", ""),
    ("scope count not kept", F, "    out.scopePoints = out.points.size() + out.outsideDomain;\n", ""),
    ("a sample ran its scope", F, "  if (enumeration.sampled || enumeration.gate != GateStatus::Open)\n    return false;\n", "  if (enumeration.gate != GateStatus::Open)\n    return false;\n"),
    ("a closed gate ran its scope", F, "  if (enumeration.sampled || enumeration.gate != GateStatus::Open)\n    return false;\n", "  if (enumeration.sampled)\n    return false;\n"),
    ("points outside the recorded scope", F, "    if (!parametersInScope(enumeration.scope, point.parameters))\n      return false;\n", ""),
    ("repeated points counted", F, "  return seen.size() + enumeration.outsideDomain == enumeration.scopePoints;\n", "  return enumeration.points.size() + enumeration.outsideDomain == enumeration.scopePoints;\n"),
    ("outside points not counted", F, "  return seen.size() + enumeration.outsideDomain == enumeration.scopePoints;\n", "  return seen.size() == enumeration.scopePoints;\n"),
    ("any count ran its scope", F, "  return seen.size() + enumeration.outsideDomain == enumeration.scopePoints;\n", "  return true;\n"),
    # G5b-M4a: one domain claim over a union of groups.
    ("segment mixes groups admitted", D, "      if (set.gate->listedOf[site] != listed) {", "      if (false) {"),
    ("set placement skipped, finite", D, "  if (set && !placeSegments(program, request, *set, why))\n    return CheckedDomain::unknown(\"the request is refused: \" + why);\n  std::map<unsigned, unsigned> segmentOf", "  std::map<unsigned, unsigned> segmentOf"),
    ("set placement skipped, symbolic", D, "  if (set && !placeSegments(program, request, *set, why))\n    return CheckedDomain::unknown(\"the request is refused: \" + why);\n  if (!acceptCertificate(", "  if (!acceptCertificate("),
    ("roots that may alias leave nothing open", D, "      if (pair.relation == CellRelation::MayAlias && !set.aliasOpen)", "      if (false && !set.aliasOpen)"),
    ("alias gate ignored, finite", D, "  auto apart = [&](Fact fact) {\n    return set && set->aliasOpen ? unknownFact(*set->aliasOpen) : fact;\n  };", "  auto apart = [&](Fact fact) {\n    return fact;\n  };"),
    ("alias gate ignored, symbolic", D, "    auto apart = [&](Fact fact) {\n      return set && set->aliasOpen ? unknownFact(*set->aliasOpen) : fact;\n    };", "    auto apart = [&](Fact fact) {\n      return fact;\n    };"),
    ("pairs of two roots asked", D, "        if (set && first.output->root != second.output->root)\n          return true;\n", ""),
    ("every pair across segments skipped", D, "        if (set && first.output->root != second.output->root)\n", "        if (set)\n"),
    ("set binding drops the groups", D, "  for (const SiteGroup *group : canonical)\n    out += groupBinding(*group) + \";\";\n", ""),
    ("set binding is a group binding", D, "  std::string out = program.fingerprint + \"|set|\";", "  std::string out = program.fingerprint + \"|\";"),
    ("listing order kept", D, "  std::stable_sort(out.begin(), out.end(),\n                   [&](const SiteGroup *a, const SiteGroup *b) { return first(a) < first(b); });\n", ""),
    ("segment groups by listing", D, "    segmentGroups.push_back(std::find(canonical.begin(), canonical.end(), listed[index]) - canonical.begin());", "    segmentGroups.push_back(index);"),
    ("set gate ignored", D, "  if (!uniteGroups(program, listed, gate, why) || !traceIsFixed(program, why))\n    return finish(CheckedSetDomain::unknown(why), {}, {});", "  if ((uniteGroups(program, listed, gate, why), false) || !traceIsFixed(program, why))\n    return finish(CheckedSetDomain::unknown(why), {}, {});"),
    ("flat request over a set", D, "  if (request.segments.empty())\n    return finish(CheckedSetDomain::unknown(\"a claim over a set needs a segmented request\"), {}, {});\n", ""),
    ("zero limit accepted, set", D, "  if (!options.termination.solver.timeoutMilliseconds || !options.termination.solver.resourceLimit)\n    return CheckedSetDomain::unknown(\"a solver limit of zero is no limit\");", "  if (false)\n    return CheckedSetDomain::unknown(\"a solver limit of zero is no limit\");"),
    ("closed gate checked, set", D, "  if (enumeration.gate != GateStatus::Open)\n    return CheckedSetDomain::unknown(\"the domain gate is closed: \" + enumeration.reason);\n", ""),
    # G5b-M4b-1: Theorem 17 over a set of groups.
    ("set reduction refutation of the computation dropped", D, "    if (claim && claim->status() == ComputationStatus::Refuted) {\n      out.claim = ComputationStatus::Refuted;\n      out.refuted = RefutedBy::Computation;", "    if (false) {\n      out.claim = ComputationStatus::Refuted;\n      out.refuted = RefutedBy::Computation;"),
    ("set reduction refutation of the domain dropped", D, "    if (claim && claim->status() == ComputationStatus::Refuted) {\n      out.claim = ComputationStatus::Refuted;\n      out.refuted = RefutedBy::Domain;", "    if (false) {\n      out.claim = ComputationStatus::Refuted;\n      out.refuted = RefutedBy::Domain;"),
    ("set reduction refuted by the domain called the computation's", D, "      out.refuted = RefutedBy::Domain;", "      out.refuted = RefutedBy::Computation;"),
    ("set reduction witness dropped", D, "      out.counterexample = claim->witness();\n", ""),
    ("set reduction symbolic from finite", D, "    out.claimTier = whole ? ComputationTier::ScopeEnumerated : ComputationTier::Point;\n    out.every = computationFinite.holdsForEveryRun();", "    out.claimTier = ComputationTier::Symbolic;\n    out.every = computationFinite.holdsForEveryRun();"),
    ("set reduction symbolic without the domain", D, "  if (cs && computationSymbolic.status() == ComputationStatus::Verified && ds &&\n      domainSymbolic.status() == ComputationStatus::Verified && domainSymbolic.exactOrder()) {", "  if (cs && computationSymbolic.status() == ComputationStatus::Verified) {"),
    ("set reduction symbolic without the computation", D, "  if (cs && computationSymbolic.status() == ComputationStatus::Verified && ds &&\n      domainSymbolic.status() == ComputationStatus::Verified && domainSymbolic.exactOrder()) {", "  if (ds && domainSymbolic.status() == ComputationStatus::Verified && domainSymbolic.exactOrder()) {"),
    ("set reduction exact order not needed", D, "      domainSymbolic.status() == ComputationStatus::Verified && domainSymbolic.exactOrder()) {", "      domainSymbolic.status() == ComputationStatus::Verified) {"),
    ("set reduction alias gate dropped", D, "      if (pair.relation != CellRelation::Distinct) {", "      if (false) {"),
    ("set reduction computation binding unchecked", D, "    if (claim.binding() != compBinding || claim.groupSites() != listedSites)\n      return refuse(what);", "    if (false)\n      return refuse(what);"),
    ("set reduction domain binding unchecked", D, "    if (!same)\n      return refuse(what);", "    if (false)\n      return refuse(what);"),
    ("set reduction from the first group alone", D, "  CheckedComputation compFinite = checkGroupSetFinite(program, enumeration, groups, computation);", "  CheckedComputation compFinite = checkGroupSetFinite(program, enumeration, groups.take_front(1), computation);"),
    ("set reduction integer fact without trivial", D, "  if (out.claim == ComputationStatus::Verified && out.trivial.status == FactStatus::Proven) {\n    out.integer = isFloat ? unknownFact(\"not an integer fold\") : out.trivial;\n    out.floating = isFloat ? out.trivial : unknownFact(\"not a float fold\");\n  } else if (out.claim != ComputationStatus::Verified) {\n", "  if (out.claim == ComputationStatus::Verified) {\n    out.integer = isFloat ? unknownFact(\"not an integer fold\") : out.trivial;\n    out.floating = isFloat ? out.trivial : unknownFact(\"not a float fold\");\n  } else if (out.claim != ComputationStatus::Verified) {\n"),
    ("set reduction float fact from the integer rule", D, "  if (out.claim == ComputationStatus::Verified && out.trivial.status == FactStatus::Proven) {\n    out.integer = isFloat ? unknownFact(\"not an integer fold\") : out.trivial;\n    out.floating = isFloat ? out.trivial : unknownFact(\"not a float fold\");\n  } else if (out.claim != ComputationStatus::Verified) {\n", "  if (out.claim == ComputationStatus::Verified && out.trivial.status == FactStatus::Proven) {\n    out.integer = isFloat ? unknownFact(\"not an integer fold\") : out.trivial;\n    out.floating = out.trivial;\n  } else if (out.claim != ComputationStatus::Verified) {\n"),
    # G5b-M4b-2a: the integer fact over a set.
    ("set integer fact on an unverified claim", D, "  } else if (out.claim != ComputationStatus::Verified) {\n    out.integer = unknownFact(\"not an integer fold, or no claim\");\n    out.floating = unknownFact(\"not a float fold, or no claim\");\n  } else {", "  } else if (false) {\n    out.integer = unknownFact(\"not an integer fold, or no claim\");\n    out.floating = unknownFact(\"not a float fold, or no claim\");\n  } else {"),
    ("set integer fact ignores the fold", D, "    if (isFloat) {\n      // G5b-M4b-2b-1.", "    if (false) {\n      // G5b-M4b-2b-1."),
    ("set integer fact tier above the verdict", D, "        out.integer = proven(\"wrapping addition with no flag, unobserved, at the computation's tier\", out.claimTier);", "        out.integer = proven(\"wrapping addition with no flag, unobserved, at the computation's tier\", ComputationTier::Symbolic);"),
    ("set integer conditions skipped", D, "      if (integerReorderConditionsOver(program, outputs, values, why)) {", "      if (true) {"),
    ("set integer conditions over the first root", D, "      for (const SiteGroup *group : canonical)\n        if (group->output)\n          outputs.push_back(*group->output);\n", "      if (canonical.front()->output)\n        outputs.push_back(*canonical.front()->output);\n"),
    ("set witness over the first group", D, "        found = reorderWitnessOver(program, listed, point.parameters, options.limits.witnessFills,", "        found = reorderWitnessOver(program, llvm::ArrayRef<const SiteGroup *>(listed).take_front(1), point.parameters, options.limits.witnessFills,"),
    ("set refuted fact refutes the verdict", D, "        out.integer = found.status == FactStatus::Refuted ? found : unknownFact(why);\n", "        out.integer = found.status == FactStatus::Refuted ? found : unknownFact(why);\n        if (found.status == FactStatus::Refuted)\n          out.claim = ComputationStatus::Refuted;\n"),
    # G5b-M4b-2b-1: the float fact over a set.
    ("set float witness dropped", D, "      out.floating = searchWitness();", "      out.floating = unknownFact(\"not a float fold, or no claim\");"),
    ("set float proven without a witness", D, "      out.floating = searchWitness();", "      out.floating = searchWitness().status == FactStatus::Refuted ? searchWitness() : proven(\"addition is associative\", out.claimTier);"),
    ("set refuted float fact refutes the verdict", D, "      out.floating = searchWitness();", "      out.floating = searchWitness();\n      if (out.floating.status == FactStatus::Refuted)\n        out.claim = ComputationStatus::Refuted;"),
    ("judge NaN reversal unnamed", R, "               (reversed.isNaN() ? \" gives NaN, so that run would stop\" : \" changes its final bits\");", "               \" changes its final bits\";"),
    # G5b-M4b-2b-2: designed memories. The search: only after the fill sequence, only when fills were asked,
    # only by the judge, at every point looked at, over the whole union.
    ("designed search with no fills asked", R, "  if (!fills || replay.outcome != ReplayOutcome::Equal)\n    return out;\n  for (EntryState &designed", "  if (replay.outcome != ReplayOutcome::Equal)\n    return out;\n  for (EntryState &designed"),
    ("designed memory not judged", R, "    if (!reversalDiffers(program, group, designed, replay.prefix, detail))\n      continue;\n", "    (void)reversalDiffers(program, group, designed, replay.prefix, detail);\n"),
    ("designed witness not named", R, "    out.detail = detail + \", at a designed memory\";", "    out.detail = detail;"),
    ("designed witness memory dropped", R, "    out.memory = std::move(designed);\n", ""),
    ("designed set witness judges the first group", R, "  return searchDesigned(program, *united, replay, parameters, fills, maxCells);", "  return searchDesigned(program, *listed.front(), replay, parameters, fills, maxCells);"),
    ("T13 designed search dropped", D, "      if (designed.status == FactStatus::Refuted)\n        return designed;\n", "      (void)designed;\n"),
    ("T13 designed witness replaces a drawn one", D, "      if (found.status == FactStatus::Refuted)\n        return found;\n    }\n    // G5b-M4b-2b-2:", "      if (found.status == FactStatus::Refuted)\n        break;\n    }\n    // G5b-M4b-2b-2:"),
    ("set designed search dropped", D, "          if (designed.status == FactStatus::Refuted) {\n            found = designed;\n            break;\n          }\n", "          (void)designed;\n"),
    ("set designed witness replaces a drawn one", D, "      if (found.status != FactStatus::Refuted)\n        for (const PointEvidence *point : lookedAt) {", "      if (true)\n        for (const PointEvidence *point : lookedAt) {"),
    ("set designed search at the first point only", D, "          if (designed.status == FactStatus::Refuted) {\n            found = designed;\n            break;\n          }\n", "          if (designed.status == FactStatus::Refuted)\n            found = designed;\n          break;\n"),
    ("set designed search over the first group", D, "          Fact designed = designedWitnessOver(program, listed, point->parameters, options.limits.witnessFills,", "          Fact designed = designedWitnessOver(program, llvm::ArrayRef<const SiteGroup *>(listed).take_front(1), point->parameters, options.limits.witnessFills,"),
    # The family: where it applies, its targets, its patterns, the operand roles, what is left out.
    ("designed without nsw", R, "  if (!isFloat && (!(leader.reductionFlags & FlagNSW) || (leader.reductionFlags & FlagNUW)))", "  if (!isFloat && (leader.reductionFlags & FlagNUW))"),
    ("designed with nuw", R, "  if (!isFloat && (!(leader.reductionFlags & FlagNSW) || (leader.reductionFlags & FlagNUW)))", "  if (!isFloat && !(leader.reductionFlags & FlagNSW))"),
    ("designed sites of two flag sets", R, "    if (!site.output || site.reduction != leader.reduction || site.reductionFlags != leader.reductionFlags ||\n        site.multiplyFlags != leader.multiplyFlags)", "    if (!site.output || site.reduction != leader.reduction)"),
    ("designed cell with a constant aimed at", R, "    if (folded.size() < 2 || prefix.count(target))\n      continue;\n", "    if (folded.size() < 2)\n      continue;\n"),
    ("designed cell of one contribution counted", R, "    if (folded.size() < 2 || prefix.count(target))\n      continue;\n", "    if (prefix.count(target))\n      continue;\n"),
    ("designed first target only", R, "    if (targets++ == designedTargets)\n      break;\n", "    if (targets++ == 1)\n      break;\n"),
    ("designed maximum weakened", R, "    auto maximum = [](unsigned w) { return APInt::getSignedMaxValue(w); };", "    auto maximum = [](unsigned w) { return APInt::getSignedMaxValue(w) - 1; };"),
    ("designed minimum pattern dropped", R, "    return {{maximum, minus, plus, plus}, {minimum, plus, minus, plus}};", "    return {{maximum, minus, plus, plus}};"),
    ("designed binary32 at 2^53", R, "  double big = foldWidth == 32 ? 16777216.0 : 9007199254740992.0;", "  double big = 9007199254740992.0;"),
    ("designed largest pattern dropped", R, "  return {{value(big), value(1.0), value(-big), value(1.0)},\n          {value(largest), value(largest), value(-largest), value(1.0)}};", "  return {{value(big), value(1.0), value(-big), value(1.0)}};"),
    ("designed first contribution in the second operand only", R, "      for (bool firstInSecondOperand : {true, false})", "      for (bool firstInSecondOperand : {true})"),
    ("designed last contribution in the second operand only", R, "        for (bool lastInSecondOperand : {true, false}) {", "        for (bool lastInSecondOperand : {true}) {"),
    ("designed operand in a fresh root set", R, "            if (!settable(key)) {\n              consistent = false;\n              return;\n            }\n", ""),
    ("designed memory of two values kept", R, "          if (!consistent || std::find(listed.begin(), listed.end(), cells) != listed.end())", "          if (std::find(listed.begin(), listed.end(), cells) != listed.end())"),
    ("designed memory listed twice", R, "          if (!consistent || std::find(listed.begin(), listed.end(), cells) != listed.end())", "          if (!consistent)"),
    ("set wrapping algebra rescues a refuted order", D, "      out.claim = ComputationStatus::Refuted;\n      out.refuted = RefutedBy::Domain;", "      out.claim = computationFinite.fold() == FoldForm::ModularInteger ? ComputationStatus::Verified : ComputationStatus::Refuted;\n      out.refuted = RefutedBy::Domain;"),
    ("set reduction whole scope from the domain alone", D, "    bool whole = computationFinite.tier() == ComputationTier::ScopeEnumerated &&\n                 domainFinite.tier() == ComputationTier::ScopeEnumerated;", "    bool whole = domainFinite.tier() == ComputationTier::ScopeEnumerated;"),
    ("set reduction event count unchecked", D, "      if (counted != facts.events) {", "      if (false) {"),
    ("set reduction order contradiction unchecked", D, "      if (respects && found->second < computationFinite.groupPoints().size() &&\n          !computationFinite.groupPoints()[found->second].listedOrder) {", "      if (false) {"),
    # Statement-level work, phase 4 (docs/GENERALIZED_GEMM_STATEMENT_BV.md Section 11).
    ("identities not applied to guards", P, "    Term term = applyIdentities(terms, original, identities);", "    Term term = original;"),
    ("views proposed without the sample check", P, "        if (fitsEvents(view, d, mapped))", "        if (true)"),
    ("certificate never strengthened", N, "      if (certificate && entry.request && request.closedFormViews && !out.identities.empty()) {", "      if (false) {"),
    ("eligibility ignores affinity", N, "                        allAffine && v.domainSymbolic().exactOrder() && reorder;", "                        v.domainSymbolic().exactOrder() && reorder;"),
    ("products called affine", N, "      return (constantOnly(t->args[0]) && affine(t->args[1])) ||", "      return (affine(t->args[0]) && affine(t->args[1])) ||"),
]

EQUIVALENT = [
    ("box multiplicity unchecked", "a defensive check that cannot fire: constantBox admits a box only "
     "when the guard and every other level's bounds read no repeat level and the repeat bounds read "
     "parameters alone, so S(v) = S_rest(v) x R(v), and with Theorem 12 at the point each logical "
     "point has exactly |R(v)| events, which is the product. Evidence: every ConstantBox case of "
     "DomainTest and of the oracle's multiplicity check."),
    ("box multiplicity unchecked, segment", "the same defensive check inside a segment, which cannot fire "
     "for the same reason: a segment's box is admitted under the conditions of Theorem 14 over the "
     "segment's own levels, and Theorem 15 at the point gives each logical point of the segment "
     "exactly |R(v)| events. Evidence: the sib_repeat case of testSegments and the oracle's "
     "segment multiplicity check."),
    ("mu span not clamped", "a clamp that the finite check cannot see fail: the repeat levels of a "
     "box have lo < hi at every event, since each event's repeat value lies in [lo, hi), so the "
     "clamp matters only at a logical point with no event, where μ is read by no one. The "
     "symbolic μ term then differs from the product of the clamps, which is a weaker statement "
     "that the oracle does not test either. Kept as a defence; listed here with that caveat."),
    ("box ignores empty spans", "with a repeat span empty, R(v) and so S(v) are empty, no logical "
     "point exists, and μ is claimed of none: the value of the product there is never read."),
    ("T13 without exact order", "a symbolic CheckedDomain is Verified only on the path that also "
     "sets exactOrder, so the conjunct is implied by the one before it."),
    ("integer float fold admitted", "integerReorderConditions is called only on the integer "
     "branch of checkReduction, so a float fold never reaches the test it removes."),
    ("scope sampled", "with `sampled` set the enumeration lists only its sample points, and the "
     "count must still equal the scope's: a sample that lists every point of the scope is the "
     "whole enumeration, and nothing else passes the count."),
    ("T13 finite without G4a", "the finite join keeps only points that G4a's finite claim lists, "
     "and a CheckedComputation lists points only on its Verified path, so with G4a unverified the "
     "list is empty and the result Unknown either way. Evidence: the unnamed case of testGapsG5."),
    ("integer unverified admitted", "a CheckedComputation sets its fold form only on its Verified "
     "paths, so an unverified one carries the default ORDERED_FLOAT and the next condition refuses "
     "it. Evidence: the unnamed case of testGapsG5."),
    ("integer flags ignored", "an nsw or nuw reduction add stores a value whose poison is a side "
     "condition `poison reaches memory` reading the output's load, which the stop check refuses "
     "first. Evidence: the mixed_flags case of testGapsG5."),
    ("domain flat cells by index only", "G5b-M4-0 keys G5a's cells by root and index for M4's sets of "
     "groups, but within one group every site writes the group's output root: the gate admits only "
     "the direct sites of one root triple. The root in the key is then one constant, and the map "
     "groups the events exactly as before. Evidence: the identity section asks cellKey of two roots "
     "and one index directly."),
    ("domain same cell ignores roots", "within one group both sites of a pair write the group's output "
     "root, and rootRelation of a root of the program with itself is Same, so the formula is unchanged. "
     "The may-alias and apart variants of this call are killed, which shows the call is read."),
    ("domain same cell ignores roots, segment", "the same, for pairs within and across the segments of "
     "one group. In a union (G5b-M4a) a pair of two roots never reaches the formula: their relation "
     "answers first, and the mutants `pairs of two roots asked` and `every pair across segments "
     "skipped` are killed."),
    ("integer guard loads ignored", "IR-5 is a defence that no claim reaches: a guard that loads an output "
     "root, or reads state that holds a value from memory, makes the trace depend on memory contents, and the "
     "obliviousness gate (traceIsFixed, which every computation and domain claim requires) closes before any "
     "claim is made, so integerReorderConditions is never called for such a program. Evidence: set_guard and "
     "set_guard_state in the setreorder section, both gated UNSUPPORTED with every fact Unknown. Declared "
     "2026-11-12 after that analysis, before the frozen run; the four-mutant smoke run of the same day, from a "
     "tree without this entry, is the empirical record."),
    ("set reduction exact order not needed", "a symbolic CheckedSetDomain is Verified only on the path that "
     "also sets exactOrder (the set route's claim is the segmented route's, which sets the orbit with every "
     "verified claim), so the conjunct is implied by the one before it, as for `T13 without exact order` in "
     "the one-group join. Kept as a defence against a future symbolic route that verifies without an exact "
     "order. Survived the frozen run of 2026-11-12 and was declared with this reason; the harness was run "
     "again on the final tree."),
    ("set reduction event count unchecked", "a defect detector. Both claims read the one trace at a point "
     "(obliviousness, which both gates require), so their event counts agree on every tree that builds, and "
     "no test can hand the composer two genuine claims that disagree: the verdict has no public constructor "
     "for a claim. Kept as the EC check of the design; listed here with that reason."),
    ("set reduction order contradiction unchecked", "the same: M3's listed-order fact and Theorem 15 over the "
     "union read one trace, and never contradict each other on a tree that builds. OC is a defect detector, "
     "never the order's source, and no test can fabricate the contradiction."),
    ("set reduction whole scope from the domain alone", "both finite claims demote together: each ties "
     "ScopeEnumerated to enumerationCoversScope over the one scope in DomainOptions and to no stopped point, "
     "and the composer joins them only at points both list. A computation claim left open at a point while "
     "the domain is whole needs a replay that is neither equal nor refuted, which no program of the tests "
     "produces for a set; the whole-scope minimum is kept, with this caveat."),
    ("G3a unknown refutes", "G3a gives unknown only for a malformed candidate, with no events or a label "
     "list of another length. The finite route asks it only at a point whose domain exists, which the "
     "enumerator makes only for a table with events, and familyLabels gives every event one label on "
     "every axis or fails before G3a is asked. A domain is made by the enumerator alone."),
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


def limit_of(label):
    """The 512-program sweep runs about 25 minutes; every other test far less."""
    return 3600 if label == "holes" else TEST_SECONDS


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
        if label == "programs":
            own = ("driver failed" not in done.stderr and "Traceback" not in done.stderr and
                   done.stderr.strip())
        elif label in ("identity-programs", "setdomain-programs", "setreduction-programs"):
            # Its own disagreements only: a driver that failed or ran out of time is no kill.
            own = ("driver exit" not in done.stderr and "timed out" not in done.stderr and
                   "Traceback" not in done.stderr and "FAIL: " in done.stderr)
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
        return run(["ninja", "-C", self.build, "-j2", "generalized-gemm-domain-test",
                    "generalized-gemm-domain-report", "generalized-gemm-identity-report"],
                   timeout=BUILD_SECONDS, memory=COMPILE_LIMIT)

    def commands(self):
        fuzz = self.root / "research" / "generalized-gemm" / "test" / "fuzz"
        test = self.build / "generalized-gemm-domain-test"
        report = self.build / "generalized-gemm-domain-report"
        # G5b-M4-0: the identity section first, which kills its mutants in seconds, and the
        # identity oracle last, for a mutant that every other test lets through.
        out = [(name, [test, name]) for name in
               ("phase4", "identity", "setdomain", "setreduction", "setreorder", "setfloat", "designed", "reviewed", "gaps",
                "multiplicity", "siblings", "segments", "adversarial", "limits", "binding", "wrapping", "catalogue", "orders")]
        out.append(("holes", [test, "holes"]))
        out.append(("programs", [sys.executable, fuzz / "domain_oracle.py", "--driver", report,
                                 "--programs", "6", "--fills", "6", "--seed", "41", "--jobs", "1"]))
        out.append(("identity-programs", [sys.executable, fuzz / "identity_oracle.py", "--driver",
                                          self.build / "generalized-gemm-identity-report", "--programs", "12",
                                          "--seed", "41", "--jobs", "1", "--no-minimum"]))
        out.append(("setdomain-programs", [sys.executable, fuzz / "setdomain_oracle.py", "--driver", report,
                                           "--programs", "6", "--seed", "41", "--jobs", "1", "--no-minimum"]))
        out.append(("setreduction-programs", [sys.executable, fuzz / "setreduction_oracle.py", "--driver", report,
                                              "--programs", "6", "--fills", "3", "--seed", "41", "--jobs", "1",
                                              "--no-minimum"]))
        # --first: the named tests before the others, in the order given. A kill is a kill by
        # any test, so the order changes which test is named and how long a run takes, nothing else.
        first = [label for label in self.args.first.split(",") if label]
        unknown = sorted(set(first) - {label for label, _ in out})
        if unknown:
            raise RuntimeError(f"--first names no test: {unknown}")
        return sorted(out, key=lambda item: first.index(item[0]) if item[0] in first else len(first))

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
                outcome = verdict_of(label, run(command, timeout=limit_of(label)))
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
    parser.add_argument("--only", default="", help="mutant names, comma-separated, or semicolon-separated "
                        "when a name holds a comma")
    parser.add_argument("--workers", type=int, default=1)
    parser.add_argument("--first", default="", help="tests to run before the others, comma-separated, for a run "
                        "of a few mutants whose killers are known; the tests and the results are the same")
    parser.add_argument("--results", required=True, help="a JSON-lines file, appended to")
    args = parser.parse_args()
    if args.workers > 2:
        print("at most two workers: compiles take 1.6 GB each and other jobs share this machine")
        return 2

    # Names may hold a comma ("D2 increase ignored, segment"): a semicolon then separates.
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
        try:
            result = run(command, timeout=limit_of(label))
        except subprocess.TimeoutExpired:
            print(f"BASELINE TIMED OUT: {label}")
            return 2
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
