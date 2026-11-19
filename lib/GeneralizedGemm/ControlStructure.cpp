#include "GeneralizedGemm/ControlStructure.h"

#include <algorithm>
#include <functional>

using namespace mlir::generalized_gemm;

namespace mlir::generalized_gemm {

std::vector<llvm::SmallVector<unsigned>>
cyclicComponents(unsigned nodeCount,
                 llvm::ArrayRef<std::pair<unsigned, unsigned>> edges) {
  std::vector<std::vector<unsigned>> successors(nodeCount);
  std::vector<bool> selfEdge(nodeCount, false);
  for (auto [from, to] : edges) {
    successors[from].push_back(to);
    if (from == to)
      selfEdge[from] = true;
  }
  // Tarjan. Depth is bounded by the node count, which the importer limits.
  constexpr int unvisited = -1;
  std::vector<int> order(nodeCount, unvisited), low(nodeCount, 0);
  std::vector<bool> onStack(nodeCount, false);
  std::vector<unsigned> stack;
  std::vector<llvm::SmallVector<unsigned>> components;
  int counter = 0;
  std::function<void(unsigned)> visit = [&](unsigned node) {
    order[node] = low[node] = counter++;
    stack.push_back(node);
    onStack[node] = true;
    for (unsigned next : successors[node]) {
      if (order[next] == unvisited) {
        visit(next);
        low[node] = std::min(low[node], low[next]);
      } else if (onStack[next]) {
        low[node] = std::min(low[node], order[next]);
      }
    }
    if (low[node] != order[node])
      return;
    llvm::SmallVector<unsigned> component;
    unsigned member;
    do {
      member = stack.back();
      stack.pop_back();
      onStack[member] = false;
      component.push_back(member);
    } while (member != node);
    if (component.size() > 1 || selfEdge[node]) {
      llvm::sort(component);
      components.push_back(std::move(component));
    }
  };
  for (unsigned node = 0; node < nodeCount; ++node)
    if (order[node] == unvisited)
      visit(node);
  std::sort(components.begin(), components.end(),
            [](const auto &a, const auto &b) { return a.front() < b.front(); });
  return components;
}

llvm::SmallVector<unsigned> computeCutSet(const SemanticProgram &program) {
  std::vector<std::vector<unsigned>> successors(program.locationCount);
  for (const Transition &t : program.transitions)
    if (t.to < program.locationCount)
      successors[t.from].push_back(t.to);
  enum Mark { New, Active, Done };
  std::vector<Mark> mark(program.locationCount, New);
  std::vector<bool> cut(program.locationCount, false);
  std::function<void(unsigned)> visit = [&](unsigned node) {
    mark[node] = Active;
    for (unsigned next : successors[node]) {
      if (mark[next] == New)
        visit(next);
      else if (mark[next] == Active)
        cut[next] = true;
    }
    mark[node] = Done;
  };
  if (program.locationCount)
    visit(program.entryLocation);
  llvm::SmallVector<unsigned> cuts;
  for (unsigned node = 0; node < program.locationCount; ++node)
    if (cut[node])
      cuts.push_back(node);
  return cuts;
}

bool cutSetBreaksCycles(const SemanticProgram &program,
                        llvm::ArrayRef<unsigned> cuts, std::string &why) {
  std::vector<bool> isCut(program.locationCount, false);
  for (unsigned i = 0; i < cuts.size(); ++i) {
    if (cuts[i] >= program.locationCount) {
      why = "a cut is not a location";
      return false;
    }
    if (i && cuts[i - 1] >= cuts[i]) {
      why = "the cut set is not sorted and unique";
      return false;
    }
    if (cuts[i] == program.entryLocation) {
      why = "the entry cannot be a cut: it has no predecessor";
      return false;
    }
    isCut[cuts[i]] = true;
  }
  llvm::SmallVector<std::pair<unsigned, unsigned>> residual;
  for (const Transition &t : program.transitions)
    if (t.to < program.locationCount && !isCut[t.to])
      residual.push_back({t.from, t.to});
  auto remaining = cyclicComponents(program.locationCount, residual);
  if (!remaining.empty()) {
    why = "a cycle through location " + std::to_string(remaining[0][0]) +
          " avoids every cut";
    return false;
  }
  return true;
}

} // namespace mlir::generalized_gemm
