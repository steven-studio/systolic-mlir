#!/usr/bin/env python3
"""Prints the A-J loop catalogue as MLIR at one width: gen_loops.py <width>."""
import sys
W = int(sys.argv[1]) if len(sys.argv) > 1 else 8
T = f"i{W}"
MAX = (1 << (W - 1)) - 1


def simple(name, params, init, cmp_, update, entry=""):
    """for (i = init; cmp; update): cmp and update are operation text on %i."""
    args = ", ".join(f"%{p}: {T}" for p in params)
    return f"""func.func @{name}({args}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  %c2 = arith.constant 2 : {T}
  %c3 = arith.constant 3 : {T}
  %c7 = arith.constant 7 : {T}
  %max = arith.constant {MAX} : {T}
{entry}  cf.br ^h({init} : {T})
^h(%i: {T}):
  %g = arith.cmpi {cmp_} : {T}
  cf.cond_br %g, ^b, ^x
^b:
{update}
  cf.br ^h(%u : {T})
^x:
  return
}}
"""


def add(c, flag=""):
    f = f" overflow<{flag}>" if flag else ""
    return f"  %u = arith.addi %i, {c}{f} : {T}"


def sub(c, flag=""):
    f = f" overflow<{flag}>" if flag else ""
    return f"  %u = arith.subi %i, {c}{f} : {T}"


def piecewise(a, b, flag):
    f = f" overflow<{flag}>" if flag else ""
    return f"""func.func @{{name}}(%n: {T}) {{{{
  %c0 = arith.constant 0 : {T}
  %c2 = arith.constant 2 : {T}
  %a = arith.constant {a} : {T}
  %b = arith.constant {b} : {T}
  cf.br ^h(%c0 : {T})
^h(%i: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  cf.cond_br %g, ^body, ^x
^body:
  %r = arith.remsi %i, %c2 : {T}
  %e = arith.cmpi eq, %r, %c0 : {T}
  cf.cond_br %e, ^t, ^f
^t:
  %p = arith.addi %i, %a{f} : {T}
  cf.br ^h(%p : {T})
^f:
  %q = arith.addi %i, %b{f} : {T}
  cf.br ^h(%q : {T})
^x:
  return
}}}}
"""


