#pragma once
#include <stdint.h>
namespace clean_coins {
constexpr unsigned nativeSize=32,frameCount=16;
struct Frame{unsigned w,h;const uint16_t* rgb;const uint8_t* alpha;};
const Frame& frame(unsigned n);uint32_t decodedHash();
}
