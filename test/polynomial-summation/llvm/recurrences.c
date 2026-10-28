// Unsigned source arithmetic is modulo 2^64. n counts updates, including zero.
// No signed overflow assumptions, per-power sum formulas, or hand-written IR
// closed forms. clang's frontend IR and opt's actual output are retained.
#include <stdint.h>

#define POWER_SUM(NAME, TERM)                         \
  uint64_t NAME(uint64_t n) {                         \
    uint64_t s = 0;                                   \
    for (uint64_t t = 0; t < n; ++t) {                \
      uint64_t i = t + 1;                             \
      s += (TERM);                                   \
    }                                                \
    return s;                                        \
  }

POWER_SUM(sum_p1, i)
POWER_SUM(sum_p2, i * i)
POWER_SUM(sum_p3, i * i * i)
POWER_SUM(sum_p4, i * i * i * i)
POWER_SUM(sum_p5, i * i * i * i * i)
POWER_SUM(sum_p6, i * i * i * i * i * i)
POWER_SUM(sum_p7, i * i * i * i * i * i * i)
POWER_SUM(sum_p8, i * i * i * i * i * i * i * i)
POWER_SUM(general_cubic, 7 * i * i * i - 3 * i * i + 11 * i - 17)

uint64_t shifted_square(uint64_t n, uint64_t start, uint64_t step,
                        uint64_t initial) {
  uint64_t i = start, s = initial;
  for (uint64_t t = 0; t < n; ++t) {
    s += i * i;
    i += step;
  }
  return s;
}

uint64_t triangular(uint64_t n) {
  uint64_t x = 0, y = 0, z = 0;
  for (uint64_t t = 0; t < n; ++t) {
    z += y; // old y
    y += x * x; // old x
    x += 1;
  }
  return z;
}

uint64_t nonlinear_self(uint64_t n) {
  uint64_t x = 2;
  for (uint64_t t = 0; t < n; ++t)
    x = x * x + 1;
  return x;
}

uint64_t exponential(uint64_t n) {
  uint64_t x = 1;
  for (uint64_t t = 0; t < n; ++t)
    x *= 2;
  return x;
}

uint64_t unknown_exit(uint64_t n) {
  uint64_t x = 2;
  while (x < n)
    x = x * x + 1;
  return x;
}

uint64_t fixed_square(void) {
  uint64_t s = 0;
  for (uint64_t i = 1; i <= 10; ++i)
    s += i * i;
  return s;
}

uint64_t observable_square(uint64_t n, volatile uint64_t *out) {
  uint64_t s = 0;
  for (uint64_t t = 0; t < n; ++t) {
    uint64_t i = t + 1;
    s += i * i;
    *out = s;
  }
  return s;
}
