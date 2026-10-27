"""Independent source-execution and runtime-counter oracles for site identity."""
import json
from pathlib import Path
import re
import sys

mode, filename = sys.argv[1:3]
if mode == "front":
    sites = json.loads(Path(filename).read_text())["sites"]
    assert len(sites) == 1, sites
    site = sites[0]
    assert site["function"] == "shared_site" and site["report"] is True
    assert site["executions"] == 2
    assert len(site["anchors"]) == 2, site
    assert site["anchors"][1]["begin"] == site["begin"]
    assert site["anchors"][0]["begin"] < site["begin"]
elif mode == "metadata":
    directory = Path(filename)
    sites = json.loads((directory / "10-gemm-sites.json").read_text())["sites"]
    sites = [s for s in sites if s["function"] in ("same_line", "mixed")]
    assert len(sites) == 4, sites
    for name in ("same_line", "mixed"):
        pair = [s for s in sites if s["function"] == name]
        assert pair[0]["line"] == pair[1]["line"]
        assert pair[0]["col"] != pair[1]["col"]
        assert [s["executions"] for s in pair] == [16, 1]
        assert len(pair[0]["anchors"]) == 2
        assert len(pair[1]["anchors"]) == 1
        assert pair[0]["anchors"][0]["col"] != pair[0]["col"]
        assert not ({a["begin"] for a in pair[0]["anchors"]} &
                    {a["begin"] for a in pair[1]["anchors"]})
    records = re.findall(r":(\d+):(\d+): remark: affine GEMM .*? lowered as (\d+) exact region",
                         (directory / "20-remarks.txt").read_text())
    # Three lowered nests, four original sites, twelve jobs: distinct units.
    assert len(records) == 3, records
    assert sum(int(r[2]) for r in records) == 12
    for line, col, _ in records:
        matches = [s for s in sites if any(a["line"] == int(line) and
                    a["col"] == int(col) for a in s["anchors"])]
        assert len(matches) == 1, (line, col, matches)
elif mode == "run":
    policy = sys.argv[3]
    lines = Path(filename).read_text().splitlines()
    serial = 0
    for case, name in enumerate(("same_line", "mixed")):
        assert lines.pop(0) == "BEGIN " + name
        fpga = policy in ("hardware", "fallback") and not (policy == "fallback" and case == 0)
        for execution in range(17):
            serial += 1
            unit = "FPGA" if fpga and (case == 0 or execution < 16) else "CPU"
            assert lines.pop(0) == f"[systolic-cc] {unit} Executed: GEMM #{serial}"
        end = re.fullmatch(r"END jobs=(\d+) failed=(\d+) equal=1", lines.pop(0))
        assert end, lines
        jobs, failed = map(int, end.groups())
        if policy == "strict":
            assert (jobs, failed) == (0, 0)
        elif policy == "fallback" and case == 0:
            assert failed == 1 and jobs < 7, (jobs, failed)
        else:
            assert (jobs, failed) == ((7 if case == 0 else 5), 0), (jobs, failed)
    assert not lines, lines
elif mode == "tiled":
    labels = []
    checked = set()
    expected = {"m12_t4": 3, "m16_t1": 16, "m17_t8": 3,
                "mnk": 27, "mnk_swapped": 27, "mnk_commuted": 27}
    for line in Path(filename).read_text().splitlines():
        if line.startswith("[systolic-cc]"):
            labels.append(line)
        elif line.startswith("[case]"):
            name = line.split()[1].rstrip(":")
            if name in expected:
                assert len(labels) == expected[name], (name, labels)
                assert all("FPGA Executed:" in label for label in labels), (name, labels)
                checked.add(name)
            labels = []
    assert checked == set(expected), checked
else:
    raise AssertionError(mode)
