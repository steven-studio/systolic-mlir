//===- SynthesisReport.cpp - G3b driver -----------------------------------===//
//
// Prints a JSON array with one result per function. Read-only. Candidates are
// proposals until the checkers have spoken, and no result authorizes lowering.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/PolynomialAdapter.h"
#include "GeneralizedGemm/LoopCounters.h"
#include "GeneralizedGemm/SemanticImport.h"
#include "GeneralizedGemm/ClosedFormSynthesis.h"
#include "GeneralizedGemm/IndexInvariantSynthesis.h"
#include "GeneralizedGemm/InvariantSynthesis.h"
#include "GeneralizedGemm/SynthesisAnalysis.h"
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
static cl::opt<bool> slicesFlag(
    "slices", cl::init(false),
    cl::desc("add the subscript slices, loop specifications and closed forms (phase 2)"));
static cl::opt<bool> statementGrammar(
    "statement-grammar", cl::init(false),
    cl::desc("add the loop-free statement programs of Appendix G (bit-vector adaptation)"));
static cl::opt<unsigned> maxCandidates("max-candidates", cl::init(5));
static cl::opt<unsigned> maxRounds("max-rounds", cl::init(300));
static cl::opt<uint64_t> referenceFill("reference-fill", cl::init(0));
static cl::opt<bool> printEvents("print-events", cl::init(false));
static cl::opt<bool> iterationCounters(
    "iteration-counters", cl::init(false),
    cl::desc("phase 4 (I1): add one ghost counter per loop header after the certificate is found; "
             "the counters are state the search may choose"));
static cl::opt<bool> noCertificate("no-certificate", cl::init(false),
                                   cl::desc("withhold the certificate, as a test"));
static cl::opt<std::string> authorize(
    "authorize", cl::init(""), cl::desc("comma separated assumption names"));
// The inductive invariant path (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 13).
static cl::opt<bool> loopInvariants(
    "loop-invariants", cl::init(false),
    cl::desc("prove a function candidate for a scalar loop by an inductive invariant (INV-init, "
             "INV-step, INV-exit), with the finite tier as the labelled fallback"));
static cl::opt<std::string> loopFunction(
    "loop-function", cl::init(""),
    cl::desc("result:term[;result:term...] over p<id>: the function candidate (default: synthesized)"));
static cl::opt<std::string> loopInvariant(
    "loop-invariant", cl::init(""),
    cl::desc("a certificate to check instead of a search: a conjunction (&&) over s<id>, sp<id>, p<id>"));
static cl::opt<bool> loopInvariantScripts(
    "loop-invariant-scripts", cl::init(false),
    cl::desc("print every obligation's complete SMT-LIB script"));
static cl::opt<std::string> loopDegrees(
    "loop-invariant-degrees", cl::init(""),
    cl::desc("d[,d...]: monomial degrees of the null-space candidates (default 1,2)"));
static cl::opt<std::string> loopCandidates(
    "loop-invariant-candidates", cl::init("both"),
    cl::desc("both | nullspace | collisions: where equality candidates come from"));
static cl::opt<std::string> loopLevels(
    "loop-invariant-levels", cl::init(""),
    cl::desc("degree:size[,degree:size...]: the template's levels (default 1:5,2:3,2:5)"));

// Index invariants for a GEMM's subscripts (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 14).
static cl::opt<bool> indexInvariants(
    "index-invariants", cl::init(false),
    cl::desc("search for inductive invariants over the integer state at each loop header (IDX-init, "
             "IDX-step), hand them to the coordinate checker as certificate invariants, and prove "
             "each subscript's relation to the coordinates at its site (IDX-use)"));
static cl::opt<std::string> indexInvariant(
    "index-invariant", cl::init(""),
    cl::desc("cut:conjunction[;cut:conjunction...] over s<id>, sp<id>, p<id>: a certificate to check "
             "in place of the search (a debugging comparison, reported as given)"));
static cl::opt<std::string> indexRelation(
    "index-relation", cl::init(""),
    cl::desc("site:A|B|C:dimension:term[;...]: index relations to check beside the proposed ones"));
