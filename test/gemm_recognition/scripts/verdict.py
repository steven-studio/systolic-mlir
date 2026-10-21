"""
verdict.py -- compare the measured stages of one case (scripts/stages.py)
with the two layers of its manifest entry: the specification ('expected')
and the predicted behaviour of the current implementation
('current_implementation').  Shared by run_showcase.py, loop.py and
regression.py so that every runner judges a case the same way.
"""

DENSE_REASON_MARKERS = ("batched GEMM", "two logical coordinates are driven by the same loop",
                        "drives no logical coordinate", "decreases along its loop",
                        "do not enumerate it densely", "loop bounds are not static",
                        "fewer than three dimensions", "not a dense box",
                        "overflows 64-bit arithmetic",
                        # getDenseLogicalGemm's own reasons for an empty GEMM
                        # (SQ-2) and a constant coordinate (ISSUE-003, SQ-4)
                        "has no iteration (an empty GEMM", "is driven by no loop",
                        # a batched product with M = N = K = 1: element-wise
                        # (docs section 19.1, refused since 2026-10-18)
                        "a batched product with M = N = K = 1",
                        # a non-box domain proven no affine image of a box
                        # (central symmetry, docs section 19.13, since
                        # 2026-10-18: the front end translates triangles)
                        "is no affine image of a box")

# The non-box disproof's remark (docs section 19.13.4, PROVEN_INVALID): the
# claim that no decomposition makes the nest a GEMM of affine views. On a nest
# the specification finds to be a GEMM ('mathematical_gemm' / 'legal_gemm')
# the claim is a wrong proof -- a soundness failure, whatever the prediction
# said (a prediction that mirrors a wrong proof would otherwise hide it, as
# the 'every loop read' rule did for strip-mined loops with a remainder,
# review 2026-10-18 #9): reported as a mismatch, so never a pass.
DISPROOF_MARKER = "is no affine image of a box"


def unsound_disproof(spec, *said):
    """The soundness mismatch when the compiler stated the non-box disproof in
    one of `said` while the specification finds a GEMM, else None."""
    text = " | ".join(t for t in said if t)
    if DISPROOF_MARKER in text and spec.get("mathematical_gemm", spec.get("legal_gemm")):
        return ("soundness: the compiler proved the nest no GEMM of affine views ('%s', PROVEN_INVALID) "
                "but the specification finds a GEMM (%s): a wrong proof" % (
                    DISPROOF_MARKER, (spec.get("basis") or "")[:80]))
    return None


