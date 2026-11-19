#!/usr/bin/env python3
"""Stages 2 and 3 mutation test (docs/GENERALIZED_GEMM_SCOPE.md): the production gate on records
of loop nests, and the lowering from records. Each mutant weakens one check of
lib/Systolic/Transforms/NestRecordGate.cpp, of NestRecordLowering.cpp, of the pass's entry in
AffineMatmulToSystolic.cpp, of the records mode and the emission in
GeneralizedMatmulToSystolic.cpp, or of the binding in NestBinding.cpp, in a copy of the production
sources built in a scratch directory; the copy unmodified must pass first. The tests are the
production tests of the gate and of the lowering from records: test/Systolic/nest_records.mlir,
nest_records_refusals.mlir, nest_records_belts.mlir, nest_records_driven.mlir,
nest_records_driven_refusals.mlir, for scf.for nests nest_records_scf.mlir and
nest_records_scf_refusals.mlir, for scf.while nests nest_records_while.mlir and
nest_records_while_refusals.mlir, and for nests whose loops carry their subscripts
nest_records_carried.mlir, nest_records_carried_refusals.mlir and nest_records_carried_tables.mlir. Every result is one of:

  KILLED          a test failed by its own check (FileCheck), and nothing crashed.
  SURVIVED        both tests passed.
  EQUIVALENT      not run: listed below with the reason.
  HARNESS_ERROR   a stale pattern, a mutant that does not build, a crash.

Only KILLED and EQUIVALENT let a run pass. Nothing in the source tree is changed: the copy is
taken once, each mutant changes one file of it and is undone after. Results are appended to
--results with the hash of the files they are about; a run skips what that file already holds.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys

G = "lib/Systolic/Transforms/NestRecordGate.cpp"
P = "lib/Systolic/Transforms/AffineMatmulToSystolic.cpp"
R = "lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp"
B = "lib/Systolic/Transforms/NestBinding.cpp"
L = "lib/Systolic/Transforms/NestRecordLowering.cpp"

MUTANTS = [
    # Reading the file.
    ("the format not checked", G, "    if (!schema || *schema != kNestRecordSchema) {\n", "    if (!schema) {\n"),
    ("two objects of one function read", G, "    if (!out.functions.try_emplace(*name, function).second) {\n",
     "    if (!out.functions.try_emplace(*name, function).second && false) {\n"),
    ("a nest with no number read", G, "      if (!ordinal || *ordinal < 0) {\n", "      if (!ordinal && false) {\n"),
    ("two records of one nest read", G, "      if (!byOrdinal.try_emplace((unsigned)*ordinal, nest).second) {\n",
     "      if (!byOrdinal.try_emplace((unsigned)*ordinal, nest).second && false) {\n"),
    # The record: about this nest of this input.
    ("the number of roots not compared", G, "  if (recordRoots->size() != roots->roots.size())\n",
     "  if (recordRoots->size() < roots->roots.size())\n"),
    ("a root of the function not compared", G,
     "    if (!same)\n      return refuse(\"root \" + std::to_string(id) +",
     "    if (false)\n      return refuse(\"root \" + std::to_string(id) +"),
    ("a claim to authorize read", G, "  if (*functionAuthorizes || *authorizes)\n", "  if (false)\n"),
    ("the binding's scheme not compared", G, "  if (*scheme != kNestBindingScheme)\n", "  if (false)\n"),
    ("the binding not compared", G, "  if (*sha != nest->binding->sha256)\n", "  if (false)\n"),
    ("the index width not compared", G, "  if (*width != (int64_t)indexWidth)\n", "  if (false)\n"),
    # The record: complete and verified.
    ("the status not read", G, "  if (*status != \"VERIFIED\" && !viewsUnmapped) {\n", "  if (false) {\n"),
    ("the ordered fold not read", G, "  if (!*orderedFold)\n", "  if (false)\n"),
    ("the refusals not read", G, "  if (!refusals->empty() && !viewsUnmapped)\n    return refuse(\"the record has refusals\");\n",
     "  if (false)\n    return refuse(\"the record has refusals\");\n"),
    # Stage 3: a record VERIFIED but for the affine closed forms of its views.
    ("unmapped views read without record-driven", G,
     "  if (unmappedViews && *status == \"VERIFIED_NOT_MAPPED\" && !refusals->empty()) {\n",
     "  if (*status == \"VERIFIED_NOT_MAPPED\" && !refusals->empty()) {\n"),
    ("a refusal of another kind taken for an unmapped view", G,
     "      if (!text || !isUnmappedViewRefusal(*text))\n        viewsUnmapped = false;\n",
     "      if (!text)\n        viewsUnmapped = false;\n"),
    ("no proposed closed form not recognized", G,
     "    if (text == (\"the view of \" + name + \" has no proposed closed form\").str())\n      return true;\n",
     "    if (false)\n      return true;\n"),
    ("a dimension that is not affine not recognized", G,
     "    if (rest.consume_front((\" of the view of \" + name + \" is not affine: \").str()))\n      return true;\n",
     "    if (false)\n      return true;\n"),
    ("any refusal taken for an unmapped view", G, "  return false;\n}\n\n/// The research tool's names of the fast-math flags",
     "  return true;\n}\n\n/// The research tool's names of the fast-math flags"),
    ("the scope not read", G, "  if (!scope->empty())\n", "  if (false)\n"),
    ("Theorem 13's tier not read", G, "  if (*t13Status != \"VERIFIED\" || *t13Tier != \"SYMBOLIC\" || *t13Logical != \"SYMBOLIC\")\n",
     "  if (*t13Status != \"VERIFIED\")\n"),
    ("the frame not read", G, "  if (*otherWrites != 0)\n", "  if (false)\n"),
    ("a domain that is no box read", G, "  if (!*box || *guard != \"true\")\n", "  if (false)\n"),
    # The record: in agreement with the recognizer.
    ("M, N and K not compared", G, "    if (*extent != dense->extent[axis])\n", "    if (false)\n"),
    ("the loop of a coordinate not compared", G, "    if (!level || *level != (int64_t)dense->sourceDim[axis])\n",
     "    if (!level)\n"),
    ("the number of levels not compared", G, "  if (levels->size() != 3)\n", "  if (levels->size() < 3)\n"),
    ("a level not compared", G,
     "    if (expected >= 3 || *axis != kAxisName[expected] || *descending ||\n"
     "        *lower != band[l].getConstantLowerBound() || *upper != band[l].getConstantUpperBound())\n",
     "    if (expected >= 3)\n"),
    ("an operand's root not compared", G, "    if (*rootNumber != (int64_t)rootOf[index])\n", "    if (false)\n"),
    ("a view not proved read", G, "    if (!*proved)\n", "    if (false)\n"),
    ("the number of dimensions not compared", G, "    if (dimensions->size() != 2)\n", "    if (dimensions->size() < 2)\n"),
    ("a dimension of a view not compared", G,
     "          *level != (int64_t)dense->sourceDim[operand.roles[role]] || *coefficient != mine.scale ||\n"
     "          *constant != mine.offset)\n",
     "          *level != (int64_t)dense->sourceDim[operand.roles[role]])\n"),
    ("the element type not compared", G, "  if (*element != myElement)\n", "  if (false)\n"),
    ("the update's operations not compared", G, "  if (*reduction != \"fadd\" || *multiply != \"fmul\" || *sites != 1 || !*sitesAgree)\n",
     "  if (false)\n"),
    ("the accumulator's place not compared", G, "  if (*first != accumulatorFirst)\n", "  if (false)\n"),
    ("the flags not compared", G, "  if (recordReduction != myReduction || recordMultiply != myMultiply)\n", "  if (false)\n"),
    ("a flag that is no string read", G, "  if (!wellFormed)\n    return refuse(\"the record is incomplete: a flag is no string\");\n",
     "  if (false)\n    return refuse(\"the record is incomplete: a flag is no string\");\n"),
    ("the source's flag not compared", G, "  if (!sourceFlag || *sourceFlag != reassoc)\n", "  if (!sourceFlag)\n"),
    ("a separation the record does not use read", G, "    if (!*used)\n      continue;\n", "    if (false)\n      continue;\n"),
    ("a proof between two arguments read", G, "      if (roots->roots[*a].argument && roots->roots[*b].argument)\n",
     "      if (false)\n"),
    ("a separation of no known kind read", G, "    if (*kind != \"assumption\")\n", "    if (false)\n"),
    ("an assumption the alias gate does not check read", G,
     "    if (!is(rootOf[0], rootOf[2]) && !is(rootOf[1], rootOf[2]))\n", "    if (false)\n"),
    # The record: about a nest of the shape it describes (nest_records_belts.mlir).
    ("batch loops read", G,
     "  if (dense->numBatchDims || !matmul.demotedLoops.empty() || !matmul.reductionBatchLoops.empty())\n",
     "  if (false)\n"),
    ("an assignment read", G, "  if (!matmul.accumulates)\n", "  if (false)\n"),
    ("a band of other than three loops read", G, "  if (band.size() != 3)\n", "  if (false)\n"),
    ("bounds that are no constants, or a step other than 1, read", G,
     "    if (!loop.hasConstantLowerBound() || !loop.hasConstantUpperBound() || loop.getStepAsInt() != 1)\n",
     "    if (false)\n"),
    ("a coordinate that is no loop's counter read", G, "    if (!unit)\n", "    if (false)\n"),
    ("a view the record does not compare read", G,
     "    if (layout.general || layout.roleDim[0] < 0 || layout.roleDim[1] < 0 || type.getRank() != 2 ||\n"
     "        layout.hasBatchTerms(0) || layout.hasBatchTerms(1) || operand.symbolic)\n",
     "    if (false)\n"),
    # The pass's entry (P) and the records mode it hands over to (R).
    ("the option not read", P, "    if (!nestRecords.empty()) {\n      runWithNestRecords();\n",
     "    if (false) {\n      runWithNestRecords();\n"),
    ("the gate's refusal ignored", R, "      if (!decision.agrees) {\n", "      if (false) {\n"),
    ("the other paths taken with records", R,
     "    if (generalAffine || kernelCosetOffload || certifiedOffloadFirst || commonFiberOffload || fpDistribute) {\n",
     "    if (false) {\n"),
    ("the index width taken as 16", R,
     "        DataLayout(module).getTypeSizeInBits(IndexType::get(module.getContext())).getFixedValue();\n",
     "        16;\n"),
    # Stage 3: the option (the pass's entry and the mode), the alias gate, the floating-point
    # policy, the emission.
    ("record-driven not read", R, "    if (recordDriven)\n      module.walk(", "    if (false)\n      module.walk("),
    ("record-driven taken without records", P, "    if (recordDriven && nestRecords.empty()) {\n", "    if (false) {\n"),
    ("the alias gate of the lowering from records not read", R,
     "      if (!provablyDisjointFromOutput(operand, C, assumeNoAlias)) {\n", "      if (false) {\n"),
    ("the floating-point policy of the lowering from records not read", R,
     "    if (!g.reassoc && !allowFpReassociation) {\n", "    if (false) {\n"),
    ("the table budget option ignored", R, "*records, bound, loop, indexWidth, recordTableBudget, why);",
     "*records, bound, loop, indexWidth, mlir::systolic::kDefaultRecordTableBudget, why);"),
    ("the operands not gathered", R, "        copy(x, /*scatter=*/false);\n", "        (void)x;\n"),
    ("C not scattered back", R, "      copy(2, /*scatter=*/true);\n", ""),
    # Stage 3, the nest's own checks (NestRecordLowering.cpp).
    ("an affine.for with bounds that are no constants or a step read", L,
     "      if (!a.hasConstantLowerBound() || !a.hasConstantUpperBound() || a.getStepAsInt() != 1)\n",
     "      if (false)\n"),
    # scf.for bands.
    ("a loop carrying values not refused first", L, "    } else if (op->getNumResults() != 0) {\n",
     "    } else if (false) {\n"),
    ("an scf.for not taken for a loop of the band", L,
     "    } else if (auto f = dyn_cast<scf::ForOp>(op)) {\n      std::optional<int64_t> lo =",
     "    } else if (auto f = dyn_cast<scf::ForOp>(op); false) {\n      std::optional<int64_t> lo ="),
    ("an scf.for's lower bound not read", L,
     "      std::optional<int64_t> lo = getConstantIntValue(f.getLowerBound());\n",
     "      std::optional<int64_t> lo = std::optional<int64_t>(0);\n"),
    ("an scf.for's upper bound not read", L,
     "      std::optional<int64_t> hi = getConstantIntValue(f.getUpperBound());\n",
     "      std::optional<int64_t> hi = getConstantIntValue(f.getLowerBound());\n"),
    ("an scf.for step other than 1 read", L, "      if (!step || *step != 1)\n", "      if (!step)\n"),
    ("an unsigned scf.for from a negative bound read", L,
     "      if (f.getUnsignedCmp() && (*lo < 0 || *hi < 0))\n", "      if (f.getUnsignedCmp() && *hi < 0)\n"),
    ("an unsigned scf.for to a negative bound read", L,
     "      if (f.getUnsignedCmp() && (*lo < 0 || *hi < 0))\n", "      if (f.getUnsignedCmp() && *lo < 0)\n"),
    ("an scf.for's counter not read", L, "      loop.counter = f.getInductionVar();\n", "      loop.counter = Value();\n"),
    ("a constant between two loops refused", L,
     "      } else if (!isa<arith::ConstantOp>(child) && !loop.steps(&child)) {\n",
     "      } else if (!loop.steps(&child)) {\n"),
    ("anything between two loops taken for a constant", L,
     "      } else if (!isa<arith::ConstantOp>(child) && !loop.steps(&child)) {\n",
     "      } else if (false) {\n"),
    # scf.while bands: the counting loop (readCountingWhile), what it carries, where it sits.
    ("an scf.while's other carried values not judged first", L, "      if (w.getInits().size() != 1)\n",
     "      if (false)\n"),
    ("a carried index's start left out of the tables", L,
     "      std::optional<int64_t> value = form.constant;\n", "      std::optional<int64_t> value = int64_t(0);\n"),
    ("a carried index's step left out of the tables", L,
     "        value = term ? llvm::checkedAdd(*value, *term) : std::nullopt;\n",
     "        value = term ? value : std::nullopt;\n"),
    ("a carried index left at its first value by the tables", L, "      values[at] = *value;\n",
     "      values[at] = form.constant;\n"),
    ("an scf.while not taken for a loop of the band", L, "    } else if (auto w = dyn_cast<scf::WhileOp>(op)) {\n",
     "    } else if (auto w = dyn_cast<scf::WhileOp>(op); false) {\n"),
    ("the step of an scf.while's counter taken for more than the loop's own", L,
     "      } else if (!isa<arith::ConstantOp>(child) && !loop.steps(&child)) {\n",
     "      } else if (!isa<arith::ConstantOp>(child)) {\n"),
    ("an scf.while's counter not read", L, "      loop.counter = counting->counter;\n", "      loop.counter = Value();\n"),
    ("an scf.while's lower bound not read", L, "      loop.lower = counting->lower;\n", "      loop.lower = 0;\n"),
    ("an scf.while's upper bound not read", L, "      loop.upper = counting->upper;\n",
     "      loop.upper = counting->lower;\n"),
    ("an scf.while's before region read for its body", L, "      loop.body = w.getAfterBody();\n",
     "      loop.body = w.getBeforeBody();\n"),
    ("the before region's argument read for the body's counter", L, "  out.counter = inBody;\n",
     "  out.counter = counter;\n"),
    ("a counter that is no index read", L, "  if (!counter.getType().isIndex()) {\n", "  if (false) {\n"),
    ("more than a comparison in the before region read", L, "    if (!cmp || compare)\n", "    if (false)\n"),
    ("a comparison that is not the loop's condition read", L,
     "  if (!compare || condition.getCondition() != compare.getResult())\n", "  if (!compare)\n"),
    ("a counter not handed to the body read", L,
     "  if (condition.getArgs().size() != count || condition.getArgs()[c] != counter)\n",
     "  if (condition.getArgs().size() != count)\n"),
    ("a comparison other than counter < bound read", L,
     "      (predicate == arith::CmpIPredicate::slt || predicate == arith::CmpIPredicate::ult)) {\n", "      true) {\n"),
    ("a comparison other than bound > counter read", L,
     "             (predicate == arith::CmpIPredicate::sgt || predicate == arith::CmpIPredicate::ugt)) {\n",
     "             true) {\n"),
    ("counter == bound read for counter != bound", L,
     "  } else if (predicate == arith::CmpIPredicate::ne && (compare.getLhs() == counter) != (compare.getRhs() == counter)) {\n",
     "  } else if (predicate == arith::CmpIPredicate::eq && (compare.getLhs() == counter) != (compare.getRhs() == counter)) {\n"),
    ("an unsigned scf.while from a negative start read", L,
     "  if (isUnsigned && (*lower < 0 || *upper < 0))\n", "  if (isUnsigned && *upper < 0)\n"),
    ("an unsigned scf.while to a negative bound read", L,
     "  if (isUnsigned && (*lower < 0 || *upper < 0))\n", "  if (isUnsigned && *lower < 0)\n"),
    ("a counter != bound loop from above its bound read", L, "  if (isNotEqual && *lower > *upper)\n", "  if (false)\n"),
    ("a step other than counter + 1 read", L, "  if (!plusOne)\n", "  if (false)\n"),
    ("a step by another constant read", L,
     "  bool plusOne = add && ((add.getLhs() == inBody && getConstantIntValue(add.getRhs()) == std::optional<int64_t>(1)) ||\n",
     "  bool plusOne = add && ((add.getLhs() == inBody && getConstantIntValue(add.getRhs())) ||\n"),
    ("a step by another constant read, the constant first", L,
     "                         (add.getRhs() == inBody && getConstantIntValue(add.getLhs()) == std::optional<int64_t>(1)));\n",
     "                         (add.getRhs() == inBody && getConstantIntValue(add.getLhs())));\n"),
    ("a float carried value not said to be a reduction", L, "    } else if (!type.isIntOrIndex()) {\n",
     "    } else if (false) {\n"),
    ("a value carried unchanged not said to be", L, "    if (inPlace && next == after->getArgument(n)) {\n",
     "    if (false) {\n"),
    ("an scf.while with no carried value not said to have no counter", L, "  if (loop.getInits().empty())\n",
     "  if (false)\n"),
    # The indices a loop carries (IndexRecurrence): read from the loop, never from the record.
    ("a carried value that is no index read as a recurrence", L,
     "  if (!inBody.getType().isIndex()) {\n    std::string type;\n    llvm::raw_string_ostream(type) << inBody.getType();\n"
     "    why = value + \" has type \"",
     "  if (false) {\n    std::string type;\n    llvm::raw_string_ostream(type) << inBody.getType();\n"
     "    why = value + \" has type \""),
    ("a recurrence started at no constant read as started at 2", L,
     "  std::optional<int64_t> start = getConstantIntValue(init);\n",
     "  std::optional<int64_t> start = getConstantIntValue(init).value_or(2);\n"),
    ("a recurrence stepped by no constant read as stepped by 1", L,
     "    if (add.getLhs() == carried)\n      return getConstantIntValue(add.getRhs());\n",
     "    if (add.getLhs() == carried)\n      return getConstantIntValue(add.getRhs()).value_or(1);\n"),
    ("a carried value updated otherwise read as stepped by 1", L,
     "  std::optional<int64_t> step = constantStep(next, inBody, out.update);\n",
     "  std::optional<int64_t> step = constantStep(next, inBody, out.update).value_or(1);\n"),
    ("a recurrence whose last value is read after the loop taken", L,
     "  if (!result.use_empty()) {\n    why = value + \" has its last value read",
     "  if (false) {\n    why = value + \" has its last value read"),
    ("a recurrence's subtraction read as an addition", L, "    return -*by;\n", "    return *by;\n"),
    ("a recurrence's start left out of its form", L,
     "      std::optional<int64_t> constant = back ? llvm::checkedSub(r.start, *back) : std::nullopt;\n",
     "      std::optional<int64_t> constant = back ? llvm::checkedSub(int64_t(0), *back) : std::nullopt;\n"),
    ("the loop's lower bound left out of a recurrence's form", L,
     "      std::optional<int64_t> back = llvm::checkedMul(r.step, band[p].lower);\n",
     "      std::optional<int64_t> back = llvm::checkedMul(r.step, int64_t(0));\n"),
    ("a recurrence's step left out of its form", L, "      form.coefficient[p] = r.step;\n",
     "      form.coefficient[p] = 1;\n"),
    ("a recurrence put on another loop's counter", L, "      form.coefficient[p] = r.step;\n",
     "      form.coefficient[(p + 2) % 3] = r.step;\n"),
    ("a float carried by an scf.for not said to be a reduction", L,
     "    if (!inBody.getType().isIntOrIndex()) {\n      why = \"a loop of the nest (scf.for) carries",
     "    if (false) {\n      why = \"a loop of the nest (scf.for) carries"),
    ("an scf.while's carried index not handed to the body as it is read", L,
     "    if (condition.getArgs()[n] != before->getArgument(n))\n", "    if (false)\n"),
    ("an scf.while's counter not chosen by its comparison", L, "    c = read[0];\n", "    c = 0;\n"),
    ("an scf.for's carried values not judged first", L,
     "      if (op->getNumResults() != 0)\n        readCarriedIndices(f, carried);\n",
     "      if (false)\n        readCarriedIndices(f, carried);\n"),
    ("an scf.for's carried values not read by the band", L,
     "      if (op->getNumResults() != 0) {\n        std::string reason;\n",
     "      if (false) {\n        std::string reason;\n"),
    ("an scf.while's carried indices not handed to the band", L,
     "      loop.carried = std::move(counting->carried);\n", "      (void)counting->carried;\n"),
    ("the outermost loop's body read for the update", L, "  Block *body = band.back().body;\n",
     "  Block *body = band.front().body;\n"),
    ("more than three loops read", L, "    if (band.size() > 3)\n", "    if (false)\n"),
    ("a band that is no perfect nest read", L, "    if (nested > 1 || others != 0)\n", "    if (false)\n"),
    ("the record's level count not compared", L, "  if (levels->size() != 3)\n", "  if (false)\n"),
    ("a level not compared with its loop", L,
     "    if (a == 3 || seen[a] || *descending || !end || *hi != *end || *hi <= *lo)\n",
     "    if (a == 3)\n"),
    ("a level of another extent taken for its loop plus a constant", L,
     "    if (a == 3 || seen[a] || *descending || !end || *hi != *end || *hi <= *lo)\n",
     "    if (a == 3 || seen[a] || *descending || *hi <= *lo)\n"),
    ("a level that is its loop plus a constant taken with no closed forms", L,
     "    if (*by != 0 && at.viewsUnmapped)\n", "    if (false)\n"),
    ("a level's shift left out of the record's forms", L,
     "      std::optional<int64_t> term = llvm::checkedMul(form.coefficient[l], shift[l]);\n",
     "      std::optional<int64_t> term = llvm::checkedMul(form.coefficient[l], int64_t(0));\n"),
    ("a level's shift taken the other way", L, "    shift[l] = *by;\n", "    shift[l] = -*by;\n"),
    ("the record's lower bound taken for the loop's", L, "    lower[a] = band[l].lower;\n",
     "    lower[a] = *lo;\n"),
    ("an axis not compared with its level", L, "    if (!listed || *listed != (int64_t)l)\n", "    if (!listed)\n"),
    ("a batch axis read", L, "  if (const json::Array *batch = axes->getArray(\"P\"); batch && !batch->empty())\n",
     "  if (false)\n"),
    ("the extents not compared", L, "    if (*e != extent[a])\n", "    if (false)\n"),
    ("an operation the update does not have read", L,
     "      return refuse(std::string(\"the loop body holds \") + op.getName().getStringRef().str() +",
     "      if (false) return refuse(std::string(\"the loop body holds \") + op.getName().getStringRef().str() +"),
    ("the body's operations not counted", L,
     "  if (loads.size() != 3 || stores.size() != 1 || products.size() != 1 || sums.size() != 1)\n",
     "  if (loads.size() < 3 || stores.empty() || products.empty() || sums.empty())\n"),
    ("a store of something other than the sum read", L, "  if (!store || stored != add.getResult())\n", "  if (!store)\n"),
    ("an addition not of the product read", L, "  if (!accumulatorFirst && add.getLhs() != mul.getResult())\n",
     "  if (false)\n"),
    ("an accumulator that is no load read", L,
     "  if (!accumulatorLoad || !llvm::is_contained(loads, accumulatorLoad))\n", "  if (!accumulatorLoad)\n"),
    ("a product of the accumulator read", L,
     "    if (!op || !llvm::is_contained(loads, op) || op == accumulatorLoad)\n", "    if (!op)\n"),
    ("a squared load read", L, "  if (factor[0] == factor[1])\n", "  if (false)\n"),
    ("an accumulator from another memref read", L, "  if (c->memref != store->memref)\n", "  if (false)\n"),
    ("an update not in f32 read", L, "  if (!element.isF32() || mul.getType() != element)\n", "  if (false)\n"),
    ("the record's element type not compared with the nest's", L,
     "  if (*elementName != nestRecordElementName(element))\n", "  if (false)\n"),
    ("the record's update not compared with the nest's", L,
     "  if (*reduction != \"fadd\" || *multiply != \"fmul\" || *sites != 1 || !*sitesAgree)\n", "  if (false)\n"),
    ("the record's accumulator side not compared with the nest's", L, "  if (*first != accumulatorFirst)\n",
     "  if (false)\n"),
    ("a record flag that is no string read by the lowering", L,
     "  if (!namesOf(*reductionFlags, recordReduction) || !namesOf(*multiplyFlags, recordMultiply))\n",
     "  if ((namesOf(*reductionFlags, recordReduction), namesOf(*multiplyFlags, recordMultiply), false))\n"),
    ("the record's flags not compared with the nest's", L,
     "  if (recordReduction != nestRecordFlagNames(add.getFastmath()) ||\n"
     "      recordMultiply != nestRecordFlagNames(mul.getFastmath()))\n", "  if (false)\n"),
    ("the record's source flag not compared with the nest's", L, "  if (!sourceFlag || *sourceFlag != reassoc)\n",
     "  if (!sourceFlag)\n"),
    ("the record's C not compared with the stored root", L, "    if (rc != rootNumber[2])\n", "    if (false)\n"),
    ("the record's A and B not compared with the product's roots", L,
     "    } else if (r1 == rootNumber[0] && r0 == rootNumber[1] && r0 != r1) {\n", "    } else if (true) {\n"),
    ("an operand that is no static identity memref read", L,
     "    if (!type[x] || !type[x].hasStaticShape() || !type[x].getLayout().isIdentity() || type[x].getRank() < 1 ||\n"
     "        type[x].getElementType() != element)\n", "    if (!type[x])\n"),
    ("a separation the record does not use read by the lowering", L, "    if (!*used)\n      continue;\n",
     "    if (false)\n      continue;\n"),
    ("a proof between two arguments read by the lowering", L,
     "      if (at.roots->roots[*a].argument && at.roots->roots[*b].argument)\n", "      if (false)\n"),
    ("a separation of no known kind read by the lowering", L, "    if (*kind != \"assumption\")\n", "    if (false)\n"),
    ("an assumption the lowering's alias gate does not check read", L,
     "    if (!is(rootNumber[0], rootNumber[2]) && !is(rootNumber[1], rootNumber[2]))\n", "    if (false)\n"),
    ("a record form with a parameter read", L,
     "    if (!affine || !*affine || !constant || !terms || !parameters || !parameters->empty())\n",
     "    if (!affine || !*affine || !constant || !terms || !parameters)\n"),
    ("C's read index not read as a form", L, "    if (!indexForms(*c, cLoad))\n", "    if (false)\n"),
    ("a view the record does not prove read by the formulas", L,
     "      if (!proved(x))\n        return refuse(std::string(\"the record does not prove the view of \") + name);\n",
     "      if (false)\n        return refuse(std::string(\"the record does not prove the view of \") + name);\n"),
    ("the update's read and write of C not compared", L, "      if (x == 2 && nest != cLoad)\n", "      if (false)\n"),
    ("the record's dimension count not compared", L,
     "      if (!dimensions || (int64_t)dimensions->size() != rank)\n", "      if (!dimensions)\n"),
    ("the record's form not compared with the nest's index", L, "        if (!(mine == nest[d]))\n", "        if (false)\n"),
    ("an index outside the memref read by the formulas", L, "        if (*lo < 0 || *hi >= type[x].getDimSize(d))\n",
     "        if (false)\n"),
    ("an explicit view's stride not checked", L, "        if (*stride[r] <= 0)\n", "        if (false)\n"),
    ("C's explicit view not checked one to one", L,
     "          if ((dr != 0 || dc != 0) && dc > -roleExtent[1] && dc < roleExtent[1])\n", "          if (false)\n"),
    ("the tables' budget not read", L, "  if (!iterations || *iterations > tableBudget)\n", "  if (!iterations)\n"),
    ("a value from before the nest read by the tables", L, "    } else if (!inside) {\n", "    } else if (false) {\n"),
    ("an index computed in another width read by the tables", L, "      if (!isSixtyFour(operand.getType())) {\n",
     "      if (false) {\n"),
    ("an index outside the memref read by the tables", L,
     "      if (indexValues[d] < 0 || indexValues[d] >= type[x].getDimSize(d)) {\n", "      if (false) {\n"),
    ("the tables' read and write of C not compared", L, "        if (*offsets[2] != *offsets[3])\n", "        if (false)\n"),
    ("an address that depends on another coordinate read", L, "          else if (slot != *offsets[x])\n",
     "          else if (false)\n"),
    ("the record's closed form not compared with the tables", L, "            if (expected != *offsets[x])\n",
     "            if (false)\n"),
    ("two cells of C at one address read by the tables", L, "      if (!inserted)\n", "      if (false)\n"),
    # The binding: what the production pass computes must be what the research tool computed.
    ("the function's name left out of the binding", B, "     << \"function: \" << function.getSymName() << \"\\n\"\n",
     "     << \"function: \\n\"\n"),
    ("a root's llvm.noalias left out of the binding", B,
     "      os << \"argument \" << root.argNumber << (root.noalias ? \", llvm.noalias\" : \"\");\n",
     "      os << \"argument \" << root.argNumber;\n"),
    ("locations in the binding", B, "  module.get()->print(os, OpPrintingFlags().printGenericOpForm());\n",
     "  module.get()->print(os, OpPrintingFlags().printGenericOpForm().enableDebugInfo());\n"),
]

# Not run: a check whose condition never holds once the checks before it have passed, for any
# input, records edited by hand included.
EQUIVALENT = [
    ("the lowering's own reassoc fact not compared", G, "  if (reassoc != matmul.reductionMayReassociate)\n",
     "with an accumulation, which a check before it requires, the recognizer's reductionMayReassociate is the "
     "reassoc bit of the flags of the addition the GEMM stores, the very arith.addf this gate reads, and "
     "flagNames lists that bit as reassoc whatever other bits are set (fast included): the two never differ"),
    ("another use of a value of the update read", L, "    if (!op->getResult(0).hasOneUse())\n",
     "the checks before it admit exactly three loads, one multiplication and one addition of f32 values and one "
     "store, the product of two loads other than the accumulator's, the sum of the product and the accumulator, "
     "the store of the sum; every other operation of the body is index arithmetic, which takes no f32 value: each "
     "of the five values has exactly the one use those checks name"),
    ("an access of the update that is no load or store read", L, "  if (!c || !f0 || !f1)\n",
     "accessOf answers every memref.load and affine.load, and the body check admitted no other load"),
    ("an operand that is no root of the function read by the lowering", L, "    if (r0 < 0 || r1 < 0 || rc < 0)\n",
     "the binding, which a record check before requires, refuses a nest that reads a memref that is no root of "
     "its function"),
    ("a program index width other than 64 read by the tables", L, "  if (indexWidth != 64)\n",
     "systolic-opt registers no DLTI dialect, so a module's index width is the data layout's default, 64 bits; a "
     "record of another width fails the record check of the index width before"),
    ("memory in an address read by the tables", L,
     "    } else if (isa<memref::LoadOp, affine::AffineLoadOp>(op)) {\n",
     "a load inside the band is one of the body's three loads, which the body check requires to be the update's "
     "f32 operands, each used once by it; none can be an index, and a load before the nest is refused by the "
     "check before (a value computed before the nest)"),
    ("an operation the tables do not evaluate read", L,
     "                    affine::AffineApplyOp, affine::AffineMinOp, affine::AffineMaxOp>(op)) {\n"
     "      why = (what + \" uses \"",
     "every operation inside the band other than the update is one the body check admitted, the same list as this "
     "one; an operation before the nest is refused by the check before (a value computed before the nest)"),
    ("a value of another width read by the tables", L, "    if (!isSixtyFour(value.getType())) {\n",
     "every value admitted is an index operand of an access (of index type) or the operand of an operation, whose "
     "width the loop over operands checks before admitting it"),
]

BUILD_SECONDS = 3600
TEST_SECONDS = 900
CRASH = ("Stack dump", "PLEASE submit a bug report", "Segmentation fault", "core dumped", "Assertion `",
         "terminate called", "exit status: -", "exit status: 134", "exit status: 139")


def run(command, cwd=None, timeout=TEST_SECONDS):
    process = subprocess.Popen([str(c) for c in command], cwd=cwd, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, start_new_session=True)
    try:
        text, _ = process.communicate(timeout=timeout)
        return process.returncode, text, False
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGKILL)  # The group it started, by its id.
        except ProcessLookupError:
            pass
        text, _ = process.communicate()
        return None, text, True


def tree_hash(root, files):
    digest = hashlib.sha256()
    for name in files:
        digest.update(name.encode())
        digest.update((root / name).read_bytes())
    for path in sorted((root / "test/Systolic").glob("nest_records*")) + sorted((root / "test/Systolic/Inputs").glob("*nest_records*")):
        digest.update(str(path.relative_to(root)).encode())
        digest.update(path.read_bytes())
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--scratch", required=True)
    parser.add_argument("--llvm-dir", required=True)
    parser.add_argument("--mlir-dir", required=True)
    parser.add_argument("--lit", required=True)
    parser.add_argument("--results", required=True)
    parser.add_argument("--jobs", type=int, default=4, help="build jobs")
    parser.add_argument("--only", default="")
    args = parser.parse_args()
    root = Path(args.source).resolve()
    scratch = Path(args.scratch).resolve()
    copy, build = scratch / "src", scratch / "build"
    files = sorted({m[1] for m in MUTANTS} | {e[1] for e in EQUIVALENT})
    tree = tree_hash(root, files)
    results = Path(args.results)
    done = {}
    if results.exists():
        for line in results.read_text().splitlines():
            entry = json.loads(line)
            if entry["tree"] != tree:
                sys.exit(f"{results} is about other files ({entry['tree'][:12]})")
            done[entry["mutant"]] = entry
    names = [m[0] for m in MUTANTS] + [e[0] for e in EQUIVALENT]
    if len(set(names)) != len(names):
        sys.exit("two mutants have one name")
    chosen = [n for n in args.only.split(";") if n] or [m[0] for m in MUTANTS]

    def record(name, result, detail):
        entry = {"mutant": name, "result": result, "detail": detail, "tree": tree}
        done[name] = entry
        with results.open("a") as out:
            out.write(json.dumps(entry) + "\n")
        print(f"{result:14s} {name}" + (f"  [{detail}]" if detail else ""), flush=True)

    # The copy, once: what the production build reads.
    marker = scratch / "TREE"
    if not marker.exists():
        if copy.exists():
            sys.exit(f"{copy} exists and has no marker: refusing to use it")
        copy.mkdir(parents=True)
        for part in ("CMakeLists.txt", "include", "lib", "tools", "runtime", "test"):
            source = root / part
            if source.is_dir():
                shutil.copytree(source, copy / part, ignore=shutil.ignore_patterns("__pycache__", "logs", "*.o"))
            else:
                shutil.copy2(source, copy / part)
        marker.write_text(tree)
    elif marker.read_text() != tree:
        sys.exit(f"{scratch} holds a copy of other files")
    for _, file, _, _ in MUTANTS:
        if (copy / file).read_bytes() != (root / file).read_bytes():
            sys.exit(f"{copy / file} is not the tree's: a mutant was left in it")
    code, text, _ = run(["cmake", "-G", "Ninja", "-S", copy, "-B", build, "-DCMAKE_BUILD_TYPE=Release",
                         f"-DLLVM_DIR={args.llvm_dir}", f"-DMLIR_DIR={args.mlir_dir}", "-DCMAKE_C_COMPILER=/usr/bin/gcc",
                         "-DCMAKE_CXX_COMPILER=/usr/bin/g++", f"-DLLVM_EXTERNAL_LIT={args.lit}"], timeout=BUILD_SECONDS)
    if code != 0:
        sys.exit("cmake failed:\n" + text[-2000:])

    def build_tools():
        return run(["ninja", "-C", build, f"-j{args.jobs}", "systolic-opt", "systolic-translate", "systolic-cost"],
                   timeout=BUILD_SECONDS)

    def tests():
        code, text, timed_out = run([args.lit, "-v", build / "test", "--filter", "nest_records"])
        if timed_out:
            return "HARNESS_ERROR", "the tests timed out"
        if code == 0:
            return "SURVIVED", ""
        if any(marker in text for marker in CRASH):
            return "HARNESS_ERROR", "a crash: " + next(m for m in CRASH if m in text)
        mismatch = re.search(r"error: ([A-Z][A-Z0-9]*(?:-[A-Z]+(?:-[0-9]+)?)?): (expected string not found in input|is not on the "
                             r"line after the previous match|excluded string found in input|found here)", text)
        if mismatch:
            return "KILLED", f"{mismatch.group(1)}: {mismatch.group(2)}"
        # A check whose input is empty: what it expects (a remark, an error) was never printed.
        empty = re.search(r"FileCheck error: '<stdin>' is empty\.\s*(?:#\s*\|?\s*)*FileCheck command line: [^\n]*"
                          r"--check-prefix=([A-Z][A-Z0-9]*)", text)
        if empty:
            return "KILLED", f"{empty.group(1)}: the input is empty, what the check expects never came"
        return "HARNESS_ERROR", f"exit {code} with no check that failed: " + text[-1500:].replace("\n", " | ")

    if "BASELINE" not in done:
        code, text, _ = build_tools()
        if code != 0:
            sys.exit("the copy does not build:\n" + text[-2000:])
        result, detail = tests()
        if result != "SURVIVED":
            sys.exit(f"the copy does not pass: {result} {detail}")
        record("BASELINE", "PASSED", "")
    for name, _, _, reason in [(e[0], e[1], e[2], e[3]) for e in EQUIVALENT]:
        if name not in done:
            record(name, "EQUIVALENT", reason)
    for name, file, pattern, replacement in MUTANTS:
        if name not in chosen or name in done:
            continue
        path = copy / file
        original = path.read_text()
        if original.count(pattern) != 1:
            record(name, "HARNESS_ERROR", f"the pattern occurs {original.count(pattern)} times")
            continue
        try:
            path.write_text(original.replace(pattern, replacement))
            code, text, _ = build_tools()
            if code != 0:
                record(name, "HARNESS_ERROR", "the mutant does not build: " + text[-300:].replace("\n", " "))
                continue
            record(name, *tests())
        finally:
            path.write_text(original)
    code, _, _ = build_tools()
    if code != 0:
        sys.exit("the copy does not build after the last mutant was undone")
    counts = {}
    for name in chosen + [e[0] for e in EQUIVALENT]:
        if name in done:
            counts[done[name]["result"]] = counts.get(done[name]["result"], 0) + 1
    print(", ".join(f"{n} {r}" for r, n in sorted(counts.items())))
    bad = [n for n in chosen if n not in done or done[n]["result"] not in ("KILLED", "EQUIVALENT")]
    for name in bad:
        print("NOT KILLED:", name, file=sys.stderr)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
