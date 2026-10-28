#include "Systolic/PolynomialSummation.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace systolic::polynomial;

namespace {
Rational readRational(std::istream &input) {
  std::string token;
  if (!(input >> token))
    throw std::invalid_argument("missing rational");
  auto slash = token.find('/');
  if (slash == std::string::npos)
    return Rational(Integer(token));
  return Rational(Integer(token.substr(0, slash)),
                  Integer(token.substr(slash + 1)));
}
Integer readInteger(std::istream &input) {
  Rational value = readRational(input);
  if (value.denominator() != 1)
    throw std::invalid_argument("expected integer");
  return value.numerator();
}
unsigned readSize(std::istream &input) {
  Integer value = readInteger(input);
  if (value < 0 || value > MaxDegree + 1)
    throw std::invalid_argument("size outside P0 limit");
  return value.convert_to<unsigned>();
}
void printCoefficients(const std::vector<Rational> &values) {
  if (values.empty())
    std::cout << "0/1";
  for (unsigned i = 0; i < values.size(); ++i) {
    if (i)
      std::cout << ',';
    std::cout << values[i];
  }
}
void checkIdentity(const Polynomial &sum, const Polynomial &summand,
                   const Rational &initial) {
  if (sum.difference() != summand || sum.shift(Integer(1)) - sum != summand ||
      sum.evaluate(Integer(0)) != initial)
    throw std::logic_error("exact identity / initial condition failed");
}
void run(std::istream &input) {
  std::string command;
  input >> command;
  if (command == "binom") {
    Integer n = readInteger(input);
    unsigned k = readSize(input);
    std::cout << "binom\t" << binomial(n, k);
  } else if (command == "sum") {
    unsigned size = readSize(input);
    std::vector<Rational> coefficients;
    for (unsigned i = 0; i < size; ++i)
      coefficients.push_back(readRational(input));
    Rational start = readRational(input), step = readRational(input);
    Rational initial = readRational(input);
    Integer n = readInteger(input);
    if (n < 0)
      throw std::invalid_argument("negative iteration count");
    Polynomial summand = Polynomial::fromMonomial(coefficients)
                             .composeAffine(start, step);
    Polynomial sum = summand.integrate(initial);
    checkIdentity(sum, summand, initial);
    std::cout << "sum\t" << sum.evaluate(n) << '\t';
    printCoefficients(sum.coefficients());
    std::cout << '\t';
    printCoefficients(sum.toMonomial());
  } else if (command == "tri") {
    std::string mode;
    input >> mode;
    unsigned width = readInteger(input).convert_to<unsigned>();
    Integer n = readInteger(input);
    Rational x0 = readRational(input), step = readRational(input);
    Rational y0 = readRational(input), z0 = readRational(input);
    unsigned size = readSize(input);
    std::vector<Rational> coefficients;
    for (unsigned i = 0; i < size; ++i)
      coefficients.push_back(readRational(input));
    Expression x = Expression::variable("x"), y = Expression::variable("y");
    Expression polynomial;
    for (auto it = coefficients.rbegin(); it != coefficients.rend(); ++it)
      polynomial = polynomial * x + Expression(*it);
    AnalysisOptions options;
    if (mode == "math")
      options.mode = IntegerMode::Mathematical;
    else if (mode == "mod")
      options.mode = IntegerMode::Modular;
    else if (mode == "signed")
      options.mode = IntegerMode::SignedNoOverflow;
    else
      throw std::invalid_argument("unknown mode");
    options.bitWidth = width;
    options.maxIterations = n;
    // Intentionally out of dependency order. All increments read OLD values.
    auto result = analyze({{"z", z0, y}, {"y", y0, polynomial},
                           {"x", x0, Expression(step)}}, options);
    if (result.status == Status::Unknown) {
      std::cout << "unknown\t" << result.reason;
    } else {
      const auto &forms = result.integerLifts;
      checkIdentity(forms.at("x"), Polynomial::constant(step), x0);
      checkIdentity(forms.at("y"),
                    Polynomial::fromMonomial(coefficients).composeAffine(x0, step), y0);
      checkIdentity(forms.at("z"), forms.at("y"), z0);
      std::cout << "tri\t" << result.valueAt("x", n) << '\t'
                << result.valueAt("y", n) << '\t' << result.valueAt("z", n);
    }
  } else {
    throw std::invalid_argument("unknown command");
  }
  std::string extra;
  if (input >> extra)
    throw std::invalid_argument("trailing input");
  std::cout << '\n';
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--demo") {
      Polynomial index({Rational(0), Rational(1)});
      for (unsigned k = 1; k <= 5; ++k) {
        Polynomial sum = index.pow(k).sumFrom(Integer(1), true);
        std::cout << "sum(i^" << k << ", i=1..n): binomial=[";
        printCoefficients(sum.coefficients());
        std::cout << "] monomial=[";
        printCoefficients(sum.toMonomial());
        std::cout << "]\n";
      }
      return 0;
    }
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.empty())
        continue;
      std::istringstream input(line);
      run(input);
    }
  } catch (const std::exception &error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
  }
}
