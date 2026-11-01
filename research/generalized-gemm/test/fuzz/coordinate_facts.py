"""The facts of a coordinate candidate, recomputed from the definitions.

Shares nothing with the C++: dictionaries over events, and breadth-first search
where the library uses union-find. Used by both G3a oracles.
"""
import itertools


def kernel(values):
    ids = {}
    return tuple(ids.setdefault(v, len(ids)) for v in values)


def components(views):
    """The join of the kernels of `views`, by search over the shared-value graph."""
    size = len(views[0])
    members = {}
    for position, view in enumerate(views):
        for t in range(size):
            members.setdefault((position, view[t]), []).append(t)
    label = [None] * size
    for start in range(size):
        if label[start] is not None:
            continue
        label[start] = start
        stack = [start]
        while stack:
            t = stack.pop()
            for position, view in enumerate(views):
                for other in members[(position, view[t])]:
                    if label[other] is None:
                        label[other] = start
                        stack.append(other)
    return kernel(label)


def first_conflict(events, key, value):
    first = {}
    for e in range(len(events)):
        k = key(e)
        if k not in first:
            first[k] = e
        elif value(first[k]) != value(e):
            return first[k], e
    return None


def classify(i, j, k):
    if i == j == k == 1:
        return "scalar"
    if k == 1:
        return "outer" if i >= 2 and j >= 2 else "scaled_vector"
    if i >= 2 and j >= 2:
        return "gemm"
    return "dot" if i == 1 and j == 1 else "gemv"


def facts(events, c):
    """`events`: dicts with site, x, h, h_read, f, g. `c`: dict of label lists."""
    n = len(events)
    P, I, J, K = c["p"], c["i"], c["j"], c["k"]
    h = [tuple(e["h"]) for e in events]
    r = [tuple(e["h_read"]) for e in events]
    f = [tuple(e["f"]) for e in events]
    g = [tuple(e["g"]) for e in events]
    points = [(e["site"], tuple(e["x"])) for e in events]
    cx = []
    out = {}

    read = next((e for e in range(n) if h[e] != r[e]), None)
    if read is not None:
        cx.append(("output_read_differs", read, read))
    function = first_conflict(events, lambda e: points[e], lambda e: (P[e], I[e], J[e], K[e]))
    if function:
        cx.append(("not_a_function_of_point",) + function)
    dep = {}
    for name, key, view in (("h", lambda e: (P[e], I[e], J[e]), h),
                            ("f", lambda e: (P[e], I[e], K[e]), f),
                            ("g", lambda e: (P[e], K[e], J[e]), g)):
        found = first_conflict(events, key, lambda e: view[e])
        dep[name] = found is None
        if found:
            cx.append(("dependence_" + name,) + found)
    valid = read is None and function is None and all(dep.values())
    out["validity"] = {"decomposition_valid": valid, "output_read_matches": read is None,
                       "function_of_point": function is None, "dependence_h": dep["h"],
                       "dependence_f": dep["f"], "dependence_g": dep["g"]}

    inj = {}
    for name, view, value in (("h", h, lambda e: (P[e], I[e], J[e])),
                              ("f", f, lambda e: (P[e], I[e], K[e])),
                              ("g", g, lambda e: (P[e], K[e], J[e]))):
        found = first_conflict(events, lambda e: view[e], value)
        inj[name] = found is None
        if found:
            cx.append(("view_not_injective_" + name,) + found)
    share = False
    for view in (h, f, g):
        seen = {}
        for e in range(n):
            if seen.setdefault(view[e], P[e]) != P[e]:
                share = True
    out["injectivity"] = {"h": inj["h"], "f": inj["f"], "g": inj["g"], "batches_share_views": share}

    image = {}
    for e in range(n):
        q = (P[e], I[e], J[e], K[e])
        image[q] = image.get(q, 0) + 1
    batches = []
    total = 0
    absent = None
    for p in sorted(set(P)):
        which = [e for e in range(n) if P[e] == p]
        Is, Js, Ks = sorted({I[e] for e in which}), sorted({J[e] for e in which}), sorted({K[e] for e in which})
        here = sum(1 for q in image if q[0] == p)
        missing = len(Is) * len(Js) * len(Ks) - here
        total += missing
        batches.append({"batch": p, "i": len(Is), "j": len(Js), "k": len(Ks), "points": here,
                        "events": len(which), "missing": str(missing)})
        if missing and absent is None:
            absent = next((p, a, b, d) for a, b, d in itertools.product(Is, Js, Ks)
                          if (p, a, b, d) not in image)
    if absent is not None:
        cx.append(("missing_point", absent))
    box = total == 0
    out["cartesian"] = {"form": "box" if box else "partial", "missing_points": str(total)}

    low, high = min(image.values()), max(image.values())
    triples = {}
    for e in range(n):
        triples[(h[e], f[e], g[e])] = triples.get((h[e], f[e], g[e]), 0) + 1
    out["multiplicity"] = {
        "form": "unit" if high == 1 else "uniform" if low == high else "nonuniform",
        "min": low, "max": high,
        "triple_min": min(triples.values()), "triple_max": max(triples.values()),
        "duplicate_iteration_points": n - len(set(points))}
    if low != high:
        reference = image[(P[0], I[0], J[0], K[0])]
        second = next(e for e in range(1, n) if image[(P[e], I[e], J[e], K[e])] != reference)
        cx.append(("multiplicity_differs", 0, second))

    classes = {classify(b["i"], b["j"], b["k"]) for b in batches}
    if len(classes) > 1:
        shape = "mixed"
    elif classes == {"scalar"} and len(batches) > 1:
        shape = "element_wise"
    else:
        shape = next(iter(classes))
    out["shape"] = {"class": shape, "events": n, "triples": len(triples), "points": len(image),
                    "distinct_p": len(set(P)), "distinct_i": len(set(I)),
                    "distinct_j": len(set(J)), "distinct_k": len(set(K)), "batches": batches}
    strict = all(b["i"] >= 2 and b["j"] >= 2 and b["k"] >= 2 for b in batches)
    injective = all(inj.values())
    unit = high == 1
    out["strict_non_degenerate"] = strict
    out["genuine"] = valid and box and unit and injective and strict
    out["unique_up_to_relabelling"] = valid and box and injective
    open_count = 0
    if valid:
        open_count = 1 + (not box) + (not unit) + (not inj["h"]) + (not inj["f"]) + (not inj["g"]) + share + (not strict)
    out["eligibility"] = {"form": "for_later_verification" if valid else "not_eligible",
                          "open_count": open_count}
    out["counterexamples"] = cx
    return out