def classify_mlir(entry, st):
    """The MLIR-level cases (manifest kind 'mlir'): the recognizer, the dense
    partition and the lowering pass, nothing else.  'offloaded' here means
    'the lowering pass replaced the nest'; the specification layer is judged
    on that.  Verdicts as classify(), minus the runtime ones."""
    spec = entry["expected"]
    cur = entry["current_implementation"]
    actual = {"kind": "mlir", "frontend": "not_applicable", "driver": "not_applicable",
              "execute": "not_applicable", "invocations": None}
    mism, notes = [], []
    di, lo = st.get("discovery"), st.get("lowering")
    for key, stg in (("discovery", di), ("lowering", lo)):
        if stg and stg.get("result") == "tool_error":
            return "tool_error", {key: "tool_error", "reason": stg.get("reason")}, \
                ["%s: tool_error: %s" % (key, stg.get("reason"))], notes
    if not di or not lo:
        return "tool_error", actual, ["discovery or lowering did not run"], notes
    actual["recognition"] = di.get("recognition")
    actual["discovery_recognition"] = di.get("recognition")
    actual["views"] = di.get("views", {})
    if di.get("recognition") != cur["recognition"]:
        mism.append("recognition: predicted %s, got %s" % (cur["recognition"], di.get("recognition")))
    dense = di.get("dense") if di.get("recognition") == "accept" else "not_reached"
    actual["dense_partition"] = dense
    actual["dense_reason"] = di.get("dense_reason", "")
    if dense != cur["dense_partition"]:
        mism.append("dense_partition: predicted %s, got %s (%s)" % (cur["dense_partition"], dense, di.get("dense_reason", "")))
    if lo["result"] == "lowered":
        low = "supported"
    elif lo["result"] == "recognized_not_lowered" and dense == "supported":
        low = "unsupported"
    else:
        low = "not_reached"
    actual["lowering"] = low
    actual["lowering_result"] = lo["result"]
    actual["lowering_reason"] = lo.get("reason", "")
    actual["lowered_regions"] = lo.get("regions")
    actual["matmul_tiles"] = lo.get("matmul_tiles")
    if low != cur["lowering"]:
        mism.append("lowering: predicted %s, got %s (%s)" % (cur["lowering"], low, lo.get("reason", "")))
    # the rejection reason: the lowering pass's remark (or the discovery
    # pass's dense reason) must carry the predicted wording
    if cur.get("reject_reason") and cur.get("reject_stage") in ("dense_partition", "lowering"):
        said = (lo.get("reason", "") + " | " + di.get("dense_reason", "")).lower()
        if cur["reject_reason"][:48].lower() not in said:
            mism.append("rejection reason: predicted '%s', the compiler said '%s'" % (
                cur["reject_reason"], lo.get("reason") or di.get("dense_reason") or "(nothing)"))
    bad_proof = unsound_disproof(spec, lo.get("reason"), di.get("dense_reason"))
    if bad_proof:
        mism.append(bad_proof)
    for operand, want in (spec.get("views") or {}).items():
        got = di.get("views", {}).get(operand)
        if got != want:
            mism.append("view %s: specification %s, the recognizer reports %s" % (operand, want, got))
    offloaded = lo["result"] == "lowered"
    if spec["recognition"] == "UNDETERMINED":
        actual["matches_specification"] = None
        if mism:
            return "unexpected_stage", actual, mism, notes
        if not offloaded:
            return "expected_refusal", actual, [], notes + [
                "%s: the specification does not decide this case; the lowering pass refused it at the "
                "predicted stage with the predicted reason (conservative; not a pass)" % spec.get("spec_question")]
        return "spec_open", actual, [], notes + ["%s: the specification does not decide this case; the "
                                                "compiler lowered it" % spec.get("spec_question")]
    spec_says_offload = spec["recognition"] == "ACCEPT"
    actual["matches_specification"] = (offloaded == spec_says_offload)
    if offloaded and not spec_says_offload:
        return "false_positive", actual, mism + [
            "the specification says %s (%s) but the lowering pass replaced the nest" % (spec["recognition"], spec["basis"][:80])], notes
    if not offloaded and spec_says_offload:
        if not mism:
            return "known_false_negative", actual, ["specification: ACCEPT; compiler: %s (%s) -- %s" % (
                lo["result"], lo.get("reason") or di.get("dense_reason") or "refused at recognition",
                entry.get("issue") or "no issue recorded yet")], notes
        return "unexpected_stage", actual, mism, notes
    if mism:
        if cur["lowering"] != "supported" and offloaded and spec_says_offload:
            return "fixed_issue", actual, mism, notes + ["%s no longer reproduces" % (entry.get("issue") or "the predicted refusal")]
        return "unexpected_stage", actual, mism, notes
    return "pass", actual, mism, notes


