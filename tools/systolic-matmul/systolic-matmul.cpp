//===- systolic-matmul.cpp - C = A x B on the FPGA, from stdin ------------===//
//
//   ./systolic-matmul [--dump-ir=DIR] [--quiet]
//
// reads
//
//   M N K
//   A[0][0] ... A[0][K-1]        M rows of K integers
//   ...
//   B[0][0] ... B[0][N-1]        K rows of N integers
//   ...
//
// and computes C[M][N] = A x B on the 1x8x8 + 3x4x4 systolic-array FPGA,
// through the whole compiler:
//
//   linalg.matmul (built for this M x N x K)
//     -> --systolic-binary-search-tile-matmul   systolic.matmul_tile per
//                                               invocation: device, schedule,
//                                               K folded by k_max
//     -> --lower-systolic-matmul-tile-to-fpga   runtime call per tile
//     -> --systolic-runtime-to-llvm             LLVM dialect
//     -> JIT, run: fpga_matmul_tiled_auto_scheduled -> DPTI -> FPGA
//
// The result is compared element by element with an exact integer CPU
// reference. Nothing on this path computes C on the CPU: the CPU only adds
// the FPGA's per-invocation results of one output tile (the bitstream has no
// C-in port) and computes the reference it is checked against.
//
// Exit status: 0 PASS, 1 FAIL (mismatch), 2 bad input, 3 compilation failed,
// 4 FPGA/runtime error.
//
//===----------------------------------------------------------------------===//

#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "fpga_matmul_tiled.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/ExecutionEngine/CRunnerUtils.h"
#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "mlir/ExecutionEngine/OptUtils.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"

#include "llvm/ExecutionEngine/Orc/Mangling.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

using namespace mlir;

namespace {

// The fleet of the bitstream on the board (tag
// tile-pipelined-1x8x8-3x4x4-20261009). The symbols are the ones
// --lower-systolic-matmul-tile-to-fpga maps onto the RTL's device IDs 0..3;
// k_max is its operand-buffer depth, tile_overhead the H measured on the
// fold RTL (docs/PROVENANCE.md).
constexpr const char *kFleet =
    "  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = "
    "output_stationary k_max = 16 tile_overhead = 95\n"
    "  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = "
    "output_stationary k_max = 16 tile_overhead = 95\n"
    "  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = "
    "output_stationary k_max = 16 tile_overhead = 95\n"
    "  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = "
    "output_stationary k_max = 16 tile_overhead = 95\n";

constexpr int64_t kMaxDim = 4096;

struct Options {
  std::string dumpDir;
  bool quiet = false;
};

struct Problem {
  int64_t M = 0, N = 0, K = 0;
  std::vector<int64_t> A, B;  // row-major, as typed
};

// One systolic.matmul_tile after Linalg -> Systolic, for the report.
struct TileInfo {
  int64_t row = 0, col = 0, k0 = 0;
  int64_t m = 0, n = 0, k = 0;
  std::string device;
  int64_t estCycles = 0, startCycle = 0;
  int64_t deviceId = -1;  // from the lowered runtime call
};

[[noreturn]] void usage(int status) {
  std::fprintf(status ? stderr : stdout,
               "usage: systolic-matmul [--dump-ir=DIR] [--quiet]\n"
               "\n"
               "Reads from stdin:\n"
               "  M N K\n"
               "  A: M rows of K integers\n"
               "  B: K rows of N integers\n"
               "and computes C = A x B on the 1x8x8 + 3x4x4 FPGA through\n"
               "Linalg -> Systolic -> FPGA lowering -> runtime -> DPTI,\n"
               "then checks C against an exact CPU reference.\n"
               "\n"
               "  --dump-ir=DIR  write each stage's IR to DIR:\n"
               "                 00-linalg.mlir 10-systolic.mlir 20-fpga.mlir\n"
               "                 30-llvm.mlir 40-llvm.ll\n"
               "  --quiet        print only the verification result\n"
               "\n"
               "Environment (runtime): SYSTOLIC_RUNTIME_TRACE=1 one line per\n"
               "FPGA invocation, SYSTOLIC_DPTI_TRACE=1 every DPTI packet,\n"
               "SYSTOLIC_DPTI_TIMEOUT_MS result timeout (default 5000).\n"
               "Exit status: 0 PASS, 1 FAIL, 2 bad input, 3 compile error,\n"
               "4 FPGA/runtime error.\n");
  std::exit(status);
}

Options parseArgs(int argc, char **argv) {
  Options opts;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help")
      usage(0);
    else if (arg == "-q" || arg == "--quiet")
      opts.quiet = true;
    else if (arg.rfind("--dump-ir=", 0) == 0)
      opts.dumpDir = arg.substr(std::strlen("--dump-ir="));
    else {
      std::fprintf(stderr, "systolic-matmul: unknown argument '%s'\n\n",
                   arg.c_str());
      usage(2);
    }
  }
  return opts;
}

