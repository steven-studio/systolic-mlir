#include "Systolic/MLIRPolynomialRecurrence.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <stdexcept>

using namespace systolic::polynomial;
using namespace mlir;

namespace {
unsigned checks = 0, accepted = 0, rejected = 0, values = 0;

void check(bool condition, const std::string &message) {
  ++checks;
  if (!condition) throw std::runtime_error(message);
}

std::string snapshot(Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream, OpPrintingFlags().printGenericOpForm().enableDebugInfo());
  return text;
}

struct Expected {
  bool proven;
  const char *initial;
  int increment;
};

// Independent, source-width expectations. In particular, 255:i8 and true:i1
// have their sign bit set even though their unsigned values are positive.
const std::map<std::string, Expected> expected = {
    {"plain_negative", {true, "255", 1}},
    {"nneg_zero", {true, "0", 1}},
    {"nneg_positive", {true, "7", 1}},
    {"nneg_signed_max", {true, "127", 1}},
    {"nneg_signed_min", {false, "", 0}},
    {"nneg_minus_one", {false, "", 0}},
    {"nneg_unsigned_max", {false, "", 0}},
    {"nneg_i1_zero", {true, "0", 1}},
    {"nneg_i1_one", {false, "", 0}},
    {"nneg_wide_max", {true, "18446744073709551615", 1}},
    {"nneg_wide_min", {false, "", 0}},
    {"plain_backedge", {true, "0", 255}},
    {"nneg_backedge_positive", {true, "0", 127}},
    {"nneg_backedge_negative", {false, "", 0}},
    {"nneg_chain_negative", {false, "", 0}},
    {"nneg_truncated_sign", {false, "", 0}},
    {"nneg_extui_chain_nonnegative", {true, "255", 1}},
    {"nneg_extsi_chain_negative", {false, "", 0}},
    {"nneg_dynamic_initial", {false, "", 0}},
    {"nneg_dynamic_backedge", {false, "", 0}},
    {"nneg_dead", {false, "", 0}},
    {"nneg_canceled_backedge", {false, "", 0}},
};

void checkUnknown(const cfg::Result &result, const std::string &name) {
  ++rejected;
  check(result.status == Status::Unknown, name + ": must reject poison or unproven cast");
  check(result.solution.status == Status::Unknown, name + ": no partial proof status");
  check(result.states.empty() && result.solution.integerLifts.empty() && !result.header,
        name + ": no fabricated states, header, or polynomial forms");
  check(!result.reason.empty(), name + ": explicit rejection reason");
  if (name.find("dynamic") == std::string::npos)
    check(result.reason.find("nneg") != std::string::npos, name + ": identify violated nneg");
  check(cfg::formatResult(result).find("binomial_coefficients") == std::string::npos,
        name + ": UNKNOWN report exposes no forms");
  bool cannotEvaluate = false;
  try { (void)result.solution.valueAt("s1", 0); }
  catch (const std::logic_error &) { cannotEvaluate = true; }
  check(cannotEvaluate, name + ": UNKNOWN cannot be evaluated");
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::invalid_argument("expected extui-nneg fixture path");
    DialectRegistry registry;
    registry.insert<arith::ArithDialect, cf::ControlFlowDialect, func::FuncDialect>();
    MLIRContext context(registry);
    auto module = parseSourceFile<ModuleOp>(argv[1], &context);
    check(bool(module), "fixture must parse and verify");
    check(succeeded(verify(*module)), "all fixtures must be valid MLIR, including poison cases");
    std::string original = snapshot(module.get());
    unsigned functions = 0;
    for (auto function : module->getOps<func::FuncOp>()) {
      ++functions;
      const auto name = function.getSymName().str();
      const auto &spec = expected.at(name);
      for (auto mode : {IntegerMode::Modular, IntegerMode::SignedNoOverflow, IntegerMode::Mathematical}) {
        cfg::Options options;
        options.mode = mode;
        options.maxIterations = 10;
        const auto result = cfg::analyze(function, options);
        check(cfg::formatResult(result) == cfg::formatResult(cfg::analyze(function, options)),
              name + ": deterministic analysis");
        if (!spec.proven) {
          checkUnknown(result, name);
          if (mode != IntegerMode::Modular) {
            options.noOverflowAssumption = "all original arithmetic fits signed range";
            checkUnknown(cfg::analyze(function, options), name);
          }
          continue;
        }
        ++accepted;
        check(result.status == Status::Proven, name + ": provably valid cast accepted");
        check(result.solution.status == Status::Proven && result.states.size() == 2,
              name + ": complete proven result");
        const Integer initial(spec.initial);
        const auto &sum = result.solution.integerLifts.at("s1");
        check(result.states[1].initial == Rational(initial), name + ": independent initial value");
        check(sum.evaluate(0) == Rational(initial), name + ": base identity");
        check(sum.difference() == Polynomial::constant(Rational(spec.increment)),
              name + ": exact finite-difference identity");
        // Direct arbitrary-precision updates, without deriving a polynomial.
        Integer direct = initial;
        for (int t = 0; t <= 10; ++t) {
          check(result.solution.valueAt("s0", t) == t, name + ": direct induction value");
          check(result.solution.valueAt("s1", t) == direct, name + ": direct accumulation value");
          values += 2;
          direct += spec.increment;
        }
      }
    }
    check(functions == 22 && functions == expected.size(), "all focused fixtures exercised");
    check(snapshot(module.get()) == original, "analysis leaves all fixture IR unchanged");
    std::cout << "PASS: " << checks << " extui-nneg checks; " << functions
              << " verified functions; " << accepted << " accepted mode cases; "
              << rejected << " UNKNOWN cases including assumptions; " << values
              << " independent direct-update comparisons\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
    return 1;
  }
}
