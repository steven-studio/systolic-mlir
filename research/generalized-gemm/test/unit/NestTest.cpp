// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md: the copy of a nest, what it keeps and what it
// refuses, the hand-over of separation from the function, and the record read off the
// existing analyses. test/fuzz/nest_oracle.py recomputes the records' content with no
// library code.
#include "GeneralizedGemm/NestRecord.h"
#include "GeneralizedGemm/SiteSets.h"
#include "GeneralizedGemm/Z3Backend.h"
#include "Systolic/NestBinding.h"

#include "TestSupport.h"

#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APFloat.h"

#include <chrono>
#include <map>

using namespace mlir::generalized_gemm;

static mlir::MLIRContext *context;
static Z3Backend z3Backend;

struct Module {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  explicit Module(const std::string &text) : module(parse(*context, text)) {
    if (!module) {
      fprintf(stderr, "cannot parse:\n%s\n", text.c_str());
      exit(2);
    }
  }
  mlir::func::FuncOp function(const std::string &name) {
    return module->lookupSymbol<mlir::func::FuncOp>(name);
  }
  std::string text() {
    std::string out;
    llvm::raw_string_ostream stream(out);
    module->print(stream, mlir::OpPrintingFlags().printGenericOpForm().enableDebugInfo());
    return out;
  }
};

/// C[i + ci, j + cj] += A[i, k] * B[k, j] over a box, as three affine loops. `tag` keeps
/// the names of two nests of one function apart.
static std::string gemm(const std::string &tag, const std::string &a, const std::string &b,
                        const std::string &c, unsigned m, unsigned n, unsigned k,
                        const std::string &element = "i32", const std::string &flag = "") {
  bool isFloat = element[0] == 'f';
  std::string type = "memref<4x4x" + element + ">";
  std::string i = "%i" + tag, j = "%j" + tag, l = "%k" + tag;
  return "  affine.for " + i + " = 0 to " + std::to_string(m) + " {\n" +
         "    affine.for " + j + " = 0 to " + std::to_string(n) + " {\n" +
         "      affine.for " + l + " = 0 to " + std::to_string(k) + " {\n" +
         "        %x" + tag + " = affine.load " + a + "[" + i + ", " + l + "] : " + type + "\n" +
         "        %y" + tag + " = affine.load " + b + "[" + l + ", " + j + "] : " + type + "\n" +
         "        %z" + tag + " = affine.load " + c + "[" + i + ", " + j + "] : " + type + "\n" +
         "        %m" + tag + " = arith.mul" + (isFloat ? "f" : "i") + " %x" + tag + ", %y" + tag + " : " + element + "\n" +
         "        %s" + tag + " = arith.add" + (isFloat ? "f" : "i") + " %z" + tag + ", %m" + tag + " " + flag + " : " + element + "\n" +
         "        affine.store %s" + tag + ", " + c + "[" + i + ", " + j + "] : " + type + "\n" +
         "      }\n    }\n  }\n";
}

static std::string arguments(std::initializer_list<const char *> names, const std::string &element = "i32") {
  std::string out;
  for (const char *name : names)
    out += std::string(out.empty() ? "" : ", ") + name + ": memref<4x4x" + element + ">";
  return out;
}

static unsigned countOps(mlir::Operation *root, llvm::StringRef name) {
  unsigned count = 0;
  root->walk([&](mlir::Operation *op) { count += op->getName().getStringRef() == name; });
  return count;
}

static const char *kPrograms = R"mlir(
func.func @inputs(%a: memref<8xi32>, %c: memref<8xi32>, %p: memref<2xindex>, %n: index, %w: i32) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %m = arith.addi %n, %c1 : index
  %t = memref.load %p[%c0] : memref<2xindex>
  memref.store %c1, %p[%c0] : memref<2xindex>
  scf.for %i = %t to %m step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = arith.muli %x, %w : i32
    %z = memref.load %c[%i] : memref<8xi32>
    %s = arith.addi %z, %y : i32
    memref.store %s, %c[%n] : memref<8xi32>
  }
  return
}
func.func @carried(%a: memref<4xi32>, %b: memref<4xi32>) -> i32 {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  %sum = scf.for %k = %c0 to %c4 step %c1 iter_args(%acc = %zero) -> (i32) {
    %x = memref.load %a[%k] : memref<4xi32>
    %y = memref.load %b[%k] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %acc, %m : i32
    scf.yield %s : i32
  }
  return %sum : i32
}
func.func @unused(%a: memref<4xi32>, %b: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  %sum = scf.for %k = %c0 to %c4 step %c1 iter_args(%acc = %zero) -> (i32) {
    %x = memref.load %a[%k] : memref<4xi32>
    memref.store %x, %b[%k] : memref<4xi32>
    scf.yield %x : i32
  }
  return
}
func.func private @callee(%a: memref<4xi32>)
func.func @calls(%a: memref<4xi32>, %b: memref<4xi32>) {
  affine.for %i = 0 to 4 {
    func.call @callee(%a) : (memref<4xi32>) -> ()
  }
  affine.for %j = 0 to 4 {
    %x = affine.load %a[%j] : memref<4xi32>
    affine.store %x, %b[%j] : memref<4xi32>
  }
  return
}
func.func @views(%a: memref<4xi32>, %b: memref<4xi32>) {
  %v = memref.cast %a : memref<4xi32> to memref<?xi32>
  affine.for %i = 0 to 4 {
    %x = affine.load %v[%i] : memref<?xi32>
    affine.store %x, %b[%i] : memref<4xi32>
  }
  return
}
func.func @parallel(%a: memref<4xi32>, %b: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  scf.parallel (%i) = (%c0) to (%c4) step (%c1) {
    %x = memref.load %a[%i] : memref<4xi32>
    memref.store %x, %b[%i] : memref<4xi32>
  }
  affine.for %j = 0 to 4 {
    affine.parallel (%k) = (0) to (4) {
      %y = affine.load %b[%k] : memref<4xi32>
      affine.store %y, %a[%k] : memref<4xi32>
    }
  }
  return
}
func.func @vectors(%b: memref<4xi32>) {
  %v = "test.make"() : () -> vector<2xi32>
  affine.for %i = 0 to 4 {
    "test.use"(%v) : (vector<2xi32>) -> ()
  }
  return
}
func.func @branches(%a: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  cf.br ^head(%c0 : index)
^head(%i: index):
  %more = arith.cmpi ult, %i, %n : index
  cf.cond_br %more, ^body, ^exit
^body:
  %c1 = arith.constant 1 : index
  %next = arith.addi %i, %c1 : index
  cf.br ^head(%next : index)
^exit:
  return
}
func.func @regions(%p: i1, %a: memref<4xi32>) {
  scf.execute_region {
    cf.cond_br %p, ^one, ^two
  ^one:
    cf.br ^two
  ^two:
    scf.yield
  }
  affine.for %i = 0 to 4 {
    %x = affine.load %a[%i] : memref<4xi32>
    affine.store %x, %a[%i] : memref<4xi32>
  }
  return
}
func.func @noloops(%a: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %x = memref.load %a[%c0] : memref<4xi32>
  memref.store %x, %a[%c0] : memref<4xi32>
  return
}
func.func @conditional(%a: memref<4xi32>, %b: memref<4xi32>, %p: i1) {
  %c0 = arith.constant 0 : index
  scf.if %p {
    affine.for %i = 0 to 4 {
      %x = affine.load %a[%i] : memref<4xi32>
      affine.store %x, %b[%i] : memref<4xi32>
    }
  } else {
    %y = memref.load %a[%c0] : memref<4xi32>
    memref.store %y, %b[%c0] : memref<4xi32>
  }
  affine.for %j = 0 to 4 {
    %z = affine.load %b[%j] : memref<4xi32>
    affine.store %z, %a[%j] : memref<4xi32>
  }
  return
}
func.func @allocs(%a: memref<2xi32>, %n: index, %b: memref<3xi32>, %p: i1) {
  %c0 = arith.constant 0 : index
  %t = memref.alloc() : memref<5xi32>
  affine.for %i = 0 to 2 {
    %x = affine.load %a[%i] : memref<2xi32>
    affine.store %x, %t[%i] : memref<5xi32>
  }
  %u = memref.alloca() : memref<7xi32>
  affine.for %j = 0 to 3 {
    %y = affine.load %b[%j] : memref<3xi32>
    affine.store %y, %u[%j] : memref<7xi32>
  }
  scf.if %p {
    %w = memref.alloc() : memref<9xi32>
    %v = memref.load %t[%c0] : memref<5xi32>
    memref.store %v, %w[%c0] : memref<9xi32>
  }
  %q = memref.alloc() : memref<11xi32>
  affine.for %k = 0 to 2 {
    %z = affine.load %u[%k] : memref<7xi32>
    affine.store %z, %q[%k] : memref<11xi32>
  }
  return
}
memref.global "private" @table : memref<4xi32> = dense<[1, 2, 3, 4]>
func.func @globals(%b: memref<4xi32>) {
  affine.for %i = 0 to 4 {
    %g = memref.get_global @table : memref<4xi32>
    %x = affine.load %g[%i] : memref<4xi32>
    affine.store %x, %b[%i] : memref<4xi32>
  }
  affine.for %j = 0 to 4 {
    %y = affine.load %b[%j] : memref<4xi32>
    affine.store %y, %b[%j] : memref<4xi32>
  }
  return
}
func.func @symbols(%b: memref<4xi32>) {
  affine.for %i = 0 to 4 {
    %f = func.constant @callee : (memref<4xi32>) -> ()
    %x = affine.load %b[%i] : memref<4xi32>
    affine.store %x, %b[%i] : memref<4xi32>
  }
  return
}
func.func @threaded(%a: memref<4xi32>, %b: memref<4xi32>) {
  %stop = arith.constant false
  %zero = arith.constant 0 : index
  %res = scf.while (%m = %a) : (memref<4xi32>) -> memref<4xi32> {
    scf.condition(%stop) %m : memref<4xi32>
  } do {
  ^bb0(%m2: memref<4xi32>):
    %x = memref.load %b[%zero] : memref<4xi32>
    memref.store %x, %m2[%zero] : memref<4xi32>
    scf.yield %m2 : memref<4xi32>
  }
  affine.for %i = 0 to 4 {
    %y = affine.load %a[%i] : memref<4xi32>
    affine.store %y, %b[%i] : memref<4xi32>
  }
  return
}
func.func @attrs(%a: memref<4xi32> {llvm.noalias}, %b: memref<4xi32>) attributes {custom.flag} {
  %t = memref.alloc() : memref<4xi32>
  affine.for %i = 0 to 4 {
    %x = affine.load %a[%i] : memref<4xi32>
    %y = affine.load %b[%i] : memref<4xi32>
    %z = arith.addi %x, %y : i32
    affine.store %z, %t[%i] : memref<4xi32>
  }
  return
}
)mlir";

static const OutlinedNest &nestOf(const FunctionOutline &outline, unsigned ordinal) {
  static OutlinedNest none;
  return ordinal < outline.nests.size() ? outline.nests[ordinal] : none;
}