// ---------------------------------------------------------------- input

class TokenReader {
public:
  explicit TokenReader(std::istream &in) : in_(in) {}

  // Reads one integer; on failure prints what was expected and exits(2).
  int64_t next(const std::string &what) {
    std::string tok;
    if (!(in_ >> tok)) {
      std::fprintf(stderr,
                   "systolic-matmul: input ended while reading %s\n",
                   what.c_str());
      std::exit(2);
    }
    errno = 0;
    char *end = nullptr;
    const long long v = std::strtoll(tok.c_str(), &end, 10);
    if (errno != 0 || end == tok.c_str() || *end != '\0') {
      std::fprintf(stderr,
                   "systolic-matmul: %s must be an integer, got '%s'\n",
                   what.c_str(), tok.c_str());
      std::exit(2);
    }
    return v;
  }

  bool atEnd() {
    std::string tok;
    return !(in_ >> tok);
  }

private:
  std::istream &in_;
};

Problem readProblem() {
  const bool tty = isatty(STDIN_FILENO);
  TokenReader rd(std::cin);
  Problem p;

  if (tty)
    std::fprintf(stderr, "M N K: ");
  p.M = rd.next("M");
  p.N = rd.next("N");
  p.K = rd.next("K");
  for (auto [name, v] : {std::pair<const char *, int64_t>{"M", p.M},
                         {"N", p.N},
                         {"K", p.K}}) {
    if (v < 1 || v > kMaxDim) {
      std::fprintf(stderr, "systolic-matmul: %s = %" PRId64
                           " is outside 1..%" PRId64 "\n",
                   name, v, kMaxDim);
      std::exit(2);
    }
  }

  p.A.resize(static_cast<size_t>(p.M * p.K));
  p.B.resize(static_cast<size_t>(p.K * p.N));

  if (tty)
    std::fprintf(stderr, "A (%" PRId64 " rows of %" PRId64 " integers):\n",
                 p.M, p.K);
  for (int64_t i = 0; i < p.M; ++i)
    for (int64_t k = 0; k < p.K; ++k)
      p.A[i * p.K + k] = rd.next("A[" + std::to_string(i) + "][" +
                                 std::to_string(k) + "]");

  if (tty)
    std::fprintf(stderr, "B (%" PRId64 " rows of %" PRId64 " integers):\n",
                 p.K, p.N);
  for (int64_t k = 0; k < p.K; ++k)
    for (int64_t j = 0; j < p.N; ++j)
      p.B[k * p.N + j] = rd.next("B[" + std::to_string(k) + "][" +
                                 std::to_string(j) + "]");

  if (!tty && !rd.atEnd())
    std::fprintf(stderr, "systolic-matmul: warning: ignoring input after "
                         "B[%" PRId64 "][%" PRId64 "]\n",
                 p.K - 1, p.N - 1);
  return p;
}

// --------------------------------------------------------------- output

void printMatrix(const char *title, int64_t rows, int64_t cols,
                 const std::vector<double> &v) {
  int width = 1;
  for (double x : v) {
    char buf[64];
    width = std::max(width, std::snprintf(buf, sizeof buf, "%.9g", x));
  }
  std::printf("%s\n", title);
  for (int64_t i = 0; i < rows; ++i) {
    std::printf(" ");
    for (int64_t j = 0; j < cols; ++j)
      std::printf(" %*.9g", width, v[i * cols + j]);
    std::printf("\n");
  }
}

