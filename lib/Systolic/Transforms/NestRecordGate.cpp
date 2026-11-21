//===- NestRecordGate.cpp - Records of loop nests as a gate --------------===//
//
// See NestRecordGate.h. Every check below can only refuse: a record that passes all of
// them lets the lowering run its own checks, and nothing else.
//
//===----------------------------------------------------------------------===//
#include "Systolic/NestRecordGate.h"

#include "Systolic/AffineMatmulPartitioning.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>

using namespace mlir;
using namespace mlir::systolic;
namespace json = llvm::json;

namespace {

using Fields = NestRecordFields;

const char *const kAxisName[] = {"I", "J", "K"};

/// The research tool's name of an element type: f32, f64, or bv<width>.
std::string elementName(Type type) {
  if (auto f = dyn_cast<FloatType>(type))
    return "f" + std::to_string(f.getWidth());
  if (auto i = dyn_cast<IntegerType>(type))
    return "bv" + std::to_string(i.getWidth());
  return "?";
}

/// The research tool's refusals (NestRecord.cpp, mappingRefusals) that say only that a view
/// has no affine closed form: none was proposed, or a dimension of the one proposed is not
/// affine. Stage 3's tables read such views off the nest itself. Every other refusal about a
/// view (no root, not proved, a parameter, a coordinate that is no role) is no such refusal.
bool isUnmappedViewRefusal(StringRef text) {
  for (StringRef name : {"A", "B", "C"}) {
    if (text == ("the view of " + name + " has no proposed closed form").str())
      return true;
    StringRef rest = text;
    if (!rest.consume_front("dimension "))
      continue;
    size_t digits = 0;
    while (digits < rest.size() && llvm::isDigit(rest[digits]))
      ++digits;
    if (digits == 0)
      continue;
    rest = rest.drop_front(digits);
    if (rest.consume_front((" of the view of " + name + " is not affine: ").str()))
      return true;
  }
  return false;
}

/// The research tool's names of the fast-math flags, in its order.
std::vector<std::string> flagNames(arith::FastMathFlags flags) {
  static const std::pair<arith::FastMathFlags, const char *> names[] = {
      {arith::FastMathFlags::reassoc, "reassoc"}, {arith::FastMathFlags::nnan, "nnan"},
      {arith::FastMathFlags::ninf, "ninf"},       {arith::FastMathFlags::nsz, "nsz"},
      {arith::FastMathFlags::arcp, "arcp"},       {arith::FastMathFlags::contract, "contract"},
      {arith::FastMathFlags::afn, "afn"}};
  std::vector<std::string> out;
  for (const auto &[bit, name] : names)
    if (arith::bitEnumContainsAll(flags, bit))
      out.push_back(name);
  return out;
}

std::vector<std::string> namesOf(const json::Array &array, bool &wellFormed) {
  std::vector<std::string> out;
  for (const json::Value &value : array) {
    std::optional<StringRef> name = value.getAsString();
    if (!name) {
      wellFormed = false;
      return out;
    }
    out.push_back(name->str());
  }
  return out;
}

std::string joined(const std::vector<std::string> &names) {
  return names.empty() ? std::string("none") : llvm::join(names, ", ");
}

} // namespace

namespace mlir::systolic {

FailureOr<NestRecordFile> NestRecordFile::load(StringRef path, std::string &error) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer = llvm::MemoryBuffer::getFile(path, /*IsText=*/true);
  if (!buffer) {
    error = "cannot read " + path.str() + ": " + buffer.getError().message();
    return failure();
  }
  llvm::Expected<json::Value> parsed = json::parse((*buffer)->getBuffer());
  if (!parsed) {
    error = path.str() + " is no JSON: " + llvm::toString(parsed.takeError());
    return failure();
  }
  NestRecordFile out;
  out.document = std::make_shared<json::Value>(std::move(*parsed));
  const json::Array *functions = out.document->getAsArray();
  if (!functions) {
    error = path.str() + " is no array of function objects";
    return failure();
  }
  for (const json::Value &value : *functions) {
    const json::Object *function = value.getAsObject();
    if (!function) {
      error = path.str() + ": an element is no function object";
      return failure();
    }
    std::optional<StringRef> schema = function->getString("schema");
    if (!schema || *schema != kNestRecordSchema) {
      error = path.str() + ": a function object is not of the format " + kNestRecordSchema +
              (schema ? " (it is " + schema->str() + ")" : std::string(" (it names none)"));
      return failure();
    }
    std::optional<StringRef> name = function->getString("function");
    if (!name) {
      error = path.str() + ": a function object names no function";
      return failure();
    }
    if (!out.functions.try_emplace(*name, function).second) {
      error = path.str() + ": two function objects for " + name->str();
      return failure();
    }
    const json::Array *nests = function->getArray("nests");
    if (!nests) {
      error = path.str() + ": function " + name->str() + " has no list of nests";
      return failure();
    }
    auto &byOrdinal = out.nests[*name];
    for (const json::Value &element : *nests) {
      const json::Object *nest = element.getAsObject();
      std::optional<int64_t> ordinal = nest ? nest->getInteger("nest") : std::nullopt;
      if (!ordinal || *ordinal < 0) {
        error = path.str() + ": a nest of " + name->str() + " has no number";
        return failure();
      }
      if (!byOrdinal.try_emplace((unsigned)*ordinal, nest).second) {
        error = path.str() + ": two records of nest " + std::to_string(*ordinal) + " of " + name->str();
        return failure();
      }
    }
  }
  return out;
}

