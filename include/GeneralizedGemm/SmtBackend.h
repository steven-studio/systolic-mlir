//===- SmtBackend.h - The solver boundary -----------------------*- C++ -*-===//
//
// Scripts go in as text and one of three answers comes back. Unknown covers
// every failure: a timeout, a resource limit, a parse error, a refused script,
// a solver exception. Nothing turns Unknown into a proof or a refutation.
//
// The core termination libraries depend on this interface only. They link no
// solver: Z3Backend is a separate library.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SMTBACKEND_H
#define GENERALIZEDGEMM_SMTBACKEND_H

#include <map>
#include <string>

namespace mlir::generalized_gemm {

enum class SmtAnswer { Sat, Unsat, Unknown };
const char *stringifySmtAnswer(SmtAnswer answer);

struct SmtLimits {
  unsigned timeoutMilliseconds = 10000;
  unsigned resourceLimit = 20000000; // Deterministic, unlike the timeout.
};

struct SmtResponse {
  SmtAnswer answer = SmtAnswer::Unknown;
  std::string detail;
  // Only when asked for and Sat. Constants the solver assigned, in decimal.
  std::map<std::string, std::string> model;
};

class SmtBackend {
public:
  virtual ~SmtBackend() = default;
  virtual SmtResponse check(const std::string &script, const SmtLimits &limits,
                            bool wantModel) = 0;
  virtual std::string version() const = 0;
};

/// Whether `script` holds only declare-const, define-fun and assert commands,
/// balanced, then exactly one check-sat at the end. Some solvers warn about an
/// unknown command and carry on, which would silently change the question.
bool isPlainScript(const std::string &script, std::string &why);

} // namespace mlir::generalized_gemm
#endif
