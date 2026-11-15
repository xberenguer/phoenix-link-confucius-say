#pragma once
#include "generated_assets.h"
namespace phoenix_art {
// Metadata only; no change to pa::Image or its allocation/record stride.
struct AlphaSpan {uint8_t left,right;};
// Recognizes only checked publisher registry images. Does not initialize or
// allocate; unready, copied, incompatible or fallback images use the old path.
const AlphaSpan* alphaSpans(const pa::Image& image);
bool alphaReady();
unsigned alphaMetadataBytes();
bool init();
bool ready();
unsigned frameCount();
uint32_t periodMs();
uint32_t decodedHash();
unsigned verifiedFrames();
unsigned assetReadBytes();
uint32_t assetReadHash();
const pa::Image& frame(uint32_t elapsedMs);
const pa::Image& frameIndex(unsigned index);
}
