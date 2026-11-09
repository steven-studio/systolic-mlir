//===- IdentityReport.cpp - G5b-M4-0 driver -------------------------------===//
//
// Prints, for one function, what the shared site-set rules answer, so that an oracle can
// recompute every answer with no library code:
//
//   --site-lists  groups given by hand, ';' between them, each `O,A,B:ids` with O the
//                 output root or '-' for none, and a leading '!' to mark the group
//                 unsupported: the gate's verdict, and, when it refuses, the refusal G3b,
//                 G4a and G5a each give on their symbolic routes.
//   --cells       pairs of cells, root:index/root:index: how they relate under the
//                 authorizations.
//   --drop-points removes points from the end of the enumeration first, as a test, and
//                 reports the finite tiers G3b and G4a give on what is left.
//
// For each enumerated group, the finite routes are tried with the empty family, and all
// three finite routes at the first index past the groups. Read-only. Nothing here
// authorizes anything.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/DomainAnalysis.h"
#include "GeneralizedGemm/PolynomialAdapter.h"
#include "GeneralizedGemm/SemanticImport.h"
#include "GeneralizedGemm/SiteSets.h"
#include "GeneralizedGemm/TerminationAnalysis.h"
#include "GeneralizedGemm/Z3Backend.h"

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
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"


using namespace mlir;
using namespace mlir::generalized_gemm;
namespace cl = llvm::cl;
namespace json = llvm::json;
using llvm::APInt;

static cl::opt<std::string> input(cl::Positional, cl::init("-"));
static cl::opt<std::string> onlyFunction("function", cl::init(""));
static cl::opt<unsigned> indexWidth("index-bit-width", cl::init(0));
static cl::opt<bool> lowerStructured("lower-structured", cl::init(false));
static cl::opt<std::string> scopeText("scope", cl::init(""),
                                      cl::desc("parameter:lower:upper,... as signed decimals"));
static cl::opt<unsigned> timeoutMs("timeout-ms", cl::init(3000));
static cl::opt<unsigned> resourceLimit("rlimit", cl::init(2000000));
static cl::opt<std::string> authorize("authorize", cl::init(""),
                                      cl::desc("comma separated assumption names"));
static cl::opt<std::string> siteLists("site-lists", cl::init(""),
                                      cl::desc("';' between groups [!]O,A,B:ids, O a root or '-'"));
static cl::opt<std::string> cellPairs("cells", cl::init(""),
                                      cl::desc("';' between pairs root:i,j/root:i,j"));
static cl::opt<unsigned> dropPoints("drop-points", cl::init(0),
                                    cl::desc("remove points from the enumeration's end, as a test"));

static std::optional<ParameterScope> parseScope(const SemanticProgram &program) {
  ParameterScope scope;
  if (scopeText.empty())
    return scope;
  scope.allParameters = false;
  llvm::SmallVector<llvm::StringRef> items;
  llvm::StringRef(scopeText).split(items, ',');
  for (llvm::StringRef item : items) {
    llvm::SmallVector<llvm::StringRef> parts;
    item.split(parts, ':');
    unsigned id;
    int64_t lower, upper;
    if (parts.size() != 3 || parts[0].getAsInteger(10, id) || id >= program.parameters.size() ||
        parts[1].getAsInteger(10, lower) || parts[2].getAsInteger(10, upper))
      return std::nullopt;
    unsigned width = program.parameters[id].sort.width;
    APInt low(64, lower, /*isSigned=*/true), high(64, upper, /*isSigned=*/true);
    if (!low.isSignedIntN(width) || !high.isSignedIntN(width))
      return std::nullopt;
    scope.ranges.push_back({id, low.trunc(width), high.trunc(width)});
  }
  return scope;
}

static std::optional<std::vector<unsigned>> parseIds(llvm::StringRef text) {
  std::vector<unsigned> out;
  llvm::SmallVector<llvm::StringRef> items;
  text.split(items, ',', -1, /*KeepEmpty=*/false);
  for (llvm::StringRef item : items) {
    unsigned id;
    if (item.trim().getAsInteger(10, id))
      return std::nullopt;
    out.push_back(id);
  }
  return out;
}

static std::optional<CellRef> parseCell(llvm::StringRef text, unsigned width) {
  auto [root, rest] = text.split(':');
  CellRef cell;
  if (root.trim().getAsInteger(10, cell.root))
    return std::nullopt;
  auto ids = parseIds(rest);
  if (!ids)
    return std::nullopt;
  for (unsigned v : *ids)
    cell.index.push_back(APInt(width, v));
  return cell;
}

