#include <cstdint>
#include <cstring>

static float bits_to_float(std::uint32_t x)
{
    float f;
    std::memcpy(&f, &x, sizeof(f));
    return f;
}

static std::uint32_t float_to_bits(float f)
{
    std::uint32_t x;
    std::memcpy(&x, &f, sizeof(x));
    return x;
}

extern "C" std::uint32_t fp32_mul(std::uint32_t a, std::uint32_t b)
{
    const float fa = bits_to_float(a);
    const float fb = bits_to_float(b);

    const float r = fa * fb;

    return float_to_bits(r);
}

extern "C" std::uint32_t fp32_add(std::uint32_t a, std::uint32_t b)
{
    const float fa = bits_to_float(a);
    const float fb = bits_to_float(b);

    const float r = fa + fb;

    return float_to_bits(r);
}
