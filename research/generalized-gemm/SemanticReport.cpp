//===- SemanticReport.cpp - Read-only G1 driver ---------------------------===//
//
// Prints the semantic model of each function as JSON, or runs entries from a
// JSON file against it. Used by the fuzz, lit and differential tests.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/RelationalEncoding.h"
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
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mlir::generalized_gemm;
namespace cl = llvm::cl;

static cl::opt<std::string> input(cl::Positional, cl::init("-"));
static cl::opt<std::string> onlyFunction("function", cl::init(""));
static cl::opt<unsigned> indexWidth("index-bit-width", cl::init(0));
static cl::opt<bool> lowerStructured("lower-structured", cl::init(false));
static cl::opt<std::string> runs("run", cl::init(""),
                                 cl::desc("JSON list of entries to execute"));
static cl::opt<uint64_t> fuel("fuel", cl::init(1u << 20));
static cl::opt<bool> printSMT("print-smt", cl::init(false));
static cl::opt<int> summarize("summarize", cl::init(-1),
                              cl::desc("print path counts from a location"));

// An entry: {"function": name, "parameters": ["5", ...],
//            "memory": [{"shape": [2, 2], "cells": ["1", null, ...]}, ...]}.
// Values are decimal bit patterns, unsigned, at the sort's width.
static std::optional<EntryState> parseEntry(const llvm::json::Object &object,
                                            const SemanticProgram &program) {
  EntryState entry;
  const auto *parameters = object.getArray("parameters");
  const auto *memory = object.getArray("memory");
  if (!parameters || !memory || parameters->size() != program.parameters.size() ||
      memory->size() != program.roots.size())
    return std::nullopt;
  for (unsigned i = 0; i < parameters->size(); ++i) {
    auto text = (*parameters)[i].getAsString();
    if (!text)
      return std::nullopt;
    entry.parameters.emplace_back(program.parameters[i].sort.width, *text, 10);
  }
  for (unsigned r = 0; r < memory->size(); ++r) {
    const auto *root = (*memory)[r].getAsObject();
    if (!root)
      return std::nullopt;
    RootData data;
    for (const auto &extent : *root->getArray("shape"))
      data.shape.push_back(*extent.getAsInteger());
    for (const auto &cell : *root->getArray("cells")) {
      if (auto text = cell.getAsString())
        data.cells.emplace_back(llvm::APInt(program.roots[r].sort.width, *text, 10));
      else
        data.cells.emplace_back(std::nullopt);
    }
    entry.memory.push_back(std::move(data));
  }
  return entry;
}

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "Read-only G1 semantic model report\n");
  DialectRegistry registry;
  registry.insert<affine::AffineDialect, arith::ArithDialect,
                  cf::ControlFlowDialect, func::FuncDialect, LLVM::LLVMDialect,
                  memref::MemRefDialect, scf::SCFDialect>();
  MLIRContext context(registry);
  context.allowUnregisteredDialects();
  context.loadAllAvailableDialects();
  OwningOpRef<ModuleOp> module = parseSourceFile<ModuleOp>(input, &context);
  if (!module)
    return 2;

  ImportOptions options;
  options.indexBitWidth = indexWidth;
  options.lowerStructured = lowerStructured;

  std::map<std::string, SemanticProgram> programs;
  for (auto function : module->getOps<func::FuncOp>()) {
    if (!onlyFunction.empty() && function.getSymName() != onlyFunction)
      continue;
    if (function.isExternal())
      continue;
    programs.emplace(function.getSymName().str(),
                     importFunction(function, options));
  }

  if (!runs.empty()) {
    auto buffer = llvm::MemoryBuffer::getFile(runs);
    if (!buffer) {
      llvm::errs() << "cannot read " << runs << "\n";
      return 2;
    }
    auto parsed = llvm::json::parse((*buffer)->getBuffer());
    if (!parsed) {
      llvm::errs() << llvm::toString(parsed.takeError()) << "\n";
      return 2;
    }
    RunLimits limits;
    limits.fuel = fuel;
    for (const llvm::json::Value &value : *parsed->getAsArray()) {
      const auto *object = value.getAsObject();
      std::string name = object->getString("function")->str();
      auto found = programs.find(name);
      if (found == programs.end() || !found->second.isSupported()) {
        llvm::outs() << "{\"function\":\"" << name
                     << "\",\"run_status\":\"UNSUPPORTED\"}\n";
        continue;
      }
      auto entry = parseEntry(*object, found->second);
      if (!entry) {
        llvm::errs() << "malformed entry for " << name << "\n";
        return 2;
      }
      llvm::outs() << toJSON(found->second, runConcrete(found->second, *entry, limits))
                   << "\n";
    }
    return 0;
  }

  for (auto &[name, program] : programs) {
    if (printSMT) {
      llvm::outs() << encodeTransitionRelation(program);
      continue;
    }
    llvm::outs() << toJSON(program) << "\n";
    if (summarize >= 0 && program.isSupported()) {
      SymbolicRegion region = summarizeAcyclic(program, summarize);
      llvm::outs() << name << ": paths " << region.paths.size() << " complete "
                   << region.complete << " " << region.reason << "\n";
    }
  }
  return 0;
}
