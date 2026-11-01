#include "GeneralizedGemm/CanonicalRecovery.h"

#include <numeric>
#include <utility>

using namespace llvm;

namespace mlir::generalized_gemm {

namespace {

class UnionFind {
public:
  explicit UnionFind(size_t size) : parent(size) {
    std::iota(parent.begin(), parent.end(), 0u);
  }
  uint32_t find(uint32_t x) {
    while (parent[x] != x) {
      parent[x] = parent[parent[x]];
      x = parent[x];
    }
    return x;
  }
  void unite(uint32_t a, uint32_t b) {
    a = find(a);
    b = find(b);
    if (a != b)
      parent[std::max(a, b)] = std::min(a, b);
  }

private:
  std::vector<uint32_t> parent;
};

/// Each block of a partition is one connected set, so linking every member to
/// the first member of its block is the whole join step.
void link(UnionFind &sets, const Partition &partition) {
  std::map<uint32_t, uint32_t> first;
  for (uint32_t t = 0; t < partition.size(); ++t) {
    auto [slot, added] = first.try_emplace(partition[t], t);
    if (!added)
      sets.unite(t, slot->second);
  }
}

Partition join(ArrayRef<const Partition *> parts) {
  size_t size = parts[0]->size();
  UnionFind sets(size);
  for (const Partition *part : parts)
    link(sets, *part);
  Partition labels(size);
  for (uint32_t t = 0; t < size; ++t)
    labels[t] = sets.find(t);
  return normalizePartition(labels);
}

Partition meet(const Partition &a, const Partition &b) {
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> ids;
  Partition labels;
  labels.reserve(a.size());
  for (size_t t = 0; t < a.size(); ++t)
    labels.push_back(ids.try_emplace({a[t], b[t]}, (uint32_t)ids.size()).first->second);
  return normalizePartition(labels);
}

} // namespace

void viewKernels(const AccessTable &table, Partition &kerH, Partition &kerF,
                 Partition &kerG) {
  kerH.clear();
  kerF.clear();
  kerG.clear();
  for (const AccessTriple &t : table.triples()) {
    kerH.push_back(t.h);
    kerF.push_back(t.f);
    kerG.push_back(t.g);
  }
  kerH = normalizePartition(kerH);
  kerF = normalizePartition(kerF);
  kerG = normalizePartition(kerG);
}

CanonicalPartitions joinKernels(Partition kerH, Partition kerF, Partition kerG) {
  CanonicalPartitions out;
  out.kerH = std::move(kerH);
  out.kerF = std::move(kerF);
  out.kerG = std::move(kerG);
  out.rhoI = join({&out.kerH, &out.kerF});
  out.rhoJ = join({&out.kerH, &out.kerG});
  out.rhoK = join({&out.kerF, &out.kerG});
  out.rhoP = join({&out.kerH, &out.kerF, &out.kerG});
  out.meetH = meet(out.rhoI, out.rhoJ) == out.kerH;
  out.meetF = meet(out.rhoI, out.rhoK) == out.kerF;
  out.meetG = meet(out.rhoK, out.rhoJ) == out.kerG;
  return out;
}

CoordinateCandidate canonicalCandidate(const AccessTable &table,
                                       const CanonicalPartitions &partitions) {
  CoordinateCandidate candidate;
  candidate.origin = "canonical joins";
  for (const AccessEvent &event : table.events()) {
    candidate.p.push_back(partitions.rhoP[event.triple]);
    candidate.i.push_back(partitions.rhoI[event.triple]);
    candidate.j.push_back(partitions.rhoJ[event.triple]);
    candidate.k.push_back(partitions.rhoK[event.triple]);
  }
  return candidate;
}

} // namespace mlir::generalized_gemm
