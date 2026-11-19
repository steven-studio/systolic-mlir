//===- SiteSets.h - Groups of sites and cells, for every checker -*- C++ -*-===//
//
// G5b-M4-0. One definition of what a group of sites is, and of when two cells of memory
// are one, for every checker that is handed a group or compares cells. G3b, G4a and G5a
// each kept their own copy of the group check, and the copies had drifted: only G4a's
// bounded the site ids. The domain checker compared cells by index alone.
//
//   wholeGroup    A group is exactly the direct sites of its root triple, each listed once,
//                 and every id it lists is a site of the program.
//   uniteGroups   A set of groups is whole groups, no triple twice and no site twice.
//   roots         Two roots are apart when one is a fresh allocation, which is a proof, or
//                 when their pair is authorized by name, which is an assumption to record.
//                 Different root ids say nothing by themselves.
//   cells         A cell is a root and an index tuple. Two cells of one root are one cell
//                 exactly when their indices are equal. Two cells of roots that are apart
//                 are two cells. Two cells of roots that may alias may be one cell, and
//                 nothing may say they are not.
//
// Nothing here proves a claim. A checker that rests on these answers says so.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SITESETS_H
#define GENERALIZEDGEMM_SITESETS_H

#include "GeneralizedGemm/FiniteDomain.h"
#include "GeneralizedGemm/ProofStatus.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// True when `group` is supported, has an output, lists at least one site, lists only ids
/// of sites the program has, lists none twice, and lists exactly the direct sites of its
/// root triple. False otherwise, with `why` naming the first failure in that order.
bool wholeGroup(const SemanticProgram &program, const SiteGroup &group, std::string &why);

/// The union of several whole groups (G5b-M3 for G4a, G5b-M4a for the domain).
struct GroupUnion {
  std::vector<unsigned> sites; // Every listed site, in increasing order.
  std::vector<int> listedOf;   // Per program site: the listed index of its group, or -1.
};

/// One gate for every claim over a set of groups: at least one group and at most 64, each
/// whole, no root triple listed twice, no site in two listed groups. False otherwise,
/// with `why` naming the first failure, groups visited in the listed order.
bool uniteGroups(const SemanticProgram &program, llvm::ArrayRef<const SiteGroup *> listed,
                 GroupUnion &out, std::string &why);

/// How two different roots stand, as G4a records it.
struct RootSeparation {
  bool apart = false;      // Known apart, by either basis.
  bool assumption = true;  // False only for a fresh allocation, which is a proof.
  std::string basis;       // "fresh allocation", "authorized", or empty when not apart.
  std::string name;        // The authorization that would make them apart.
};
RootSeparation rootSeparation(const SemanticProgram &program, const Authorizations &authorized,
                              unsigned first, unsigned second);

enum class CellRelation { Same, Distinct, MayAlias };
const char *stringifyCellRelation(CellRelation relation);

/// One root against another: Same for one root, Distinct when they are apart, MayAlias
/// otherwise. A root the program does not have may alias anything.
CellRelation rootRelation(const SemanticProgram &program, const Authorizations &authorized,
                          unsigned first, unsigned second);

/// A cell of the model: a root and the values of its index.
struct CellRef {
  unsigned root = 0;
  std::vector<llvm::APInt> index;
};

/// Same when the roots are one and the indices are equal, Distinct when the roots are
/// one and the indices differ or the roots are apart, MayAlias when the roots may alias
/// or the two indices differ in rank or width.
CellRelation relateCells(const SemanticProgram &program, const Authorizations &authorized,
                         const CellRef &first, const CellRef &second);

/// The key of a cell in a map of one checker's cells: its root, then its index values.
/// Equal keys are one cell. Unequal keys are two cells only for one root, or for roots
/// that are apart: a checker that compares cells of two roots asks rootRelation first.
std::vector<uint64_t> cellKey(unsigned root, llvm::ArrayRef<llvm::APInt> index);

/// The formula that two cells are one, given how their roots stand, the premise that both
/// events happen and the formula that their indices are equal. One root: `premise` and
/// `equalIndices`. Roots apart: false. Roots that may alias: `premise`, so that any fact
/// resting on the two cells being apart stays unproven.
Term sameCellFormula(TermContext &terms, CellRelation roots, Term premise, Term equalIndices);

} // namespace mlir::generalized_gemm
#endif