const json::Object *NestRecordFile::function(StringRef name) const {
  auto found = functions.find(name);
  return found == functions.end() ? nullptr : found->second;
}

const json::Object *NestRecordFile::nest(StringRef function, unsigned ordinal) const {
  auto found = nests.find(function);
  if (found == nests.end())
    return nullptr;
  auto record = found->second.find(ordinal);
  return record == found->second.end() ? nullptr : record->second;
}

BoundNests::BoundNests(ModuleOp module) {
  module.walk([&](func::FuncOp function) {
    if (function.isExternal())
      return;
    FunctionRoots roots = numberFunctionRoots(function);
    for (auto [ordinal, loop] : llvm::enumerate(outermostLoops(function))) {
      Nest nest;
      nest.function = function;
      nest.ordinal = ordinal;
      std::string why;
      FailureOr<NestBinding> binding = computeNestBinding(function, loop, ordinal, roots, why);
      if (succeeded(binding))
        nest.binding = std::move(*binding);
      else
        nest.refusal = why;
      nestOf[loop] = std::move(nest);
    }
    rootsOf[function.getOperation()] = std::move(roots);
  });
}

BoundNests::BoundNests(ModuleOp module, const BoundNests &atInput) {
  module.walk([&](func::FuncOp function) {
    if (function.isExternal())
      return;
    FunctionRoots roots = numberFunctionRoots(function);
    for (Operation *loop : outermostLoops(function)) {
      // A loop at the address of a nest of atInput is that nest only if its binding says so:
      // an operation made after one was erased may reuse its address.
      const Nest *before = atInput.find(loop);
      if (!before || before->function != function || !before->binding)
        continue;
      std::string why;
      FailureOr<NestBinding> now = computeNestBinding(function, loop, before->ordinal, roots, why);
      if (failed(now) || now->sha256 != before->binding->sha256 || now->text != before->binding->text)
        continue;
      Nest nest;
      nest.function = function;
      nest.ordinal = before->ordinal;
      nest.binding = std::move(*now);
      nestOf[loop] = std::move(nest);
    }
    rootsOf[function.getOperation()] = std::move(roots);
  });
}

const BoundNests::Nest *BoundNests::find(Operation *loop) const {
  auto found = nestOf.find(loop);
  return found == nestOf.end() ? nullptr : &found->second;
}

const FunctionRoots *BoundNests::roots(func::FuncOp function) const {
  auto found = rootsOf.find(function.getOperation());
  return found == rootsOf.end() ? nullptr : &found->second;
}

