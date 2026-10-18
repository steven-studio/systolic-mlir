//===- TestIntegerLattice.cpp - Unit tests of the integer lattice tools ---===//
//
// --test-integer-lattice: for every func.func that carries lattice.*
// attributes, runs the tool of IntegerLattice.h on them and prints the result
// to stderr, so that lit checks the exact answers (the repository has no C++
// unit-test framework; this is its stand-in for these functions):
//
//   lattice.matrix = [[...], ...]       rank, integer kernel (HNF), |det|
//   lattice.forms = [[...], ...]        common direction and multipliers
//   lattice.radix = [...], lattice.extents = [...]   mixed-radix extent
//   lattice.inject = [[lamI], [lamJ]], lattice.box = [M, N]  injectivity
//   lattice.square = [[...], ...]       det, adjugate, inverse if unimodular,
//                                       signed permutation?
//   lattice.row = [...] (with .square)  row * inverse as exact rationals
//
//===----------------------------------------------------------------------===//

#include "Systolic/IntegerLattice.h"
#include "Systolic/Passes.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

std::optional<lattice::IntVector> vectorAttr(Attribute attr) {
  auto array = dyn_cast_or_null<ArrayAttr>(attr);
  if (!array)
    return std::nullopt;
  lattice::IntVector out;
  for (Attribute element : array) {
    auto integer = dyn_cast<IntegerAttr>(element);
    if (!integer)
      return std::nullopt;
    out.push_back(integer.getInt());
  }
  return out;
}

std::optional<lattice::IntMatrix> matrixAttr(Attribute attr) {
  auto array = dyn_cast_or_null<ArrayAttr>(attr);
  if (!array)
    return std::nullopt;
  lattice::IntMatrix out;
  for (Attribute row : array) {
    std::optional<lattice::IntVector> v = vectorAttr(row);
    if (!v)
      return std::nullopt;
    out.push_back(*v);
  }
  return out;
}

void printVector(llvm::raw_ostream &os, llvm::ArrayRef<int64_t> v) {
  os << "[";
  llvm::interleaveComma(v, os);
  os << "]";
}

void printMatrix(llvm::raw_ostream &os, const lattice::IntMatrix &m) {
  os << "[";
  llvm::interleaveComma(m, os, [&](const lattice::IntVector &row) {
    printVector(os, row);
  });
  os << "]";
}

struct TestIntegerLatticePass
    : public PassWrapper<TestIntegerLatticePass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(TestIntegerLatticePass)

  StringRef getArgument() const final { return "test-integer-lattice"; }
  StringRef getDescription() const final {
    return "Print the results of the integer lattice tools for the lattice.* "
           "attributes of each function";
  }

  void runOnOperation() override {
    llvm::raw_ostream &os = llvm::errs();
    getOperation().walk([&](func::FuncOp func) {
      const StringRef name = func.getSymName();
      if (Attribute attr = func->getAttr("lattice.matrix")) {
        std::optional<lattice::IntMatrix> m = matrixAttr(attr);
        if (!m || m->empty()) {
          os << "@" << name << ": bad lattice.matrix\n";
        } else {
          const unsigned cols = (*m)[0].size();
          std::optional<lattice::ColumnEchelon> e =
              lattice::columnEchelon(*m, cols);
          os << "@" << name << ": ";
          if (!e) {
            os << "overflow (or ragged rows)\n";
          } else {
            os << "rank " << e->rank << ", kernel ";
            printMatrix(os, e->kernel);
            if (e->absDeterminant)
              os << ", |det| " << *e->absDeterminant;
            os << "\n";
          }
        }
      }
      if (Attribute attr = func->getAttr("lattice.forms")) {
        std::optional<lattice::IntMatrix> forms = matrixAttr(attr);
        std::optional<lattice::CommonDirection> d =
            forms ? lattice::commonDirection(*forms) : std::nullopt;
        os << "@" << name << ": ";
        if (!d) {
          os << "no common direction\n";
        } else {
          os << "direction ";
          printVector(os, d->direction);
          os << ", multipliers ";
          printVector(os, d->multipliers);
          os << "\n";
        }
      }
      if (Attribute attr = func->getAttr("lattice.radix")) {
        std::optional<lattice::IntVector> c = vectorAttr(attr);
        std::optional<lattice::IntVector> n =
            vectorAttr(func->getAttr("lattice.extents"));
        std::optional<int64_t> extent =
            c && n ? lattice::mixedRadixExtent(*c, *n) : std::nullopt;
        os << "@" << name << ": ";
        if (extent)
          os << "mixed radix, extent " << *extent << "\n";
        else
          os << "not a mixed radix\n";
      }
      if (Attribute attr = func->getAttr("lattice.square")) {
        std::optional<lattice::IntMatrix> m = matrixAttr(attr);
        os << "@" << name << ": ";
        std::optional<int64_t> det = m ? lattice::determinant(*m) : std::nullopt;
        std::optional<lattice::IntMatrix> adj =
            m ? lattice::adjugate(*m) : std::nullopt;
        if (!det || !adj) {
          os << "not square, or overflow\n";
        } else {
          os << "det " << *det << ", adjugate ";
          printMatrix(os, *adj);
          if (std::optional<lattice::IntMatrix> inv = lattice::unimodularInverse(*m)) {
            os << ", unimodular, inverse ";
            printMatrix(os, *inv);
          }
          os << (lattice::isSignedPermutation(*m) ? ", a signed permutation" : "");
          os << "\n";
          if (Attribute rowAttr = func->getAttr("lattice.row")) {
            std::optional<lattice::IntVector> r = vectorAttr(rowAttr);
            std::optional<lattice::RationalRow> q =
                r ? lattice::rowTimesInverse(*r, *m) : std::nullopt;
            os << "@" << name << ": row * inverse = ";
            if (!q) {
              os << "undefined (singular, wrong length, or overflow)\n";
            } else {
              printVector(os, q->numerators);
              os << " / " << q->denominator << "\n";
            }
          }
        }
      }
      if (Attribute attr = func->getAttr("lattice.inject")) {
        std::optional<lattice::IntMatrix> lam = matrixAttr(attr);
        std::optional<lattice::IntVector> box =
            vectorAttr(func->getAttr("lattice.box"));
        std::optional<lattice::BoxInjectivity> r;
        if (lam && lam->size() == 2 && box && box->size() == 2)
          r = lattice::injectiveOnBox((*lam)[0], (*lam)[1], (*box)[0],
                                      (*box)[1]);
        os << "@" << name << ": ";
        if (!r)
          os << "bad input or overflow\n";
        else if (r->injective())
          os << "injective\n";
        else
          os << "collision (" << r->collision->first << ", "
             << r->collision->second << ")\n";
      }
    });
  }
};

} // namespace

void mlir::systolic::registerTestIntegerLatticePass() {
  PassRegistration<TestIntegerLatticePass>();
}
