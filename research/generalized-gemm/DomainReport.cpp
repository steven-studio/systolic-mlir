//===- DomainReport.cpp - G5a driver -------------------------------------===//
//
// Prints a JSON array with one result per function. Read-only. A verified claim is
// about the domain, its order and the reduction, within the fragment and scope it
// states, and no result authorizes lowering.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/DomainAnalysis.h"
#include "GeneralizedGemm/IndexInvariantSynthesis.h"
#include "GeneralizedGemm/PolynomialAdapter.h"
#include "GeneralizedGemm/LoopCounters.h"
#include "GeneralizedGemm/SemanticImport.h"
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

static cl::opt<std::string> input(cl::Positional, cl::init("-"));
static cl::opt<std::string> onlyFunction("function", cl::init(""));
static cl::opt<unsigned> indexWidth("index-bit-width", cl::init(0));
static cl::opt<bool> lowerStructured("lower-structured", cl::init(false));
static cl::opt<std::string> scopeText(
    "scope", cl::init(""), cl::desc("parameter:lower:upper,... as signed decimals"));
static cl::opt<unsigned> timeoutMs("timeout-ms", cl::init(10000));
static cl::opt<unsigned> resourceLimit("rlimit", cl::init(20000000));
static cl::opt<uint64_t> maxPoints("max-points", cl::init(256));
static cl::opt<unsigned> maxLevel("max-level", cl::init(3));
static cl::opt<int> fixedLevel("grammar-level", cl::init(-1),
                               cl::desc("use only this grammar level"));
static cl::opt<bool> statementGrammar(
    "statement-grammar", cl::init(false),
    cl::desc("add the loop-free statement programs of Appendix G (bit-vector adaptation)"));
static cl::opt<unsigned> maxCandidates("max-candidates", cl::init(5));
static cl::opt<unsigned> maxRounds("max-rounds", cl::init(300));
static cl::opt<uint64_t> referenceFill("reference-fill", cl::init(0));
static cl::opt<bool> printEvents("print-events", cl::init(false));
static cl::opt<uint64_t> maxBox("max-box", cl::init(1u << 20));
static cl::opt<unsigned> candidatesPerGroup("candidates-per-group", cl::init(2));
static cl::opt<std::string> groupSets(
    "groups", cl::init(""),
    cl::desc("G5b-M4a: domain claims over sets of groups, ';' between sets, each a comma "
             "separated list of group indices, `all` or `all-reversed`"));
static cl::opt<bool> segmentsFlag("segments", cl::init(false),
                                  cl::desc("propose segments by site, in place of the search; with --family, "
                                           "the family names the logical loop variables and the rest repeat"));
static cl::opt<std::string> familyText(
    "family", cl::init(""),
    cl::desc("p:ids;i:ids;j:ids;k:ids, state ids per axis, in place of the search"));
static cl::opt<uint64_t> replaySteps("replay-steps", cl::init(1u << 22));
static cl::opt<uint64_t> replayNodes("replay-nodes", cl::init(1u << 23));
static cl::opt<unsigned> maxQueries("max-queries", cl::init(4000));
static cl::opt<bool> iterationCounters(
    "iteration-counters", cl::init(false),
    cl::desc("phase 4 (I1): add one ghost counter per loop header after the certificate is found; "
             "the counters are state the search may choose"));
static cl::opt<bool> closedFormViews(
    "closed-form-views", cl::init(false),
    cl::desc("phase 4 (I2, I3): closed forms of the slice variables, proved ones over the counters "
             "handed to the proposer as identities for guards, bounds and views; each view's "
             "affinity reported"));
static cl::opt<bool> noCertificate("no-certificate", cl::init(false),
                                   cl::desc("withhold the certificate, as a test"));
static cl::opt<std::string> authorize(
    "authorize", cl::init(""), cl::desc("comma separated assumption names"));

// Index invariants for a GEMM's subscripts (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 14).
static cl::opt<bool> indexInvariants(
    "index-invariants", cl::init(false),
    cl::desc("search for inductive invariants over the integer state at each loop header, hand them "
             "to every checker as certificate invariants, and hand the proposer the identities they "
             "give for the views"));