static bool refused(const FunctionOutline &outline, unsigned ordinal, const char *words) {
  const OutlinedNest &nest = nestOf(outline, ordinal);
  return !nest.module && nest.unsupported.find(words) != std::string::npos;
}

static void testOutline() {
  // Two nests on one output. Each copy has the nest's own roots, in the function's
  // order, and the function is as it was.
  std::string shared = "func.func @shared(" + arguments({"%a", "%b", "%c", "%d"}) + ") {\n" +
                       gemm("0", "%a", "%b", "%c", 2, 3, 2) + gemm("1", "%a", "%d", "%c", 3, 2, 4) + "  return\n}\n";
  Module m(shared);
  std::string before = m.text();
  Authorizations some{"roots 0 and 2 are separate", "roots 2 and 3 are separate"};
  FunctionOutline outline = outlineNests(m.function("shared"), some);
  CHECK(m.text() == before);
  CHECK(outline.unsupported.empty());
  CHECK(outline.roots.size() == 4);
  CHECK(outline.nests.size() == 2);
  if (outline.nests.size() == 2) {
    const OutlinedNest &first = outline.nests[0], &second = outline.nests[1];
    CHECK(first.ordinal == 0 && second.ordinal == 1);
    CHECK(first.statement == 0 && second.statement == 1);
    CHECK(first.conditions == 0 && second.conditions == 0);
    CHECK(first.loop == "affine.for" && second.loop == "affine.for");
    CHECK(first.function == "shared" && first.outlinedName == "shared__nest0");
    CHECK(second.outlinedName == "shared__nest1");
    CHECK(first.unsupported.empty() && second.unsupported.empty());
    CHECK(first.roots.size() == 3 && second.roots.size() == 3);
    if (first.roots.size() == 3 && second.roots.size() == 3) {
      CHECK(first.roots[0].root == 0 && first.roots[1].root == 1 && first.roots[2].root == 2);
      CHECK(second.roots[0].root == 0 && second.roots[1].root == 2 && second.roots[2].root == 3);
      for (unsigned r = 0; r < 3; ++r) {
        CHECK(first.roots[r].outlined == r);
        CHECK(second.roots[r].outlined == r);
      }
      CHECK(first.roots[0].access == NestReads && first.roots[1].access == NestReads);
      CHECK(first.roots[2].access == (NestReads | NestWrites));
      CHECK(second.roots[0].access == NestReads);
      CHECK(second.roots[1].access == (NestReads | NestWrites));
      CHECK(second.roots[2].access == NestReads);
    }
    CHECK(first.scalars.empty() && second.scalars.empty());
    // The copy is one function in a module of its own, with the nest's operations.
    for (const OutlinedNest *nest : {&first, &second}) {
      CHECK((bool)nest->module);
      if (!nest->module)
        continue;
      mlir::func::FuncOp copy = nest->outlined();
      CHECK((bool)copy);
      CHECK(succeeded(mlir::verify(nest->module.get())));
      CHECK(std::distance(nest->module.get().getBody()->begin(), nest->module.get().getBody()->end()) == 1);
      CHECK(copy.getNumArguments() == 3);
      CHECK(copy.getNumResults() == 0);
      CHECK(countOps(copy, "affine.for") == 3);
      CHECK(countOps(copy, "affine.load") == 3);
      CHECK(countOps(copy, "affine.store") == 1);
      CHECK(countOps(copy, "arith.muli") == 1 && countOps(copy, "arith.addi") == 1);
      CHECK(countOps(copy, "func.return") == 1);
    }
    // The second nest's pairs, by the function's names, and by the copy's for the hand-over.
    CHECK(second.separations.size() == 3);
    if (second.separations.size() == 3) {
      const NestSeparation &ac = second.separations[0], &ad = second.separations[1], &cd = second.separations[2];
      CHECK(ac.first == 0 && ac.second == 2 && ac.name == "roots 0 and 2 are separate");
      CHECK(ac.apart && !ac.proof && ac.basis == "authorized");
      CHECK(ac.outlinedName == "roots 0 and 1 are separate");
      CHECK(ad.first == 0 && ad.second == 3 && ad.name == "roots 0 and 3 are separate");
      CHECK(!ad.apart && !ad.proof && ad.basis.empty());
      CHECK(ad.outlinedName == "roots 0 and 2 are separate");
      CHECK(cd.first == 2 && cd.second == 3 && cd.apart && !cd.proof);
      CHECK(cd.outlinedName == "roots 1 and 2 are separate");
    }
    // The first nest's pair (0, 2) is authorized; (0, 1) and (1, 2) are not.
    CHECK(first.separations.size() == 3);
    if (first.separations.size() == 3) {
      CHECK(!first.separations[0].apart && first.separations[1].apart && !first.separations[2].apart);
      CHECK(first.separations[1].outlinedName == "roots 0 and 2 are separate");
    }
  }
  // The order, and how the two meet: on C, which both read and write. A is read by both,
  // which is no dependence. A and D are not known apart, and D is only read: none either.
  CHECK(outline.order.size() == 2);
  if (outline.order.size() == 2) {
    CHECK(outline.order[0].kind == OrderEntry::Nest && outline.order[0].nest == 0);
    CHECK(outline.order[1].kind == OrderEntry::Nest && outline.order[1].nest == 1);
  }
  unsigned onC = 0, others = 0;
  for (const NestDependence &dependence : outline.dependences) {
    bool c = dependence.relation == NestDependence::SameRoot && dependence.earlierRoot == 2 &&
             dependence.laterRoot == 2 && dependence.earlier == 0 && dependence.later == 1 &&
             dependence.earlierAccess == (NestReads | NestWrites) &&
             dependence.laterAccess == (NestReads | NestWrites);
    onC += c;
    // B (root 1) and C (root 2) are not known apart, and the second nest writes C.
    others += !c;
  }
  CHECK(onC == 1);
  // Roots not known apart, one of them written: B against C, C against A, C against D
  // (1,2), and the like. Each is listed, none is dropped.
  bool bAgainstC = false;
  for (const NestDependence &dependence : outline.dependences)
    bAgainstC = bAgainstC || (dependence.relation == NestDependence::MayAlias &&
                              dependence.earlierRoot == 1 && dependence.laterRoot == 2);
  CHECK(bAgainstC);
  CHECK(others >= 1);
  // With every pair authorized, only the shared root is left.
  Authorizations all;
  for (unsigned a = 0; a < 4; ++a)
    for (unsigned b = a + 1; b < 4; ++b)
      all.insert(separationName(a, b));
  FunctionOutline apart = outlineNests(m.function("shared"), all);
  CHECK(apart.dependences.size() == 1);
  CHECK(separationName(3, 1) == "roots 1 and 3 are separate");

  // What enters a nest. A constant is copied; an argument and a value computed before
  // the nest are arguments of the copy; a value read from memory before the nest is one
  // too, and the read is not repeated in the copy.
  Module p(kPrograms);
  std::string programs = p.text();
  FunctionOutline inputs = outlineNests(p.function("inputs"), {});
  CHECK(inputs.nests.size() == 1);
  if (inputs.nests.size() == 1) {
    const OutlinedNest &nest = inputs.nests[0];
    CHECK(nest.unsupported.empty());
    CHECK(nest.loop == "scf.for");
    CHECK(nest.statement == 5);
    CHECK(nest.constantsCopied == 1); // %c1, the step. %c0 is not read by the nest.
    CHECK(nest.roots.size() == 2);    // %a and %c. %p is read before the nest only.
    if (nest.roots.size() == 2) {
      CHECK(nest.roots[0].root == 0 && nest.roots[0].access == NestReads);
      CHECK(nest.roots[1].root == 1 && nest.roots[1].access == (NestReads | NestWrites));
    }
    CHECK(nest.scalars.size() == 4);
    if (nest.scalars.size() == 4) {
      CHECK(nest.scalars[0].source == NestScalar::FunctionArgument && nest.scalars[0].argument == 3);
      CHECK(nest.scalars[1].source == NestScalar::FunctionArgument && nest.scalars[1].argument == 4);
      CHECK(nest.scalars[0].outlined == 2 && nest.scalars[1].outlined == 3);
      CHECK(nest.scalars[0].type == "index" && nest.scalars[1].type == "i32");
      // In order of first use: the lower bound %t, then the upper bound %m.
      CHECK(nest.scalars[2].source == NestScalar::ComputedValue && nest.scalars[2].definition == "memref.load");
      CHECK(nest.scalars[3].source == NestScalar::ComputedValue && nest.scalars[3].definition == "arith.addi");
      CHECK(nest.scalars[2].outlined == 4 && nest.scalars[3].outlined == 5);
    }
    if (nest.module) {
      mlir::func::FuncOp copy = nest.outlined();
      CHECK(copy.getNumArguments() == 6);
      // Before the loop the copy holds constants and nothing else: no load, no addition.
      mlir::Block &entry = copy.getBody().front();
      unsigned constants = 0, loops = 0, other = 0;
      for (mlir::Operation &op : entry) {
        llvm::StringRef name = op.getName().getStringRef();
        if (name == "arith.constant")
          ++constants;
        else if (name == "scf.for")
          ++loops;
        else if (name != "func.return")
          ++other;
      }
      CHECK(constants == 1 && loops == 1 && other == 0);
      CHECK(countOps(copy, "memref.load") == 2); // Both inside the loop.
      CHECK(countOps(copy, "memref.store") == 1);
    }
  }
  // The statements around it, in order: a read of %p, a write of %p, the nest.
  {
    std::vector<std::pair<OrderEntry::Kind, unsigned>> seen;
    for (const OrderEntry &entry : inputs.order)
      seen.push_back({entry.kind, entry.kind == OrderEntry::Nest ? entry.nest : entry.access});
    CHECK(seen.size() == 3);
    if (seen.size() == 3) {
      CHECK(seen[0] == std::make_pair(OrderEntry::Access, (unsigned)NestReads));
      CHECK(seen[1] == std::make_pair(OrderEntry::Access, (unsigned)NestWrites));
      CHECK(seen[2] == std::make_pair(OrderEntry::Nest, 0u));
      CHECK(inputs.order[0].root == 2 && inputs.order[1].root == 2);
      CHECK(inputs.order[0].statement == 3 && inputs.order[1].statement == 4);
    }
  }

  // What is refused, in words, with no copy.
  FunctionOutline carried = outlineNests(p.function("carried"), {});
  CHECK(carried.nests.size() == 1);
  CHECK(refused(carried, 0, "yields a value that is used after it"));
  FunctionOutline unused = outlineNests(p.function("unused"), {});
  CHECK(unused.nests.size() == 1);
  CHECK(nestOf(unused, 0).unsupported.empty() && (bool)nestOf(unused, 0).module);
  FunctionOutline calls = outlineNests(p.function("calls"), {});
  CHECK(calls.nests.size() == 2);
  CHECK(refused(calls, 0, "calls a function"));
  CHECK(nestOf(calls, 1).unsupported.empty());
  // A nest with effects that are no access to a root may meet any other.
  CHECK(calls.dependences.size() == 1);
  if (calls.dependences.size() == 1)
    CHECK(calls.dependences[0].relation == NestDependence::UnknownEffects && calls.dependences[0].earlier == 0 &&
          calls.dependences[0].later == 1);
  FunctionOutline views = outlineNests(p.function("views"), {});
  CHECK(views.nests.size() == 1);
  CHECK(refused(views, 0, "the result of memref.cast"));
  // A memref the nest makes for itself is no root: a global, and a memref carried round
  // the loop. Such a nest may meet any other. The array the loop is handed is taken as
  // read and written, whatever the body does with the copy it carries.
  FunctionOutline globals = outlineNests(p.function("globals"), {});
  CHECK(globals.nests.size() == 2);
  CHECK(refused(globals, 0, "makes a memref that is the result of memref.get_global and no allocation"));
  CHECK(nestOf(globals, 1).unsupported.empty());
  CHECK(globals.dependences.size() == 1);
  if (globals.dependences.size() == 1)
    CHECK(globals.dependences[0].relation == NestDependence::UnknownEffects);
  FunctionOutline threaded = outlineNests(p.function("threaded"), {});
  CHECK(threaded.nests.size() == 2);
  CHECK(refused(threaded, 0, "makes a memref that is a block argument and no allocation"));
  if (threaded.nests.size() == 2) {
    CHECK(threaded.nests[0].roots.size() == 2);
    if (threaded.nests[0].roots.size() == 2) {
      CHECK(threaded.nests[0].roots[0].root == 0 && threaded.nests[0].roots[0].access == (NestReads | NestWrites));
      CHECK(threaded.nests[0].roots[1].root == 1 && threaded.nests[0].roots[1].access == NestReads);
    }
  }
  CHECK(threaded.dependences.size() == 1);
  if (threaded.dependences.size() == 1)
    CHECK(threaded.dependences[0].relation == NestDependence::UnknownEffects);
  // A copy that does not verify is no copy: here it names a function it does not hold.
  FunctionOutline symbols = outlineNests(p.function("symbols"), {});
  CHECK(symbols.nests.size() == 1);
  CHECK(refused(symbols, 0, "the outlined copy does not verify"));
  // A parallel loop has no order of its own, as the outermost loop or inside one.
  FunctionOutline parallel = outlineNests(p.function("parallel"), {});
  CHECK(parallel.nests.size() == 2);
  CHECK(refused(parallel, 0, "holds a parallel loop (scf.parallel), whose iterations have no order"));
  CHECK(refused(parallel, 1, "holds a parallel loop (affine.parallel), whose iterations have no order"));
  if (parallel.nests.size() == 2) {
    // What each does to the roots is still listed, and so is how they meet.
    CHECK(parallel.nests[0].roots.size() == 2 && parallel.nests[0].roots[1].access == NestWrites);
    CHECK(parallel.dependences.size() == 3);
  }
  FunctionOutline vectors = outlineNests(p.function("vectors"), {});
  CHECK(vectors.nests.size() == 1);
  CHECK(refused(vectors, 0, "a value of type vector<2xi32> enters the nest"));
  FunctionOutline branches = outlineNests(p.function("branches"), {});
  CHECK(branches.nests.empty());
  CHECK(branches.unsupported.find("more than one block") != std::string::npos);
  FunctionOutline regions = outlineNests(p.function("regions"), {});
  CHECK(regions.nests.empty());
  CHECK(regions.unsupported.find("a region with more than one block") != std::string::npos);
  FunctionOutline none = outlineNests(p.function("noloops"), {});
  CHECK(none.unsupported.empty() && none.nests.empty());
  CHECK(none.order.size() == 2);
  FunctionOutline external = outlineNests(p.function("callee"), {});
  CHECK(external.unsupported == "the function has no body" && external.nests.empty());

  // A nest under a condition is an outermost loop like any other, and says so.
  FunctionOutline conditional = outlineNests(p.function("conditional"), {});
  CHECK(conditional.nests.size() == 2);
  if (conditional.nests.size() == 2) {
    CHECK(conditional.nests[0].conditions == 1 && conditional.nests[0].statement == 1);
    CHECK(conditional.nests[1].conditions == 0 && conditional.nests[1].statement == 2);
    CHECK(conditional.nests[0].unsupported.empty() && conditional.nests[1].unsupported.empty());
  }
  // nest 0, then the else branch's read and write, then nest 1.
  CHECK(conditional.order.size() == 4);
  if (conditional.order.size() == 4) {
    CHECK(conditional.order[0].kind == OrderEntry::Nest && conditional.order[0].conditions == 1);
    CHECK(conditional.order[1].kind == OrderEntry::Access && conditional.order[1].access == NestReads &&
          conditional.order[1].root == 0 && conditional.order[1].conditions == 1);
    CHECK(conditional.order[2].kind == OrderEntry::Access && conditional.order[2].access == NestWrites &&
          conditional.order[2].root == 1);
    CHECK(conditional.order[3].kind == OrderEntry::Nest && conditional.order[3].nest == 1);
  }

  // The function's roots are numbered as the importer numbers them, and two roots stand
  // as its own rule says: checked against the model of the whole function.
  {
    mlir::func::FuncOp function = p.function("allocs");
    Authorizations authorized{"roots 0 and 1 are separate"};
    FunctionOutline allocs = outlineNests(function, authorized);
    ImportOptions options;
    options.indexBitWidth = 16;
    options.lowerStructured = true;
    SemanticProgram whole = importFunction(function, options);
    CHECK(whole.isSupported());
    CHECK(allocs.roots.size() == 6);
    CHECK(allocs.roots.size() == whole.roots.size());
    static const int extents[] = {2, 3, 5, 7, 9, 11};
    for (unsigned r = 0; r < allocs.roots.size() && r < whole.roots.size(); ++r) {
      const FunctionRoot &mine = allocs.roots[r];
      const MemoryRoot &theirs = whole.roots[r];
      CHECK(mine.id == r && theirs.id == r);
      CHECK((mine.origin == FunctionRoot::FunctionArgument) == (theirs.origin == MemoryRoot::FunctionArgument));
      if (mine.origin == FunctionRoot::FunctionArgument)
        CHECK(mine.argument == theirs.argument);
      CHECK(theirs.extents.size() == 1 && theirs.extents[0] == extents[r]);
      CHECK(mine.type == "memref<" + std::to_string(extents[r]) + "xi32>");
      CHECK(!mine.insideLoop);
    }
    CHECK(allocs.roots[0].argument == 0 && allocs.roots[1].argument == 2);
    for (unsigned a = 0; a < whole.roots.size(); ++a)
      for (unsigned b = a + 1; b < whole.roots.size(); ++b) {
        RootSeparation theirs = rootSeparation(whole, authorized, a, b);
        // A nest that uses both gives the pair; the rule is the same for every pair.
        bool found = false;
        for (const OutlinedNest &nest : allocs.nests)
          for (const NestSeparation &mine : nest.separations)
            if (mine.first == a && mine.second == b) {
              found = true;
              CHECK(mine.apart == theirs.apart);
              CHECK(mine.proof == (theirs.apart && !theirs.assumption));
              CHECK(mine.basis == theirs.basis);
              CHECK(mine.name == theirs.name);
            }
        (void)found;
      }
    CHECK(allocs.nests.size() == 3);
    if (allocs.nests.size() == 3) {
      // a -> t, b -> u, u -> q: each pair holds a fresh root, so each is apart by a proof.
      for (const OutlinedNest &nest : allocs.nests) {
        CHECK(nest.separations.size() == 1);
        if (nest.separations.size() == 1)
          CHECK(nest.separations[0].apart && nest.separations[0].proof &&
                nest.separations[0].basis == "fresh allocation" &&
                nest.separations[0].outlinedName == "roots 0 and 1 are separate");
      }
      CHECK(allocs.nests[0].roots[1].root == 2 && allocs.nests[1].roots[1].root == 3);
      CHECK(allocs.nests[2].roots[0].root == 3 && allocs.nests[2].roots[1].root == 5);
      // The second nest writes u and the third reads it; the first meets neither.
      CHECK(allocs.dependences.size() == 1);
      if (allocs.dependences.size() == 1)
        CHECK(allocs.dependences[0].earlier == 1 && allocs.dependences[0].later == 2 &&
              allocs.dependences[0].relation == NestDependence::SameRoot && allocs.dependences[0].earlierRoot == 3);
    }
    // The order names the allocations where they are.
    unsigned allocations = 0;
    for (const OrderEntry &entry : allocs.order)
      allocations += entry.kind == OrderEntry::Allocation;
    CHECK(allocations == 4);
  }

  // An argument's llvm.noalias goes with it; an allocation gets none. A function
  // attribute goes with the copy, so that the importer's refusal of it still applies.
  {
    mlir::func::FuncOp function = p.function("attrs");
    FunctionOutline attrs = outlineNests(function, {});
    CHECK(attrs.nests.size() == 1 && (bool)nestOf(attrs, 0).module);
    if (attrs.nests.size() == 1 && attrs.nests[0].module) {
      mlir::func::FuncOp copy = attrs.nests[0].outlined();
      CHECK(copy.getNumArguments() == 3);
      CHECK((bool)copy.getArgAttr(0, "llvm.noalias"));
      CHECK(!copy.getArgAttr(1, "llvm.noalias"));
      CHECK(!copy.getArgAttr(2, "llvm.noalias")); // The allocation.
      CHECK(copy->hasAttr("custom.flag"));
      CHECK(attrs.roots.size() == 3 && attrs.roots[0].restrictArgument && !attrs.roots[1].restrictArgument);
      ImportOptions options;
      options.indexBitWidth = 16;
      options.lowerStructured = true;
      SemanticProgram model = importFunction(copy, options);
      CHECK(!model.isSupported());
      // Both are arguments in the copy, and (a, b) is apart by nobody's word.
      const NestSeparation &ab = attrs.nests[0].separations[0];
      CHECK(ab.first == 0 && ab.second == 1 && !ab.apart);
      CHECK(attrs.nests[0].separations[1].proof && attrs.nests[0].separations[2].proof);
    }
    // Authorized, the pair with a noalias argument says so in its basis, and stays an
    // assumption.
    FunctionOutline named = outlineNests(function, {"roots 0 and 1 are separate"});
    if (named.nests.size() == 1 && !named.nests[0].separations.empty()) {
      const NestSeparation &ab = named.nests[0].separations[0];
      CHECK(ab.apart && !ab.proof && ab.basis == "authorized, with an llvm.noalias source contract");
    }
  }
  CHECK(p.text() == programs);

  // The module's own attributes go with the copy too, and the importer refuses it for
  // them as it refuses the function.
  {
    Module tagged(R"mlir(
module attributes {custom.module = 1 : i32} {
  func.func @f(%a: memref<4xi32>, %b: memref<4xi32>) {
    affine.for %i = 0 to 4 {
      %x = affine.load %a[%i] : memref<4xi32>
      affine.store %x, %b[%i] : memref<4xi32>
    }
    return
  }
}
)mlir");
    FunctionOutline outline = outlineNests(tagged.function("f"), {});
    CHECK(outline.nests.size() == 1 && (bool)nestOf(outline, 0).module);
    if (outline.nests.size() == 1 && outline.nests[0].module) {
      CHECK(outline.nests[0].module.get()->hasAttr("custom.module"));
      ImportOptions options;
      options.indexBitWidth = 16;
      options.lowerStructured = true;
      SemanticProgram model = importFunction(outline.nests[0].outlined(), options);
      CHECK(!model.isSupported());
      if (!model.isSupported())
        CHECK(model.unsupported->detail == "module: custom.module");
    }
  }
}

