#pragma once
#include <cstdint>
// The clean track contains more than the active respin music. Its steady
// passage repeats after1,092,854samples (24.78127s at44.1kHz); independently
// recorded active Hold play matches this passage. Retain the opening once,
// then repeat a complete measured period instead of running into later cues.
// Camera/source mapping: assets/audio/feature-loop-v0249.json. This is a
// documented reconstruction, not a recovered manufacturer audio director.
namespace feature_music {
constexpr uint32_t loopBeginFrame=529200;  //12.000000s
constexpr uint32_t loopEndFrame=1622054;   //36.781270s, exclusive
// A silence separates the final collection flourish from the preceding
// music. This4.67068s cue matches three independent recorded completions.
constexpr uint32_t collectedBeginFrame=6420078; //145.580000s, just before attack
constexpr uint32_t collectedEndFrame=6626055;   //decoded source end, exclusive
// Clean1:27celebration identified in l874jFsUyV8 and si4wPYu_5co.
// The named-award recording ends the phrase earlier than the large cash win.
// Near-zero sample boundaries avoid importing the adjacent musical passage.
constexpr uint32_t awardBeginFrame=3839977;
constexpr uint32_t majorAwardEndFrame=3983105;
constexpr uint32_t bigAwardEndFrame=4071372;
static_assert(loopEndFrame>loopBeginFrame,"Nonempty feature music loop");
static_assert(collectedEndFrame>collectedBeginFrame,"Nonempty collected cue");
}
