//===- IntegerLattice.cpp - Exact integer linear algebra ------------------===//
//
// See IntegerLattice.h. Every operation is checked 64-bit arithmetic.
//
//===----------------------------------------------------------------------===//

#include "Systolic/IntegerLattice.h"

#include "llvm/ADT/DynamicAPInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CheckedArithmetic.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <utility>

using namespace mlir::systolic;
using namespace mlir::systolic::lattice;

std::optional<int64_t> lattice::checkedAbs(int64_t v) {
  if (v == std::numeric_limits<int64_t>::min())
    return std::nullopt;
  return v < 0 ? -v : v;
}

std::optional<int64_t> lattice::gcd(int64_t a, int64_t b) {
  std::optional<int64_t> x = checkedAbs(a), y = checkedAbs(b);
  if (!x || !y)
    return std::nullopt;
  return std::gcd(*x, *y);
}

namespace {

/// rows[r][a] -= q * rows[r][b] for every row; false on overflow.
bool subtractColumn(IntMatrix &rows, unsigned a, unsigned b, int64_t q) {
  for (IntVector &row : rows) {
    std::optional<int64_t> product = llvm::checkedMul(q, row[b]);
    std::optional<int64_t> difference =
        product ? llvm::checkedSub(row[a], *product) : std::nullopt;
    if (!difference)
      return false;
    row[a] = *difference;
  }
  return true;
}

void swapColumns(IntMatrix &rows, unsigned a, unsigned b) {
  for (IntVector &row : rows)
    std::swap(row[a], row[b]);
}

bool negateColumn(IntMatrix &rows, unsigned a) {
  for (IntVector &row : rows) {
    std::optional<int64_t> magnitude = checkedAbs(row[a]);
    if (!magnitude)
      return false;
    row[a] = -row[a];
  }
  return true;
}

/// rows[a] -= q * rows[b]; false on overflow.
bool subtractRow(IntMatrix &rows, unsigned a, unsigned b, int64_t q) {
  for (unsigned c = 0; c < rows[a].size(); ++c) {
    std::optional<int64_t> product = llvm::checkedMul(q, rows[b][c]);
    std::optional<int64_t> difference =
        product ? llvm::checkedSub(rows[a][c], *product) : std::nullopt;
    if (!difference)
      return false;
    rows[a][c] = *difference;
  }
  return true;
}

/// floor(a / b) for b > 0.
int64_t floorDiv(int64_t a, int64_t b) {
  int64_t q = a / b;
  return (a % b != 0 && a < 0) ? q - 1 : q;
}

/// The Hermite normal form of the lattice the rows generate (the rows are
/// linearly independent): echelon, every pivot positive, the entries above a
/// pivot in [0, pivot). Unique for the lattice.
std::optional<IntMatrix> rowHermiteNormalForm(IntMatrix rows, unsigned cols) {
  unsigned top = 0;
  for (unsigned c = 0; c < cols && top < rows.size(); ++c) {
    for (;;) {
      std::optional<unsigned> best;
      for (unsigned r = top; r < rows.size(); ++r) {
        if (rows[r][c] == 0)
          continue;
        if (!checkedAbs(rows[r][c]))
          return std::nullopt;
        if (!best || std::abs(rows[r][c]) < std::abs(rows[*best][c]))
          best = r;
      }
      if (!best)
        break;
      std::swap(rows[top], rows[*best]);
      bool reduced = true;
      for (unsigned r = top + 1; r < rows.size(); ++r) {
        if (rows[r][c] == 0)
          continue;
        if (!subtractRow(rows, r, top, rows[r][c] / rows[top][c]))
          return std::nullopt;
        if (rows[r][c] != 0)
          reduced = false;
      }
      if (reduced)
        break;
    }
    if (rows[top][c] == 0)
      continue;
    if (rows[top][c] < 0)
      for (int64_t &v : rows[top]) {
        if (!checkedAbs(v))
          return std::nullopt;
        v = -v;
      }
    for (unsigned r = 0; r < top; ++r)
      if (!subtractRow(rows, r, top, floorDiv(rows[r][c], rows[top][c])))
        return std::nullopt;
    ++top;
  }
  return rows;
}

} // namespace