/// A function made of nests only, over argument roots: running the copies one after the
/// other, each from the memory the last one left, gives the function's memory. The
/// library's concrete evaluator runs both; no checker is involved.
static void composes(const std::string &text, const char *name, std::vector<int64_t> scalars, bool isFloat,
                     unsigned fills) {
  Module m(text);
  mlir::func::FuncOp function = m.function(name);
  ImportOptions options;
  options.indexBitWidth = 16;
  options.lowerStructured = true;
  SemanticProgram whole = importFunction(function, options);
  CHECK(whole.isSupported());
  FunctionOutline outline = outlineNests(function, {});
  CHECK(outline.unsupported.empty() && !outline.nests.empty());
  if (!whole.isSupported() || !outline.unsupported.empty())
    return;
  uint64_t seed = 20261116;
  auto next = [&]() {
    seed = seed * 6364136223846793005ull + 1442695040888963407ull;
    return (int64_t)((seed >> 33) % 13) - 6;
  };
  for (unsigned fill = 0; fill < fills; ++fill) {
    EntryState entry;
    unsigned scalar = 0;
    for (const Parameter &parameter : whole.parameters)
      entry.parameters.push_back(bits(parameter.sort.width, scalars.at(scalar++)));
    for (const MemoryRoot &root : whole.roots) {
      RootData data;
      uint64_t cells = 1;
      for (int64_t extent : root.extents) {
        data.shape.push_back(extent);
        cells *= extent;
      }
      for (uint64_t c = 0; c < cells; ++c) {
        int64_t value = next();
        data.cells.emplace_back(isFloat ? llvm::APFloat((float)value * 0.5f).bitcastToAPInt()
                                        : bits(root.sort.width, value));
      }
      entry.memory.push_back(std::move(data));
    }
    RunResult expected = runConcrete(whole, entry);
    CHECK(expected.status == RunStatus::Returned);
    std::vector<RootData> memory = entry.memory;
    bool ran = true;
    for (const OutlinedNest &nest : outline.nests) {
      CHECK((bool)nest.module);
      if (!nest.module) {
        ran = false;
        break;
      }
      SemanticProgram copy = importFunction(nest.outlined(), options);
      CHECK(copy.isSupported());
      if (!copy.isSupported()) {
        ran = false;
        break;
      }
      EntryState local;
      for (const Parameter &parameter : copy.parameters) {
        // A scalar of the copy is an argument of the function, with the same value.
        bool found = false;
        for (const NestScalar &input : nest.scalars)
          for (const Parameter &outer : whole.parameters)
            if (input.outlined == parameter.argument && input.source == NestScalar::FunctionArgument &&
                outer.kind == Parameter::Scalar && outer.argument == input.argument && !found) {
              local.parameters.push_back(entry.parameters[outer.id]);
              found = true;
            }
        CHECK(found);
      }
      for (const NestRoot &root : nest.roots)
        local.memory.push_back(memory[root.root]);
      RunResult result = runConcrete(copy, local);
      CHECK(result.status == RunStatus::Returned);
      if (result.status != RunStatus::Returned) {
        ran = false;
        break;
      }
      for (const NestRoot &root : nest.roots)
        memory[root.root] = result.memory[root.outlined];
    }
    if (!ran || expected.status != RunStatus::Returned)
      continue;
    bool equal = memory.size() == expected.memory.size();
    for (unsigned r = 0; equal && r < memory.size(); ++r) {
      equal = memory[r].cells.size() == expected.memory[r].cells.size();
      for (unsigned c = 0; equal && c < memory[r].cells.size(); ++c)
        equal = memory[r].cells[c].has_value() == expected.memory[r].cells[c].has_value() &&
                (!memory[r].cells[c] || *memory[r].cells[c] == *expected.memory[r].cells[c]);
    }
    CHECK(equal);
  }
}

