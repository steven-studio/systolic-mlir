//===- DeltaRational.cpp - Rationals with an infinitesimal ----------------===//

#include "Systolic/Exact/DeltaRational.h"

namespace systolic::exact {

DeltaRational DeltaRational::operator+(const DeltaRational &other) const {
  return DeltaRational(realPart + other.realPart, deltaPart + other.deltaPart);
}

DeltaRational DeltaRational::operator-(const DeltaRational &other) const {
  return DeltaRational(realPart - other.realPart, deltaPart - other.deltaPart);
}

DeltaRational DeltaRational::operator*(const Rational &factor) const {
  return DeltaRational(realPart * factor, deltaPart * factor);
}

bool DeltaRational::operator==(const DeltaRational &other) const {
  return realPart == other.realPart && deltaPart == other.deltaPart;
}

bool DeltaRational::operator<(const DeltaRational &other) const {
  if (realPart != other.realPart)
    return realPart < other.realPart;
  return deltaPart < other.deltaPart;
}

Rational DeltaRational::at(const Rational &epsilon) const {
  return realPart + deltaPart * epsilon;
}

std::string DeltaRational::toString() const {
  if (deltaPart.sign() == 0)
    return realPart.toString();
  return realPart.toString() + "+" + deltaPart.toString() + "*d";
}

} // namespace systolic::exact