std::optional<ColumnEchelon> lattice::columnEchelon(const IntMatrix &m,
                                                    unsigned cols) {
  for (const IntVector &row : m)
    if (row.size() != cols)
      return std::nullopt;

  // work = m U with U unimodular: the same column operations on both.
  IntMatrix work = m;
  IntMatrix unimodular(cols, IntVector(cols, 0));
  for (unsigned c = 0; c < cols; ++c)
    unimodular[c][c] = 1;
  auto subtract = [&](unsigned a, unsigned b, int64_t q) {
    return subtractColumn(work, a, b, q) &&
           subtractColumn(unimodular, a, b, q);
  };

  // Row by row, a Euclidean reduction across the columns not yet pivots: the
  // smallest nonzero magnitude moves to the pivot column and reduces the
  // others; it strictly decreases until the others are zero.
  unsigned pivot = 0;
  for (unsigned r = 0; r < m.size() && pivot < cols; ++r) {
    for (;;) {
      std::optional<unsigned> best;
      for (unsigned c = pivot; c < cols; ++c) {
        if (work[r][c] == 0)
          continue;
        if (!checkedAbs(work[r][c]))
          return std::nullopt;
        if (!best || std::abs(work[r][c]) < std::abs(work[r][*best]))
          best = c;
      }
      if (!best)
        break;
      swapColumns(work, pivot, *best);
      swapColumns(unimodular, pivot, *best);
      bool reduced = true;
      for (unsigned c = pivot + 1; c < cols; ++c) {
        if (work[r][c] == 0)
          continue;
        if (!subtract(c, pivot, work[r][c] / work[r][pivot]))
          return std::nullopt;
        if (work[r][c] != 0)
          reduced = false;
      }
      if (reduced)
        break;
    }
    if (work[r][pivot] == 0)
      continue;
    if (work[r][pivot] < 0 &&
        !(negateColumn(work, pivot) && negateColumn(unimodular, pivot)))
      return std::nullopt;
    ++pivot;
  }

  ColumnEchelon result;
  result.rank = pivot;
  // The columns of U whose image column is zero span the integer kernel: for
  // m x = 0 write x = U y; then E y = 0, and E's pivot columns are linearly
  // independent, so y vanishes on them.
  IntMatrix kernel;
  for (unsigned c = pivot; c < cols; ++c) {
    IntVector v(cols);
    for (unsigned r = 0; r < cols; ++r)
      v[r] = unimodular[r][c];
    kernel.push_back(std::move(v));
  }
  std::optional<IntMatrix> canonical = rowHermiteNormalForm(kernel, cols);
  if (!canonical)
    return std::nullopt;
  result.kernel = std::move(*canonical);

  // A square m of full rank has a lower-triangular E with a positive
  // diagonal (one pivot per row, in order); |det m| = |det E|.
  if (m.size() == cols) {
    if (pivot < cols) {
      result.absDeterminant = 0;
    } else {
      int64_t det = 1;
      for (unsigned i = 0; i < cols; ++i) {
        std::optional<int64_t> product = llvm::checkedMul(det, work[i][i]);
        if (!product)
          return std::nullopt;
        det = *product;
      }
      result.absDeterminant = det;
    }
  }
  return result;
}

std::optional<IntVector> lattice::primitive(llvm::ArrayRef<int64_t> v) {
  int64_t g = 0;
  for (int64_t x : v) {
    std::optional<int64_t> next = gcd(g, x);
    if (!next)
      return std::nullopt;
    g = *next;
  }
  IntVector out(v.begin(), v.end());
  if (g == 0)
    return out;
  int64_t sign = 1;
  for (int64_t x : v)
    if (x != 0) {
      sign = x < 0 ? -1 : 1;
      break;
    }
  for (int64_t &x : out)
    x = (x / g) * sign;
  return out;
}