RecordAtNest checkRecordAtNest(const NestRecordFile &file, const BoundNests &bound, Operation *anchor,
                               unsigned indexWidth, bool unmappedViews) {
  RecordAtNest out;
  std::string &why = out.why;
  Fields fields{why};
  // The first reason is kept: a field that is missing has already said so. Once the nest
  // is located, every reason names it.
  std::string prefix;
  auto refuse = [&](const llvm::Twine &text) {
    out.usable = false;
    if (why.empty()) {
      why = text.str();
      if (why.empty())
        why = "the record is incomplete";
    }
    why = prefix + why;
    return out;
  };

  // ---------------------------------------------------------------------------------
  // Which record: the nest as it stood at the pass's input, by function and position.
  // ---------------------------------------------------------------------------------
  auto function = anchor->getParentOfType<func::FuncOp>();
  if (!function)
    return refuse("the GEMM is in no function");
  const BoundNests::Nest *nest = bound.find(anchor);
  if (!nest)
    return refuse("the GEMM's outermost loop is not one of the outermost loops of " + function.getSymName() +
                  " as they stood at the pass's input, and a record is about one of those");
  std::string where = "nest " + std::to_string(nest->ordinal) + " of " + function.getSymName().str();
  prefix = where + ": ";
  if (!nest->binding)
    return refuse("no binding: " + nest->refusal);
  const json::Object *functionRecord = file.function(function.getSymName());
  if (!functionRecord)
    return refuse("no record of function " + function.getSymName());
  const json::Object *record = file.nest(function.getSymName(), nest->ordinal);
  if (!record)
    return refuse("the function's records have no record of this nest");

  // ---------------------------------------------------------------------------------
  // The function's roots, as the record names them.
  // ---------------------------------------------------------------------------------
  const FunctionRoots *roots = bound.roots(function);
  const json::Array *recordRoots = fields.array(*functionRecord, "roots", "function");
  if (!roots || !recordRoots)
    return refuse("");
  if (recordRoots->size() != roots->roots.size())
    return refuse("the record's function has " + std::to_string(recordRoots->size()) + " roots and " +
                  function.getSymName() + " has " + std::to_string(roots->roots.size()) +
                  ": the record is about another version of the function");
  for (auto [id, value] : llvm::enumerate(*recordRoots)) {
    const json::Object *root = value.getAsObject();
    if (!root)
      return refuse("the record is incomplete: a root of the function is no object");
    std::optional<int64_t> number = fields.integer(*root, "root", "roots[]");
    std::optional<StringRef> origin = fields.string(*root, "origin", "roots[]");
    std::optional<StringRef> type = fields.string(*root, "type", "roots[]");
    if (!number || !origin || !type)
      return refuse("");
    const FunctionRoot &mine = roots->roots[id];
    bool same = *number == (int64_t)id && *type == mine.type &&
                *origin == (mine.argument ? "argument" : "allocation");
    if (same && mine.argument) {
      std::optional<int64_t> argument = fields.integer(*root, "argument", "roots[]");
      std::optional<bool> noalias = fields.boolean(*root, "noalias", "roots[]");
      if (!argument || !noalias)
        return refuse("");
      same = *argument == (int64_t)mine.argNumber && *noalias == mine.noalias;
    }
    if (!same)
      return refuse("root " + std::to_string(id) + " of the record's function is not root " + std::to_string(id) +
                    " of " + function.getSymName() + ": the record is about another version of the function");
  }

  // ---------------------------------------------------------------------------------
  // The record: about this nest, complete, verified, and authorizing nothing.
  // ---------------------------------------------------------------------------------
  std::optional<bool> functionAuthorizes = fields.boolean(*functionRecord, "authorizes_lowering", "function");
  std::optional<bool> authorizes = fields.boolean(*record, "authorizes_lowering", "nest");
  if (!functionAuthorizes || !authorizes)
    return refuse("");
  if (*functionAuthorizes || *authorizes)
    return refuse("the record claims to authorize a lowering, and no record this pass reads does");
  const json::Object *binding = fields.object(*record, "binding", "nest");
  if (!binding)
    return refuse("");
  std::optional<StringRef> scheme = fields.string(*binding, "scheme", "binding");
  std::optional<int64_t> width = fields.integer(*binding, "index_width", "binding");
  if (!scheme || !width)
    return refuse("");
  if (*scheme != kNestBindingScheme)
    return refuse("the record's binding is of the scheme " + *scheme + ", and this pass computes " +
                  kNestBindingScheme);
  std::optional<StringRef> sha = binding->getString("sha256");
  if (!sha)
    return refuse("the record has no binding" +
                  (binding->getString("reason") ? ": " + binding->getString("reason")->str() : std::string()));
  if (*sha != nest->binding->sha256)
    return refuse("the record's binding is not this nest's: the record is about another version of "
                  "the nest, or another nest");
  if (*width != (int64_t)indexWidth)
    return refuse("the record's claims are about index values of " + std::to_string(*width) +
                  " bits, and this program's index has " + std::to_string(indexWidth));
  std::optional<StringRef> status = fields.string(*record, "status", "nest");
  std::optional<bool> orderedFold = fields.boolean(*record, "ordered_fold_verified", "nest");
  const json::Array *refusals = fields.array(*record, "refusals", "nest");
  const json::Array *scope = fields.array(*record, "scope", "nest");
  if (!status || !orderedFold || !refusals || !scope)
    return refuse("");
  // Stage 3's tables (unmappedViews): a record VERIFIED_NOT_MAPPED only because the research
  // proposed no closed form for some view; the lowering then reads those views off the nest
  // itself, over its whole static domain.
  bool viewsUnmapped = false, outputMayCollide = false, outputGathered = false;
  if (unmappedViews && *status == "VERIFIED_NOT_MAPPED" && !refusals->empty()) {
    viewsUnmapped = true;
    for (const json::Value &value : *refusals) {
      std::optional<StringRef> text = value.getAsString();
      if (text && isUnmappedViewRefusal(*text))
        continue;
      // exp/general-scatter: what only the scatter-add takes.
      if (text && text->starts_with("two logical outputs may share a cell of C")) {
        outputMayCollide = true;
        continue;
      }
      if (text && text->starts_with("dimension ") && text->contains(" of the view of C gathers at ")) {
        outputGathered = true;
        continue;
      }
      viewsUnmapped = false;
    }
    if (!viewsUnmapped)
      outputMayCollide = outputGathered = false;
  }
  if (*status != "VERIFIED" && !viewsUnmapped) {
    std::string first;
    if (!refusals->empty())
      if (std::optional<StringRef> text = (*refusals)[0].getAsString())
        first = ": " + text->str();
    return refuse("the record's status is " + *status + first);
  }
  if (!*orderedFold)
    return refuse("the record does not verify the ordered fold");
  if (!refusals->empty() && !viewsUnmapped)
    return refuse("the record has refusals");
  if (!scope->empty())
    return refuse("the record's claims hold for a range of arguments only, and the lowering is for every value");
  const json::Object *gemm = fields.object(*record, "gemm", "nest");
  if (!gemm)
    return refuse("");
  const json::Object *tiers = fields.object(*gemm, "tiers", "gemm");
  const json::Object *theorem = tiers ? fields.object(*tiers, "theorem13", "gemm.tiers") : nullptr;
  if (!theorem)
    return refuse("");
  std::optional<StringRef> t13Status = fields.string(*theorem, "status", "gemm.tiers.theorem13");
  std::optional<StringRef> t13Tier = fields.string(*theorem, "tier", "gemm.tiers.theorem13");
  std::optional<StringRef> t13Logical = fields.string(*theorem, "logical_tier", "gemm.tiers.theorem13");
  std::optional<StringRef> holdsFor = fields.string(*theorem, "holds_for", "gemm.tiers.theorem13");
  if (!t13Status || !t13Tier || !t13Logical || !holdsFor)
    return refuse("");
  if (*t13Status != "VERIFIED" || *t13Tier != "SYMBOLIC" || *t13Logical != "SYMBOLIC")
    return refuse("the record's Theorem 13 is " + *t13Status + " at " + *t13Tier + ", logical tier " + *t13Logical +
                  ", and a record is read only when all three are VERIFIED at SYMBOLIC");
  const json::Object *frame = fields.object(*gemm, "frame", "gemm");
  std::optional<int64_t> otherWrites = frame ? fields.integer(*frame, "other_writes", "gemm.frame") : std::nullopt;
  std::optional<bool> box = fields.boolean(*gemm, "box", "gemm");
  std::optional<StringRef> guard = fields.string(*gemm, "guard", "gemm");
  if (!otherWrites || !box || !guard)
    return refuse("");
  if (*otherWrites != 0)
    return refuse("the record says the nest writes memory outside the multiply-accumulate");
  if (!*box || *guard != "true")
    return refuse("the record's domain is no box");

  out.usable = true;
  out.where = where;
  out.nest = nest;
  out.function = function;
  out.roots = roots;
  out.functionRecord = functionRecord;
  out.record = record;
  out.gemm = gemm;
  out.holdsFor = holdsFor->str();
  out.viewsUnmapped = viewsUnmapped;
  out.outputMayCollide = outputMayCollide;
  out.outputGathered = outputGathered;
  return out;
}


