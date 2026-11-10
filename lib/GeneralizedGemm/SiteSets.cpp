//===- SiteSets.cpp -------------------------------------------------------===//
#include "GeneralizedGemm/SiteSets.h"

#include <algorithm>
#include <set>
#include <tuple>

using llvm::APInt;

namespace mlir::generalized_gemm {

bool wholeGroup(const SemanticProgram &program, const SiteGroup &group, std::string &why) {
  if (!group.unsupported.empty()) {
    why = group.unsupported;
    return false;
  }
  if (!group.output) {
    why = "the group has no output in memory";
    return false;
  }
  if (group.sites.empty()) {
    why = "the group has no sites";
    return false;
  }
  // Every program site is compared below, and an id past them would be read later.
  for (unsigned id : group.sites)
    if (id >= program.sites.size()) {
      why = "the group names a site the program does not have";
      return false;
    }
  // The checkers walk the list, so a site in it twice would be walked twice. The words
  // are the set route's, which refused this before any single group did.
  std::set<unsigned> listed(group.sites.begin(), group.sites.end());
  if (listed.size() != group.sites.size()) {
    why = "a site is listed twice";
    return false;
  }
  for (const MacSite &site : program.sites) {
    bool direct = site.form == ReductionForm::Direct && site.output && site.outputRead &&
                  site.outputRead->root == site.output->root;
    bool same = direct && site.output->root == *group.output && site.a.root == group.first &&
                site.b.root == group.second;
    if (same != (listed.count(site.id) != 0)) {
      why = "the group is not the sites of its roots";
      return false;
    }
  }
  return true;
}

bool uniteGroups(const SemanticProgram &program, llvm::ArrayRef<const SiteGroup *> listed,
                 GroupUnion &out, std::string &why) {
  if (listed.empty()) {
    why = "no group is listed";
    return false;
  }
  if (listed.size() > 64) {
    why = "more than 64 groups are listed";
    return false;
  }
  std::set<std::tuple<unsigned, unsigned, unsigned>> triples;
  GroupUnion united;
  united.listedOf.assign(program.sites.size(), -1);
  for (unsigned i = 0; i < listed.size(); ++i) {
    const SiteGroup &group = *listed[i];
    // A claim about part of a group is no claim about its events, here as alone.
    if (!wholeGroup(program, group, why))
      return false;
    if (!triples.insert({*group.output, group.first, group.second}).second) {
      why = "a group is listed twice";
      return false;
    }
    for (unsigned site : group.sites) {
      if (united.listedOf[site] != -1) {
        why = "a site is listed twice";
        return false;
      }
      united.listedOf[site] = i;
      united.sites.push_back(site);
    }
  }
  // Increasing, as one group's are: whatever the listing, every obligation is then asked
  // in the same order, and so is the first one that fails.
  std::sort(united.sites.begin(), united.sites.end());
  out = std::move(united);
  return true;
}

RootSeparation rootSeparation(const SemanticProgram &program, const Authorizations &authorized,
                              unsigned first, unsigned second) {
  unsigned low = std::min(first, second), high = std::max(first, second);
  RootSeparation out;
  out.name = "roots " + std::to_string(low) + " and " + std::to_string(high) + " are separate";
  if (high >= program.roots.size() || low == high)
    return out;
  const MemoryRoot &a = program.roots[low], &b = program.roots[high];
  if (a.origin == MemoryRoot::FreshAllocation || b.origin == MemoryRoot::FreshAllocation) {
    out.apart = true;
    out.assumption = false;
    out.basis = "fresh allocation";
  } else if (authorized.count(out.name)) {
    out.apart = true;
    out.basis = a.restrictArgument || b.restrictArgument
                    ? "authorized, with an llvm.noalias source contract"
                    : "authorized";
  }
  return out;
}

const char *stringifyCellRelation(CellRelation relation) {
  switch (relation) {
  case CellRelation::Same: return "SAME";
  case CellRelation::Distinct: return "DISTINCT";
  case CellRelation::MayAlias: return "MAY_ALIAS";
  }
  return "MAY_ALIAS";
}

CellRelation rootRelation(const SemanticProgram &program, const Authorizations &authorized,
                          unsigned first, unsigned second) {
  if (first >= program.roots.size() || second >= program.roots.size())
    return CellRelation::MayAlias;
  if (first == second)
    return CellRelation::Same;
  return rootSeparation(program, authorized, first, second).apart ? CellRelation::Distinct
                                                                  : CellRelation::MayAlias;
}

CellRelation relateCells(const SemanticProgram &program, const Authorizations &authorized,
                         const CellRef &first, const CellRef &second) {
  CellRelation roots = rootRelation(program, authorized, first.root, second.root);
  if (roots != CellRelation::Same)
    return roots;
  // Two indices of one root have one rank and one width. Anything else is no cell of the
  // model, and is never called distinct.
  if (first.index.size() != second.index.size())
    return CellRelation::MayAlias;
  for (unsigned n = 0; n < first.index.size(); ++n)
    if (first.index[n].getBitWidth() != second.index[n].getBitWidth())
      return CellRelation::MayAlias;
  for (unsigned n = 0; n < first.index.size(); ++n)
    if (first.index[n] != second.index[n])
      return CellRelation::Distinct;
  return CellRelation::Same;
}

std::vector<uint64_t> cellKey(unsigned root, llvm::ArrayRef<APInt> index) {
  std::vector<uint64_t> out{root};
  for (const APInt &value : index)
    out.push_back(value.getZExtValue());
  return out;
}

Term sameCellFormula(TermContext &terms, CellRelation roots, Term premise, Term equalIndices) {
  switch (roots) {
  case CellRelation::Same:
    return terms.logicalAnd(premise, equalIndices);
  case CellRelation::Distinct:
    return terms.boolean(false);
  case CellRelation::MayAlias:
    return premise;
  }
  return premise;
}

} // namespace mlir::generalized_gemm
