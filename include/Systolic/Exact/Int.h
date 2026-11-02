//===- Int.h - Exact integers of the project-owned solver -------*- C++ -*-===//
//
// The one header that names the solver's integer type. Everything else in
// include/Systolic/Exact and lib/Systolic/Exact uses Int and the functions
// below, so replacing the representation touches this file and Int.cpp only.
//
// This is the solver's dependency on LLVM, and it is an arithmetic dependency:
// Int is llvm::DynamicAPInt, so the solver links LLVMSupport for DynamicAPInt
// and the APInt code under it. The solver is independent of LLVM's solvers, and
// uses nothing from MLIR's Presburger library. It is not independent of LLVM.
//
// Int never overflows: a value that leaves 64 bits moves to an arbitrary
// precision representation. Its size is limited by memory, and by the
// coefficient limit in WorkLimits for anything a query computes.
//
// The functions here are total, because the underlying type is not: its gcd
// requires non-negative operands, its lcm divides by zero on lcm(0, 0), and
// its int64_t conversion is unchecked in a build without assertions.
// Division by zero returns std::nullopt.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_INT_H
#define SYSTOLIC_EXACT_INT_H

#include "llvm/ADT/DynamicAPInt.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace systolic::exact {

using Int = llvm::DynamicAPInt;

/// |value|.
Int magnitude(const Int &value);

/// The greatest common divisor of |a| and |b|. gcdOf(0, 0) is 0.
Int gcdOf(const Int &a, const Int &b);

/// The least common multiple of |a| and |b|. 0 when either is 0.
Int lcmOf(const Int &a, const Int &b);

/// The gcd of all the values. 0 for an empty vector or all zeros.
Int contentOf(const std::vector<Int> &values);

/// The largest integer not above numerator / denominator.
std::optional<Int> divFloor(const Int &numerator, const Int &denominator);

/// The smallest integer not below numerator / denominator.
std::optional<Int> divCeil(const Int &numerator, const Int &denominator);

/// numerator - denominator * divFloor(numerator, denominator). It has the sign
/// of the denominator, or is zero.
std::optional<Int> modFloor(const Int &numerator, const Int &denominator);

/// The number of bits of |value|. 0 for zero.
unsigned bitLength(const Int &value);

/// Decimal text: an optional '-', then digits with no leading zero. "0" for
/// zero. The same value always gives the same text.
std::string toDecimal(const Int &value);

/// The inverse of toDecimal, also accepting leading zeros. std::nullopt unless
/// the text is an optional '-' followed by at least one digit.
std::optional<Int> parseDecimal(std::string_view text);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_INT_H