/// Stage 2: the binding of a nest (Systolic/NestBinding.h), computed by the code the
/// production pass shares with the outliner. Which loop of which input a record is about:
/// the same nest of the same input gives the same binding wherever the file is, and any
/// change of the nest, of what it reads, of the function's name or of the nest's position
/// gives another.
static const char *kBound = R"mlir(
func.func @f(%a: memref<4x4xf32>, %b: memref<4x4xf32>, %c: memref<4x4xf32> {llvm.noalias}, %n: index) {
  %t = memref.alloc() : memref<4x4xf32>
  %c1 = arith.constant 1 : index
  %m = arith.addi %n, %c1 : index
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      %x = affine.load %a[%i, %j] : memref<4x4xf32>
      affine.store %x, %t[%i, %j] : memref<4x4xf32>
    }
  }
  scf.for %k = %c1 to %m step %c1 {
    %y = memref.load %t[%k, %k] : memref<4x4xf32>
    memref.store %y, %c[%k, %k] : memref<4x4xf32>
  }
  return
}
)mlir";

static std::vector<std::string> shas(const std::string &text, const char *name = "f") {
  Module m(text);
  FunctionOutline outline = outlineNests(m.function(name), {});
  std::vector<std::string> out;
  for (const OutlinedNest &nest : outline.nests)
    out.push_back(nest.bindingSha);
  return out;
}

static std::string replaced(std::string text, const std::string &from, const std::string &to) {
  size_t at = text.find(from);
  CHECK(at != std::string::npos);
  if (at != std::string::npos)
    text.replace(at, from.size(), to);
  return text;
}

static void testBinding() {
  Module m(kBound);
  mlir::func::FuncOp function = m.function("f");
  FunctionOutline outline = outlineNests(function, {});
  CHECK(outline.nests.size() == 2);
  mlir::systolic::FunctionRoots roots = mlir::systolic::numberFunctionRoots(function);
  llvm::SmallVector<mlir::Operation *> loops = mlir::systolic::outermostLoops(function);
  CHECK(loops.size() == 2 && roots.roots.size() == 4);
  // The outliner's numbering is the binding's: every nest has a binding, and it is the
  // production code's own result for that loop.
  for (unsigned k = 0; k < outline.nests.size() && k < loops.size(); ++k) {
    const OutlinedNest &nest = outline.nests[k];
    CHECK(nest.bindingRefusal.empty());
    CHECK(nest.bindingSha.size() == 64 &&
          nest.bindingSha.find_first_not_of("0123456789abcdef") == std::string::npos);
    std::string why;
    llvm::FailureOr<mlir::systolic::NestBinding> binding =
        mlir::systolic::computeNestBinding(function, loops[k], k, roots, why);
    CHECK(mlir::succeeded(binding));
    if (mlir::succeeded(binding)) {
      CHECK(binding->sha256 == nest.bindingSha);
      // What the text says the nest reads, in its order: roots by number, then scalars.
      if (k == 1) {
        const std::string &text = binding->text;
        CHECK(text.rfind("generalized-gemm-nest-binding/1\nfunction: f\nnest: 1\n", 0) == 0);
        CHECK(text.find("argument 0: root 2, argument 2, llvm.noalias : memref<4x4xf32>\n") != std::string::npos);
        CHECK(text.find("argument 1: root 3, allocation : memref<4x4xf32>\n") != std::string::npos);
        CHECK(text.find("argument 2: scalar, computed by arith.addi : index\n") != std::string::npos);
        CHECK(text.find("argument 3:") == std::string::npos);
        // The constant is copied in; no location is printed.
        CHECK(text.find("\"arith.constant\"() <{value = 1 : index}>") != std::string::npos);
        CHECK(text.find("loc(") == std::string::npos);
      }
    }
  }
  CHECK(outline.nests.size() == 2 && outline.nests[0].bindingSha != outline.nests[1].bindingSha);

  // The same nest of the same input: the same binding, parsed again, and with locations.
  std::vector<std::string> base = shas(kBound);
  CHECK(shas(kBound) == base);
  CHECK(shas(replaced(kBound, "affine.store %x, %t[%i, %j] : memref<4x4xf32>",
                      "affine.store %x, %t[%i, %j] : memref<4x4xf32> loc(\"elsewhere.c\":7:7)")) == base);
  CHECK(shas(replaced(kBound, "%c1 = arith.constant 1 : index", "%c1 = arith.constant 1 : index loc(\"x\":1:2)")) ==
        base);
  // A change of the first nest changes its binding only.
  std::vector<std::string> bound = shas(replaced(kBound, "affine.for %j = 0 to 4", "affine.for %j = 0 to 3"));
  CHECK(bound.size() == 2 && bound[0] != base[0] && bound[1] == base[1]);
  // The roots' attributes are part of what a nest reads: C's llvm.noalias is the second
  // nest's only.
  std::vector<std::string> plain =
      shas(replaced(kBound, "%c: memref<4x4xf32> {llvm.noalias}", "%c: memref<4x4xf32>"));
  CHECK(plain.size() == 2 && plain[0] == base[0] && plain[1] != base[1]);
  // A and B exchanged: the first nest reads another argument.
  std::vector<std::string> exchanged = shas(replaced(kBound, "%x = affine.load %a[", "%x = affine.load %b["));
  CHECK(exchanged.size() == 2 && exchanged[0] != base[0] && exchanged[1] == base[1]);
  // The function's name, and the nest's position, are in every binding.
  std::string renamed = replaced(kBound, "func.func @f(", "func.func @g(");
  std::vector<std::string> other = shas(renamed, "g");
  CHECK(other.size() == 2 && other[0] != base[0] && other[1] != base[1]);
  std::vector<std::string> later = shas(replaced(kBound, "  affine.for %i = 0 to 4 {",
                                                 "  affine.for %q = 0 to 1 {\n  }\n  affine.for %i = 0 to 4 {"));
  CHECK(later.size() == 3 && later[1] != base[0] && later[2] != base[1]);
  // A computed scalar is any value: what computes it, other than its operation, is not read.
  CHECK(shas(replaced(kBound, "%m = arith.addi %n, %c1 : index", "%m = arith.addi %c1, %n : index")) == base);
  CHECK(shas(replaced(kBound, "%m = arith.addi %n, %c1 : index", "%m = arith.muli %n, %c1 : index"))[1] != base[1]);

  // A memref that is no root of the function: no binding, and the reason.
  Module viewed(R"mlir(
func.func @v(%a: memref<4xf32>, %b: memref<4xf32>) {
  %w = memref.cast %a : memref<4xf32> to memref<?xf32>
  affine.for %i = 0 to 4 {
    %x = affine.load %w[%i] : memref<?xf32>
    affine.store %x, %b[%i] : memref<4xf32>
  }
  return
}
)mlir");
  FunctionOutline refused = outlineNests(viewed.function("v"), {});
  CHECK(refused.nests.size() == 1);
  if (refused.nests.size() == 1) {
    CHECK(refused.nests[0].bindingSha.empty());
    CHECK(refused.nests[0].bindingRefusal == "the loop reads a memref that is no root of the function");
  }

  // The record carries the binding, the index width of its claims and its scope as data.
  NestAnalysisOptions options;
  options.indexBitWidth = 16;
  options.timeoutMs = 20000;
  options.resourceLimit = 2000000;
  options.maxLevel = 1;
  options.candidatesPerGroup = 1;
  options.scope.push_back({3, 1, 2});
  NestRecord record = analyzeNest(outline.nests[1], options, z3Backend);
  CHECK(record.bindingSha == outline.nests[1].bindingSha && record.bindingRefusal.empty());
  CHECK(record.indexWidth == 16);
  CHECK(record.scope.size() == 0); // The second nest reads no argument: %m is a computed value.
  NestRecord first = analyzeNest(outline.nests[0], options, z3Backend);
  CHECK(first.bindingSha == outline.nests[0].bindingSha);
  llvm::json::Object json = toJSON(record, outline.roots);
  const llvm::json::Object *printed = json.getObject("binding");
  CHECK(printed != nullptr);
  if (printed) {
    CHECK(printed->getString("sha256") == std::optional<llvm::StringRef>(outline.nests[1].bindingSha));
    CHECK(printed->getInteger("index_width") == std::optional<int64_t>(16));
    CHECK(printed->getString("scheme") == std::optional<llvm::StringRef>(mlir::systolic::kNestBindingScheme));
  }
  CHECK(json.getArray("scope") && json.getArray("scope")->empty());
  // A scalar argument read by the nest, in scope: the range is printed as data.
  Module scoped(R"mlir(
func.func @s(%a: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    memref.store %x, %c[%i] : memref<8xi32>
  }
  return
}
)mlir");
  FunctionOutline withScalar = outlineNests(scoped.function("s"), {});
  CHECK(withScalar.nests.size() == 1);
  if (withScalar.nests.size() == 1) {
    NestAnalysisOptions narrow = options;
    narrow.scope = {{2, 1, 3}};
    NestRecord r = analyzeNest(withScalar.nests[0], narrow, z3Backend);
    CHECK(r.scope.size() == 1 && r.scope[0].argument == 2 && r.scope[0].lower == 1 && r.scope[0].upper == 3);
    llvm::json::Object j = toJSON(r, withScalar.roots);
    const llvm::json::Array *list = j.getArray("scope");
    CHECK(list && list->size() == 1);
    if (list && list->size() == 1) {
      const llvm::json::Object *entry = (*list)[0].getAsObject();
      CHECK(entry && entry->getInteger("argument") == std::optional<int64_t>(2) &&
            entry->getInteger("lower") == std::optional<int64_t>(1) &&
            entry->getInteger("upper") == std::optional<int64_t>(3));
    }
  }
}

