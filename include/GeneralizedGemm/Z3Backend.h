//===- Z3Backend.h - SmtBackend over the Z3 library -------------*- C++ -*-===//
//
// Each check uses a fresh context, so no state links two obligations. Errors
// are reported, never fatal: without an error handler the Z3 C API prints and
// exits the process, and the C++ API this file uses installs one. A crash
// inside the library itself cannot be contained in process.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_Z3BACKEND_H
#define GENERALIZEDGEMM_Z3BACKEND_H

#include "GeneralizedGemm/SmtBackend.h"

namespace mlir::generalized_gemm {

class Z3Backend : public SmtBackend {
public:
  SmtResponse check(const std::string &script, const SmtLimits &limits,
                    bool wantModel) override;
  std::string version() const override;
};

} // namespace mlir::generalized_gemm
#endif
