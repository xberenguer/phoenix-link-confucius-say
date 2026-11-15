#pragma once
#include <cstdint>
namespace phoenix_fx {
struct Frame { uint8_t w,h;const uint8_t* index;const uint16_t* rgb;const uint8_t* alpha; };
constexpr uint32_t wipeDurationMs=1400;
const Frame& fire(unsigned n);
const Frame& coin(unsigned n);
}