NestRecordDecision checkNestRecord(const NestRecordFile &file, const BoundNests &bound,
                                   const LogicalMatmul &matmul, unsigned indexWidth) {
  NestRecordDecision out;
  RecordAtNest at = checkRecordAtNest(file, bound, matmul.anchor, indexWidth, /*unmappedViews=*/false);
  if (!at.usable) {
    out.why = at.why;
    return out;
  }
  std::string &why = out.why;
  Fields fields{why};
  // The record is located and checked by itself; every reason below names the nest.
  std::string prefix = at.where + ": ";
  auto refuse = [&](const llvm::Twine &text) {
    out.agrees = false;
    if (why.empty()) {
      why = text.str();
      if (why.empty())
        why = "the record is incomplete";
    }
    why = prefix + why;
    return out;
  };
  Operation *anchor = matmul.anchor;
  const std::string &where = at.where;
  const BoundNests::Nest *nest = at.nest;
  const FunctionRoots *roots = at.roots;
  const json::Object *record = at.record;
  const json::Object *gemm = at.gemm;
  const std::string &holdsFor = at.holdsFor;
  // ---------------------------------------------------------------------------------
  // What the production recognizer found, in the same terms.
  // ---------------------------------------------------------------------------------
  std::string denseWhy;
  FailureOr<DenseLogicalGemm> dense = getDenseLogicalGemm(matmul, &denseWhy);
  if (failed(dense))
    return refuse("the lowering has no dense form of the GEMM to compare the record with: " + denseWhy);
  if (dense->numBatchDims || !matmul.demotedLoops.empty() || !matmul.reductionBatchLoops.empty())
    return refuse("the GEMM has batch loops, and a record of one box describes none");
  if (!matmul.accumulates)
    return refuse("the GEMM is an assignment C = A * B, and a record describes an accumulation");
  // The loops from the GEMM's outermost loop down to its store, outermost first.
  SmallVector<affine::AffineForOp, 3> band;
  for (Operation *op = matmul.outputStore->getParentOp();; op = op->getParentOp()) {
    if (!op)
      return refuse("the GEMM's store is not inside its outermost loop");
    auto loop = dyn_cast<affine::AffineForOp>(op);
    if (!loop)
      return refuse("the GEMM's store is under an operation (" + op->getName().getStringRef() +
                    ") that is no loop of the GEMM");
    band.push_back(loop);
    if (op == anchor)
      break;
  }
  std::reverse(band.begin(), band.end());
  if (band.size() != 3)
    return refuse("the GEMM's nest has " + std::to_string(band.size()) + " loops, and a record of one box has 3");
  for (affine::AffineForOp loop : band)
    if (!loop.hasConstantLowerBound() || !loop.hasConstantUpperBound() || loop.getStepAsInt() != 1)
      return refuse("a loop of the GEMM has no constant bounds or a step other than 1");
  // y = T x + c: each of I, J and K is one loop's counter as it is.
  const AffineLogicalMapping &mapping = matmul.logicalMapping;
  if (mapping.transformation.size() != 3 || mapping.offset.size() != 3)
    return refuse("the GEMM's logical mapping is not one row per coordinate I, J and K");
  for (unsigned axis = 0; axis < 3; ++axis) {
    const auto &row = mapping.transformation[axis];
    unsigned loop = dense->sourceDim[axis];
    bool unit = row.size() == 3 && mapping.offset[axis] == 0 && loop < 3 && dense->sourceDims[axis].size() == 1;
    for (unsigned column = 0; unit && column < 3; ++column)
      unit = row[column] == (column == loop ? 1 : 0);
    if (!unit)
      return refuse(std::string("the GEMM's coordinate ") + kAxisName[axis] + " is not the counter of one of its loops");
  }

  // M, N, K and the loops of I, J and K.
  const json::Object *extents = fields.object(*gemm, "extents", "gemm");
  const json::Array *levels = fields.array(*gemm, "levels", "gemm");
  const json::Object *axes = fields.object(*gemm, "axes", "gemm");
  if (!extents || !levels || !axes)
    return refuse("");
  static const char *const extentName[] = {"M", "N", "K"};
  for (unsigned axis = 0; axis < 3; ++axis) {
    std::optional<int64_t> extent = fields.integer(*extents, extentName[axis], "gemm.extents");
    if (!extent)
      return refuse("");
    if (*extent != dense->extent[axis])
      return refuse(std::string("the record's ") + extentName[axis] + " is " + std::to_string(*extent) +
                    " and the lowering's is " + std::to_string(dense->extent[axis]));
    const json::Array *list = fields.array(*axes, kAxisName[axis], "gemm.axes");
    if (!list)
      return refuse("");
    std::optional<int64_t> level = list->size() == 1 ? (*list)[0].getAsInteger() : std::nullopt;
    if (!level || *level != (int64_t)dense->sourceDim[axis])
      return refuse(std::string("the record's coordinate ") + kAxisName[axis] + " is not on loop " +
                    std::to_string(dense->sourceDim[axis]) + ", where the lowering has it");
  }
  if (levels->size() != 3)
    return refuse("the record has " + std::to_string(levels->size()) + " levels, and the GEMM's nest 3 loops");
  for (unsigned l = 0; l < 3; ++l) {
    const json::Object *level = (*levels)[l].getAsObject();
    if (!level)
      return refuse("the record is incomplete: a level is no object");
    std::optional<StringRef> axis = fields.string(*level, "axis", "gemm.levels[]");
    std::optional<bool> descending = fields.boolean(*level, "descending", "gemm.levels[]");
    std::optional<int64_t> lower = fields.integer(*level, "lower", "gemm.levels[]");
    std::optional<int64_t> upper = fields.integer(*level, "upper", "gemm.levels[]");
    if (!axis || !descending || !lower || !upper)
      return refuse("");
    unsigned expected = std::find(dense->sourceDim.begin(), dense->sourceDim.end(), l) - dense->sourceDim.begin();
    if (expected >= 3 || *axis != kAxisName[expected] || *descending ||
        *lower != band[l].getConstantLowerBound() || *upper != band[l].getConstantUpperBound())
      return refuse("level " + std::to_string(l) + " of the record is not loop " + std::to_string(l) +
                    " of the GEMM: axis, direction or bounds differ");
  }

  // The operands: roots, and per dimension the loop, the scale and the constant.
  const json::Object *views = fields.object(*gemm, "views", "gemm");
  if (!views)
    return refuse("");
  struct Operand {
    const char *name;
    Value memref;
    const std::array<OperandCoordinateView, 2> &view;
    const OperandLayout &layout;
    AffineMap symbolic;
    unsigned roles[2];
  };
  const Operand operands[] = {
      {"A", matmul.lhs, matmul.viewA, matmul.layoutA, matmul.symbolicA, {kLogicalI, kLogicalK}},
      {"B", matmul.rhs, matmul.viewB, matmul.layoutB, matmul.symbolicB, {kLogicalK, kLogicalJ}},
      {"C", matmul.output, matmul.viewC, matmul.layoutC, matmul.symbolicC, {kLogicalI, kLogicalJ}}};
  unsigned rootOf[3] = {0, 0, 0};
  for (auto [index, operand] : llvm::enumerate(operands)) {
    auto found = roots->number.find(operand.memref);
    if (found == roots->number.end())
      return refuse(std::string("operand ") + operand.name + " is no root of the function");
    rootOf[index] = found->second;
    auto type = cast<MemRefType>(operand.memref.getType());
    const OperandLayout &layout = operand.layout;
    if (layout.general || layout.roleDim[0] < 0 || layout.roleDim[1] < 0 || type.getRank() != 2 ||
        layout.hasBatchTerms(0) || layout.hasBatchTerms(1) || operand.symbolic)
      return refuse(std::string("the lowering's view of ") + operand.name +
                    " is not one role per dimension of a rank-2 array with no run-time offset, which a record "
                    "compares with");
    const json::Object *view = fields.object(*views, operand.name, "gemm.views");
    const json::Object *root = view ? fields.object(*view, "root", std::string("gemm.views.") + operand.name) : nullptr;
    std::optional<int64_t> rootNumber =
        root ? fields.integer(*root, "root", std::string("gemm.views.") + operand.name + ".root") : std::nullopt;
    std::optional<bool> proved =
        view ? fields.boolean(*view, "proved", std::string("gemm.views.") + operand.name) : std::nullopt;
    const json::Array *dimensions =
        view ? fields.array(*view, "dimensions", std::string("gemm.views.") + operand.name) : nullptr;
    if (!rootNumber || !proved || !dimensions)
      return refuse("");
    if (*rootNumber != (int64_t)rootOf[index])
      return refuse(std::string("the record's ") + operand.name + " is root " + std::to_string(*rootNumber) +
                    " and the lowering's is root " + std::to_string(rootOf[index]));
    if (!*proved)
      return refuse(std::string("the record does not prove the view of ") + operand.name);
    if (dimensions->size() != 2)
      return refuse(std::string("the record's view of ") + operand.name + " has " +
                    std::to_string(dimensions->size()) + " dimensions");
    for (unsigned d = 0; d < 2; ++d) {
      unsigned role = layout.roleDim[0] == (int)d ? 0 : 1;
      if (layout.roleDim[role] != (int)d)
        return refuse(std::string("the lowering reads no role in dimension ") + std::to_string(d) + " of " +
                      operand.name);
      const json::Object *dimension = (*dimensions)[d].getAsObject();
      if (!dimension)
        return refuse("the record is incomplete: a dimension of a view is no object");
      std::string at = std::string("gemm.views.") + operand.name + ".dimensions[]";
      std::optional<bool> affine = fields.boolean(*dimension, "affine", at);
      std::optional<int64_t> constant = fields.integer(*dimension, "constant", at);
      const json::Array *terms = fields.array(*dimension, "levels", at);
      const json::Array *parameters = fields.array(*dimension, "parameters", at);
      if (!affine || !constant || !terms || !parameters)
        return refuse("");
      const json::Object *term = terms->size() == 1 ? (*terms)[0].getAsObject() : nullptr;
      std::optional<int64_t> level = term ? term->getInteger("level") : std::nullopt;
      std::optional<int64_t> coefficient = term ? term->getInteger("coefficient") : std::nullopt;
      const OperandCoordinateView &mine = operand.view[role];
      if (!*affine || !parameters->empty() || !level || !coefficient ||
          *level != (int64_t)dense->sourceDim[operand.roles[role]] || *coefficient != mine.scale ||
          *constant != mine.offset)
        return refuse(std::string("dimension ") + std::to_string(d) + " of the record's view of " + operand.name +
                      " is not the lowering's: " + std::to_string(mine.scale) + " * loop " +
                      std::to_string(dense->sourceDim[operand.roles[role]]) + " + " + std::to_string(mine.offset));
    }
  }

  // The element type and the update, with the flags of both operations.
  auto store = cast<affine::AffineStoreOp>(matmul.outputStore);
  auto add = store.getValue().getDefiningOp<arith::AddFOp>();
  arith::MulFOp mul;
  bool accumulatorFirst = false;
  if (add) {
    mul = add.getRhs().getDefiningOp<arith::MulFOp>();
    accumulatorFirst = (bool)mul;
    if (!mul)
      mul = add.getLhs().getDefiningOp<arith::MulFOp>();
  }
  if (!add || !mul)
    return refuse("the GEMM's update is not one arith.addf of an arith.mulf");
  const json::Object *accumulation = fields.object(*gemm, "accumulation", "gemm");
  if (!accumulation)
    return refuse("");
  std::optional<StringRef> element = fields.string(*accumulation, "element", "gemm.accumulation");
  std::optional<StringRef> reduction = fields.string(*accumulation, "reduction", "gemm.accumulation");
  std::optional<StringRef> multiply = fields.string(*accumulation, "multiply", "gemm.accumulation");
  std::optional<bool> first = fields.boolean(*accumulation, "accumulator_first", "gemm.accumulation");
  std::optional<int64_t> sites = fields.integer(*accumulation, "sites", "gemm.accumulation");
  std::optional<bool> sitesAgree = fields.boolean(*accumulation, "sites_agree", "gemm.accumulation");
  const json::Array *reductionFlags = fields.array(*gemm, "reduction_flags", "gemm");
  const json::Array *multiplyFlags = fields.array(*gemm, "multiply_flags", "gemm");
  if (!element || !reduction || !multiply || !first || !sites || !sitesAgree || !reductionFlags || !multiplyFlags)
    return refuse("");
  std::string myElement = elementName(cast<MemRefType>(matmul.output.getType()).getElementType());
  if (*element != myElement)
    return refuse("the record's element type is " + *element + " and the GEMM's is " + myElement);
  if (*reduction != "fadd" || *multiply != "fmul" || *sites != 1 || !*sitesAgree)
    return refuse("the record's update is not one site of fadd over fmul");
  if (*first != accumulatorFirst)
    return refuse(std::string("the record's addition takes the accumulator ") + (*first ? "first" : "second") +
                  " and the GEMM's takes it " + (accumulatorFirst ? "first" : "second"));
  bool wellFormed = true;
  std::vector<std::string> recordReduction = namesOf(*reductionFlags, wellFormed);
  std::vector<std::string> recordMultiply = namesOf(*multiplyFlags, wellFormed);
  if (!wellFormed)
    return refuse("the record is incomplete: a flag is no string");
  std::vector<std::string> myReduction = flagNames(add.getFastmath());
  std::vector<std::string> myMultiply = flagNames(mul.getFastmath());
  if (recordReduction != myReduction || recordMultiply != myMultiply)
    return refuse("the record's flags (addition: " + joined(recordReduction) + "; multiplication: " +
                  joined(recordMultiply) + ") are not the GEMM's (" + joined(myReduction) + "; " +
                  joined(myMultiply) + ")");
  bool reassoc = llvm::is_contained(myReduction, "reassoc");
  if (reassoc != matmul.reductionMayReassociate)
    return refuse("the lowering reads the addition's reassoc flag otherwise");
  const json::Object *reassociation = fields.object(*record, "reassociation", "nest");
  if (!reassociation)
    return refuse("");
  std::optional<bool> sourceFlag = reassociation->getBoolean("source_flag_reassoc");
  if (!sourceFlag || *sourceFlag != reassoc)
    return refuse("the record's `reassociation.source_flag_reassoc` is not the GEMM's flag");

  // The separations the record rests on: the pairs the alias gate checks, or proofs.
  const json::Array *separations = fields.array(*record, "separation", "nest");
  if (!separations)
    return refuse("");
  std::vector<std::string> assumed;
  for (const json::Value &value : *separations) {
    const json::Object *separation = value.getAsObject();
    if (!separation)
      return refuse("the record is incomplete: a separation is no object");
    std::optional<bool> used = fields.boolean(*separation, "used", "separation[]");
    std::optional<StringRef> kind = fields.string(*separation, "kind", "separation[]");
    const json::Array *pair = fields.array(*separation, "roots", "separation[]");
    if (!used || !kind || !pair)
      return refuse("");
    std::optional<int64_t> a = pair->size() == 2 ? (*pair)[0].getAsInteger() : std::nullopt;
    std::optional<int64_t> b = pair->size() == 2 ? (*pair)[1].getAsInteger() : std::nullopt;
    if (!a || !b || *a < 0 || *b < 0 || *a >= (int64_t)roots->roots.size() || *b >= (int64_t)roots->roots.size())
      return refuse("the record is incomplete: a separation names no two roots of the function");
    if (!*used)
      continue;
    auto is = [&](unsigned x, unsigned y) {
      return (*a == (int64_t)x && *b == (int64_t)y) || (*a == (int64_t)y && *b == (int64_t)x);
    };
    if (*kind == "proof") {
      if (roots->roots[*a].argument && roots->roots[*b].argument)
        return refuse("the record proves two roots apart by an allocation, and both are arguments here");
      continue;
    }
    if (*kind != "assumption")
      return refuse("the record rests on a separation it does not know (" + *kind + ")");
    if (!is(rootOf[0], rootOf[2]) && !is(rootOf[1], rootOf[2]))
      return refuse("the record assumes roots " + std::to_string(*a) + " and " + std::to_string(*b) +
                    " apart, and the lowering checks only A and B against C");
    assumed.push_back("(" + std::to_string(*a) + ", " + std::to_string(*b) + ")");
  }

  out.agrees = true;
  out.summary = where + " (binding " + nest->binding->sha256.substr(0, 12) + ", index width " +
                std::to_string(indexWidth) + "): VERIFIED for " + holdsFor + "; " +
                std::to_string(dense->extent[0]) + "x" + std::to_string(dense->extent[1]) + "x" +
                std::to_string(dense->extent[2]) +
                ", the loops of I, J and K, the roots and views of A, B and C, the element type, the update and "
                "its flags agree; separations assumed: " +
                (assumed.empty() ? std::string("none") : llvm::join(assumed, ", ")) +
                ", which the lowering's alias gate checks itself";
  return out;
}

std::string nestRecordElementName(Type type) { return elementName(type); }

std::vector<std::string> nestRecordFlagNames(arith::FastMathFlags flags) { return flagNames(flags); }

} // namespace mlir::systolic