std::optional<CommonDirection>
lattice::commonDirection(llvm::ArrayRef<IntVector> forms) {
  auto isZero = [](const IntVector &f) {
    return llvm::all_of(f, [](int64_t v) { return v == 0; });
  };
  const IntVector *first = nullptr;
  for (const IntVector &f : forms)
    if (!isZero(f)) {
      first = &f;
      break;
    }
  if (!first)
    return std::nullopt;
  std::optional<IntVector> direction = primitive(*first);
  if (!direction)
    return std::nullopt;
  unsigned lead = 0;
  while ((*direction)[lead] == 0)
    ++lead;

  CommonDirection result{*direction, {}};
  for (const IntVector &f : forms) {
    if (f.size() != direction->size())
      return std::nullopt;
    if (isZero(f)) {
      result.multipliers.push_back(0);
      continue;
    }
    if (f[lead] % (*direction)[lead] != 0)
      return std::nullopt;
    const int64_t multiplier = f[lead] / (*direction)[lead];
    for (unsigned s = 0; s < f.size(); ++s) {
      std::optional<int64_t> product =
          llvm::checkedMul(multiplier, (*direction)[s]);
      if (!product || *product != f[s])
        return std::nullopt;
    }
    result.multipliers.push_back(multiplier);
  }
  return result;
}

std::optional<int64_t>
lattice::mixedRadixExtent(llvm::ArrayRef<int64_t> coefficients,
                          llvm::ArrayRef<int64_t> extents) {
  if (coefficients.size() != extents.size() || coefficients.empty())
    return std::nullopt;
  // A digit of extent 1 is a constant (Lemma 0): it constrains nothing.
  llvm::SmallVector<std::pair<int64_t, int64_t>> digits;
  for (auto [c, n] : llvm::zip(coefficients, extents)) {
    if (n < 1)
      return std::nullopt;
    if (n == 1)
      continue;
    std::optional<int64_t> magnitude = checkedAbs(c);
    if (!magnitude || *magnitude == 0)
      return std::nullopt;
    digits.push_back({*magnitude, n});
  }
  llvm::stable_sort(digits, [](const auto &x, const auto &y) {
    return x.first < y.first;
  });
  int64_t expected = 1;
  for (auto [magnitude, n] : digits) {
    if (magnitude != expected)
      return std::nullopt;
    std::optional<int64_t> next = llvm::checkedMul(expected, n);
    if (!next)
      return std::nullopt;
    expected = *next;
  }
  return expected;
}

std::optional<BoxInjectivity>
lattice::injectiveOnBox(llvm::ArrayRef<int64_t> lamI,
                        llvm::ArrayRef<int64_t> lamJ, int64_t M, int64_t N) {
  if (lamI.size() != lamJ.size() || M < 1 || N < 1)
    return std::nullopt;
  auto isZero = [](llvm::ArrayRef<int64_t> v) {
    return llvm::all_of(v, [](int64_t x) { return x == 0; });
  };
  BoxInjectivity result;
  const bool zeroI = isZero(lamI), zeroJ = isZero(lamJ);
  // A coordinate the map ignores collides along itself.
  if (zeroI && M > 1) {
    result.collision = {1, 0};
    return result;
  }
  if (zeroJ && N > 1) {
    result.collision = {0, 1};
    return result;
  }
  // Here every ignored coordinate has extent 1, and a nonzero vector times
  // dI (or dJ) vanishes only for dI = 0.
  if (zeroI || zeroJ)
    return result;

  IntMatrix columns;
  for (unsigned d = 0; d < lamI.size(); ++d)
    columns.push_back({lamI[d], lamJ[d]});
  std::optional<ColumnEchelon> echelon = columnEchelon(columns, 2);
  if (!echelon)
    return std::nullopt;
  if (echelon->rank == 2)
    return result;

  // Rank 1: lamI = a u, lamJ = b u; a dI + b dJ = 0 has the smallest nonzero
  // solution (b/g, -a/g) and all others are its multiples.
  std::optional<CommonDirection> common =
      commonDirection({IntVector(lamI.begin(), lamI.end()),
                       IntVector(lamJ.begin(), lamJ.end())});
  if (!common)
    return std::nullopt;
  const int64_t a = common->multipliers[0], b = common->multipliers[1];
  std::optional<int64_t> g = gcd(a, b);
  if (!g || *g == 0)
    return std::nullopt;
  int64_t dI = b / *g, dJ = -(a / *g);
  if (dI < 0 || (dI == 0 && dJ < 0)) {
    dI = -dI;
    dJ = -dJ;
  }
  std::optional<int64_t> absI = checkedAbs(dI), absJ = checkedAbs(dJ);
  if (!absI || !absJ)
    return std::nullopt;
  if (*absI < M && *absJ < N)
    result.collision = {dI, dJ};
  return result;
}