static void testComposition() {
  // Two nests on one output; a nest that reads what the other wrote, as an operand and
  // as the cell it accumulates into; three nests; binary32; a bound that is an argument.
  composes("func.func @f(" + arguments({"%a", "%b", "%c", "%d"}) + ") {\n" + gemm("0", "%a", "%b", "%c", 2, 3, 2) +
               gemm("1", "%a", "%d", "%c", 3, 2, 4) + "  return\n}\n",
           "f", {}, false, 8);
  composes("func.func @f(" + arguments({"%a", "%b", "%c", "%d"}) + ") {\n" + gemm("0", "%a", "%b", "%c", 3, 3, 2) +
               gemm("1", "%c", "%b", "%d", 2, 2, 3) + "  return\n}\n",
           "f", {}, false, 8);
  composes("func.func @f(" + arguments({"%a", "%b", "%c", "%d"}) + ") {\n" + gemm("0", "%a", "%b", "%c", 2, 2, 2) +
               gemm("1", "%c", "%c", "%d", 2, 2, 2) + gemm("2", "%d", "%a", "%c", 2, 2, 2) + "  return\n}\n",
           "f", {}, false, 8);
  composes("func.func @f(" + arguments({"%a", "%b", "%c", "%d"}, "f32") + ") {\n" +
               gemm("0", "%a", "%b", "%c", 2, 3, 2, "f32") + gemm("1", "%c", "%d", "%c", 2, 2, 2, "f32", "fastmath<reassoc>") +
               "  return\n}\n",
           "f", {}, true, 8);
  composes(R"mlir(
func.func @f(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %x2 = memref.load %c[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %a[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %a[%j] : memref<8xi32>
  }
  return
}
)mlir",
           "f", {5}, false, 8);
}

/// A record that maps: C[I, J] += A[I, K] * B[K, J] over a 2 x 3 x 4 box, as a record
/// of it reads, with every fact a mapping rests on.
static NestRecord mappedRecord() {
  NestRecord r;
  static const char axes[] = {'I', 'J', 'K'};
  for (unsigned l = 0; l < 3; ++l) {
    NestLevel level;
    level.axis = axes[l];
    level.lower = 0;
    level.upper = 2 + l;
    level.extent = 2 + l;
    level.lowerText = "0";
    level.upperText = std::to_string(2 + l);
    r.levels.push_back(level);
  }
  r.guard = "true";
  r.box = true;
  r.extents = std::array<uint64_t, 3>{2, 3, 4};
  static const unsigned reads[3][2] = {{0, 1}, {0, 2}, {2, 1}};
  for (unsigned view = 0; view < 3; ++view) {
    NestView &one = r.views[view];
    one.root = view;
    one.proposed = one.affine = one.proved = true;
    one.fact = "PROVEN at SYMBOLIC";
    for (unsigned d = 0; d < 2; ++d) {
      NestViewDimension dimension;
      dimension.affine = true;
      dimension.levels.push_back({reads[view][d], 1});
      dimension.text = "level" + std::to_string(reads[view][d]);
      one.dimensions.push_back(dimension);
    }
  }
  return r;
}

static NestMappingFacts mappingFacts() {
  NestMappingFacts facts;
  facts.singleOrder = facts.logicalSymbolic = facts.exactOrder = facts.unitMultiplicity = facts.outputInjective = true;
  facts.logicalTier = "SYMBOLIC";
  facts.multiplicity = "UNIT";
  facts.outputInjectiveFact = "PROVEN at SYMBOLIC";
  return facts;
}

/// Each condition of a mapping, alone: the record that maps gives no refusal, and with one
/// thing changed it gives the refusal of that thing. No checker makes most of these
/// changes on any input (a proposed view left unproved while the fold is verified, an
/// inexact order under a verified Theorem 13), so they are made here, on the data.
static void testMapping() {
  CHECK(mappingRefusals(mappedRecord(), mappingFacts()).empty());
  auto withFacts = [](void (*change)(NestMappingFacts &)) {
    NestMappingFacts facts = mappingFacts();
    change(facts);
    return mappingRefusals(mappedRecord(), facts);
  };
  auto withRecord = [](void (*change)(NestRecord &)) {
    NestRecord r = mappedRecord();
    change(r);
    return mappingRefusals(r, mappingFacts());
  };
  auto only = [](const std::vector<std::string> &refusals, const char *words) {
    return refusals.size() == 1 && refusals[0].find(words) != std::string::npos;
  };
  CHECK(only(withFacts([](NestMappingFacts &f) { f.singleOrder = false; }),
             "the candidate has no single order over its family"));
  CHECK(only(withFacts([](NestMappingFacts &f) { f.logicalSymbolic = false; f.logicalTier = "POINT"; }),
             "the coordinates I, J and K are not verified over the scope: the logical tier is POINT"));
  CHECK(only(withFacts([](NestMappingFacts &f) { f.exactOrder = false; }), "the order is not proved exact"));
  CHECK(only(withFacts([](NestMappingFacts &f) { f.unitMultiplicity = false; f.multiplicity = "COUNTED"; }),
             "a point of the order has more than one event: the multiplicity is COUNTED"));
  CHECK(only(withFacts([](NestMappingFacts &f) { f.outputInjective = false; f.outputInjectiveFact = "UNKNOWN"; }),
             "two logical outputs may share a cell of C: the fact is UNKNOWN"));
  CHECK(only(withRecord([](NestRecord &r) { r.guard = "(guard)"; }), "the domain has a guard, so it is no box: (guard)"));
  CHECK(only(withRecord([](NestRecord &r) { r.flagsAgree = false; }),
             "the sites of the group do not agree on their operation and flags"));
  // Levels.
  CHECK(only(withRecord([](NestRecord &r) { r.levels[1].lower.reset(); r.levels[1].lowerText = "(low)"; }),
             "no constant extent: the lower bound of level 1 (axis J) is (low)"));
  CHECK(only(withRecord([](NestRecord &r) { r.levels[2].upper.reset(); r.levels[2].upperText = "(high)"; }),
             "no constant extent: the upper bound of level 2 (axis K) is (high)"));
  CHECK(only(withRecord([](NestRecord &r) { r.levels[0].extent = 0; }), "level 0 (axis I) is empty"));
  CHECK(only(withRecord([](NestRecord &r) { r.levels[2].descending = true; }),
             "level 2 (axis K) descends, and a descending level is not mapped"));
  {
    // A fourth level that is no coordinate: a repeat. Nothing else is wrong.
    std::vector<std::string> refusals = withRecord([](NestRecord &r) {
      NestLevel level = r.levels[0];
      level.axis = 'R';
      r.levels.push_back(level);
    });
    CHECK(only(refusals, "1 level(s) of the order are no coordinate of the family"));
    refusals = withRecord([](NestRecord &r) {
      NestLevel level = r.levels[0];
      level.axis = '?';
      r.levels.push_back(level);
    });
    CHECK(only(refusals, "1 level(s) of the order are coordinates of two axes"));
    refusals = withRecord([](NestRecord &r) {
      NestLevel level = r.levels[0];
      level.axis = 'P';
      r.levels.push_back(level);
    });
    CHECK(only(refusals, "the family has a batch coordinate (axis P), and batches are not mapped"));
    refusals = withRecord([](NestRecord &r) {
      NestLevel level = r.levels[2];
      r.levels.push_back(level);
    });
    CHECK(only(refusals, "axis K has 2 levels, and a box has one"));
  }
  // Views.
  CHECK(only(withRecord([](NestRecord &r) { r.views[1].root = -1; }), "the root of A is not a root of the function"));
  CHECK(only(withRecord([](NestRecord &r) { r.views[2].proposed = false; }), "the view of B has no proposed closed form"));
  CHECK(only(withRecord([](NestRecord &r) { r.views[1].proved = false; r.views[1].fact = "UNKNOWN at PROPOSED"; }),
             "the view of A is not proved over the scope: UNKNOWN at PROPOSED"));
  CHECK(only(withRecord([](NestRecord &r) { r.views[0].dimensions[1].affine = false; r.views[0].dimensions[1].text = "(t)"; }),
             "dimension 1 of the view of C is not affine: (t)"));
  CHECK(only(withRecord([](NestRecord &r) { r.views[2].dimensions[0].parameters.push_back({0, 1}); }),
             "dimension 0 of the view of B reads a parameter, and a symbolic offset is not mapped"));
  // Each operand reads its own two roles: A may not read J, B may not read I, C may not read K.
  CHECK(only(withRecord([](NestRecord &r) { r.views[1].dimensions[0].levels.push_back({1, 1}); }),
             "dimension 0 of the view of A reads level 1 (axis J), which is no role of A"));
  CHECK(only(withRecord([](NestRecord &r) { r.views[2].dimensions[1].levels.push_back({0, 1}); }),
             "dimension 1 of the view of B reads level 0 (axis I), which is no role of B"));
  CHECK(only(withRecord([](NestRecord &r) { r.views[0].dimensions[0].levels.push_back({2, 1}); }),
             "dimension 0 of the view of C reads level 2 (axis K), which is no role of C"));
}