static cl::opt<std::string> indexInvariant(
    "index-invariant", cl::init(""),
    cl::desc("cut:conjunction[;cut:conjunction...] over s<id>, sp<id>, p<id>: a certificate to check "
             "in place of the search (a debugging comparison, reported as given)"));

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
    if (parts.size() != 3 || parts[0].getAsInteger(10, id) ||
        id >= program.parameters.size())
      return std::nullopt;
    unsigned width = program.parameters[id].sort.width;
    int64_t lower, upper;
    if (parts[1].getAsInteger(10, lower) || parts[2].getAsInteger(10, upper))
      return std::nullopt;
    llvm::APInt low(64, lower, /*isSigned=*/true), high(64, upper, /*isSigned=*/true);
    if (!low.isSignedIntN(width) || !high.isSignedIntN(width))
      return std::nullopt;
    scope.ranges.push_back({id, low.trunc(width), high.trunc(width)});
  }
  return scope;
}

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "G5a domain and reduction semantics\n");
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

  ImportOptions importOptions;
  importOptions.indexBitWidth = indexWidth;
  importOptions.lowerStructured = lowerStructured;
  Z3Backend backend;
  bool first = true;
  llvm::outs() << "[\n";
  for (auto function : module->getOps<func::FuncOp>()) {
    if (function.isExternal())
      continue;
    if (!onlyFunction.empty() && function.getSymName() != onlyFunction)
      continue;
    SemanticProgram program = importFunction(function, importOptions);
    TerminationOptions termination;
    SynthesisOptions options;
    if (program.isSupported()) {
      auto scope = parseScope(program);
      if (!scope) {
        llvm::errs() << "malformed --scope for " << program.functionName << "\n";
        return 2;
      }
      termination.scope = options.scope = *scope;
    }
    termination.limits.solver.timeoutMilliseconds = timeoutMs;
    termination.limits.solver.resourceLimit = resourceLimit;
    llvm::SmallVector<llvm::StringRef> names;
    llvm::StringRef(authorize).split(names, ',', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef name : names)
      termination.authorized.insert(name.str());
    options.termination = termination.limits;
    options.authorized = termination.authorized;
    options.limits.maxPoints = maxPoints;
    options.maxLevel = maxLevel;
    options.synthesis.maxCandidates = maxCandidates;
    options.synthesis.maxRounds = maxRounds;
    options.synthesis.solver = termination.limits.solver;
    options.referenceFill = referenceFill;
    if (fixedLevel >= 0)
      options.grammar = GrammarSpec::atLevel(fixedLevel);
    options.statementGrammar = statementGrammar;
    std::optional<TerminationCertificate> certificate;
    if (!noCertificate && program.isSupported()) {
      auto hints = polynomialRankHints(program, function, importOptions);
      certificate = analyzeTermination(program, backend, termination, hints).certificate;
    }
    // Phase 4 (I1): the certificate is about the imported program; the counters are
    // ghost state it never mentions, so it stays valid, and the checkers re-check it.
    SemanticProgram original = program;
    if (iterationCounters && program.isSupported())
      addIterationCounters(program);
    DomainAnalysisRequest request;
    request.synthesis = options;
    request.synthesis.limits.maxPoints = maxPoints;
    request.domain.scope = options.scope;
    request.domain.termination = termination.limits;
    request.domain.authorized = termination.authorized;
    request.domain.limits.maxBox = maxBox;
    request.domain.limits.maxQueries = maxQueries;
    request.domain.computation.replay.maxSteps = replaySteps;
    request.domain.computation.replay.maxNodes = replayNodes;
    request.proposal.maxBox = maxBox;
    request.maxCandidates = candidatesPerGroup;
    request.segmented = segmentsFlag;
    request.closedFormViews = closedFormViews;
    request.closedForms.scope = options.scope;
    request.closedForms.solver = termination.limits.solver;
    request.closedForms.referenceFill = referenceFill;
    {
      llvm::SmallVector<llvm::StringRef> sets;
      llvm::StringRef(groupSets).split(sets, ';', -1, /*KeepEmpty=*/false);
      for (llvm::StringRef text : sets) {
        DomainSetRequest set;
        set.all = text == "all" || text == "all-reversed";
        set.reversed = text == "all-reversed";
        llvm::SmallVector<llvm::StringRef> items;
        if (!set.all)
          text.split(items, ',', -1, /*KeepEmpty=*/false);
        for (llvm::StringRef item : items) {
          unsigned g;
          if (item.trim().getAsInteger(10, g)) {
            llvm::errs() << "malformed --groups\n";
            return 2;
          }
          set.groups.push_back(g);
        }
        request.groupSets.push_back(std::move(set));
      }
    }
    if (!familyText.empty() && program.isSupported()) {
      CoordinateFamily family;
      family.origin = "given on the command line";
      llvm::SmallVector<llvm::StringRef> axes;
      llvm::StringRef(familyText).split(axes, ';');
      bool ok = axes.size() == 4;
      for (unsigned axis = 0; ok && axis < 4; ++axis) {
        llvm::StringRef body = axes[axis];
        ok = body.size() >= 2 && body[0] == "pijk"[axis] && body[1] == ':';
        llvm::SmallVector<llvm::StringRef> ids;
        if (ok)
          body.drop_front(2).split(ids, ',', -1, /*KeepEmpty=*/false);
        for (llvm::StringRef text : ids) {
          unsigned id;
          ok = ok && !text.getAsInteger(10, id) && id < program.state.size() &&
               program.state[id].sort.kind == Sort::BV;
          if (ok)
            family.axes[axis].push_back(program.terms->state(id, program.state[id].sort));
        }
      }
      if (!ok) {
        llvm::errs() << "malformed --family for " << program.functionName << "\n";
        return 2;
      }
      request.family = family;
    }
    // Section 14: the invariants are proved first (IDX-init, IDX-step) and conjoined into a
    // copy of the certificate that the certificate checker must accept; that copy is what
    // the analysis is handed, and every checker in it asks checkCertificate again. The
    // identities are proposals for the views.
    bool indexOn = (indexInvariants || !indexInvariant.empty()) && program.isSupported();
    IndexInvariantSearchOptions indexOptions;
    IndexInvariantSearch indexSearch;
    std::vector<IndexIdentity> identities;
    const TerminationCertificate *handed = certificate ? &*certificate : nullptr;
    if (indexOn) {
      indexOptions.check.scope = options.scope;
      indexOptions.check.solver = termination.limits.solver;
      indexOptions.check.referenceFill = referenceFill;
      indexOptions.termination = termination.limits;
      indexOptions.authorized = termination.authorized;
      std::optional<IndexInvariantCertificate> given;
      if (!indexInvariant.empty()) {
        std::string why;
        given = parseIndexInvariants(program, indexInvariant, why);
        if (!given) {
          llvm::errs() << "malformed --index-invariant " << why << "\n";
          return 2;
        }
      }
      indexSearch = findIndexInvariants(program, handed, given ? &*given : nullptr, indexOptions, backend);
      if (indexSearch.strengthened) {
        handed = &*indexSearch.strengthened;
        identities = indexIdentities(program, indexSearch);
        for (const IndexIdentity &identity : identities) {
          StateIdentity proposal;
          proposal.state = identity.state;
          proposal.term = identity.term;
          proposal.cut = identity.cut;
          proposal.origin = identity.origin;
          request.indexIdentities.push_back(std::move(proposal));
        }
      }
    }
    DomainResult result = analyzeDomains(program, handed, request, backend);
    if (!first)
      llvm::outs() << ",\n";
    first = false;
    std::string text = toJSON(program, result, printEvents);
    if (iterationCounters && program.isSupported()) {
      // The counters' record, appended to the object: the JSON above ends with "}".
      std::vector<std::vector<llvm::APInt>> points;
      for (const PointEvidence &point : result.enumeration.points)
        points.push_back(point.parameters);
      std::string counters = iterationCountersJSON(original, program, points, referenceFill, options.limits);
      size_t close = text.rfind('}');
      if (close != std::string::npos)
        text = text.substr(0, close) + ",\n  \"iteration_counters\": " + counters + "\n}";
    }
    if (indexOn) {
      std::string record = indexInvariantsJSON(program, indexSearch, {}, nullptr, indexOptions, identities);
      // Rendered like the rest of the object.
      if (auto parsed = llvm::json::parse(record)) {
        record.clear();
        llvm::raw_string_ostream stream(record);
        stream << llvm::formatv("{0:2}", *parsed);
      } else {
        llvm::consumeError(parsed.takeError());
      }
      size_t close = text.rfind('}');
      if (close != std::string::npos)
        text = text.substr(0, close) + ",\n  \"index_invariants\": " + record + "\n}";
    }
    llvm::outs() << text;
  }
  llvm::outs() << "\n]\n";
  return 0;
}
