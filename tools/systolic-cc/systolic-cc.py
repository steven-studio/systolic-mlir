#!/usr/bin/env python3
"""systolic-cc -- compile a C program, running the matrix multiplications
the compiler recognizes on the systolic-array FPGA.

    systolic-cc.py [options] FILE.c -o PROGRAM

Function by function:

  1. tools/systolic-cfront (clang 24 -ast-dump=json) translates every
     function inside the supported C subset to MLIR; the others (main, I/O,
     anything outside the subset) are reported as notes and stay C.
  2. --affine-matmul-to-systolic -- the existing GEMM recognition and exact
     Affine Partitioning over the systolic.device fleet -- decides which of
     them hold a GEMM it can lower; its remarks say why the others do not.
  3. Those functions ("offloaded") go through the existing pipeline:
     scheduled systolic.matmul_tile -> --lower-systolic-matmul-tile-to-fpga
     -> --systolic-runtime-to-llvm -> mlir-translate -> llc.
  4. The rest of the program is compiled by clang 24 as written, except that
     each offloaded function's definition is renamed NAME__systolic_reference
     (a prototype of NAME takes its place).
  5. A generated C shim defines NAME with its original C signature: it passes
     the arrays to the compiled kernel, then asks the runtime whether any
     FPGA invocation failed (runtime/offload/systolic_offload.h) -- a failure
     is reported as such and the program exits 3 instead of continuing with
     an incomplete result. With --verify it also runs NAME__systolic_reference
     on copies of the arrays NAME writes and compares: a mismatch is reported
     as a numerical error (exit 4).
  6. Everything is linked with a runtime: --runtime=soft (default), the
     software model of the runtime ABI, NOT the FPGA; --runtime=board, the
     real runtime (DPTI to the programmed board).

Intermediate files are kept in PROGRAM.systolic/ (--keep-dir overrides).

Exit status: 0 compiled; 1 a compilation step failed (its diagnostics on
stderr); 2 usage or tool-chain error.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.realpath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CFRONT = os.path.join(ROOT, "tools", "systolic-cfront", "systolic-cfront.py")
OFFLOAD_RT = os.path.join(ROOT, "runtime", "offload")

KERNEL_SUFFIX = "__systolic_kernel"
REFERENCE_SUFFIX = "__systolic_reference"
C_TYPES = {"f32": "float", "f64": "double"}


class Failed(Exception):
    pass


def log(msg):
    print("systolic-cc: " + msg, file=sys.stderr)


class Driver:
    def __init__(self, args):
        self.args = args
        self.keep = args.keep_dir
        os.makedirs(self.keep, exist_ok=True)
        self.commands = open(os.path.join(self.keep, "commands.txt"), "w")

    def path(self, name):
        return os.path.join(self.keep, name)

    def run(self, cmd, what, capture=False):
        self.commands.write(" ".join(_quote(c) for c in cmd) + "\n")
        self.commands.flush()
        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode != 0:
            sys.stderr.write(proc.stderr)
            raise Failed("%s failed (exit %d): %s" % (what, proc.returncode,
                                                      cmd[0]))
        return proc

    # ------------------------------------------------------------- the fleet
    def fleet_k_max(self):
        a = self.args
        if a.runtime == "board":
            config = a.fpga_config or os.environ.get("SYSTOLIC_FPGA_CONFIG")
            if not config:
                raise Failed("--runtime=board needs --fpga-config (or "
                             "SYSTOLIC_FPGA_CONFIG): the fpga_config.txt of "
                             "the bitstream on the board")
            rt = os.path.join(a.build_dir, "bin", "runtime-dispatch-test")
            proc = subprocess.run([rt, "--print-config"], capture_output=True,
                                  text=True,
                                  env=dict(os.environ,
                                           SYSTOLIC_FPGA_CONFIG=config))
            if proc.returncode != 0:
                raise Failed("the runtime refuses %s: %s" %
                             (config, proc.stdout + proc.stderr))
            vals = dict(l.split("=", 1) for l in proc.stdout.splitlines()
                        if "=" in l)
            self.fpga_config = config
            return int(vals["K_MAX_8X8"]), int(vals["K_MAX_4X4"])
        return a.k_max_8x8, a.k_max_4x4

    # ------------------------------------------------------------------ main
    def compile(self):
        a = self.args
        opt = os.path.join(a.build_dir, "bin", "systolic-opt")
        k8, k4 = self.fleet_k_max()
        fleet = "--systolic-declare-fpga-fleet=k-max-8x8=%d k-max-4x4=%d" % \
            (k8, k4)
        phys = "--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=%d " \
               "phys-k-max-4x4=%d" % (k8, k4)
        cdefs = ["-D" + d for d in a.defines] + ["-I" + i for i in a.includes]
        fdefs = ["-D" + d for d in a.defines] + ["-I" + i for i in a.includes]

        src = os.path.realpath(a.source)
        shutil.copyfile(src, self.path("00-source.c"))

        # 1. C -> MLIR for every function in the subset.
        proc = self.run(["python3", CFRONT, "--clang", a.clang,
                         "-std=" + a.std, "--skip-unsupported",
                         "--symbol-suffix=" + KERNEL_SUFFIX,
                         "--interface", self.path("10-interface.json"),
                         src, "-o", self.path("10-frontend.mlir")] + fdefs,
                        "the C front end")
        sys.stderr.write(proc.stderr)
        interface = json.load(open(self.path("10-interface.json")))
        functions = {f["name"]: f for f in interface["functions"]}

        # 2. Which of them hold a GEMM the existing pass lowers.
        offload = []
        if functions:
            proc = self.run([opt, fleet, "--affine-matmul-to-systolic=report=true",
                             self.path("10-frontend.mlir"),
                             "-o", self.path("20-systolic.mlir")],
                            "GEMM recognition / Affine Partitioning")
            remarks = proc.stderr
            open(self.path("20-remarks.txt"), "w").write(remarks)
            tiles = tiles_per_function(open(self.path("20-systolic.mlir")).read())
            for name, fn in functions.items():
                n = tiles.get(fn["symbol"], 0)
                why = None
                if n == 0:
                    why = "no GEMM the systolic arrays can run was " \
                          "recognized in it" + remark_reasons(remarks, fn)
                elif fn["storage_class"] == "static" or fn["inline"]:
                    why = "it is static or inline (offloading replaces an " \
                          "external definition)"
                elif fn["from_macro"]:
                    why = "its definition comes from a macro expansion"
                elif any(p["kind"] == "memref" and p["leading_min"] < 0
                         for p in fn["params"]):
                    why = "it reads an array before its first element"
                elif any(p["kind"] == "memref" and p["elem"] != "f32"
                         for p in fn["params"]):
                    why = "the FPGA datapath is fp32 and it uses double " \
                          "arrays"
                if why:
                    log("%s:%d:%d: note: '%s' stays on the host: %s" %
                        (a.source, fn["line"], fn["col"], name, why))
                else:
                    log("%s:%d:%d: note: '%s' is offloaded: %d systolic tile "
                        "invocation(s) on the 1x8x8 + 3x4x4 fleet" %
                        (a.source, fn["line"], fn["col"], name, n))
                    offload.append(fn)
        plan = {"source": src, "runtime": a.runtime, "k_max_8x8": k8,
                "k_max_4x4": k4, "verify": a.verify,
                "offloaded": [f["name"] for f in offload],
                "translated_not_offloaded":
                    sorted(set(functions) - {f["name"] for f in offload})}
        json.dump(plan, open(self.path("25-plan.json"), "w"), indent=1)

        objects = []
        # 3. The offloaded functions through the FPGA pipeline.
        if offload:
            self.run(["python3", CFRONT, "--clang", a.clang, "-std=" + a.std,
                      "--symbol-suffix=" + KERNEL_SUFFIX, src,
                      "-o", self.path("30-kernels.mlir")] + fdefs +
                     ["--function=" + f["name"] for f in offload],
                     "the C front end (offloaded functions)")
            self.run([opt, fleet, "--affine-matmul-to-systolic",
                      self.path("30-kernels.mlir"),
                      "-o", self.path("40-systolic.mlir")],
                     "Affine -> Systolic")
            self.run([opt, phys, self.path("40-systolic.mlir"),
                      "-o", self.path("50-fpga.mlir")], "FPGA lowering")
            self.run([opt, "--systolic-runtime-to-llvm",
                      self.path("50-fpga.mlir"),
                      "-o", self.path("60-llvm.mlir")],
                     "bufferization / LLVM dialect")
            self.run([os.path.join(a.llvm_build, "bin", "mlir-translate"),
                      "--mlir-to-llvmir", self.path("60-llvm.mlir"),
                      "-o", self.path("70-kernels.ll")], "mlir-translate")
            self.run([os.path.join(a.llvm_build, "bin", "llc"), "-O0",
                      "-filetype=obj", "-relocation-model=pic",
                      self.path("70-kernels.ll"),
                      "-o", self.path("70-kernels.o")], "llc")
            objects.append(self.path("70-kernels.o"))

        # 4. The program itself, with the offloaded definitions renamed.
        host = rewrite_host(open(src, "rb").read().decode(), src, offload)
        open(self.path("80-host.c"), "w").write(host)
        base_flags = ["-std=" + a.std, a.opt_level] + cdefs + \
            ["-I" + os.path.dirname(src)]
        self.run([a.clang] + base_flags + ["-c", self.path("80-host.c"),
                                           "-o", self.path("80-host.o")],
                 "compiling the program")
        objects.append(self.path("80-host.o"))

        # 5. The call shims.
        if offload:
            open(self.path("85-shim.c"), "w").write(
                shim_source(offload, a.verify, a.verify_rtol, a.source))
            self.run([a.clang, "-std=c11", "-O1", "-I" + OFFLOAD_RT, "-c",
                      self.path("85-shim.c"), "-o", self.path("85-shim.o")],
                     "compiling the call shims")
            objects.append(self.path("85-shim.o"))

        # 6. Link with a runtime.
        link = [a.clang] + objects + ["-I" + OFFLOAD_RT]
        if a.runtime == "soft":
            link += [os.path.join(OFFLOAD_RT, "soft_runtime.c"),
                     "-DSOFT_DEFAULT_K_MAX_8X8=%d" % k8,
                     "-DSOFT_DEFAULT_K_MAX_4X4=%d" % k4]
        else:
            ftdi = subprocess.run(["pkg-config", "--libs", "libftdi1"],
                                  capture_output=True, text=True).stdout.split()
            link += [os.path.join(OFFLOAD_RT, "board_stats.c"),
                     "-I" + os.path.join(ROOT, "runtime"),
                     os.path.join(a.build_dir, "runtime",
                                  "libsystolic_fpga_runtime.a")] + ftdi + \
                ["-lstdc++"]
        if offload:
            libdir = os.path.join(a.llvm_build, "lib")
            link += ["-L" + libdir, "-lmlir_c_runner_utils",
                     "-Wl,-rpath," + libdir]
        link += ["-lm", "-o", a.output]
        self.run(link, "linking")
        log("wrote %s (%d function(s) offloaded, runtime: %s; intermediates "
            "in %s)" % (a.output, len(offload),
                        "software model, NOT the FPGA" if a.runtime == "soft"
                        else "FPGA board", self.keep))


def _quote(s):
    return s if re.fullmatch(r"[\w@%+=:,./-]+", s) else "'" + \
        s.replace("'", "'\\''") + "'"


def tiles_per_function(mlir):
    """func symbol -> number of systolic.matmul_tile ops in its body."""
    out, current = {}, None
    for line in mlir.splitlines():
        m = re.match(r"\s*func\.func (?:\w+ )?@(\S+?)\(", line)
        if m:
            current = m.group(1)
            out.setdefault(current, 0)
        elif current and "systolic.matmul_tile" in line:
            out[current] += 1
    return out


def remark_reasons(remarks, fn):
    """The pass's own reasons for a function's nests (its remarks carry the
    source line of the loop nest; the function spans its definition)."""
    reasons = re.findall(r":(\d+):\d+: remark: affine GEMM not lowered to "
                         r"systolic\.matmul_tile: (.*)", remarks)
    lines = [r for r in reasons
             if fn["line"] <= int(r[0]) <= fn.get("end_line", fn["line"])]
    return "".join(" (line %s: %s)" % r for r in lines[:1])


def rewrite_host(text, src, offload):
    """The program with each offloaded function's definition renamed to
    NAME__systolic_reference, a prototype of NAME before it, and #line
    directives so that diagnostics keep pointing at the source."""
    data = text.encode()
    edits = []
    for fn in offload:
        begin, body, name_at = fn["def_begin"], fn["body_begin"], \
            fn["name_offset"]
        prototype = data[begin:body].decode().rstrip()
        line = data[:begin].count(b"\n") + 1
        col = begin - (data.rfind(b"\n", 0, begin) + 1)
        insert = "\n%s;\n#line %d \"%s\"\n%s" % (prototype, line, src,
                                                 " " * col)
        edits.append((begin, begin, insert))
        edits.append((name_at, name_at + fn["name_length"],
                      fn["name"] + REFERENCE_SUFFIX))
    for start, end, new in sorted(edits, reverse=True):
        data = data[:start] + new.encode() + data[end:]
    return '#line 1 "%s"\n' % src + data.decode()


def declarator(ctype, name):
    """A C declaration of `name` with the type clang printed."""
    if "(*" in ctype:
        i = ctype.index(")")
        return ctype[:i] + " " + name + ctype[i:]
    return ctype + " " + name


def shim_source(offload, verify, rtol, source):
    out = ["/* Generated by systolic-cc: the C entry points of the functions "
           "it offloaded",
           " * from %s. */" % os.path.basename(source),
           "#include \"systolic_offload.h\"",
           "#include <math.h>", "#include <stdint.h>", "#include <stdio.h>",
           "#include <stdlib.h>", "#include <string.h>", "",
           "static void close_runtime(void) { systolic_offload_close(); }",
           "static void once(void) {",
           "  static int done;",
           "  if (!done) { done = 1; atexit(close_runtime); }",
           "}", ""]
    for fn in offload:
        params = fn["params"]
        decl = ", ".join(declarator(p["ctype"], p["name"]) for p in params)
        kargs, kdecl = [], []
        for p in params:
            if p["kind"] == "memref":
                ct = C_TYPES[p["elem"]]
                sizes = [p["leading_extent"]] + p["inner_dims"]
                strides = []
                acc = 1
                for d in reversed(sizes[1:]):
                    strides.insert(0, acc)
                    acc *= d
                strides.insert(0, acc)
                ptr = "(%s *)%s" % (ct, p["name"])
                kargs += [ptr, ptr, "0"] + [str(s) for s in sizes] + \
                    [str(s) for s in strides]
                kdecl += ["%s *" % ct] * 2 + ["int64_t"] * (1 + 2 * len(sizes))
            elif p["kind"] == "scalar":
                kargs.append(p["name"])
                kdecl.append(C_TYPES[p["elem"]])
            else:
                kargs.append(p["name"])
                kdecl.append("int")
        name, symbol = fn["name"], fn["symbol"]
        where = "%s:%d" % (os.path.basename(source), fn["line"])
        out.append("void %s(%s);" % (symbol, ", ".join(kdecl) or "void"))
        if verify:
            out.append("void %s%s(%s);" % (name, REFERENCE_SUFFIX, decl or "void"))
        out.append("void %s(%s) {" % (name, decl or "void"))
        out.append("  systolic_offload_stats_t before, after;")
        out.append("  once();")
        out.append("  systolic_offload_stats(&before);")
        written = [p for p in params if p["kind"] == "memref" and p["written"]]
        if verify:
            for p in written:
                ct = C_TYPES[p["elem"]]
                count = p["leading_extent"]
                for d in p["inner_dims"]:
                    count *= d
                out.append("  %s *ref_%s = malloc(%d * sizeof(%s) + 1);" %
                           (ct, p["name"], count, ct))
                out.append("  memcpy(ref_%s, %s, %d * sizeof(%s));" %
                           (p["name"], p["name"], count, ct))
            refargs = []
            for p in params:
                if p in written:
                    refargs.append("(void *)ref_" + p["name"])
                else:
                    refargs.append(p["name"])
            out.append("  %s%s(%s);" % (name, REFERENCE_SUFFIX,
                                        ", ".join(refargs)))
        out.append("  %s(%s);" % (symbol, ", ".join(kargs)))
        out.append("  systolic_offload_stats(&after);")
        out.append("  unsigned long long inv = 0;")
        out.append("  for (int d = 0; d < 4; ++d)")
        out.append("    inv += after.per_device[d] - before.per_device[d];")
        out.append("  if (after.failed != before.failed) {")
        out.append("    fprintf(stderr, \"systolic-cc: %s (%s): RUNTIME "
                   "FAILURE: %%llu of the FPGA invocations failed (first rc "
                   "%%d) on %%s; the result is incomplete -- a runtime / "
                   "transport failure, not a numerical result\\n\", "
                   "after.failed - before.failed, after.first_error, "
                   "after.backend);" % (name, where))
        out.append("    exit(3);")
        out.append("  }")
        if verify:
            out.append("  unsigned long long checked = 0, bad = 0;")
            for p in written:
                ct = C_TYPES[p["elem"]]
                count = p["leading_extent"]
                for d in p["inner_dims"]:
                    count *= d
                out.append("  for (long e = 0; e < %d; ++e) {" % count)
                out.append("    %s got = ((%s *)%s)[e], want = ref_%s[e];" %
                           (ct, ct, p["name"], p["name"]))
                if rtol > 0:
                    out.append("    int same = fabs((double)got - want) <= "
                               "%r * fmax(fabs((double)got), fabs((double)"
                               "want));" % rtol)
                else:
                    out.append("    int same = memcmp(&got, &want, "
                               "sizeof got) == 0;")
                out.append("    ++checked;")
                out.append("    if (!same && bad++ == 0)")
                out.append("      fprintf(stderr, \"systolic-cc: %s (%s): "
                           "first mismatch %s[%%ld]: FPGA path %%.9g, CPU "
                           "reference %%.9g\\n\", e, (double)got, "
                           "(double)want);" % (name, where, p["name"]))
                out.append("  }")
                out.append("  free(ref_%s);" % p["name"])
            out.append("  if (bad) {")
            out.append("    fprintf(stderr, \"systolic-cc: %s (%s): NUMERICAL "
                       "MISMATCH: %%llu of %%llu elements differ from the CPU "
                       "reference (%s; %%llu invocations on %%s)\\n\", bad, "
                       "checked, inv, after.backend);" %
                       (name, where, "bitwise" if rtol <= 0 else
                        "relative tolerance %g" % rtol))
            out.append("    exit(4);")
            out.append("  }")
            out.append("  fprintf(stderr, \"systolic-cc: %s (%s): verified: "
                       "%%llu elements equal the CPU reference (%s); %%llu "
                       "invocations (8x8 %%llu, 4x4 %%llu/%%llu/%%llu) on "
                       "%%s\\n\", checked, inv, "
                       "after.per_device[0] - before.per_device[0], "
                       "after.per_device[1] - before.per_device[1], "
                       "after.per_device[2] - before.per_device[2], "
                       "after.per_device[3] - before.per_device[3], "
                       "after.backend);" %
                       (name, where, "bitwise" if rtol <= 0 else
                        "relative tolerance %g" % rtol))
        out.append("}")
        out.append("")
    return "\n".join(out) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("source")
    p.add_argument("-o", dest="output", required=True)
    p.add_argument("--clang", default=os.environ.get("SYSTOLIC_CFRONT_CLANG"),
                   help="clang 24 (default: $SYSTOLIC_CFRONT_CLANG)")
    p.add_argument("--build-dir", default=os.environ.get(
        "SYSTOLIC_BUILD", os.path.join(ROOT, "build")))
    p.add_argument("--llvm-build", default=os.environ.get(
        "LLVM_BUILD", os.path.expanduser("~/work/llvm-project/build")))
    p.add_argument("--runtime", choices=["soft", "board"], default="soft")
    p.add_argument("--fpga-config",
                   help="--runtime=board: the bitstream's fpga_config.txt "
                        "(default $SYSTOLIC_FPGA_CONFIG)")
    p.add_argument("--k-max-8x8", type=int, default=16,
                   help="--runtime=soft: the fleet's k_max (default 16)")
    p.add_argument("--k-max-4x4", type=int, default=16)
    p.add_argument("--verify", action="store_true",
                   help="check every offloaded call against the function "
                        "compiled for the CPU")
    p.add_argument("--verify-rtol", type=float, default=0.0,
                   help="relative tolerance of --verify (default 0: bitwise)")
    p.add_argument("--keep-dir", help="where the intermediates go "
                                      "(default PROGRAM.systolic)")
    p.add_argument("-D", dest="defines", action="append", default=[])
    p.add_argument("-I", dest="includes", action="append", default=[])
    p.add_argument("-std", dest="std", default="c11")
    p.add_argument("-O", dest="opt", default="0",
                   help="optimization level of the host code (default 0)")
    a = p.parse_args()
    a.opt_level = "-O" + a.opt
    if not a.clang:
        log("no clang 24 given (--clang or SYSTOLIC_CFRONT_CLANG)")
        return 2
    a.keep_dir = a.keep_dir or a.output + ".systolic"
    try:
        Driver(a).compile()
    except Failed as e:
        log("error: " + str(e))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
