// REQUIRES: cfront-clang
//
// Loop headers the front end must refuse although they look like counting
// down (or up): a zero step never reaches the bound; a step against the
// condition's direction runs until the counter overflows; an unsigned counter
// counting down past 0 wraps around and never ends. Each fails with its own
// diagnostic and produces no MLIR.
//
// RUN: not %cfront --function=zero_step_down %s -o %t.1 2>&1 | FileCheck %s --check-prefix=ZERODOWN
// RUN: not %cfront --function=zero_step_up %s -o %t.2 2>&1 | FileCheck %s --check-prefix=ZEROUP
// RUN: not %cfront --function=up_condition_down_step %s -o %t.3 2>&1 | FileCheck %s --check-prefix=UPDOWN
// RUN: not %cfront --function=down_condition_up_step %s -o %t.4 2>&1 | FileCheck %s --check-prefix=DOWNUP
// RUN: not %cfront --function=unsigned_ge_zero %s -o %t.5 2>&1 | FileCheck %s --check-prefix=UNSIGNEDGE
// RUN: not %cfront --function=unsigned_skips_zero %s -o %t.6 2>&1 | FileCheck %s --check-prefix=UNSIGNEDGT
// RUN: not test -e %t.1

void zero_step_down(float a[restrict 8]) {
  for (int i = 7; i >= 0; i -= 0)
    a[i] = 1.0f;
}

void zero_step_up(float a[restrict 8]) {
  for (int i = 0; i < 8; i += 0)
    a[i] = 1.0f;
}

void up_condition_down_step(float a[restrict 8]) {
  for (int i = 0; i < 8; i--)
    a[0] = 1.0f;
}

void down_condition_up_step(float a[restrict 8]) {
  for (int i = 7; i > 0; i += 2)
    a[0] = 1.0f;
}

// i >= 0 always holds for an unsigned counter: 5, 3, 1, then 1 - 2 wraps.
void unsigned_ge_zero(float a[restrict 8]) {
  for (unsigned i = 5; i >= 0; i -= 2)
    a[0] = 1.0f;
}

// 5, 3, 1, then 1 - 2 wraps past 0 and i > 0 holds again.
void unsigned_skips_zero(float a[restrict 8]) {
  for (unsigned i = 5; i > 0; i -= 2)
    a[0] = 1.0f;
}

// ZERODOWN: downward_unsupported.c:18:27: error: the loop step must not be zero
// ZEROUP: downward_unsupported.c:23:26: error: the loop step must not be zero
// UPDOWN: downward_unsupported.c:28:26: error: the loop step must be positive (the condition '<' counts up)
// DOWNUP: downward_unsupported.c:33:26: error: the loop step must be negative (the condition '>' counts down)
// UNSIGNEDGE: downward_unsupported.c:39:3: error: the loop counter could overflow 'unsigned int'
// UNSIGNEDGT: downward_unsupported.c:45:3: error: the loop counter could overflow 'unsigned int'
