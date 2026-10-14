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
                        "fewer than three dimensions", "not a dense box")


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
    """
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
            if (lo.get("M"), lo.get("N"), lo.get("K")) != (e["I"], e["J"], e["K"]):
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
    if cur.get("reject_reason") and dr.get("reason") and cur["reject_stage"] in ("lowering", "dense_partition", "driver"):
        if cur["reject_reason"][:40].lower() not in dr["reason"].lower():
            notes.append("rejection wording differs: predicted '%s', driver said '%s'" % (cur["reject_reason"], dr["reason"]))

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


