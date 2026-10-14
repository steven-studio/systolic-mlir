//===- SystolicTilePlan.cpp - The binary-search tile plan of one GEMM -----===//
//
// Moved out of SystolicBinarySearchTileMatmulPass.cpp unchanged in substance,
// so that the affine path (AffineMatmulToSystolic.cpp) decomposes, assigns
// and schedules a GEMM exactly as the linalg path does.
//
//===----------------------------------------------------------------------===//

#include "Systolic/SystolicTilePlan.h"

#include "Systolic/CostModel.h"
#include "Systolic/DeviceConfig.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/BuiltinTypes.h"

#include <algorithm>
#include <string>

using namespace mlir;
using namespace mlir::systolic;

FailureOr<SystolicFleet> mlir::systolic::collectSystolicFleet(ModuleOp module) {
  SystolicFleet fleet;
  bool ok = true;
  int64_t nextAcceleratorId = 0;

  module.walk([&](DeviceOp device) {
    const int64_t rows = device.getRows();
    const int64_t cols = device.getCols();

    if (rows <= 0 || cols <= 0 || rows != cols) {
      device.emitError()
          << "systolic.device must have a positive square geometry";
      ok = false;
      return;
    }

    if (IntegerAttr attr = device.getTileOverheadAttr()) {
      if (attr.getInt() < 0) {
        device.emitError() << "tile_overhead must be non-negative";
        ok = false;
        return;
      }
    }

    // Reduction capacity of one invocation. A device that declares k_max
    // gets every tile's reduction folded into ceil(K / k_max) invocations; a
    // device that does not keeps K whole.
    if (IntegerAttr attr = device.getKMaxAttr()) {
      if (attr.getInt() <= 0) {
        device.emitError() << "k_max must be positive";
        ok = false;
        return;
      }
    }

    // Everything the cost model prices an invocation with (k_max, H, the
    // read and write terms) -- read once, through the same helper every
    // costing pass uses.
    SystolicArrayResource resource;
    resource.arraySize = rows;
    resource.acceleratorId = nextAcceleratorId++;
    resource.config = getArrayConfig(device);

    if (!::systolic::isValidArrayConfig(resource.config)) {
      device.emitError() << "systolic.device has cost parameters the cost "
                            "model cannot use";
      ok = false;
      return;
    }

    fleet.symbols.push_back(
        FlatSymbolRefAttr::get(device.getContext(), device.getSymName()));
    fleet.resources.push_back(resource);
  });

  if (!ok)
    return failure();

  // k_max is per accelerator and the geometries are independent: the 8x8 and
  // the 4x4s of one fleet may declare different values, and none of them has
  // to be a power of two. Within ONE geometry they must agree, and so must
  // every other cost parameter: the optimizer bills one cost per geometry
  // (the cost model's tile cost, sum over I(g) = ceil(K / k_max(g))
  // invocations) and spreads a geometry's tiles round-robin over its
  // accelerators.
  const auto &resources = fleet.resources;
  for (size_t i = 0; i < resources.size(); ++i) {
    for (size_t j = 0; j < i; ++j) {
      if (resources[i].arraySize != resources[j].arraySize)
        continue;
      const ::systolic::ArrayConfig &a = resources[i].config;
      const ::systolic::ArrayConfig &b = resources[j].config;
      if (a.kMax != b.kMax) {
        auto describe = [](int64_t kMax) {
          return kMax > 0 ? std::to_string(kMax) : std::string("none");
        };
        module.emitError()
            << "systolic.device " << fleet.symbols[i] << " declares k_max = "
            << describe(a.kMax) << " but " << fleet.symbols[j]
            << " of the same " << resources[i].arraySize << "x"
            << resources[i].arraySize
            << " geometry declares k_max = " << describe(b.kMax)
            << "; the binary-search decomposition bills one cost per "
               "geometry, so every accelerator of one geometry must "
               "declare the same k_max";
        return failure();
      }
      if (a.tileOverhead != b.tileOverhead ||
          a.readBytesPerCycle != b.readBytesPerCycle ||
          a.writeBytesPerCycle != b.writeBytesPerCycle ||
          a.readOverhead != b.readOverhead ||
          a.writeOverhead != b.writeOverhead ||
          a.operandLanes != b.operandLanes || a.kWindow != b.kWindow ||
          a.elementBytes != b.elementBytes) {
        module.emitError()
            << "systolic.device " << fleet.symbols[i] << " and "
            << fleet.symbols[j] << " of the same " << resources[i].arraySize
            << "x" << resources[i].arraySize
            << " geometry declare different cost parameters "
               "(tile_overhead, read/write terms, operand_lanes or "
               "k_window); the binary-search decomposition bills one "
               "cost per geometry";
        return failure();
      }
    }
  }

  return fleet;
}

