#include "GeneralizedGemm/Z3Backend.h"

#include <z3++.h>

#include <exception>

using namespace mlir::generalized_gemm;

namespace mlir::generalized_gemm {

SmtResponse Z3Backend::check(const std::string &script, const SmtLimits &limits,
                             bool wantModel) {
  SmtResponse response;
  std::string why;
  if (!isPlainScript(script, why)) {
    response.detail = "script refused: " + why;
    return response;
  }
  try {
    z3::context context;
    z3::solver solver(context);
    z3::params parameters(context);
    parameters.set("timeout", limits.timeoutMilliseconds);
    parameters.set("rlimit", limits.resourceLimit);
    solver.set(parameters);
    solver.from_string(script.c_str());
    context.check_error();
    switch (solver.check()) {
    case z3::unsat:
      response.answer = SmtAnswer::Unsat;
      break;
    case z3::sat:
      response.answer = SmtAnswer::Sat;
      break;
    case z3::unknown:
      response.detail = solver.reason_unknown();
      return response;
    }
    context.check_error();
    if (response.answer == SmtAnswer::Sat && wantModel) {
      z3::model model = solver.get_model();
      for (unsigned i = 0; i < model.num_consts(); ++i) {
        z3::func_decl declaration = model.get_const_decl(i);
        z3::expr value = model.get_const_interp(declaration);
        if ((value.is_bv() || value.is_int()) && value.is_numeral())
          response.model[declaration.name().str()] = value.get_decimal_string(0);
      }
    }
  } catch (const z3::exception &error) {
    response = SmtResponse();
    response.detail = std::string("solver error: ") + error.msg();
  } catch (const std::exception &error) {
    response = SmtResponse();
    response.detail = std::string("solver failure: ") + error.what();
  } catch (...) {
    response = SmtResponse();
    response.detail = "solver failure";
  }
  return response;
}

std::string Z3Backend::version() const {
  unsigned major, minor, build, revision;
  Z3_get_version(&major, &minor, &build, &revision);
  return "z3 " + std::to_string(major) + "." + std::to_string(minor) + "." +
         std::to_string(build);
}

} // namespace mlir::generalized_gemm
