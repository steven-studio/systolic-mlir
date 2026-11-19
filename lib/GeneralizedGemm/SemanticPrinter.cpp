#include "GeneralizedGemm/SemanticPrinter.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <map>

using namespace mlir::generalized_gemm;
using llvm::APInt;
using llvm::json::OStream;

namespace {

std::string bits(const APInt &value) {
  llvm::SmallString<32> text;
  value.toStringUnsigned(text, 10);
  return text.str().str();
}
std::string ref(Term t) { return "t" + std::to_string(t->id); }

void termList(OStream &j, llvm::ArrayRef<Term> list) {
  j.array([&] {
    for (Term t : list)
      j.value(ref(t));
  });
}
void assignments(OStream &j, llvm::ArrayRef<std::pair<unsigned, Term>> list) {
  j.array([&] {
    for (const auto &[id, term] : list)
      j.array([&] {
        j.value((int64_t)id);
        j.value(ref(term));
      });
  });
}

void printProgram(OStream &j, const SemanticProgram &p) {
  j.attribute("function", p.functionName);
  j.attribute("fingerprint", p.fingerprint);
  j.attribute("index_bit_width", (int64_t)p.indexBitWidth);
  if (p.unsupported) {
    j.attribute("status", "UNSUPPORTED");
    j.attribute("reason", stringifyUnsupportedKind(p.unsupported->kind));
    j.attribute("detail", p.unsupported->detail);
    return;
  }
  j.attribute("status", "MODELLED");
  j.attribute("structured_lowering", p.structuredLoweringApplied);
  j.attribute("locations", (int64_t)p.locationCount);
  j.attributeArray("parameters", [&] {
    for (const Parameter &x : p.parameters)
      j.object([&] {
        j.attribute("id", (int64_t)x.id);
        j.attribute("kind", x.kind == Parameter::Scalar ? "scalar" : "dimension");
        j.attribute("sort", x.sort.str());
        j.attribute("argument", (int64_t)x.argument);
      });
  });
  j.attributeArray("state", [&] {
    for (const StateVar &x : p.state)
      j.object([&] {
        j.attribute("id", (int64_t)x.id);
        j.attribute("sort", x.sort.str());
        j.attribute("location", (int64_t)x.location);
        j.attribute("loop_carried", x.loopCarried);
      });
  });
  j.attributeArray("roots", [&] {
    for (const MemoryRoot &x : p.roots)
      j.object([&] {
        j.attribute("id", (int64_t)x.id);
        j.attribute("origin", x.origin == MemoryRoot::FunctionArgument
                                  ? "argument" : "allocation");
        j.attribute("sort", x.sort.str());
        j.attribute("restrict", x.restrictArgument);
      });
  });
  j.attributeArray("root_pairs", [&] {
    for (const RootPairStatus &x : p.rootPairs)
      j.object([&] {
        j.attribute("first", (int64_t)x.first);
        j.attribute("second", (int64_t)x.second);
        j.attribute("status", x.separated.name());
        j.attribute("detail", x.separated.detail());
      });
  });
  j.attributeArray("assumptions", [&] {
    for (const ProofStatus &x : p.assumptions)
      j.value(x.detail());
  });
  j.attribute("domain", stringifyDomainForm(importedDomainEvidence().form));
  j.attribute("control_oblivious", p.controlOblivious);
  j.attribute("address_oblivious", p.addressOblivious);

  std::map<unsigned, Term> used;
  std::function<void(Term)> collect = [&](Term t) {
    if (!t || !used.emplace(t->id, t).second)
      return;
    for (Term a : t->args)
      collect(a);
  };
  j.attributeArray("transitions", [&] {
    for (const Transition &t : p.transitions)
      j.object([&] {
        collect(t.guard);
        j.attribute("from", (int64_t)t.from);
        j.attribute("slot", (int64_t)t.slot);
        j.attribute("to", (int64_t)t.to);
        j.attribute("guard", ref(t.guard));
        j.attributeBegin("assign");
        assignments(j, t.assignments);
        j.attributeEnd();
        j.attributeBegin("poison");
        assignments(j, t.shadowAssignments);
        j.attributeEnd();
        j.attributeBegin("memory");
        assignments(j, t.memoryUpdates);
        j.attributeEnd();
        for (const auto &[id, term] : t.assignments) collect(term);
        for (const auto &[id, term] : t.shadowAssignments) collect(term);
        for (const auto &[id, term] : t.memoryUpdates) collect(term);
        j.attributeArray("side_conditions", [&] {
          for (const SideCondition &s : t.sideConditions)
            j.object([&] {
              collect(s.violated);
              j.attribute("kind", stringifySideKind(s.kind));
              j.attribute("reason", s.reason);
              j.attribute("op", (int64_t)s.opIndex);
              j.attribute("term", ref(s.violated));
            });
        });
        j.attributeArray("effects", [&] {
          for (const MemoryEffect &e : t.effects)
            j.object([&] {
              collect(e.value);
              j.attribute("kind", e.kind == MemoryEffect::Read ? "read" : "write");
              j.attribute("root", (int64_t)e.root);
              j.attribute("op", (int64_t)e.opIndex);
              j.attributeBegin("index");
              termList(j, e.index);
              j.attributeEnd();
              for (Term i : e.index) collect(i);
              j.attribute("value", ref(e.value));
            });
        });
        if (!t.returns.empty()) {
          j.attributeBegin("returns");
          termList(j, t.returns);
          j.attributeEnd();
          for (Term r : t.returns) collect(r);
        }
      });
  });
  j.attributeArray("sites", [&] {
    for (const MacSite &s : p.sites)
      j.object([&] {
        j.attribute("id", (int64_t)s.id);
        j.attribute("location", (int64_t)s.location);
        j.attribute("form", s.form == ReductionForm::Direct ? "direct"
                                                            : "scalar_accumulator");
        j.attribute("reduction", s.reduction == ReductionOp::FAdd ? "fadd" : "add");
        j.attribute("reduction_flags", (int64_t)s.reductionFlags);
        j.attribute("multiply_flags", (int64_t)s.multiplyFlags);
        j.attribute("oblivious", s.oblivious);
        j.attributeArray("coordinates", [&] {
          for (unsigned c : s.coordinates)
            j.value((int64_t)c);
        });
        auto access = [&](const char *key, const MacAccess &a) {
          j.attributeObject(key, [&] {
            j.attribute("root", (int64_t)a.root);
            j.attributeBegin("index");
            termList(j, a.index);
            j.attributeEnd();
            for (Term i : a.index) collect(i);
          });
        };
        access("a", s.a);
        access("b", s.b);
        if (s.output) access("output", *s.output);
        if (s.outputRead) access("output_read", *s.outputRead);
        if (s.accumulator)
          j.attribute("accumulator", (int64_t)*s.accumulator);
      });
  });
  j.attributeArray("terms", [&] {
    for (const auto &[id, term] : used)
      j.value(ref(term) + " = " + describeNode(term));
  });
}

void printRun(OStream &j, const RunResult &run) {
  j.attribute("run_status", stringifyRunStatus(run.status));
  j.attribute("run_reason", run.reason);
  j.attribute("steps", (int64_t)run.steps);
  j.attribute("strict_refinement_of_permissive_float",
              run.strictRefinementOfPermissiveFloat);
  j.attributeArray("returns", [&] {
    for (unsigned i = 0; i < run.returns.size(); ++i)
      j.value(run.returnPoison[i] ? std::string("poison") : bits(run.returns[i]));
  });
  j.attributeArray("events", [&] {
    for (const MacEvent &e : run.events)
      j.object([&] {
        j.attribute("site", (int64_t)e.site);
        j.attribute("step", (int64_t)e.step);
        auto list = [&](const char *key, const std::vector<APInt> &values) {
          j.attributeArray(key, [&] {
            for (const APInt &v : values)
              j.value(bits(v));
          });
        };
        list("x", e.coordinates);
        list("f", e.f);
        list("g", e.g);
        list("h", e.h);
        list("h_read", e.hRead);
        j.attribute("a", bits(e.aValue));
        j.attribute("b", bits(e.bValue));
        j.attribute("update", bits(e.updateValue));
      });
  });
  j.attributeArray("memory", [&] {
    for (const RootData &root : run.memory)
      j.array([&] {
        for (const auto &cell : root.cells) {
          if (cell)
            j.value(bits(*cell));
          else
            j.value(nullptr);
        }
      });
  });
}

} // namespace

namespace mlir::generalized_gemm {

std::string toJSON(const SemanticProgram &program) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  OStream j(stream, 1);
  j.object([&] { printProgram(j, program); });
  stream.flush();
  return text;
}

std::string toJSON(const SemanticProgram &program, const RunResult &run) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  OStream j(stream);
  j.object([&] {
    j.attribute("function", program.functionName);
    printRun(j, run);
  });
  stream.flush();
  return text;
}

} // namespace mlir::generalized_gemm