def classify(entry, st):
    """Compare the actual stage results with both layers of the manifest:
    the specification (entry['expected']) and the predicted behaviour of
    the current implementation (entry['current_implementation']).

    Returns (verdict, actual, mismatches, notes).  Verdicts:
      pass                  actual == specification, program check passed
      fixed_issue           actual == specification although the current
                            implementation was predicted to refuse: a
                            recorded false negative no longer reproduces
      known_false_negative  the specification says ACCEPT, the compiler
                            refused exactly as predicted (an open issue)
      false_positive        the specification says REJECT / OUT_OF_SCOPE and
                            the compiler offloaded the nest
      unexpected_stage      a stage did not do what the current implementation
                            was predicted to do (behaviour changed: analyse)
      fail_runtime          the compiled program's result differs from the
                            references, --verify reported a mismatch, the
                            runtime reported failed invocations, or a crash
      test_bug              the program's references disagree among themselves
      tool_error            a tool is missing or crashed; not tested
      expected_refusal      the specification does not decide the case (a
                            recorded spec question, entry['expected']
                            ['spec_question']), every stage did what was
                            predicted -- including the predicted refusal
                            reason where the compiler states one -- the
                            program's check passed, and the compiler did NOT
                            offload it: a conservative refusal. Not a pass
                            (reported and counted on its own), not a failure
      spec_open             the same, but the compiler offloaded the nest: a
                            choice the specification has not made -- blocking
                            until the spec question is decided
    MLIR-level cases (kind 'mlir') are judged by classify_mlir() on the
    recognition, dense-partition and lowering stages only.
    """
    if entry.get("kind") == "mlir" or st.get("kind") == "mlir":
        return classify_mlir(entry, st)
    spec = entry["expected"]
    cur = entry["current_implementation"]
    actual = {}
    mism = []      # vs the current-implementation prediction
    notes = []

    for key in ("frontend", "discovery", "lowering", "driver"):
        s = st.get(key)
        if s and s.get("result") in ("tool_error", "compile_error", "no_note_for_kernel"):
            return "tool_error", {key: s.get("result"), "reason": s.get("reason")}, \
                ["%s: %s: %s" % (key, s.get("result"), s.get("reason"))], notes

    fe = st.get("frontend", {})
    actual["frontend"] = fe.get("result")
    if fe.get("result") != cur["frontend"]:
        mism.append("frontend: predicted %s, got %s (%s)" % (cur["frontend"], fe.get("result"), fe.get("reason", "")))

    lo = st.get("lowering")
    di = st.get("discovery")
    if lo:
        rec = "accept" if lo["result"] in ("lowered", "recognized_not_lowered") else "reject"
        actual["recognition"] = rec
        actual["discovery_recognition"] = di.get("recognition") if di else None
        if rec != cur["recognition"]:
            mism.append("recognition: predicted %s, got %s (lowering pass: %s %s)" % (
                cur["recognition"], rec, lo["result"], lo.get("reason", "")))
        if di and di.get("recognition") != cur["recognition"]:
            if cur.get("hoisting_required") and di.get("recognition") == "reject":
                notes.append("discovery (no hoisting) rejects, as expected for a repetition loop inside the nest")
            elif rec == cur["recognition"]:
                notes.append("discovery pass says %s while the lowering pass says %s" % (di.get("recognition"), rec))
        if lo["result"] == "lowered":
            dense = "supported"
        elif lo["result"] == "recognized_not_lowered":
            dense = "unsupported" if any(m in lo.get("reason", "") for m in DENSE_REASON_MARKERS) else "supported"
        else:
            dense = "not_reached"
        if di and di.get("dense") in ("supported", "unsupported") and not cur.get("hoisting_required"):
            if di["dense"] != dense and lo["result"] != "not_recognized":
                notes.append("discovery dense=%s (%s) vs lowering-derived dense=%s" % (di["dense"], di.get("dense_reason"), dense))
            if lo["result"] == "not_recognized":
                dense = di["dense"]
        actual["dense_partition"] = dense
        actual["dense_reason"] = (di or {}).get("dense_reason") or (lo.get("reason") if dense == "unsupported" else "")
        if dense != cur["dense_partition"]:
            mism.append("dense_partition: predicted %s, got %s (%s)" % (cur["dense_partition"], dense, actual["dense_reason"]))
        if lo["result"] == "lowered":
            low = "supported"
        elif lo["result"] == "recognized_not_lowered" and dense == "supported":
            low = "unsupported"
        else:
            low = "not_reached"
        actual["lowering"] = low
        actual["lowering_reason"] = lo.get("reason", "") if low == "unsupported" else ""
        actual["lowered_regions"] = lo.get("regions")
        actual["matmul_tiles"] = lo.get("matmul_tiles")
        if low != cur["lowering"]:
            mism.append("lowering: predicted %s, got %s (%s)" % (cur["lowering"], low, lo.get("reason", "")))
        if lo["result"] == "lowered" and entry.get("logical_extents"):
            e = entry["logical_extents"]
            # Either factor may be A (P0.2, docs/AFFINE_GEMM_MAPPING.md section
            # 5): A x B into C, or the transposed problem B^T x A^T into C^T
            # (M and N exchanged) -- both valid; the program check and --verify
            # establish the result.
            if (lo.get("M"), lo.get("N"), lo.get("K")) not in (
                    (e["I"], e["J"], e["K"]), (e["J"], e["I"], e["K"])):
                mism.append("lowering: extents %sx%sx%s, the case's logical GEMM is %dx%dx%d" % (
                    lo.get("M"), lo.get("N"), lo.get("K"), e["I"], e["J"], e["K"]))
    else:
        for k in ("recognition", "dense_partition", "lowering"):
            actual[k] = "not_reached"
            if cur[k] != "not_reached" and cur["frontend"] == "translated":
                mism.append("%s: not reached (front end: %s)" % (k, fe.get("result")))

    dr = st.get("driver", {})
    actual["driver"] = dr.get("result")
    actual["driver_reason"] = dr.get("reason", "")
    actual["planned_invocations"] = dr.get("planned_invocations")
    actual["planned_per_call"] = dr.get("planned_per_call")
    if dr.get("result") != cur["driver"]:
        mism.append("driver: predicted %s, got %s (%s)" % (cur["driver"], dr.get("result"), dr.get("reason", "")))
    # The rejection reason.  systolic-cc --explain quotes the compiler's own
    # diagnostic (the front end's error, the dense-partition / lowering
    # remark, the driver's policy), so for those stages the predicted wording
    # must be in it -- a refusal for another reason is a behaviour change to
    # analyse (unexpected_stage), not a pass.  The recognizer itself gives no
    # reason (recognizeLogicalMatmul returns a bare failure), so a prediction
    # for that stage cannot be checked against the output: noted only.
    if cur.get("reject_reason") and dr.get("reason") is not None and dr.get("result") != "offloaded":
        said = dr["reason"].lower()
        want = cur["reject_reason"][:40].lower()
        if cur["reject_stage"] in ("frontend", "dense_partition", "lowering", "driver") and want not in said:
            mism.append("rejection reason: predicted '%s' (%s), the compiler said '%s'" % (
                cur["reject_reason"], cur["reject_stage"], dr["reason"]))
        elif cur["reject_stage"] == "recognition":
            notes.append("refused at recognition (the recognizer states no reason; predicted: %s)" % cur["reject_reason"][:60])
    bad_proof = unsound_disproof(spec, actual.get("dense_reason"), (lo or {}).get("reason"), dr.get("reason"))
    if bad_proof:
        mism.append(bad_proof)

    exe = st.get("execute", {})
    actual["execute"] = exe.get("result")
    actual["invocations"] = exe.get("invocations")
    actual["backend"] = exe.get("backend")
    actual["hardware"] = exe.get("hardware")
    actual["failed_invocations"] = exe.get("failed_invocations")
    actual["verified"] = len(exe.get("verified_lines", []))
    actual["mismatch_lines"] = exe.get("mismatch_lines", [])
    runtime_bad = []
    if exe.get("result") == "test_bug":
        return "test_bug", actual, mism, notes
    if exe.get("result") != "pass":
        runtime_bad.append("program result: %s (rc %s)" % (exe.get("result"), exe.get("rc")))
    if exe.get("mismatch_lines"):
        runtime_bad.append("systolic-cc --verify: " + "; ".join(exe["mismatch_lines"])[:300])
    if exe.get("failed_invocations"):
        runtime_bad.append("%d failed invocation(s) reported by the runtime" % exe["failed_invocations"])
    if entry.get("runtime_check") == "signed_zero" and exe:
        # numerically equal, the sign of zero may differ (docs sections 6, 20):
        # the program counts those elements; --verify compared numerically
        actual["sign_of_zero_only"] = exe.get("sign_of_zero_only")
        if exe.get("sign_of_zero_only") is None:
            runtime_bad.append("runtime_check signed_zero: the program reported no sign-of-zero count")
        else:
            notes.append("runtime_check signed_zero: numerically equal to the references; %d element(s) "
                         "differ in the sign of zero only (+0 for -0, accepted); --verify: %s" % (
                             exe["sign_of_zero_only"], "; ".join(exe.get("verified_lines", []))[:160]))
    if dr.get("result") == "offloaded":
        if not exe.get("verified_lines") and not exe.get("mismatch_lines"):
            runtime_bad.append("offloaded but systolic-cc --verify printed no 'verified' line")
        planned = dr.get("planned_per_call")
        if planned is not None and exe.get("invocations") is not None and exe["invocations"] != planned:
            runtime_bad.append("runtime made %d invocation(s), the plan says %d per call" % (exe["invocations"], planned))
    cpu = st.get("cpu")
    if cpu and cpu.get("result") not in (None, "pass"):
        return "test_bug", actual, mism + ["cpu: %s" % cpu.get("result")], notes
    if runtime_bad:
        return "fail_runtime", actual, mism + runtime_bad, notes

    # the specification layer
    offloaded = dr.get("result") == "offloaded"
    if spec["recognition"] == "UNDETERMINED":
        # a recorded spec question: the program's result was checked above;
        # the compiler's choice is recorded, not judged
        actual["matches_specification"] = None
        if mism:
            return "unexpected_stage", actual, mism, notes
        if not offloaded:
            return "expected_refusal", actual, [], notes + [
                "%s: the specification does not decide this case; the compiler kept it on the host at the "
                "predicted stage with the predicted reason, the program's check passed (conservative; not a "
                "pass)" % spec.get("spec_question")]
        return "spec_open", actual, [], notes + ["%s: the specification does not decide this case; the "
                                                "compiler offloaded the nest, the program's check passed" % (
                                                    spec.get("spec_question"))]
    spec_says_offload = spec["recognition"] == "ACCEPT"
    actual["matches_specification"] = (offloaded == spec_says_offload)
    if offloaded and not spec_says_offload:
        return "false_positive", actual, mism + [
            "the specification says %s (%s) but the compiler offloaded the nest" % (spec["recognition"], spec["basis"][:80])], notes
    if not offloaded and spec_says_offload:
        if not mism:
            return "known_false_negative", actual, [
                "specification: ACCEPT; compiler: %s (%s) -- %s" % (
                    dr.get("result"), dr.get("reason") or actual.get("lowering_reason") or actual.get("dense_reason") or "refused at recognition, no reason given",
                    entry.get("issue") or "no issue recorded yet")], notes
        return "unexpected_stage", actual, mism, notes
    if mism:
        if cur["driver"] != "offloaded" and offloaded and spec_says_offload:
            return "fixed_issue", actual, mism, notes + ["%s no longer reproduces" % (entry.get("issue") or "the predicted refusal")]
        return "unexpected_stage", actual, mism, notes
    return "pass", actual, mism, notes