def compare(report, expected):
    """The fields that differ, as text. Empty when the report is exact."""
    bad = []
    for section in ("validity", "cartesian", "multiplicity", "injectivity"):
        for key, value in expected[section].items():
            if report[section][key] != value:
                bad.append(f"{section}.{key}: {report[section][key]} != {value}")
    for key, value in expected["shape"].items():
        if report["shape"][key] != value:
            bad.append(f"shape.{key}: {report['shape'][key]} != {value}")
    for key in ("strict_non_degenerate", "genuine", "unique_up_to_relabelling"):
        if report[key] != expected[key]:
            bad.append(f"{key}: {report[key]} != {expected[key]}")
    eligibility = report["eligibility"]
    if eligibility["form"] != expected["eligibility"]["form"]:
        bad.append("eligibility")
    if len(eligibility["open_obligations"]) != expected["eligibility"]["open_count"]:
        bad.append("open obligations")
    got = []
    for c in report["counterexamples"]:
        if c["kind"] == "missing_point":
            got.append(("missing_point", tuple(c["labels"])))
        else:
            got.append((c["kind"], c["first"], c["second"]))
    if got != expected["counterexamples"]:
        bad.append(f"counterexamples: {got} != {expected['counterexamples']}")
    return bad


def check_support(events, c, support, function_of_point):
    """Each listed set determines its coordinate and no smaller set does, and the
    report is absent only when some coordinate is not determined by all of x."""
    size = len(events[0]["x"])

    def determined(axis, entries):
        given = None if axis == "p" else "p"
        seen = {}
        for e, event in enumerate(events):
            key = (c[given][e] if given else 0,) + tuple(event["x"][a] for a in entries)
            if seen.setdefault(key, c[axis][e]) != c[axis][e]:
                return False
        return True

    if support is None:
        if function_of_point and all(determined(axis, range(size)) for axis in "pijk"):
            return "support is absent, and x determines every coordinate"
        return ""
    for axis in "pijk":
        listed = support[axis]
        if not determined(axis, listed):
            return f"support of {axis} does not determine it"
        if listed and any(determined(axis, smaller) for smaller in
                          itertools.combinations(range(size), len(listed) - 1)):
            return f"support of {axis} is not smallest"
    return ""
