// Shared helpers for the G1 C++ tests. No gtest in this repository.
#ifndef GENERALIZEDGEMM_TESTSUPPORT_H
#define GENERALIZEDGEMM_TESTSUPPORT_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/SemanticImport.h"
#include "GeneralizedGemm/SemanticPrinter.h"
#include "GeneralizedGemm/SymbolicEvaluator.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdio>
#include <string>
#include <vector>

namespace gg = mlir::generalized_gemm;
using llvm::APInt;

inline int failures = 0, checks = 0;
#define CHECK(condition)                                                       \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(condition)) {                                                        \
      ++failures;                                                              \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);     \
    }                                                                          \
  } while (false)
#define CHECK_THROWS(expression)                                               \
  do {                                                                         \
    ++checks;                                                                  \
    bool thrown = false;                                                       \
    try {                                                                      \
      (void)(expression);                                                      \
    } catch (const gg::TermError &) {                                          \
      thrown = true;                                                           \
    }                                                                          \
    if (!thrown) {                                                             \
      ++failures;                                                              \
      fprintf(stderr, "FAIL %s:%d: no throw: %s\n", __FILE__, __LINE__,        \
              #expression);                                                    \
    }                                                                          \
  } while (false)

inline void configure(mlir::MLIRContext &context) {
  mlir::DialectRegistry registry;
  registry.insert<mlir::affine::AffineDialect, mlir::arith::ArithDialect,
                  mlir::cf::ControlFlowDialect, mlir::func::FuncDialect,
                  mlir::LLVM::LLVMDialect, mlir::memref::MemRefDialect,
                  mlir::scf::SCFDialect>();
  context.appendDialectRegistry(registry);
  context.allowUnregisteredDialects();
  context.loadAllAvailableDialects();
}

inline mlir::OwningOpRef<mlir::ModuleOp>
parse(mlir::MLIRContext &context, const std::string &text, bool verify = true) {
  mlir::ParserConfig config(&context, verify);
  return mlir::parseSourceString<mlir::ModuleOp>(text, config);
}

/// Imports @f, or the named function, from `text`.
inline gg::SemanticProgram importText(const std::string &text,
                                      unsigned indexBits = 64,
                                      bool structured = false,
                                      const char *name = "f",
                                      gg::ImportLimits limits = {}) {
  mlir::MLIRContext context;
  configure(context);
  auto module = parse(context, text, /*verify=*/false);
  if (!module) {
    fprintf(stderr, "parse failure:\n%s\n", text.c_str());
    ++failures;
    gg::SemanticProgram refused;
    refused.unsupported = gg::UnsupportedReason{gg::UnsupportedKind::NotVerified, "parse"};
    return refused;
  }
  gg::ImportOptions options;
  options.indexBitWidth = indexBits;
  options.lowerStructured = structured;
  options.limits = limits;
  return gg::importFunction(module->lookupSymbol<mlir::func::FuncOp>(name), options);
}

inline APInt bits(unsigned width, int64_t value) {
  return APInt(width, (uint64_t)value, /*isSigned=*/false, /*implicitTrunc=*/true);
}

/// An entry with the given scalar parameters, a dense root per `memory` value
/// list, and dimension parameters filled from `shapes`.
struct Root {
  std::vector<uint64_t> shape;
  std::vector<int64_t> cells; // Empty: uninitialized (fresh allocation).
};
inline gg::EntryState entryFor(const gg::SemanticProgram &p,
                               const std::vector<int64_t> &scalars,
                               const std::vector<Root> &roots = {}) {
  gg::EntryState entry;
  unsigned next = 0;
  for (const gg::Parameter &parameter : p.parameters) {
    if (parameter.kind == gg::Parameter::Scalar)
      entry.parameters.push_back(bits(parameter.sort.width, scalars.at(next++)));
    else
      for (const gg::MemoryRoot &root : p.roots)
        if (root.origin == gg::MemoryRoot::FunctionArgument &&
            root.argument == parameter.argument)
          entry.parameters.push_back(bits(parameter.sort.width,
              (int64_t)roots.at(root.id).shape.at(parameter.dimension)));
  }
  for (unsigned r = 0; r < p.roots.size(); ++r) {
    gg::RootData data;
    const Root &root = roots.at(r);
    data.shape.assign(root.shape.begin(), root.shape.end());
    uint64_t cells = 1;
    for (uint64_t s : root.shape)
      cells *= s;
    for (uint64_t c = 0; c < cells; ++c) {
      if (root.cells.empty())
        data.cells.emplace_back(std::nullopt);
      else
        data.cells.emplace_back(bits(p.roots[r].sort.width, root.cells.at(c)));
    }
    entry.memory.push_back(std::move(data));
  }
  return entry;
}

#endif