static json::Array points(const std::vector<std::vector<APInt>> &list) {
  json::Array out;
  for (const auto &point : list) {
    json::Array values;
    for (const APInt &v : point)
      values.push_back((int64_t)v.getSExtValue());
    out.push_back(std::move(values));
  }
  return out;
}

/// One hand-made group: `[!]O,A,B:ids`.
static std::optional<SiteGroup> parseGroup(llvm::StringRef text) {
  SiteGroup group;
  if (text.consume_front("!"))
    group.unsupported = "marked unsupported by the driver";
  auto [roots, ids] = text.split(':');
  llvm::SmallVector<llvm::StringRef> parts;
  roots.split(parts, ',');
  if (parts.size() != 3 || parts[1].getAsInteger(10, group.first) ||
      parts[2].getAsInteger(10, group.second))
    return std::nullopt;
  if (parts[0] != "-") {
    unsigned output;
    if (parts[0].getAsInteger(10, output))
      return std::nullopt;
    group.output = output;
  }
  auto list = parseIds(ids);
  if (!list)
    return std::nullopt;
  group.sites = *list;
  return group;
}

static json::Value rootOrNull(const std::optional<MacAccess> &access) {
  return access ? json::Value((int64_t)access->root) : json::Value(nullptr);
}

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "G5b-M4-0 site sets and cells\n");
  DialectRegistry registry;
  registry.insert<affine::AffineDialect, arith::ArithDialect, cf::ControlFlowDialect, func::FuncDialect,
                  LLVM::LLVMDialect, memref::MemRefDialect, scf::SCFDialect>();
  MLIRContext context(registry);
  context.allowUnregisteredDialects();
  context.loadAllAvailableDialects();
  OwningOpRef<ModuleOp> module = parseSourceFile<ModuleOp>(input, &context);
  if (!module)
    return 2;
  ImportOptions importOptions;
  importOptions.indexBitWidth = indexWidth;
  importOptions.lowerStructured = lowerStructured;
  Z3Backend backend;
  json::Array results;
  for (auto function : module->getOps<func::FuncOp>()) {
    if (function.isExternal() || (!onlyFunction.empty() && function.getSymName() != onlyFunction))
      continue;
    SemanticProgram program = importFunction(function, importOptions);
    if (!program.isSupported()) {
      results.push_back(json::Object{{"function", program.functionName}, {"supported", false}});
      continue;
    }
    auto scope = parseScope(program);
    if (!scope) {
      llvm::errs() << "malformed --scope\n";
      return 2;
    }
    TerminationOptions termination;
    termination.scope = *scope;
    termination.limits.solver.timeoutMilliseconds = timeoutMs;
    termination.limits.solver.resourceLimit = resourceLimit;
    llvm::SmallVector<llvm::StringRef> names;
    llvm::StringRef(authorize).split(names, ',', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef name : names)
      termination.authorized.insert(name.str());
    auto hints = polynomialRankHints(program, function, importOptions);
    auto certificate = analyzeTermination(program, backend, termination, hints).certificate;
    const TerminationCertificate *proof = certificate ? &*certificate : nullptr;
    EnumerationOptions enumerate;
    enumerate.scope = *scope;
    enumerate.termination = termination.limits;
    enumerate.authorized = termination.authorized;
    DomainEnumeration enumeration = enumerateDomain(program, proof, enumerate, backend);
    unsigned width = program.indexBitWidth ? program.indexBitWidth : 64;

    json::Array sites;
    for (const MacSite &site : program.sites)
      sites.push_back(json::Object{{"id", (int64_t)site.id},
                                   {"direct_form", site.form == ReductionForm::Direct},
                                   {"output", rootOrNull(site.output)},
                                   {"read", rootOrNull(site.outputRead)},
                                   {"a", (int64_t)site.a.root},
                                   {"b", (int64_t)site.b.root}});
    json::Array roots;
    for (const MemoryRoot &root : program.roots)
      roots.push_back(json::Object{{"id", (int64_t)root.id},
                                   {"fresh", root.origin == MemoryRoot::FreshAllocation}});
    json::Array groups;
    for (const SiteGroup &group : enumeration.groups) {
      json::Array listed;
      for (unsigned id : group.sites)
        listed.push_back((int64_t)id);
      groups.push_back(json::Object{
          {"output", group.output ? json::Value((int64_t)*group.output) : json::Value(nullptr)},
          {"first", (int64_t)group.first},
          {"second", (int64_t)group.second},
          {"sites", std::move(listed)},
          {"unsupported", group.unsupported}});
    }

    // Hand-made groups: the gate, and, when it refuses, each symbolic route's words.
    SymbolicCheckOptions coordinates;
    coordinates.scope = *scope;
    coordinates.termination = termination.limits;
    coordinates.authorized = termination.authorized;
    ComputationOptions computation;
    computation.scope = *scope;
    computation.termination = termination.limits;
    computation.authorized = termination.authorized;
    DomainOptions domain;
    domain.scope = *scope;
    domain.termination = termination.limits;
    domain.authorized = termination.authorized;
    const CoordinateFamily none{};
    const DomainRequest nothing{};
    json::Array lists;
    llvm::SmallVector<llvm::StringRef> listTexts;
    llvm::StringRef(siteLists).split(listTexts, ';', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef text : listTexts) {
      auto group = parseGroup(text);
      if (!group) {
        llvm::errs() << "malformed --site-lists\n";
        return 2;
      }
      std::string why;
      bool whole = wholeGroup(program, *group, why);
      json::Object entry{{"spec", text.str()}, {"whole", whole}, {"reason", why}};
      if (!whole) {
        entry["g3b"] = checkFamilySymbolic(program, proof, *group, none, nullptr, coordinates, backend).reason();
        entry["g4a"] = checkComputationSymbolic(program, proof, *group, computation, backend).reason();
        entry["g5a"] = checkDomainSymbolic(program, proof, *group, none, nothing, domain, backend).reason();
      }
      lists.push_back(std::move(entry));
    }

    // Cells.
    json::Array cells;
    llvm::SmallVector<llvm::StringRef> pairTexts;
    llvm::StringRef(cellPairs).split(pairTexts, ';', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef text : pairTexts) {
      auto [left, right] = text.split('/');
      auto a = parseCell(left, width), b = parseCell(right, width);
      if (!a || !b) {
        llvm::errs() << "malformed --cells\n";
        return 2;
      }
      cells.push_back(json::Object{
          {"spec", text.str()},
          {"relation", stringifyCellRelation(relateCells(program, termination.authorized, *a, *b))}});
    }

    // The finite routes, on the enumeration with points removed as asked.
    DomainEnumeration kept = enumeration;
    for (unsigned n = 0; n < dropPoints && !kept.points.empty(); ++n)
      kept.points.pop_back();
    unsigned past = kept.groups.size();
    json::Object pastGroups{
        {"g3b", checkFamilyFinite(program, kept, past, none).reason()},
        {"g4a", checkComputationFinite(program, kept, past, computation).reason()},
        {"g5a", checkDomainFinite(program, kept, past, none, nothing, domain).reason()}};
    json::Array finite;
    for (unsigned g = 0; g < kept.groups.size(); ++g) {
      if (!kept.groups[g].unsupported.empty())
        continue;
      CheckedFamily family = checkFamilyFinite(program, kept, g, none);
      json::Object empty{{"status", stringifyFamilyStatus(family.status())},
                         {"tier", stringifyEvidenceTier(family.tier())},
                         {"refutes", stringifyRefutes(family.refutes())},
                         {"reason", family.reason()}};
      if (family.failingPoint())
        empty["failing_point"] = points({*family.failingPoint()});
      CheckedComputation g4 = checkComputationFinite(program, kept, g, computation);
      finite.push_back(json::Object{{"group", (int64_t)g},
                                    {"empty_family", std::move(empty)},
                                    {"g4a", json::Object{{"status", stringifyComputationStatus(g4.status())},
                                                         {"tier", stringifyComputationTier(g4.tier())},
                                                         {"reason", g4.reason()}}}});
    }
    std::vector<std::vector<APInt>> pointList;
    json::Array pointReasons;
    for (const PointEvidence &point : kept.points) {
      pointList.push_back(point.parameters);
      pointReasons.push_back(point.reason);
    }
    results.push_back(json::Object{{"function", program.functionName},
                                   {"supported", true},
                                   {"certificate", proof != nullptr},
                                   {"gate", stringifyGateStatus(enumeration.gate)},
                                   {"outside_domain", (int64_t)enumeration.outsideDomain},
                                   {"sites", std::move(sites)},
                                   {"roots", std::move(roots)},
                                   {"groups", std::move(groups)},
                                   {"site_lists", std::move(lists)},
                                   {"cells", std::move(cells)},
                                   {"dropped", (int64_t)(enumeration.points.size() - kept.points.size())},
                                   {"points", points(pointList)},
                                   {"point_reasons", std::move(pointReasons)},
                                   {"gate_reason", enumeration.reason},
                                   {"ran_its_scope", enumerationRanItsScope(kept)},
                                   {"past_groups", std::move(pastGroups)},
                                   {"finite", std::move(finite)},
                                   {"authorizes_lowering", false}});
  }
  llvm::outs() << llvm::formatv("{0:2}", json::Value(std::move(results))) << "\n";
  return 0;
}