static NestAnalysisOptions testOptions() {
  NestAnalysisOptions options;
  options.indexBitWidth = 16;
  // The resource limit is the bound that decides, and it does not depend on the machine's
  // load; the time limit is far above what any query here takes.
  options.timeoutMs = 20000;
  options.resourceLimit = 2000000;
  options.maxLevel = 1;
  options.candidatesPerGroup = 1;
  return options;
}

static std::vector<NestRecord> analyze(Module &m, const char *name, const Authorizations &authorized,
                                       FunctionOutline &outline, NestAnalysisOptions options = testOptions()) {
  outline = outlineNests(m.function(name), authorized);
  std::vector<NestRecord> records;
  for (const OutlinedNest &nest : outline.nests)
    records.push_back(analyzeNest(nest, options, z3Backend));
  return records;
}

static bool says(const std::vector<std::string> &texts, const char *words) {
  for (const std::string &text : texts)
    if (text.find(words) != std::string::npos)
      return true;
  return false;
}

/// The three views of a plain product: C[I, J], A[I, K], B[K, J], each a level with
/// scale one and no offset.
static bool plainViews(const NestRecord &r) {
  static const char roles[3][2] = {{'I', 'J'}, {'I', 'K'}, {'K', 'J'}};
  for (unsigned view = 0; view < 3; ++view) {
    const NestView &one = r.views[view];
    if (!one.proposed || !one.affine || !one.proved || one.dimensions.size() != 2)
      return false;
    for (unsigned d = 0; d < 2; ++d) {
      const NestViewDimension &dimension = one.dimensions[d];
      if (!dimension.affine || dimension.constant != 0 || !dimension.offset || *dimension.offset != 0 ||
          dimension.roles.size() != 1 || dimension.roles[0].first != roles[view][d] ||
          dimension.roles[0].second != 1 || !dimension.parameters.empty())
        return false;
    }
  }
  return true;
}

static const Authorizations abc{"roots 0 and 2 are separate", "roots 1 and 2 are separate"};

static void testProduct() {
  // A. One product over a constant box, in binary32 with reassoc, in binary32 without,
  // and in 32-bit integers.
  {
    std::string text = "func.func @mm(" + arguments({"%a", "%b", "%c"}, "f32") + ") {\n" +
                       gemm("0", "%a", "%b", "%c", 2, 3, 4, "f32", "fastmath<reassoc>") + "  return\n}\n" +
                       "func.func @strict(" + arguments({"%a", "%b", "%c"}, "f32") + ") {\n" +
                       gemm("0", "%a", "%b", "%c", 2, 3, 4, "f32") + "  return\n}\n" + "func.func @mmi(" +
                       arguments({"%a", "%b", "%c"}) + ") {\n" + gemm("0", "%a", "%b", "%c", 2, 3, 4) + "  return\n}\n";
    Module m(text);
    FunctionOutline outline;
    std::vector<NestRecord> mm = analyze(m, "mm", abc, outline);
    CHECK(mm.size() == 1);
    if (mm.size() == 1) {
      const NestRecord &r = mm[0];
      CHECK(r.status == NestRecord::Verified);
      CHECK(r.refusals.empty());
      CHECK(r.function == "mm" && r.nest == 0 && r.loop == "affine.for" && r.statement == 0);
      CHECK(r.sites == 1 && r.groups == 1 && r.sitesInGroup == 1);
      CHECK(r.orderedFoldVerified);
      CHECK(r.box && r.guard == "true");
      CHECK(r.extents.has_value());
      if (r.extents)
        CHECK((*r.extents)[0] == 2 && (*r.extents)[1] == 3 && (*r.extents)[2] == 4);
      CHECK(r.levels.size() == 3);
      if (r.levels.size() == 3) {
        CHECK(r.levels[0].axis == 'I' && r.levels[1].axis == 'J' && r.levels[2].axis == 'K');
        for (const NestLevel &level : r.levels)
          CHECK(!level.descending && level.lower && *level.lower == 0 && level.extent);
        CHECK(*r.levels[0].upper == 2 && *r.levels[1].upper == 3 && *r.levels[2].upper == 4);
      }
      CHECK(r.views[0].root == 2 && r.views[1].root == 0 && r.views[2].root == 1);
      CHECK(plainViews(r));
      CHECK(r.reduction == "fadd" && r.multiply == "fmul" && r.fold == "ORDERED_FLOAT");
      CHECK(r.reductionFlags == FlagReassoc && r.multiplyFlags == 0);
      CHECK(r.flagsAgree && r.accumulatorFirst);
      CHECK(r.otherWrites == 0 && r.otherReads == 0);
      // Permitted by the flag, which is no proof: the fold's order is still the claim.
      CHECK(r.reassociationPermitted && r.reassociationBasis == "source flag");
      CHECK(r.sourceFlagReassoc && *r.sourceFlagReassoc);
      CHECK(says(r.assumptions, "roots 0 and 2 are separate: authorized, not proved, and used"));
      CHECK(says(r.assumptions, "roots 1 and 2 are separate: authorized, not proved, and used"));
      llvm::json::Object json = toJSON(r, outline.roots);
      CHECK(json.getBoolean("authorizes_lowering") == std::optional<bool>(false));
      CHECK(json.getString("status") == std::optional<llvm::StringRef>("VERIFIED"));
      CHECK(json.getBoolean("ordered_fold_verified") == std::optional<bool>(true));
      CHECK(json.getBoolean("reassociation_permitted") == std::optional<bool>(true));
      CHECK(json.getObject("gemm") != nullptr);
    }
    std::vector<NestRecord> strict = analyze(m, "strict", abc, outline);
    CHECK(strict.size() == 1);
    if (strict.size() == 1) {
      // The same nest without the flag: the same verified fold, and no permission.
      const NestRecord &r = strict[0];
      CHECK(r.status == NestRecord::Verified);
      CHECK(r.orderedFoldVerified);
      CHECK(!r.reassociationPermitted && r.reassociationBasis == "none");
      CHECK(r.sourceFlagReassoc && !*r.sourceFlagReassoc);
      CHECK(r.reductionFlags == 0);
      CHECK(r.extents.has_value() && plainViews(r));
    }
    std::vector<NestRecord> mmi = analyze(m, "mmi", abc, outline);
    CHECK(mmi.size() == 1);
    if (mmi.size() == 1) {
      const NestRecord &r = mmi[0];
      CHECK(r.status == NestRecord::Verified);
      CHECK(r.orderedFoldVerified);
      CHECK(r.reduction == "add" && r.multiply == "mul" && r.fold == "MODULAR_INTEGER");
      // For integers the permission is a proof, and there is no flag to read.
      CHECK(r.reassociationPermitted && r.reassociationBasis == "proof");
      CHECK(!r.sourceFlagReassoc.has_value());
      CHECK(r.extents.has_value() && plainViews(r));
      if (r.extents)
        CHECK((*r.extents)[0] == 2 && (*r.extents)[1] == 3 && (*r.extents)[2] == 4);
    }
    // D. A may overlap C: with no name for (A, C) the fold is not verified, and the
    // record says which roots are not known apart. Nothing is assumed.
    std::vector<NestRecord> alias = analyze(m, "mmi", {"roots 1 and 2 are separate"}, outline);
    CHECK(alias.size() == 1);
    if (alias.size() == 1) {
      const NestRecord &r = alias[0];
      CHECK(r.status == NestRecord::NotVerified);
      CHECK(!r.orderedFoldVerified);
      CHECK(says(r.refusals, "A (root 0) may overlap C (root 2)"));
      CHECK(says(r.refusals, "the ordered fold is not verified"));
      CHECK(!says(r.assumptions, "roots 0 and 2"));
    }
  }
}

static void testNests() {
  // B and C. Two nests on one output, and a nest that reads the other's output: each is
  // verified by itself, from any memory at its entry.
  {
    std::string text = "func.func @shared(" + arguments({"%a", "%b", "%c", "%d"}) + ") {\n" +
                       gemm("0", "%a", "%b", "%c", 2, 3, 2) + gemm("1", "%a", "%d", "%c", 3, 2, 4) + "  return\n}\n" +
                       "func.func @reads(" + arguments({"%a", "%b", "%c", "%d"}) + ") {\n" +
                       gemm("0", "%a", "%b", "%c", 2, 2, 3) + gemm("1", "%c", "%b", "%d", 2, 3, 2) + "  return\n}\n";
    Module m(text);
    Authorizations all;
    for (unsigned a = 0; a < 4; ++a)
      for (unsigned b = a + 1; b < 4; ++b)
        all.insert(separationName(a, b));
    FunctionOutline outline;
    std::vector<NestRecord> shared = analyze(m, "shared", all, outline);
    CHECK(shared.size() == 2);
    if (shared.size() == 2) {
      CHECK(shared[0].status == NestRecord::Verified && shared[1].status == NestRecord::Verified);
      CHECK(shared[0].nest == 0 && shared[1].nest == 1);
      CHECK(shared[0].views[0].root == 2 && shared[0].views[1].root == 0 && shared[0].views[2].root == 1);
      CHECK(shared[1].views[0].root == 2 && shared[1].views[1].root == 0 && shared[1].views[2].root == 3);
      if (shared[0].extents && shared[1].extents) {
        CHECK((*shared[0].extents)[0] == 2 && (*shared[0].extents)[1] == 3 && (*shared[0].extents)[2] == 2);
        CHECK((*shared[1].extents)[0] == 3 && (*shared[1].extents)[1] == 2 && (*shared[1].extents)[2] == 4);
      }
      // The names used are the function's, though the second copy numbers its roots 0, 1, 2.
      CHECK(says(shared[1].assumptions, "roots 0 and 2 are separate: authorized, not proved, and used"));
      CHECK(says(shared[1].assumptions, "roots 2 and 3 are separate: authorized, not proved, and used"));
      CHECK(!says(shared[1].assumptions, "roots 1 and 2 are separate"));
      llvm::json::Object json = toJSON(outline, shared);
      CHECK(json.getBoolean("authorizes_lowering") == std::optional<bool>(false));
      CHECK(json.getString("schema") == std::optional<llvm::StringRef>("generalized-gemm-nest/2"));
      const llvm::json::Array *nests = json.getArray("nests");
      CHECK(nests && nests->size() == 2);
      const llvm::json::Array *dependences = json.getArray("dependences");
      CHECK(dependences && dependences->size() == 1);
    }
    // Names over the second copy's own roots are no names of the function: with only
    // them, (A, C) = (0, 2) holds by luck and (D, C) = (3, 2) is not known.
    std::vector<NestRecord> wrong =
        analyze(m, "shared", {"roots 0 and 2 are separate", "roots 1 and 2 are separate"}, outline);
    CHECK(wrong.size() == 2);
    if (wrong.size() == 2) {
      CHECK(wrong[0].status == NestRecord::Verified);
      CHECK(wrong[1].status == NestRecord::NotVerified);
      CHECK(says(wrong[1].refusals, "B (root 3) may overlap C (root 2)"));
    }
    std::vector<NestRecord> reads = analyze(m, "reads", all, outline);
    CHECK(reads.size() == 2);
    if (reads.size() == 2) {
      CHECK(reads[0].status == NestRecord::Verified && reads[1].status == NestRecord::Verified);
      CHECK(reads[1].views[0].root == 3 && reads[1].views[1].root == 2 && reads[1].views[2].root == 1);
      // The later nest reads C, which the earlier one writes: listed, and nothing more.
      bool flow = false;
      for (const NestDependence &dependence : outline.dependences)
        flow = flow || (dependence.relation == NestDependence::SameRoot && dependence.earlierRoot == 2 &&
                        (dependence.earlierAccess & NestWrites) && dependence.laterAccess == NestReads);
      CHECK(flow);
    }
  }
}

