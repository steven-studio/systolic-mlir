//===- ControlStructure.h - Cycles of a SemanticProgram ---------*- C++ -*-===//
//
// Phase G2a (docs/GENERALIZED_GEMM_G2A.md). Graph facts over the transitions of
// a SemanticProgram. Nothing here reads a guard: these are syntactic edges, so
// a cycle found here may be infeasible, and an acyclic graph has no cycle.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_CONTROLSTRUCTURE_H
#define GENERALIZEDGEMM_CONTROLSTRUCTURE_H

#include "GeneralizedGemm/SemanticProgram.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <string>
#include <utility>
#include <vector>

namespace mlir::generalized_gemm {

/// The strongly connected components that contain a cycle: more than one node,
/// or one node with a self edge. Nodes of each are sorted, and components are
/// sorted by their first node.
std::vector<llvm::SmallVector<unsigned>>
cyclicComponents(unsigned nodeCount,
                 llvm::ArrayRef<std::pair<unsigned, unsigned>> edges);

/// A cut set: the targets of the retreating edges of a depth-first walk from
/// the entry, sorted. Every cycle contains one. The result is a proposal: use
/// cutSetBreaksCycles to check it.
llvm::SmallVector<unsigned> computeCutSet(const SemanticProgram &program);

/// Whether `cuts` is sorted, duplicate free, inside the program, without the
/// entry, and leaves no cycle once every edge into a cut is removed. On false,
/// `why` names the failure.
bool cutSetBreaksCycles(const SemanticProgram &program,
                        llvm::ArrayRef<unsigned> cuts, std::string &why);

} // namespace mlir::generalized_gemm
#endif
