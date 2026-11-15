#pragma once
#include "generated_assets.h"
#include <cstddef>
#include <cstdint>

namespace entry_art {
// Pure bounded decoder shared by the target and sanitizer tests. Packets are
// either 1..128 literal palette indices or 1..128 transparent pixels.
bool decode(const uint8_t* source,size_t bytes,const uint16_t* palette,const uint8_t* opacity,size_t count,uint16_t* rgb,uint8_t* alpha);
bool init();
bool ready();
const pa::Image& sprite();
unsigned storageBytes();
uint32_t decodedHash();
}