static void testRefusals() {
  // D. A bound that is an argument; a view that is not affine; a view with an offset and
  // a scale; a root allocated in the function; a loop that carries a value.
  {
    Module m(R"mlir(
func.func @symbolic(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %j] : memref<8x8xi32>
        %z = memref.load %c[%i, %j] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @square(%a: memref<8x4xi32>, %b: memref<4x3xi32>, %c: memref<3x3xi32>) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 4 {
        %q = arith.muli %i, %i : index
        %x = memref.load %a[%q, %k] : memref<8x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x3xi32>
        %z = affine.load %c[%i, %j] : memref<3x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<3x3xi32>
      }
    }
  }
  return
}
func.func @offsets(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>) {
  affine.for %i = 1 to 3 {
    affine.for %j = 0 to 3 {
      affine.for %k = 2 to 4 {
        %x = affine.load %a[%i + 1, %k] : memref<8x8xi32>
        %y = affine.load %b[%k * 2, %j] : memref<8x8xi32>
        %z = affine.load %c[%i, %j + 2] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j + 2] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @fresh(%a: memref<3x4xi32>, %b: memref<4x3xi32>, %out: memref<3x3xi32>) {
  %c = memref.alloc() : memref<3x3xi32>
  %zero = arith.constant 0 : i32
  affine.for %i0 = 0 to 3 {
    affine.for %j0 = 0 to 3 {
      affine.store %zero, %c[%i0, %j0] : memref<3x3xi32>
    }
  }
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 4 {
        %x = affine.load %a[%i, %k] : memref<3x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x3xi32>
        %z = affine.load %c[%i, %j] : memref<3x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<3x3xi32>
      }
    }
  }
  affine.for %i2 = 0 to 3 {
    affine.for %j2 = 0 to 3 {
      %v = affine.load %c[%i2, %j2] : memref<3x3xi32>
      affine.store %v, %out[%i2, %j2] : memref<3x3xi32>
    }
  }
  return
}
func.func @carried(%a: memref<4xi32>, %b: memref<4xi32>) -> i32 {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  %sum = scf.for %k = %c0 to %c4 step %c1 iter_args(%acc = %zero) -> (i32) {
    %x = memref.load %a[%k] : memref<4xi32>
    %y = memref.load %b[%k] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %acc, %m : i32
    scf.yield %s : i32
  }
  return %sum : i32
}
)mlir");
    FunctionOutline outline;
    NestAnalysisOptions scoped = testOptions();
    scoped.scope.push_back({3, 1, 2});
    std::vector<NestRecord> symbolic = analyze(m, "symbolic", abc, outline, scoped);
    CHECK(symbolic.size() == 1);
    if (symbolic.size() == 1) {
      const NestRecord &r = symbolic[0];
      // Verified, and no box for a partitioner: M is not a constant.
      CHECK(r.status == NestRecord::VerifiedNotMapped);
      CHECK(r.orderedFoldVerified);
      CHECK(!r.extents.has_value());
      CHECK(r.box);
      CHECK(says(r.refusals, "no constant extent: the upper bound of level 0 (axis I)"));
      CHECK(says(r.refusals, "argument 3 of the function"));
      CHECK(r.refusals.size() == 1);
      CHECK(says(r.assumptions, "argument 3 of the function is in [1, 2]"));
      CHECK(r.scalars.size() == 1 && r.scalars[0].argument == 3);
      CHECK(r.levels.size() == 3 && !r.levels[0].upper && r.levels[1].extent && *r.levels[1].extent == 3);
    }
    std::vector<NestRecord> square = analyze(m, "square", abc, outline);
    CHECK(square.size() == 1);
    if (square.size() == 1) {
      const NestRecord &r = square[0];
      CHECK(r.status == NestRecord::VerifiedNotMapped);
      CHECK(r.orderedFoldVerified);
      CHECK(says(r.refusals, "dimension 0 of the view of A is not affine"));
      CHECK(!r.views[1].affine && r.views[0].affine && r.views[2].affine);
      CHECK(r.views[1].dimensions.size() == 2 && !r.views[1].dimensions[0].affine &&
            r.views[1].dimensions[1].affine);
    }
    std::vector<NestRecord> offsets = analyze(m, "offsets", abc, outline);
    CHECK(offsets.size() == 1);
    if (offsets.size() == 1) {
      const NestRecord &r = offsets[0];
      CHECK(r.status == NestRecord::Verified);
      if (r.extents)
        CHECK((*r.extents)[0] == 2 && (*r.extents)[1] == 3 && (*r.extents)[2] == 2);
      CHECK(r.levels.size() == 3);
      if (r.levels.size() == 3)
        CHECK(*r.levels[0].lower == 1 && *r.levels[1].lower == 0 && *r.levels[2].lower == 2);
      // Over the level values: A[i + 1, k], B[2k, j], C[i, j + 2]. Over coordinates from
      // zero, with i = I + 1 and k = K + 2: A[I + 2, K + 2], B[2K + 4, J], C[I + 1, J + 2].
      auto dimension = [&](unsigned view, unsigned d) -> const NestViewDimension & {
        static NestViewDimension none;
        return r.views[view].dimensions.size() > d ? r.views[view].dimensions[d] : none;
      };
      auto is = [&](unsigned view, unsigned d, int64_t constant, char role, int64_t scale, int64_t offset) {
        const NestViewDimension &one = dimension(view, d);
        return one.affine && one.constant == constant && one.roles.size() == 1 && one.roles[0].first == role &&
               one.roles[0].second == scale && one.offset && *one.offset == offset && one.levels.size() == 1 &&
               one.levels[0].second == scale;
      };
      CHECK(is(1, 0, 1, 'I', 1, 2));
      CHECK(is(1, 1, 0, 'K', 1, 2));
      CHECK(is(2, 0, 0, 'K', 2, 4));
      CHECK(is(2, 1, 0, 'J', 1, 0));
      CHECK(is(0, 0, 0, 'I', 1, 1));
      CHECK(is(0, 1, 2, 'J', 1, 2));
    }
    // A root allocated in the function is apart from the operands by a proof: nothing
    // is authorized, and the record carries no assumption about roots.
    std::vector<NestRecord> fresh = analyze(m, "fresh", {}, outline);
    CHECK(fresh.size() == 3);
    if (fresh.size() == 3) {
      CHECK(fresh[0].status == NestRecord::NoGemm && fresh[2].status == NestRecord::NoGemm);
      CHECK(says(fresh[0].refusals, "no multiply-accumulate site"));
      const NestRecord &r = fresh[1];
      CHECK(r.status == NestRecord::Verified);
      CHECK(r.views[0].root == 3 && r.views[1].root == 0 && r.views[2].root == 1);
      CHECK(!says(r.assumptions, "are separate"));
      unsigned proofs = 0, used = 0;
      for (unsigned s = 0; s < r.separations.size(); ++s) {
        const NestSeparation &separation = r.separations[s];
        if (separation.second == 3) {
          CHECK(separation.apart && separation.proof && separation.basis == "fresh allocation");
          ++proofs;
          used += r.separationUsed[s];
        } else {
          CHECK(!separation.apart); // A against B: both read, and nobody said.
        }
      }
      CHECK(proofs == 2 && used == 2);
      CHECK(outline.roots.size() == 4 && outline.roots[3].origin == FunctionRoot::FreshAllocation);
      // The initialisation, the product and the copy out meet on the allocated root. The
      // copy out writes an argument, which is not known apart from the two the product
      // reads: listed too, since nobody said they are apart.
      unsigned onAllocated = 0, notKnownApart = 0;
      for (const NestDependence &dependence : outline.dependences) {
        onAllocated += dependence.relation == NestDependence::SameRoot && dependence.earlierRoot == 3 &&
                       dependence.laterRoot == 3;
        notKnownApart += dependence.relation == NestDependence::MayAlias && dependence.earlier == 1 &&
                         dependence.later == 2 && dependence.laterRoot == 2 &&
                         dependence.laterAccess == NestWrites && dependence.earlierAccess == NestReads;
      }
      CHECK(onAllocated == 3 && notKnownApart == 2);
      CHECK(outline.dependences.size() == 5);
    }
    std::vector<NestRecord> carried = analyze(m, "carried", {}, outline);
    CHECK(carried.size() == 1);
    if (carried.size() == 1) {
      const NestRecord &r = carried[0];
      CHECK(r.status == NestRecord::Unsupported);
      CHECK(!r.orderedFoldVerified && !r.reassociationPermitted);
      CHECK(says(r.refusals, "the loop yields a value that is used after it"));
      CHECK(r.outlinedName.empty() && !r.candidate);
      llvm::json::Object json = toJSON(r, outline.roots);
      CHECK(json.getBoolean("authorizes_lowering") == std::optional<bool>(false));
      CHECK(json.get("gemm") && json.get("gemm")->getAsNull().has_value());
    }
  }}