static cl::opt<std::string> indexDegrees(
    "index-invariant-degrees", cl::init(""),
    cl::desc("d[,d...]: monomial degrees of the candidates at each cut (default 1)"));
static cl::opt<unsigned> indexSamples(
    "index-invariant-samples", cl::init(24),
    cl::desc("parameter points whose runs give the candidate search its states"));
static cl::opt<bool> indexScripts(
    "index-invariant-scripts", cl::init(false),
    cl::desc("print every IDX obligation's complete SMT-LIB script"));

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
  cl::ParseCommandLineOptions(argc, argv, "G3b coordinate synthesis\n");
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
    std::string terminationVerdict, terminationReason;
    if (!noCertificate && program.isSupported()) {
      auto hints = polynomialRankHints(program, function, importOptions);
      TerminationResult analysis = analyzeTermination(program, backend, termination, hints);
      certificate = analysis.certificate;
      terminationVerdict = stringifyTerminationVerdict(analysis.verdict.kind());
      terminationReason = analysis.verdict.reason();
    }
    // Section 14: invariants over the integer state at the cuts, proposed by a search or
    // given, proved by IDX-init and IDX-step, and conjoined into a copy of the
    // certificate that the certificate checker must accept. That copy is what the
    // search and the coordinate checker are handed below, and each asks checkCertificate
    // about it again: an invariant that is not inductive reaches no checker.
    bool indexOn = (indexInvariants || !indexInvariant.empty()) && program.isSupported();
    IndexInvariantSearchOptions indexOptions;
    IndexInvariantSearch indexSearch;
    const TerminationCertificate *handed = certificate ? &*certificate : nullptr;
    if (indexOn) {
      indexOptions.check.scope = options.scope;
      indexOptions.check.solver = termination.limits.solver;
      indexOptions.check.scripts = indexScripts;
      indexOptions.check.referenceFill = referenceFill;
      indexOptions.termination = termination.limits;
      indexOptions.authorized = termination.authorized;
      indexOptions.samplePoints = indexSamples;
      if (!indexDegrees.empty()) {
        indexOptions.degrees.clear();
        llvm::SmallVector<llvm::StringRef> items;
        llvm::StringRef(indexDegrees).split(items, ',');
        for (llvm::StringRef item : items) {
          unsigned degree;
          if (item.getAsInteger(10, degree) || !degree || degree > 3) {
            llvm::errs() << "malformed --index-invariant-degrees\n";
            return 2;
          }
          indexOptions.degrees.push_back(degree);
        }
      }
      std::optional<IndexInvariantCertificate> givenInvariant;
      if (!indexInvariant.empty()) {
        std::string why;
        givenInvariant = parseIndexInvariants(program, indexInvariant, why);
        if (!givenInvariant) {
          llvm::errs() << "malformed --index-invariant " << why << "\n";
          return 2;
        }
      }
      indexSearch = findIndexInvariants(program, handed, givenInvariant ? &*givenInvariant : nullptr,
                                        indexOptions, backend);
      if (indexSearch.strengthened)
        handed = &*indexSearch.strengthened;
    }
    // Phase 4 (I1): the certificate is about the imported program; the counters are
    // ghost state it never mentions, so it stays valid, and the checkers re-check it.
    SemanticProgram original = program;
    if (iterationCounters && program.isSupported())
      addIterationCounters(program);
    SynthesisResult result = synthesizeProgram(program, handed, options, backend);
    if (!first)
      llvm::outs() << ",\n";
    first = false;
    std::vector<std::pair<std::string, std::string>> extras;
    std::vector<std::vector<llvm::APInt>> points;
    for (const PointEvidence &point : result.enumeration.points)
      points.push_back(point.parameters);
    if (indexOn) {
      std::string why;
      auto givenRelations = parseIndexRelations(program, indexRelation, why);
      if (!givenRelations) {
        llvm::errs() << "malformed --index-relation " << why << "\n";
        return 2;
      }
      SymbolicCheckOptions symbolic;
      symbolic.scope = options.scope;
      symbolic.termination = options.termination;
      symbolic.authorized = options.authorized;
      symbolic.limits = options.symbolic;
      std::vector<IndexCandidateCheck> checks =
          checkIndexCandidates(program, certificate ? &*certificate : nullptr, indexSearch, result,
                               *givenRelations, indexOptions, symbolic, backend);
      extras.push_back({"index_invariants",
                        indexInvariantsJSON(program, indexSearch, checks, &result, indexOptions)});
    }
    if (slicesFlag && program.isSupported()) {
      ClosedFormOptions closed;
      closed.scope = options.scope;
      closed.solver = termination.limits.solver;
      closed.referenceFill = referenceFill;
      SliceSynthesisResult slices = synthesizeClosedForms(program, points, closed, backend);
      extras.push_back({"closed_forms", closedFormsJSON(program, slices)});
    }
    if (loopInvariants && program.isSupported()) {
      InvariantSearchOptions search;
      search.check.scope = options.scope;
      search.check.solver = termination.limits.solver;
      search.check.scripts = loopInvariantScripts;
      if (loopCandidates != "both" && loopCandidates != "nullspace" && loopCandidates != "collisions") {
        llvm::errs() << "malformed --loop-invariant-candidates\n";
        return 2;
      }
      search.equalityCandidates = loopCandidates;
      if (!loopDegrees.empty()) {
        search.nullSpaceDegrees.clear();
        llvm::SmallVector<llvm::StringRef> items;
        llvm::StringRef(loopDegrees).split(items, ',');
        for (llvm::StringRef item : items) {
          unsigned degree;
          if (item.getAsInteger(10, degree) || !degree || degree > 4) {
            llvm::errs() << "malformed --loop-invariant-degrees\n";
            return 2;
          }
          search.nullSpaceDegrees.push_back(degree);
        }
      }
      if (!loopLevels.empty()) {
        search.levels.clear();
        llvm::SmallVector<llvm::StringRef> items;
        llvm::StringRef(loopLevels).split(items, ',');
        for (llvm::StringRef item : items) {
          auto [d, z] = item.split(':');
          unsigned degree, size;
          if (d.getAsInteger(10, degree) || z.getAsInteger(10, size) || !degree || degree > 3 ||
              size < 1 || size > 9) {
            llvm::errs() << "malformed --loop-invariant-levels\n";
            return 2;
          }
          search.levels.push_back({degree, size});
        }
      }
      std::optional<LoopFunction> given;
      if (!loopFunction.empty()) {
        LoopFunction f;
        f.origin = "given";
        f.results.assign(program.returnSorts.size(), nullptr);
        llvm::SmallVector<llvm::StringRef> items;
        llvm::StringRef(loopFunction).split(items, ';', -1, /*KeepEmpty=*/false);
        for (llvm::StringRef item : items) {
          auto [index, text] = item.split(':');
          unsigned j;
          std::string why;
          Term term = index.trim().getAsInteger(10, j) || j >= f.results.size()
                          ? nullptr
                          : parseLoopTerm(program, text.str(), why);
          if (!term) {
            llvm::errs() << "malformed --loop-function '" << item << "': " << why << "\n";
            return 2;
          }
          f.results[j] = term;
        }
        given = f;
      }
      std::optional<InvariantCertificate> givenCertificate;
      if (!loopInvariant.empty()) {
        std::string why;
        Term inv = parseLoopTerm(program, loopInvariant, why);
        if (!inv) {
          llvm::errs() << "malformed --loop-invariant: " << why << "\n";
          return 2;
        }
        givenCertificate = InvariantCertificate{conjuncts(inv), "given"};
      }
      LoopInvariantSearch proof =
          proveLoopFunction(program, given ? &*given : nullptr,
                            givenCertificate ? &*givenCertificate : nullptr, search, backend);
      extras.push_back({"loop_invariant",
                        loopInvariantJSON(program, proof, search, terminationVerdict,
                                          terminationReason, loopInvariantScripts)});
    }
    if (iterationCounters && program.isSupported())
      extras.push_back({"iteration_counters",
                        iterationCountersJSON(original, program, points, referenceFill, options.limits)});
    llvm::outs() << toJSON(program, result, printEvents, extras);
  }
  llvm::outs() << "\n]\n";
  return 0;
}