out = []
out.append(simple("a_wrap", ["n"], "%c0", "slt, %i, %n", add("%c1")))
out.append(simple("a_nsw", ["n"], "%c0", "slt, %i, %n", add("%c1", "nsw")))
out.append(simple("a_le_wrap", ["n"], "%c0", "sle, %i, %n", add("%c1")))
out.append(simple("a_le_nsw", ["n"], "%c0", "sle, %i, %n", add("%c1", "nsw")))
out.append(simple("b3_wrap", ["n"], "%c0", "slt, %i, %n", add("%c3")))
out.append(simple("b3_nsw", ["n"], "%c0", "slt, %i, %n", add("%c3", "nsw")))
out.append(simple("b2_wrap", ["n"], "%c0", "slt, %i, %n", add("%c2")))
out.append(simple("b2_nsw", ["n"], "%c0", "slt, %i, %n", add("%c2", "nsw")))
out.append(piecewise(3, 7, "").replace("{name}", "c_wrap").replace("{{", "{").replace("}}", "}"))
out.append(piecewise(3, 7, "nsw").replace("{name}", "c_nsw").replace("{{", "{").replace("}}", "}"))
out.append(simple("d_dec", ["n"], "%n", "sgt, %i, %c0", sub("%c1")))
out.append(simple("d_dec_ge", ["n"], "%n", "sge, %i, %c0", sub("%c1")))
out.append(simple("d_dec3", ["n"], "%n", "sgt, %i, %c0", sub("%c3")))
out.append(simple("d_dec3_nsw", ["n"], "%n", "sgt, %i, %c0", sub("%c3", "nsw")))
out.append(simple("g_u3", ["n"], "%c0", "ult, %i, %n", add("%c3")))
out.append(simple("g_u2", ["n"], "%c0", "ult, %i, %n", add("%c2")))
out.append(simple("g_ule", ["n"], "%c0", "ule, %i, %n", add("%c1")))
out.append(simple("g_uge0", ["n"], "%n", "uge, %i, %c0", sub("%c1")))
out.append(simple("g_dec3", ["n"], "%n", "ugt, %i, %c0", sub("%c3")))
out.append(simple("g_ne2", ["n"], "%n", "ne, %i, %c0", sub("%c2")))
out.append(simple("g_ne1", ["n"], "%c0", "ne, %i, %n", add("%c1")))
out.append(simple("g_nuw", ["n"], "%c0", "ult, %i, %n", add("%c3", "nuw")))
out.append(simple("h_even", ["n"], "%c0", "slt, %i, %max", add("%c2")))
out.append(f"""func.func @h_true() {{
  %t = arith.constant true
  cf.br ^h
^h:
  cf.cond_br %t, ^h, ^x
^x:
  return
}}
""")
out.append(simple("h_dbl_wrap", ["n"], "%c1", "slt, %i, %n", f"  %u = arith.muli %i, %c2 : {T}"))
out.append(simple("j_dbl_nsw", ["n"], "%c1", "slt, %i, %n", f"  %u = arith.muli %i, %c2 overflow<nsw> : {T}"))
out.append(simple("j_sq_nsw", ["n"], "%c2", "slt, %i, %n", f"  %u = arith.muli %i, %i overflow<nsw> : {T}"))
out.append(f"""func.func @j_halve(%n: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%n : {T})
^h(%x: {T}):
  %g = arith.cmpi sgt, %x, %c0 : {T}
  cf.cond_br %g, ^b, ^e
^b:
  %o = arith.andi %x, %c1 : {T}
  %odd = arith.cmpi eq, %o, %c1 : {T}
  cf.cond_br %odd, ^t, ^f
^t:
  %d = arith.subi %x, %c1 : {T}
  cf.br ^h(%d : {T})
^f:
  %s = arith.shrsi %x, %c1 : {T}
  cf.br ^h(%s : {T})
^e:
  return
}}
""")
for flag, name in (("", "j_step_wrap"), (" overflow<nsw>", "j_step_nsw")):
    out.append(f"""func.func @{name}(%n: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%c0, %c1 : {T}, {T})
^h(%i: {T}, %k: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  cf.cond_br %g, ^b, ^e
^b:
  %p = arith.addi %i, %k{flag} : {T}
  %q = arith.addi %k, %c1{flag} : {T}
  cf.br ^h(%p, %q : {T}, {T})
^e:
  return
}}
""")
out.append(f"""func.func @j_collatz(%n: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  %c3 = arith.constant 3 : {T}
  cf.br ^h(%n : {T})
^h(%x: {T}):
  %g = arith.cmpi ne, %x, %c1 : {T}
  cf.cond_br %g, ^b, ^e
^b:
  %o = arith.andi %x, %c1 : {T}
  %even = arith.cmpi eq, %o, %c0 : {T}
  cf.cond_br %even, ^t, ^f
^t:
  %s = arith.shrui %x, %c1 : {T}
  cf.br ^h(%s : {T})
^f:
  %m = arith.muli %x, %c3 : {T}
  %a = arith.addi %m, %c1 : {T}
  cf.br ^h(%a : {T})
^e:
  return
}}
""")
for name, start in (("e_rect", "%c0"), ("e_tri", "%i")):
    out.append(f"""func.func @{name}(%n: {T}, %m: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^o(%c0 : {T})
^o(%i: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  cf.cond_br %g, ^pre, ^x
^pre:
  cf.br ^in({start} : {T})
^in(%j: {T}):
  %h = arith.cmpi slt, %j, %m : {T}
  cf.cond_br %h, ^b, ^l
^b:
  %u = arith.addi %j, %c1 : {T}
  cf.br ^in(%u : {T})
^l:
  %v = arith.addi %i, %c1 : {T}
  cf.br ^o(%v : {T})
^x:
  return
}}
""")
out.append(f"""func.func @e_lex(%n: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%c0, %c0 : {T}, {T})
^h(%i: {T}, %j: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  cf.cond_br %g, ^b, ^x
^b:
  %p = arith.cmpi sgt, %j, %c0 : {T}
  cf.cond_br %p, ^dec, ^inc
^dec:
  %d = arith.subi %j, %c1 : {T}
  cf.br ^h(%i, %d : {T}, {T})
^inc:
  %v = arith.addi %i, %c1 : {T}
  cf.br ^h(%v, %v : {T}, {T})
^x:
  return
}}
""")
out.append(f"""func.func @t3_swap(%a: {T}, %b: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%a, %b : {T}, {T})
^h(%x: {T}, %y: {T}):
  %g = arith.cmpi sgt, %x, %c0 : {T}
  %h = arith.cmpi sgt, %y, %c0 : {T}
  %both = arith.andi %g, %h : i1
  cf.cond_br %both, ^b, ^e
^b:
  %d = arith.subi %y, %c1 : {T}
  cf.br ^h(%d, %x : {T}, {T})
^e:
  return
}}
""")
out.append(f"""func.func @f_div(%n: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  %c9 = arith.constant 5 : {T}
  cf.br ^h(%n : {T})
^h(%x: {T}):
  %g = arith.cmpi sgt, %x, %c0 : {T}
  cf.cond_br %g, ^b, ^e
^b:
  %d = arith.subi %x, %c1 : {T}
  %q = arith.divsi %c9, %d : {T}
  cf.br ^h(%d : {T})
^e:
  return
}}
""")
out.append(f"""func.func @f_dead_poison(%n: {T}) -> {T} {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  %big = arith.addi %n, %c1 overflow<nsw> : {T}
  cf.br ^h(%c0 : {T})
^h(%i: {T}):
  %g = arith.cmpi ult, %i, %c1 : {T}
  cf.cond_br %g, ^b, ^x
^b:
  %u = arith.addi %i, %c1 : {T}
  cf.br ^h(%u : {T})
^x:
  return %big : {T}
}}
""")
out.append(f"""func.func private @unknown({T}) -> {T}
func.func @i_external(%n: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%c0 : {T})
^h(%i: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  cf.cond_br %g, ^b, ^x
^b:
  %v = call @unknown(%i) : ({T}) -> {T}
  %u = arith.addi %i, %c1 : {T}
  cf.br ^h(%u : {T})
^x:
  return
}}
func.func @helper(%v: {T}, %s: {T}) -> {T} {{
  %r = arith.addi %v, %s : {T}
  return %r : {T}
}}
func.func @i_inlined(%n: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%c0 : {T})
^h(%i: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  cf.cond_br %g, ^b, ^x
^b:
  %u = call @helper(%i, %c1) : ({T}, {T}) -> {T}
  cf.br ^h(%u : {T})
^x:
  return
}}
func.func @i_recursive(%n: {T}) -> {T} {{
  %r = call @i_recursive(%n) : ({T}) -> {T}
  return %r : {T}
}}
""")
# Multiple exits, multiple successors, symbolic steps and bounds, two entries.
out.append(f"""func.func @m_two_exits(%n: {T}, %k: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%c0 : {T})
^h(%i: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  cf.cond_br %g, ^b, ^x
^b:
  %e = arith.cmpi eq, %i, %k : {T}
  cf.cond_br %e, ^y, ^l
^l:
  %u = arith.addi %i, %c1 : {T}
  cf.br ^h(%u : {T})
^x:
  return
^y:
  return
}}
""")
out.append(f"""func.func @m_switch(%n: {T}) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%n : {T})
^h(%x: {T}):
  cf.switch %x : {T}, [default: ^d, 0: ^a, 1: ^a]
^d:
  %s = arith.cmpi slt, %x, %c0 : {T}
  cf.cond_br %s, ^a, ^l
^l:
  %y = arith.subi %x, %c1 : {T}
  cf.br ^h(%y : {T})
^a:
  return
}}
""")
out.append(f"""func.func @s_step(%n: {T}, %s: {T}) {{
  %c0 = arith.constant 0 : {T}
  cf.br ^h(%c0 : {T})
^h(%i: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  cf.cond_br %g, ^b, ^x
^b:
  %u = arith.addi %i, %s overflow<nsw> : {T}
  cf.br ^h(%u : {T})
^x:
  return
}}
""")
out.append(f"""func.func @s_bounds(%a: {T}, %b: {T}) {{
  %c1 = arith.constant 1 : {T}
  cf.br ^h(%a : {T})
^h(%i: {T}):
  %g = arith.cmpi slt, %i, %b : {T}
  cf.cond_br %g, ^l, ^x
^l:
  %u = arith.addi %i, %c1 : {T}
  cf.br ^h(%u : {T})
^x:
  return
}}
""")
out.append(f"""func.func @m_irreducible(%n: {T}, %c: i1) {{
  %c0 = arith.constant 0 : {T}
  %c1 = arith.constant 1 : {T}
  cf.cond_br %c, ^a(%c0 : {T}), ^b(%c0 : {T})
^a(%i: {T}):
  %g = arith.cmpi slt, %i, %n : {T}
  %u = arith.addi %i, %c1 : {T}
  cf.cond_br %g, ^b(%u : {T}), ^x
^b(%j: {T}):
  %h = arith.cmpi slt, %j, %n : {T}
  %v = arith.addi %j, %c1 : {T}
  cf.cond_br %h, ^a(%v : {T}), ^x
^x:
  return
}}
""")
print("\n".join(out))