static void testEffects() {
  // What a nest does beside the fold, and what its source does not permit.
  {
    Module m(R"mlir(
// The output set to zero inside the nest, before the reduction loop.
func.func @zeroed(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>) {
  %zero = arith.constant 0 : i32
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 3 {
      affine.store %zero, %c[%i, %j] : memref<4x4xi32>
      affine.for %k = 0 to 2 {
        %x = affine.load %a[%i, %k] : memref<4x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x4xi32>
        %z = affine.load %c[%i, %j] : memref<4x4xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<4x4xi32>
      }
    }
  }
  return
}
// Another write in the nest, to an array of its own.
func.func @extra(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>, %e: memref<4x4xi32>) {
  %five = arith.constant 5 : i32
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 3 {
      affine.store %five, %e[%i, %j] : memref<4x4xi32>
      affine.for %k = 0 to 2 {
        %x = affine.load %a[%i, %k] : memref<4x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x4xi32>
        %z = affine.load %c[%i, %j] : memref<4x4xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<4x4xi32>
      }
    }
  }
  return
}
// A flagged integer addition.
func.func @nsw(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 2 {
        %x = affine.load %a[%i, %k] : memref<4x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x4xi32>
        %z = affine.load %c[%i, %j] : memref<4x4xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m overflow<nsw> : i32
        affine.store %s, %c[%i, %j] : memref<4x4xi32>
      }
    }
  }
  return
}
// Two logical outputs on one cell: C[i + j].
func.func @diagonal(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<8xi32>) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 2 {
        %x = affine.load %a[%i, %k] : memref<4x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x4xi32>
        %z = affine.load %c[%i + %j] : memref<8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i + %j] : memref<8xi32>
      }
    }
  }
  return
}
// Two products in one body.
func.func @twosites(%a: memref<4x4xi32>, %b: memref<4x4xi32>, %c: memref<4x4xi32>, %d: memref<4x4xi32>) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 2 {
        %x = affine.load %a[%i, %k] : memref<4x4xi32>
        %y = affine.load %b[%k, %j] : memref<4x4xi32>
        %z = affine.load %c[%i, %j] : memref<4x4xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<4x4xi32>
        %z2 = affine.load %d[%i, %j] : memref<4x4xi32>
        %s2 = arith.addi %z2, %m : i32
        affine.store %s2, %d[%i, %j] : memref<4x4xi32>
      }
    }
  }
  return
}
)mlir");
    Authorizations all;
    for (unsigned a = 0; a < 4; ++a)
      for (unsigned b = a + 1; b < 4; ++b)
        all.insert(separationName(a, b));
    FunctionOutline outline;
    // The output set to zero inside the nest: an assignment, and no fold from the value
    // at the nest's entry. The claim holds at the points run and is not proved over the
    // scope, so it is no fact of the record, and neither is the integer fact beside it.
    std::vector<NestRecord> zeroed = analyze(m, "zeroed", all, outline);
    CHECK(zeroed.size() == 1);
    if (zeroed.size() == 1) {
      const NestRecord &r = zeroed[0];
      CHECK(r.status == NestRecord::NotVerified);
      CHECK(!r.orderedFoldVerified);
      CHECK(says(r.refusals, "the ordered fold is verified at POINT only, and not over the scope"));
      CHECK(!r.reassociationPermitted && r.reassociationBasis == "none");
    }
    // Another write in the nest, verified apart from the fold's cells: the fold is the
    // group's, and the nest is more than the fold.
    std::vector<NestRecord> extra = analyze(m, "extra", all, outline);
    CHECK(extra.size() == 1);
    if (extra.size() == 1) {
      const NestRecord &r = extra[0];
      CHECK(r.status == NestRecord::NotVerified);
      CHECK(!r.orderedFoldVerified);
      CHECK(r.otherWrites == 1);
      CHECK(says(r.refusals, "the nest writes memory outside the multiply-accumulate: 1 write(s)"));
      CHECK(r.refusals.size() == 1);
    }
    // A flagged integer addition: the fold is verified for the runs that complete, and
    // another order may overflow where this one does not, so none is permitted.
    std::vector<NestRecord> nsw = analyze(m, "nsw", all, outline);
    CHECK(nsw.size() == 1);
    if (nsw.size() == 1) {
      const NestRecord &r = nsw[0];
      CHECK(r.status == NestRecord::Verified);
      CHECK(r.orderedFoldVerified);
      CHECK(!r.reassociationPermitted && r.reassociationBasis == "none");
      CHECK(!r.sourceFlagReassoc.has_value());
      CHECK(r.reductionFlags == FlagNSW);
      CHECK(says(r.assumptions, "the claims are about the runs of the nest that complete"));
    }
    // Two logical outputs on one cell, C[i + j]: a verified fold, and no M x N output.
    std::vector<NestRecord> diagonal = analyze(m, "diagonal", all, outline);
    CHECK(diagonal.size() == 1);
    if (diagonal.size() == 1) {
      const NestRecord &r = diagonal[0];
      CHECK(r.status == NestRecord::VerifiedNotMapped);
      CHECK(r.orderedFoldVerified);
      CHECK(says(r.refusals, "two logical outputs may share a cell of C"));
      CHECK(r.refusals.size() == 1);
    }
    // Two products in one body are two groups: no record of one GEMM.
    std::vector<NestRecord> twosites = analyze(m, "twosites", all, outline);
    CHECK(twosites.size() == 1);
    if (twosites.size() == 1) {
      const NestRecord &r = twosites[0];
      CHECK(r.status == NestRecord::NotVerified);
      CHECK(r.sites == 2 && r.groups == 2 && !r.candidate);
      CHECK(says(r.refusals, "the nest has 2 groups of multiply-accumulate sites"));
    }
  }
  // Domains and views the record does not map, each said in its own words.
  {
    Module m(R"mlir(
// A domain with a hole: no box.
func.func @holed(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %c3 step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
        %skip = arith.cmpi eq, %i, %j : index
        scf.if %skip {
        } else {
          %x = memref.load %a[%i, %k] : memref<8x8xi32>
          %y = memref.load %b[%k, %j] : memref<8x8xi32>
          %z = memref.load %c[%i, %j] : memref<8x8xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<8x8xi32>
        }
      }
    }
  }
  return
}
// A view written with a subtraction: A[i - 1, k].
func.func @minus(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>) {
  %c1 = arith.constant 1 : index
  affine.for %i = 1 to 3 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 2 {
        %q = arith.subi %i, %c1 : index
        %x = memref.load %a[%q, %k] : memref<8x8xi32>
        %y = affine.load %b[%k, %j] : memref<8x8xi32>
        %z = affine.load %c[%i, %j] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
// A view written as a multiple of a sum: B[(k + 1) * 2, j].
func.func @scaled(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>) {
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 2 {
        %t = arith.addi %k, %c1 : index
        %u = arith.muli %t, %c2 : index
        %x = affine.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%u, %j] : memref<8x8xi32>
        %z = affine.load %c[%i, %j] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
// A view of A that also reads J: A[i + j, k].
func.func @skewed(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>) {
  affine.for %i = 0 to 2 {
    affine.for %j = 0 to 3 {
      affine.for %k = 0 to 2 {
        %x = affine.load %a[%i + %j, %k] : memref<8x8xi32>
        %y = affine.load %b[%k, %j] : memref<8x8xi32>
        %z = affine.load %c[%i, %j] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        affine.store %s, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
)mlir");
    FunctionOutline outline;
    // A domain with a hole is exact under its guard, and no box.
    std::vector<NestRecord> holed = analyze(m, "holed", abc, outline);
    CHECK(holed.size() == 1);
    if (holed.size() == 1) {
      const NestRecord &r = holed[0];
      CHECK(r.status == NestRecord::VerifiedNotMapped);
      CHECK(r.orderedFoldVerified);
      CHECK(!r.box && !r.extents && r.guard != "true");
      CHECK(says(r.refusals, "the domain has a guard, so it is no box"));
      CHECK(says(r.refusals, "the coordinates I, J and K are not verified over the scope: the logical tier is PROPOSED"));
      CHECK(r.refusals.size() == 2);
    }
    // A view the proposer gives no closed form for: written with a subtraction, and as
    // a multiple of a sum. The fold is verified; the view is not given, and not guessed.
    std::vector<NestRecord> minus = analyze(m, "minus", abc, outline);
    CHECK(minus.size() == 1);
    if (minus.size() == 1) {
      const NestRecord &r = minus[0];
      CHECK(r.status == NestRecord::VerifiedNotMapped);
      CHECK(r.orderedFoldVerified && !r.views[1].proposed && r.views[1].dimensions.empty());
      CHECK(says(r.refusals, "the view of A has no proposed closed form"));
      CHECK(r.refusals.size() == 1);
    }
    std::vector<NestRecord> scaled = analyze(m, "scaled", abc, outline);
    CHECK(scaled.size() == 1);
    if (scaled.size() == 1) {
      const NestRecord &r = scaled[0];
      CHECK(r.status == NestRecord::VerifiedNotMapped);
      CHECK(says(r.refusals, "the view of B has no proposed closed form"));
      CHECK(r.refusals.size() == 1);
    }
    // A[i + j, k]: A reads J, which is no role of A. Whatever family the search settles
    // on, the record says which level a view reads that it may not.
    std::vector<NestRecord> skewed = analyze(m, "skewed", abc, outline);
    CHECK(skewed.size() == 1);
    if (skewed.size() == 1) {
      const NestRecord &r = skewed[0];
      CHECK(r.status == NestRecord::VerifiedNotMapped);
      CHECK(!r.box && !r.extents);
      CHECK(says(r.refusals, "of the view of A reads level 1 (axis J), which is no role of A"));
      CHECK(says(r.refusals, "level(s) of the order are no coordinate of the family"));
      CHECK(says(r.refusals, "a point of the order has more than one event: the multiplicity is UNKNOWN"));
    }
  }
  CHECK(std::string(stringifyNestStatus(NestRecord::Verified)) == "VERIFIED");
  CHECK(std::string(stringifyNestStatus(NestRecord::VerifiedNotMapped)) == "VERIFIED_NOT_MAPPED");
  CHECK(flagNames(FlagReassoc | FlagNNaN, true) == (std::vector<std::string>{"reassoc", "nnan"}));
  CHECK(flagNames(FlagNSW | FlagNUW, false) == (std::vector<std::string>{"nsw", "nuw"}));
}

int main(int argc, char **argv) {
  setvbuf(stdout, nullptr, _IOLBF, 0);
  mlir::MLIRContext local;
  configure(local);
  context = &local;
  std::string only = argc > 1 ? argv[1] : "";
  auto run = [&](const char *name, void (*test)()) {
    if (only.empty() || only == name) {
      auto begin = std::chrono::steady_clock::now();
      test();
      printf("%s: %.1f s, %d checks, %d failures\n", name,
             std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count(), checks, failures);
    }
  };
  run("outline", testOutline);
  run("binding", testBinding);
  run("mapping", testMapping);
  run("composition", testComposition);
  run("product", testProduct);
  run("nests", testNests);
  run("refusals", testRefusals);
  run("effects", testEffects);
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