template <typename T>
std::vector<double> asDouble(const std::vector<T> &v) {
  return std::vector<double>(v.begin(), v.end());
}

// ------------------------------------------------------------- compiler

std::string buildLinalgModule(const Problem &p) {
  const std::string a = std::to_string(p.M) + "x" + std::to_string(p.K);
  const std::string b = std::to_string(p.K) + "x" + std::to_string(p.N);
  const std::string c = std::to_string(p.M) + "x" + std::to_string(p.N);
  std::ostringstream os;
  os << "// systolic-matmul input: C[" << p.M << "][" << p.N << "] = A["
     << p.M << "][" << p.K << "] x B[" << p.K << "][" << p.N << "]\n"
     << "module {\n"
     << kFleet
     << "  func.func @systolic_matmul(%A: tensor<" << a << "xf32>, %B: tensor<"
     << b << "xf32>, %C: tensor<" << c << "xf32>) -> tensor<" << c
     << "xf32> attributes {llvm.emit_c_interface} {\n"
     << "    %0 = linalg.matmul ins(%A, %B : tensor<" << a
     << "xf32>, tensor<" << b << "xf32>) outs(%C : tensor<" << c
     << "xf32>) -> tensor<" << c << "xf32>\n"
     << "    return %0 : tensor<" << c << "xf32>\n"
     << "  }\n"
     << "}\n";
  return os.str();
}

void dumpStage(const Options &opts, const char *file, Operation *op) {
  if (opts.dumpDir.empty())
    return;
  std::error_code ec;
  llvm::raw_fd_ostream os(opts.dumpDir + "/" + file, ec);
  if (ec) {
    std::fprintf(stderr, "systolic-matmul: cannot write %s/%s: %s\n",
                 opts.dumpDir.c_str(), file, ec.message().c_str());
    return;
  }
  op->print(os);
  os << "\n";
}

LogicalResult runStage(ModuleOp module, const char *what,
                       void (*build)(OpPassManager &)) {
  PassManager pm(module.getContext());
  build(pm);
  if (failed(pm.run(module))) {
    std::fprintf(stderr, "systolic-matmul: %s failed\n", what);
    return failure();
  }
  return success();
}

int64_t staticOffset(Value v, unsigned dim) {
  if (auto slice = v.getDefiningOp<tensor::ExtractSliceOp>()) {
    ArrayRef<int64_t> offs = slice.getStaticOffsets();
    if (dim < offs.size() && !ShapedType::isDynamic(offs[dim]))
      return offs[dim];
  }
  return 0;
}

std::vector<TileInfo> collectTiles(ModuleOp module) {
  std::vector<TileInfo> tiles;
  module.walk([&](systolic::MatmulTileOp op) {
    TileInfo t;
    t.row = staticOffset(op.getA(), 0);
    t.k0 = staticOffset(op.getA(), 1);
    t.col = staticOffset(op.getB(), 1);
    t.m = op.getM();
    t.n = op.getN();
    t.k = op.getK();
    if (auto dev = op.getDeviceAttr())
      t.device = dev.getValue().str();
    if (auto e = op.getEstCyclesAttr())
      t.estCycles = e.getInt();
    if (auto s = op.getStartCycleAttr())
      t.startCycle = s.getInt();
    tiles.push_back(t);
  });
  return tiles;
}

// The device-ID operand (#6) of each runtime call, in program order.
std::vector<int64_t> collectCallDeviceIds(ModuleOp module) {
  std::vector<int64_t> ids;
  module.walk([&](LLVM::CallOp call) {
    auto callee = call.getCallee();
    if (!callee || *callee != "fpga_matmul_tiled_auto_scheduled")
      return;
    int64_t id = -1;
    if (call.getNumOperands() == 9)
      if (auto cst = call.getOperand(6).getDefiningOp<arith::ConstantIntOp>())
        id = cst.value();
    ids.push_back(id);
  });
  return ids;
}

// -------------------------------------------------------------- the run

struct Report {
  std::vector<TileInfo> tiles;
  size_t runtimeCalls = 0;
  fpga_runtime_stats_t stats{};
  double fpgaSeconds = 0;
};

using MatmulFn = void (*)(StridedMemRefType<float, 2> *,
                          StridedMemRefType<float, 2> *,
                          StridedMemRefType<float, 2> *,
                          StridedMemRefType<float, 2> *);