FailureOr<SystolicTilePlan>
mlir::systolic::planSystolicMatmul(Operation *diagOp, int64_t M, int64_t N,
                                   int64_t K, const SystolicFleet &fleet,
                                   const SystolicOptimizerOptions &options) {
  ArrayRef<SystolicArrayResource> resources = fleet.resources;

  auto optimization =
      optimizeSystolicBinarySearch(M, N, K, resources, options);
  if (failed(optimization))
    return diagOp->emitError()
           << "binary-search systolic optimization failed for " << M << "x"
           << N << "x" << K;

  SystolicTilePlan plan;
  plan.M = M;
  plan.N = N;
  plan.K = K;

  // --------------------------------------------------------------------
  // The compute clock of every accelerator.
  //
  // The optimizer's schedule is on the full timeline: a tile holds its
  // accelerator for every invocation's read, compute and write. est_cycles
  // has always meant an invocation's compute duration -- in every costing
  // pass, and in the hardware scheduler (systolic_hw_scheduler.sv), which
  // takes it as req_compute_cycles. The IR keeps that meaning and carries
  // the optimizer's schedule projected onto compute time: on each
  // accelerator, in the order the optimizer runs its tiles, an invocation's
  // start_cycle is where the previous one's compute term ends. The full
  // timeline is in the pass remark (`report`) and in systolic-cost.
  // --------------------------------------------------------------------

  SmallVector<int64_t> tileComputeStart(optimization->decomposition.size(),
                                        0);
  {
    SmallVector<size_t> order(optimization->decomposition.size());
    for (size_t i = 0; i < order.size(); ++i)
      order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      const ScheduledSystolicTile &x = optimization->schedule[a];
      const ScheduledSystolicTile &y = optimization->schedule[b];
      if (x.tile.acceleratorId != y.tile.acceleratorId)
        return x.tile.acceleratorId < y.tile.acceleratorId;
      return x.startCycle < y.startCycle;
    });
    int64_t accelerator = -1;
    int64_t clock = 0;
    for (size_t idx : order) {
      const SystolicExecutionTask &task = optimization->decomposition[idx];
      if (task.acceleratorId != accelerator) {
        accelerator = task.acceleratorId;
        clock = 0;
      }
      tileComputeStart[idx] = clock;
      if (task.acceleratorId < 0 ||
          task.acceleratorId >= static_cast<int64_t>(resources.size()))
        return diagOp->emitError()
               << "optimizer returned invalid accelerator id "
               << task.acceleratorId;
      const ::systolic::TileCost tileCost =
          ::systolic::estimateTileCost(K, resources[task.acceleratorId].config);
      if (!tileCost.ok ||
          __builtin_add_overflow(clock, tileCost.compute, &clock))
        return diagOp->emitError()
               << "cost model cannot price a " << task.size << "x"
               << task.size << " tile of reduction " << K;
    }
  }

  // --------------------------------------------------------------------
  // Exactly the optimizer's spatial decomposition.
  //
  // Each spatial task covers a tileM x tileN block of C. Its reduction is
  // folded by the assigned device's k_max, so the block runs as
  //
  //   tileM x k_j  @  k_j x tileN  ->  tileM x tileN
  //
  // with sum_j k_j = K, chained through c_in. The spatial decomposition and
  // its device assignment are the optimizer's, unchanged; folding only
  // splits each task's reduction.
  // --------------------------------------------------------------------

  for (size_t taskIndex = 0; taskIndex < optimization->decomposition.size();
       ++taskIndex) {
    const SystolicExecutionTask &task = optimization->decomposition[taskIndex];

    if (task.size <= 0)
      return diagOp->emitError()
             << "optimizer returned non-positive tile size";

    if (task.acceleratorId < 0 ||
        task.acceleratorId >= static_cast<int64_t>(fleet.symbols.size()))
      return diagOp->emitError()
             << "optimizer returned invalid accelerator id "
             << task.acceleratorId;

    if (task.row < 0 || task.column < 0 || task.row >= M || task.column >= N)
      return diagOp->emitError()
             << "optimizer returned invalid tile origin "
             << "(" << task.row << ", " << task.column << ")";

    // Valid portion of this spatial tile. The optimizer may use an
    // internally padded domain; a tile touching the boundary is clipped to
    // the original matrix.
    const int64_t m = std::min<int64_t>(task.size, M - task.row);
    const int64_t n = std::min<int64_t>(task.size, N - task.column);

    // A padding-only tile would have been outside the original domain. The
    // optimizer promises not to emit one; keep the guard as an invariant.
    if (m <= 0 || n <= 0)
      continue;

    // decomposition[i] and schedule[i] describe the same tile: the task
    // names the spatial tile and its accelerator, the schedule entry the
    // interval the tile holds that accelerator for (every invocation's
    // read, compute and write).
    auto noSchedule = [&]() {
      return diagOp->emitError()
             << "optimizer decomposition task has no matching "
                "schedule entry for tile ("
             << task.row << ", " << task.column << ", size=" << task.size
             << ", accelerator=" << task.acceleratorId << ")";
    };
    if (taskIndex >= optimization->schedule.size())
      return noSchedule();

    const ScheduledSystolicTile &scheduledTile =
        optimization->schedule[taskIndex];

    if (scheduledTile.tile.row != task.row ||
        scheduledTile.tile.column != task.column ||
        scheduledTile.tile.size != task.size ||
        scheduledTile.tile.acceleratorId != task.acceleratorId)
      return noSchedule();

    if (scheduledTile.startCycle < 0 ||
        scheduledTile.endCycle < scheduledTile.startCycle)
      return diagOp->emitError()
             << "optimizer returned invalid schedule interval for tile ("
             << task.row << ", " << task.column << ", size=" << task.size
             << ")";

    // ------------------------------------------------------------------
    // K-folding.
    //
    // One systolic.matmul_tile is one invocation of its device: the operand
    // buffers of accelerator d hold at most k_max(d) reduction steps, so a
    // tile whose reduction is longer runs as I(d) = ceil(K / k_max(d))
    // invocations, invocation j covering [j * k_max(d), min(K, (j + 1) *
    // k_max(d))), chained through c_in. A device without k_max keeps K
    // whole (I = 1).
    //
    // Invocation j runs exactly k_j steps (k_max is a capacity) and costs
    // what the cost model says it costs: the last one is the short one.
    // est_cycles is invocation j's compute term, start_cycle the tile's
    // compute start plus the compute terms of invocations 0 .. j-1. Their
    // read + compute + write must add up to exactly the interval the
    // optimizer scheduled the tile over; anything else would mean the
    // optimizer and the IR were priced by two different models.
    // ------------------------------------------------------------------

    const ::systolic::ArrayConfig &deviceConfig =
        resources[task.acceleratorId].config;

    const int64_t kStep = deviceConfig.kMax > 0 ? deviceConfig.kMax : K;
    const int64_t numInvocations =
        ::systolic::countInvocations(K, deviceConfig);

    SmallVector<::systolic::InvocationCost> invocationCosts;
    int64_t sequentialCycles = 0;
    for (int64_t j = 0; j < numInvocations; ++j) {
      ::systolic::InvocationCost cost =
          ::systolic::estimateInvocationCost(K, j, deviceConfig);
      if (!cost.ok || __builtin_add_overflow(sequentialCycles,
                                             cost.sequential(),
                                             &sequentialCycles))
        return diagOp->emitError()
               << "cost model cannot price invocation " << j << " of tile ("
               << task.row << ", " << task.column << ")";
      invocationCosts.push_back(cost);
    }

    if (numInvocations < 1 ||
        numInvocations != (K + kStep - 1) / kStep ||
        sequentialCycles !=
            scheduledTile.endCycle - scheduledTile.startCycle)
      return diagOp->emitError()
             << "optimizer scheduled tile (" << task.row << ", "
             << task.column << ") over "
             << scheduledTile.endCycle - scheduledTile.startCycle
             << " cycles, but its " << numInvocations
             << " invocations cost " << sequentialCycles;

    SystolicTileBlock block;
    block.row = task.row;
    block.column = task.column;
    block.m = m;
    block.n = n;
    block.acceleratorId = task.acceleratorId;

    int64_t computeClock = tileComputeStart[taskIndex];
    for (int64_t j = 0; j < numInvocations; ++j) {
      const int64_t k0 = j * kStep;
      const int64_t kc = std::min<int64_t>(kStep, K - k0);
      if (kc != invocationCosts[j].k)
        return diagOp->emitError()
               << "invocation " << j << " covers " << kc
               << " reduction steps but was priced for "
               << invocationCosts[j].k;

      SystolicTileInvocation invocation;
      invocation.k0 = k0;
      invocation.k = kc;
      invocation.estCycles = invocationCosts[j].compute;
      invocation.startCycle = computeClock;
      computeClock += invocation.estCycles;
      block.invocations.push_back(invocation);
    }

    plan.blocks.push_back(std::move(block));
  }

  plan.optimization = std::move(*optimization);
  return plan;
}