// -----------------------------------------------------------------------
// Determinants, adjugates, inverses: exact in arbitrary precision
// (llvm::DynamicAPInt), converted back to 64-bit only when every value fits
// -- a wider exact intermediate representation with an explicit range check
// before 64-bit arithmetic. (presburger::IntMatrix::determinant is not used:
// its row swaps do not flip the sign, so it returns |det| for some inputs.)
// -----------------------------------------------------------------------

namespace {

using BigMatrix = llvm::SmallVector<llvm::SmallVector<llvm::DynamicAPInt>>;

std::optional<BigMatrix> bigSquare(const IntMatrix &m) {
  const unsigned n = m.size();
  if (n == 0)
    return std::nullopt;
  BigMatrix out;
  for (const IntVector &row : m) {
    if (row.size() != n)
      return std::nullopt;
    llvm::SmallVector<llvm::DynamicAPInt> r;
    for (int64_t v : row)
      r.push_back(llvm::DynamicAPInt(v));
    out.push_back(std::move(r));
  }
  return out;
}

/// The signed determinant by Bareiss' fraction-free elimination: every
/// division is exact, every intermediate value is a minor of the matrix.
llvm::DynamicAPInt bareissDeterminant(BigMatrix m) {
  const unsigned n = m.size();
  llvm::DynamicAPInt sign(1), previous(1);
  for (unsigned k = 0; k + 1 < n; ++k) {
    if (m[k][k] == 0) {
      unsigned pivot = k;
      while (pivot < n && m[pivot][k] == 0)
        ++pivot;
      if (pivot == n)
        return llvm::DynamicAPInt(0);
      std::swap(m[k], m[pivot]);
      sign = -sign;
    }
    for (unsigned i = k + 1; i < n; ++i) {
      for (unsigned j = k + 1; j < n; ++j)
        m[i][j] = (m[i][j] * m[k][k] - m[i][k] * m[k][j]) / previous;
      m[i][k] = llvm::DynamicAPInt(0);
    }
    previous = m[k][k];
  }
  return sign * m[n - 1][n - 1];
}

/// A DynamicAPInt as int64_t when it fits.
std::optional<int64_t> toInt64(const llvm::DynamicAPInt &v) {
  const llvm::DynamicAPInt hi(std::numeric_limits<int64_t>::max());
  const llvm::DynamicAPInt lo(std::numeric_limits<int64_t>::min());
  if (v > hi || v < lo)
    return std::nullopt;
  return int64_t(llvm::int64fromDynamicAPInt(v));
}

/// adj(m) in arbitrary precision: adj[r][c] = (-1)^(r + c) det(m without row
/// c and column r), so that adj m = m adj = det(m) I.
BigMatrix bigAdjugate(const BigMatrix &m) {
  const unsigned n = m.size();
  BigMatrix adj(n, llvm::SmallVector<llvm::DynamicAPInt>(n, llvm::DynamicAPInt(0)));
  if (n == 1) {
    adj[0][0] = llvm::DynamicAPInt(1);
    return adj;
  }
  for (unsigned r = 0; r < n; ++r)
    for (unsigned c = 0; c < n; ++c) {
      BigMatrix minor;
      for (unsigned i = 0; i < n; ++i) {
        if (i == c)
          continue;
        llvm::SmallVector<llvm::DynamicAPInt> row;
        for (unsigned j = 0; j < n; ++j)
          if (j != r)
            row.push_back(m[i][j]);
        minor.push_back(std::move(row));
      }
      llvm::DynamicAPInt cofactor = bareissDeterminant(minor);
      adj[r][c] = ((r + c) % 2 == 1) ? -cofactor : cofactor;
    }
  return adj;
}

std::optional<IntMatrix> toIntMatrix(const BigMatrix &m) {
  IntMatrix out;
  for (const auto &row : m) {
    IntVector r;
    for (const llvm::DynamicAPInt &v : row) {
      std::optional<int64_t> x = toInt64(v);
      if (!x)
        return std::nullopt;
      r.push_back(*x);
    }
    out.push_back(std::move(r));
  }
  return out;
}

} // namespace