StridedMemRefType<float, 2> describe(float *data, int64_t rows,
                                     int64_t cols) {
  StridedMemRefType<float, 2> d;
  d.basePtr = data;
  d.data = data;
  d.offset = 0;
  d.sizes[0] = rows;
  d.sizes[1] = cols;
  d.strides[0] = cols;
  d.strides[1] = 1;
  return d;
}

} // namespace

int main(int argc, char **argv) {
  const Options opts = parseArgs(argc, argv);
  const Problem prob = readProblem();
  const int64_t M = prob.M, N = prob.N, K = prob.K;

  // ---- user input -> matrix representation (binary32, as the FPGA sees it)
  int64_t maxA = 0, maxB = 0;
  for (int64_t v : prob.A) maxA = std::max<int64_t>(maxA, std::llabs(v));
  for (int64_t v : prob.B) maxB = std::max<int64_t>(maxB, std::llabs(v));
  // Every input, product and partial sum is an integer of magnitude
  // <= K * maxA * maxB; below 2^24 each is exact in binary32 whatever order
  // the array adds them in, so the FPGA must equal the integer reference.
  const long double bound =
      static_cast<long double>(K) * maxA * maxB;
  const bool exact = maxA <= (1 << 24) && maxB <= (1 << 24) &&
                     bound <= static_cast<long double>(1 << 24);

  std::vector<float> A(prob.A.begin(), prob.A.end());
  std::vector<float> B(prob.B.begin(), prob.B.end());
  std::vector<float> C(static_cast<size_t>(M * N), 0.0f);

  if (!opts.dumpDir.empty())
    llvm::sys::fs::create_directories(opts.dumpDir);

  if (!opts.quiet) {
    std::printf("systolic-matmul: C[%" PRId64 "][%" PRId64 "] = A[%" PRId64
                "][%" PRId64 "] x B[%" PRId64 "][%" PRId64
                "] on the 1x8x8 + 3x4x4 FPGA\n\n",
                M, N, M, K, K, N);
    printMatrix(("A[" + std::to_string(M) + "][" + std::to_string(K) + "]")
                    .c_str(),
                M, K, asDouble(prob.A));
    printMatrix(("B[" + std::to_string(K) + "][" + std::to_string(N) + "]")
                    .c_str(),
                K, N, asDouble(prob.B));
    std::printf("\n");
    if (!exact)
      std::printf("warning: K*max|A|*max|B| = %.0Lf exceeds 2^24; binary32 "
                  "rounding can make the FPGA differ from the exact "
                  "reference\n\n",
                  bound);
  }

  // ---- compiler: matrix -> Linalg -> Systolic -> FPGA lowering -> LLVM ---
  DialectRegistry registry;
  systolic::registerSystolicPipelineDialects(registry);
  registerBuiltinDialectTranslation(registry);
  registerLLVMDialectTranslation(registry);
  MLIRContext context(registry);
  context.loadAllAvailableDialects();

  OwningOpRef<ModuleOp> module =
      parseSourceString<ModuleOp>(buildLinalgModule(prob), &context);
  if (!module) {
    std::fprintf(stderr, "systolic-matmul: internal error: generated "
                         "linalg module does not parse\n");
    return 3;
  }
  dumpStage(opts, "00-linalg.mlir", module->getOperation());

  Report report;

  if (failed(runStage(*module, "Linalg -> Systolic (binary-search tiling)",
                      [](OpPassManager &pm) {
                        pm.addPass(systolic::
                                       createSystolicBinarySearchTileMatmulPass());
                      })))
    return 3;
  dumpStage(opts, "10-systolic.mlir", module->getOperation());
  report.tiles = collectTiles(*module);

  if (failed(runStage(*module, "Systolic -> FPGA runtime lowering",
                      [](OpPassManager &pm) {
                        pm.addPass(systolic::createTileMatmulForFpgaPass());
                      })))
    return 3;
  dumpStage(opts, "20-fpga.mlir", module->getOperation());
  {
    const std::vector<int64_t> ids = collectCallDeviceIds(*module);
    report.runtimeCalls = ids.size();
    for (size_t i = 0; i < ids.size() && i < report.tiles.size(); ++i)
      report.tiles[i].deviceId = ids[i];
  }

  if (failed(runStage(*module, "bufferization / LLVM lowering",
                      systolic::buildSystolicRuntimeToLLVMPipeline)))
    return 3;
  dumpStage(opts, "30-llvm.mlir", module->getOperation());

  if (!opts.quiet) {
    std::printf("Pipeline\n");
    std::printf("  [1] Linalg     linalg.matmul %" PRId64 "x%" PRId64
                " * %" PRId64 "x%" PRId64 " -> %" PRId64 "x%" PRId64 "\n",
                M, K, K, N, M, N);
    std::printf("  [2] Systolic   %zu systolic.matmul_tile (binary-search "
                "decomposition, K folded by k_max = 16)\n",
                report.tiles.size());
    for (size_t i = 0; i < report.tiles.size(); ++i) {
      const TileInfo &t = report.tiles[i];
      std::printf("        #%-3zu C[%" PRId64 ":%" PRId64 ", %" PRId64
                  ":%" PRId64 "] k[%" PRId64 ":%" PRId64 "]  on @%s"
                  " (device %" PRId64 ")  start_cycle=%" PRId64
                  " est_cycles=%" PRId64 "\n",
                  i, t.row, t.row + t.m, t.col, t.col + t.n, t.k0,
                  t.k0 + t.k, t.device.c_str(), t.deviceId, t.startCycle,
                  t.estCycles);
    }
    std::printf("  [3] FPGA       %zu call(s) of fpga_matmul_tiled_auto_scheduled"
                " (m, k, n, A, B, C, device, start, est)\n",
                report.runtimeCalls);
  }

  // ---- LLVM -> native code (JIT) -----------------------------------------
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  std::string llPath =
      opts.dumpDir.empty() ? std::string() : opts.dumpDir + "/40-llvm.ll";
  auto optimize = makeOptimizingTransformer(/*optLevel=*/2, /*sizeLevel=*/0,
                                            /*targetMachine=*/nullptr);
  auto transformer = [&](llvm::Module *m) -> llvm::Error {
    if (llvm::Error err = optimize(m))
      return err;
    if (!llPath.empty()) {
      std::error_code ec;
      llvm::raw_fd_ostream os(llPath, ec);
      if (!ec)
        m->print(os, nullptr);
    }
    return llvm::Error::success();
  };

  ExecutionEngineOptions engineOptions;
  engineOptions.transformer = transformer;
  auto maybeEngine = ExecutionEngine::create(*module, engineOptions);
  if (!maybeEngine) {
    std::string msg = llvm::toString(maybeEngine.takeError());
    std::fprintf(stderr, "systolic-matmul: JIT compilation failed: %s\n",
                 msg.c_str());
    return 3;
  }
  std::unique_ptr<ExecutionEngine> engine = std::move(*maybeEngine);

  // The compiled code calls the FPGA runtime linked into this executable,
  // and memrefCopy (mlir_c_runner_utils) for the strided tile staging.
  engine->registerSymbols([](llvm::orc::MangleAndInterner interner) {
    llvm::orc::SymbolMap map;
    map[interner("fpga_matmul_tiled_auto_scheduled")] = {
        llvm::orc::ExecutorAddr::fromPtr(&fpga_matmul_tiled_auto_scheduled),
        llvm::JITSymbolFlags::Exported};
    map[interner("memrefCopy")] = {
        llvm::orc::ExecutorAddr::fromPtr(&memrefCopy),
        llvm::JITSymbolFlags::Exported};
    return map;
  });
  engine->initialize();

  auto maybeFn = engine->lookup("_mlir_ciface_systolic_matmul");
  if (!maybeFn) {
    std::string msg = llvm::toString(maybeFn.takeError());
    std::fprintf(stderr, "systolic-matmul: JIT lookup failed: %s\n",
                 msg.c_str());
    return 3;
  }
  auto fn = reinterpret_cast<MatmulFn>(*maybeFn);

  if (!opts.quiet)
    std::printf("  [4] LLVM       JIT-compiled _mlir_ciface_systolic_matmul\n");

  // ---- run: runtime -> DPTI -> FPGA -> C -----------------------------------
  StridedMemRefType<float, 2> mA = describe(A.data(), M, K);
  StridedMemRefType<float, 2> mB = describe(B.data(), K, N);
  StridedMemRefType<float, 2> mC = describe(C.data(), M, N);
  StridedMemRefType<float, 2> mR{};

  fpga_runtime_reset_stats();
  const auto t0 = std::chrono::steady_clock::now();
  fn(&mR, &mA, &mB, &mC);
  const auto t1 = std::chrono::steady_clock::now();
  report.fpgaSeconds = std::chrono::duration<double>(t1 - t0).count();
  fpga_runtime_get_stats(&report.stats);
  fpga_runtime_close();

  if (!opts.quiet) {
    const fpga_runtime_stats_t &s = report.stats;
    std::printf("  [5] Runtime    %llu FPGA invocation(s): device 0 (8x8) %llu,"
                " device 1 %llu, device 2 %llu, device 3 %llu (4x4)\n",
                s.invocations, s.invocations_per_device[0],
                s.invocations_per_device[1], s.invocations_per_device[2],
                s.invocations_per_device[3]);
    std::printf("                 DPTI: %llu B to the FPGA, %llu B back, "
                "%.3f s\n\n",
                s.bytes_to_fpga, s.bytes_from_fpga, report.fpgaSeconds);
  }

  if (report.stats.failed_calls != 0 ||
      report.stats.invocations != report.runtimeCalls) {
    std::printf("FPGA dispatch: FAIL -- %llu of %zu runtime call(s) failed "
                "(first error %d); C is not valid\n",
                report.stats.failed_calls, report.runtimeCalls,
                report.stats.first_error);
    std::printf("Verification: FAIL\n");
    return 4;
  }

  std::vector<double> fpga(static_cast<size_t>(M * N));
  for (int64_t i = 0; i < M; ++i)
    for (int64_t j = 0; j < N; ++j)
      fpga[i * N + j] = mR.data[mR.offset + i * mR.strides[0] +
                                j * mR.strides[1]];
  if (mR.basePtr && mR.basePtr != C.data())
    std::free(mR.basePtr);  // a buffer the compiled code allocated for us

  // ---- CPU reference: C_ref[i][j] = sum_k A[i][k] * B[k][j], exact --------
  std::vector<int64_t> ref(static_cast<size_t>(M * N), 0);
  for (int64_t i = 0; i < M; ++i)
    for (int64_t j = 0; j < N; ++j) {
      int64_t acc = 0;
      for (int64_t k = 0; k < K; ++k)
        acc += prob.A[i * K + k] * prob.B[k * N + j];
      ref[i * N + j] = acc;
    }

  // ---- verification, element by element ---------------------------------
  int64_t mismatches = 0;
  int64_t firstI = -1, firstJ = -1;
  for (int64_t i = 0; i < M; ++i)
    for (int64_t j = 0; j < N; ++j)
      if (fpga[i * N + j] != static_cast<double>(ref[i * N + j])) {
        if (mismatches == 0) {
          firstI = i;
          firstJ = j;
        }
        ++mismatches;
      }

  if (!opts.quiet) {
    printMatrix(("FPGA result C[" + std::to_string(M) + "][" +
                 std::to_string(N) + "]")
                    .c_str(),
                M, N, fpga);
    printMatrix(("CPU reference C_ref[" + std::to_string(M) + "][" +
                 std::to_string(N) + "]")
                    .c_str(),
                M, N, asDouble(ref));
    std::printf("\n");
  }

  std::printf("M N K = %" PRId64 " %" PRId64 " %" PRId64
              "   FPGA invocations = %llu   mismatches = %" PRId64
              " / %" PRId64 "\n",
              M, N, K, report.stats.invocations, mismatches, M * N);
  if (mismatches == 0) {
    std::printf("Verification: PASS\n");
    return 0;
  }
  std::printf("first mismatch: i=%" PRId64 " j=%" PRId64
              "  FPGA=%.9g  CPU=%" PRId64 "\n",
              firstI, firstJ, fpga[firstI * N + firstJ],
              ref[firstI * N + firstJ]);
  std::printf("Verification: FAIL\n");
  return 1;
}
