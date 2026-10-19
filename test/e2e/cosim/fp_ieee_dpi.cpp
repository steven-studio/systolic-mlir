// fp_ieee_dpi.cpp -- the two DPI functions fp_ieee.sv imports, on their own.
// BENCH ONLY.
//
// test/e2e/cosim/dpti_cosim.cpp carries the same two functions next to the
// socket server; a bench that uses fp_ieee.sv without the socket (the
// tb_dpti_direct bench) links this file instead. Same arithmetic, byte for
// byte: binary32 round-to-nearest-even on the host, denormals flushed to
// zero on the way in and out (the Xilinx floating-point IP's behaviour).

#include <cstdint>
#include <cstring>

namespace {

uint32_t ftz(uint32_t bits) {
  if ((bits & 0x7f800000u) == 0)
    return bits & 0x80000000u;
  return bits;
}

float as_float(uint32_t bits) {
  float f;
  std::memcpy(&f, &bits, sizeof f);
  return f;
}

uint32_t as_bits(float f) {
  uint32_t bits;
  std::memcpy(&bits, &f, sizeof bits);
  return bits;
}

} // namespace

extern "C" {

unsigned int cosim_f32_mul(unsigned int a, unsigned int b) {
  return ftz(as_bits(as_float(ftz(a)) * as_float(ftz(b))));
}

unsigned int cosim_f32_add(unsigned int a, unsigned int b) {
  return ftz(as_bits(as_float(ftz(a)) + as_float(ftz(b))));
}

} // extern "C"