std::optional<int64_t> lattice::determinant(const IntMatrix &m) {
  std::optional<BigMatrix> big = bigSquare(m);
  if (!big)
    return std::nullopt;
  return toInt64(bareissDeterminant(*big));
}

std::optional<IntMatrix> lattice::adjugate(const IntMatrix &m) {
  std::optional<BigMatrix> big = bigSquare(m);
  if (!big)
    return std::nullopt;
  return toIntMatrix(bigAdjugate(*big));
}

std::optional<IntMatrix> lattice::unimodularInverse(const IntMatrix &m) {
  std::optional<BigMatrix> big = bigSquare(m);
  if (!big)
    return std::nullopt;
  const llvm::DynamicAPInt det = bareissDeterminant(*big);
  if (det != 1 && det != -1)
    return std::nullopt;
  BigMatrix inverse = bigAdjugate(*big);
  if (det == -1)
    for (auto &row : inverse)
      for (llvm::DynamicAPInt &v : row)
        v = -v;
  return toIntMatrix(inverse);
}

std::optional<RationalRow> lattice::rowTimesInverse(llvm::ArrayRef<int64_t> r,
                                                    const IntMatrix &m) {
  std::optional<BigMatrix> big = bigSquare(m);
  if (!big || r.size() != m.size())
    return std::nullopt;
  llvm::DynamicAPInt det = bareissDeterminant(*big);
  if (det == 0)
    return std::nullopt;
  const BigMatrix adj = bigAdjugate(*big);
  const unsigned n = m.size();
  // (r adj)_c = sum_i r_i adj[i][c]; lambda = (r adj) / det, reduced, with a
  // positive denominator.
  llvm::SmallVector<llvm::DynamicAPInt> numerators(n, llvm::DynamicAPInt(0));
  for (unsigned c = 0; c < n; ++c)
    for (unsigned i = 0; i < n; ++i)
      numerators[c] += llvm::DynamicAPInt(r[i]) * adj[i][c];
  if (det < 0) {
    det = -det;
    for (llvm::DynamicAPInt &v : numerators)
      v = -v;
  }
  llvm::DynamicAPInt g = det;
  for (const llvm::DynamicAPInt &v : numerators)
    g = llvm::gcd(g, llvm::abs(v));
  if (g > 1) {
    det /= g;
    for (llvm::DynamicAPInt &v : numerators)
      v /= g;
  }
  RationalRow out;
  std::optional<int64_t> den = toInt64(det);
  if (!den)
    return std::nullopt;
  out.denominator = *den;
  for (const llvm::DynamicAPInt &v : numerators) {
    std::optional<int64_t> x = toInt64(v);
    if (!x)
      return std::nullopt;
    out.numerators.push_back(*x);
  }
  return out;
}

bool lattice::isSignedPermutation(const IntMatrix &m) {
  const unsigned n = m.size();
  if (n == 0)
    return false;
  llvm::SmallVector<bool> columnUsed(n, false);
  for (const IntVector &row : m) {
    if (row.size() != n)
      return false;
    std::optional<unsigned> nonzero;
    for (unsigned c = 0; c < n; ++c) {
      if (row[c] == 0)
        continue;
      if (nonzero || (row[c] != 1 && row[c] != -1) || columnUsed[c])
        return false;
      nonzero = c;
    }
    if (!nonzero)
      return false;
    columnUsed[*nonzero] = true;
  }
  return true;
}
