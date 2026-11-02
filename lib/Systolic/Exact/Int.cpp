//===- Int.cpp - Exact integers of the project-owned solver ---------------===//

#include "Systolic/Exact/Int.h"

#include <algorithm>
#include <cstdint>

namespace systolic::exact {

Int magnitude(const Int &value) { return value < Int(0) ? -value : value; }

Int gcdOf(const Int &a, const Int &b) {
  // Magnitudes first: INT64_MIN has no 64-bit magnitude, and the fast path
  // would otherwise hand it to std::gcd.
  return llvm::gcd(magnitude(a), magnitude(b));
}

Int lcmOf(const Int &a, const Int &b) {
  if (a == Int(0) || b == Int(0))
    return Int(0);
  return magnitude(a) / gcdOf(a, b) * magnitude(b);
}

Int contentOf(const std::vector<Int> &values) {
  Int content(0);
  for (const Int &value : values)
    content = gcdOf(content, value);
  return content;
}

std::optional<Int> divFloor(const Int &numerator, const Int &denominator) {
  if (denominator == Int(0))
    return std::nullopt;
  return llvm::floorDiv(numerator, denominator);
}

std::optional<Int> divCeil(const Int &numerator, const Int &denominator) {
  if (denominator == Int(0))
    return std::nullopt;
  return llvm::ceilDiv(numerator, denominator);
}

std::optional<Int> modFloor(const Int &numerator, const Int &denominator) {
  const std::optional<Int> quotient = divFloor(numerator, denominator);
  if (!quotient)
    return std::nullopt;
  return numerator - denominator * *quotient;
}

unsigned bitLength(const Int &value) {
  Int rest = magnitude(value);
  unsigned bits = 0;
  const Int word(std::int64_t(1) << 32);
  while (rest >= word) {
    rest = rest / word;
    bits += 32;
  }
  // rest < 2^32 here, so the conversion is exact.
  std::uint64_t low = static_cast<std::uint64_t>(static_cast<std::int64_t>(rest));
  while (low != 0) {
    low >>= 1;
    ++bits;
  }
  return bits;
}

std::string toDecimal(const Int &value) {
  if (value == Int(0))
    return "0";
  // Base 10^18 digits, so each one fits an int64_t and the arithmetic stays in
  // Int; the text does not depend on how the value is stored.
  const Int base(std::int64_t(1000000000000000000));
  std::vector<std::int64_t> digits;
  for (Int rest = magnitude(value); rest != Int(0); rest = rest / base)
    digits.push_back(static_cast<std::int64_t>(rest % base));
  std::string text = value < Int(0) ? "-" : "";
  text += std::to_string(digits.back());
  for (std::size_t i = digits.size() - 1; i-- > 0;) {
    const std::string part = std::to_string(digits[i]);
    text.append(18 - part.size(), '0');
    text += part;
  }
  return text;
}

std::optional<Int> parseDecimal(std::string_view text) {
  const bool negative = !text.empty() && text.front() == '-';
  if (negative)
    text.remove_prefix(1);
  if (text.empty())
    return std::nullopt;
  Int value(0);
  for (char c : text) {
    if (c < '0' || c > '9')
      return std::nullopt;
    value = value * Int(10) + Int(c - '0');
  }
  return negative ? Int(-value) : value;
}

} // namespace systolic::exact
