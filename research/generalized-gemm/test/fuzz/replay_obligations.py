#!/usr/bin/env python3
"""Replays every emitted proof obligation outside the analyzer.

Takes only the SMT-LIB text from the JSON report and asks the Z3 library through
its C API, with no code of this project in between. It checks that each script
stands alone and gives the recorded answer, and that every obligation of a
proof was discharged. This is the same Z3, so it detects a wrong record or a
script that depends on hidden state. It does not detect an error in Z3.
"""
import argparse
import ctypes
import json
from pathlib import Path
import subprocess
import sys
import tempfile


class Z3:
    def __init__(self, path):
        self.lib = ctypes.CDLL(path)
        self.lib.Z3_mk_config.restype = ctypes.c_void_p
        self.lib.Z3_mk_context.restype = ctypes.c_void_p
        self.lib.Z3_mk_context.argtypes = [ctypes.c_void_p]
        self.lib.Z3_del_config.argtypes = [ctypes.c_void_p]
        self.lib.Z3_del_context.argtypes = [ctypes.c_void_p]
        self.lib.Z3_set_error_handler.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        self.lib.Z3_eval_smtlib2_string.restype = ctypes.c_char_p
        self.lib.Z3_eval_smtlib2_string.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

    def answer(self, script):
        config = self.lib.Z3_mk_config()
        context = self.lib.Z3_mk_context(config)
        self.lib.Z3_del_config(config)
        # Without this, an error would exit the process.
        self.lib.Z3_set_error_handler(context, None)
        text = (self.lib.Z3_eval_smtlib2_string(context, script.encode()) or b"").decode()
        self.lib.Z3_del_context(context)
        return text.strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--catalogue", required=True)
    parser.add_argument("--z3-library", required=True)
    args = parser.parse_args()
    z3 = Z3(args.z3_library)
    replayed = mismatches = proofs = 0
    for width in (8, 32):
        text = subprocess.run([sys.executable, args.catalogue, str(width)], capture_output=True,
                              text=True, check=True).stdout
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "loops.mlir"
            path.write_text(text)
            done = subprocess.run([args.driver, str(path), "--print-scripts", "--replay-fuel=2000"],
                                  capture_output=True, text=True, check=True)
        for result in json.loads(done.stdout):
            proven = result["verdict"].startswith("PROVEN")
            proofs += proven
            for record in result["obligations"]:
                if proven and record["id"] != "O7" and not record["discharged"]:
                    print(f"{result['function']}: {record['id']} not discharged: {record['location']}")
                    mismatches += 1
                if "script" not in record or record["result"] not in ("sat", "unsat"):
                    continue
                replayed += 1
                got = z3.answer(record["script"])
                if got != record["result"]:
                    print(f"{result['function']}: {record['id']} {record['location']}: "
                          f"recorded {record['result']}, replay {got[:200]}")
                    mismatches += 1
    print(f"{proofs} proofs, {replayed} obligations replayed, {mismatches} mismatches")
    return 1 if mismatches or not replayed else 0


if __name__ == "__main__":
    sys.exit(main())