Value mlir::systolic::emitSystolicTilePlan(OpBuilder &builder, Location loc,
                                           Value A, Value B, Value C,
                                           const SystolicTilePlan &plan,
                                           const SystolicFleet &fleet) {
  Type elementType = cast<RankedTensorType>(A.getType()).getElementType();
  SmallVector<OpFoldResult> strides(2, builder.getIndexAttr(1));

  Value result = C;
  for (const SystolicTileBlock &block : plan.blocks) {
    OpFoldResult offM = builder.getIndexAttr(block.row);
    OpFoldResult offN = builder.getIndexAttr(block.column);

    SmallVector<OpFoldResult> cOff{offM, offN};
    SmallVector<OpFoldResult> cSize{builder.getIndexAttr(block.m),
                                    builder.getIndexAttr(block.n)};

    Value cTile = tensor::ExtractSliceOp::create(builder, loc, result, cOff,
                                                 cSize, strides);

    auto tileTy = RankedTensorType::get({block.m, block.n}, elementType);

    for (const SystolicTileInvocation &inv : block.invocations) {
      SmallVector<OpFoldResult> aOff{offM, builder.getIndexAttr(inv.k0)};
      SmallVector<OpFoldResult> aSize{builder.getIndexAttr(block.m),
                                      builder.getIndexAttr(inv.k)};
      Value aTile =
          tensor::ExtractSliceOp::create(builder, loc, A, aOff, aSize, strides);

      SmallVector<OpFoldResult> bOff{builder.getIndexAttr(inv.k0), offN};
      SmallVector<OpFoldResult> bSize{builder.getIndexAttr(inv.k),
                                      builder.getIndexAttr(block.n)};
      Value bTile =
          tensor::ExtractSliceOp::create(builder, loc, B, bOff, bSize, strides);

      cTile = MatmulTileOp::create(
          builder, loc, tileTy, aTile, bTile, cTile,
          builder.getI64IntegerAttr(block.m),
          builder.getI64IntegerAttr(block.n), builder.getI64IntegerAttr(inv.k),
          /*device=*/fleet.symbols[block.acceleratorId],
          /*est_cycles=*/builder.getI64IntegerAttr(inv.estCycles),
          /*start_cycle=*/builder.getI64IntegerAttr(inv.startCycle));
    }

    result = tensor::InsertSliceOp::create(builder, loc, cTile, result, cOff,
                                           cSize, strides);
  }
  return result;
}
