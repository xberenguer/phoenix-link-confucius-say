#pragma once
#include <cstdint>
// Five-recording reconstruction, three active cameras per body/tail interval.
// Not an originalvendorbank. Root owns deviceacceptance/listening qualification.
namespace twin_music42 {
constexpr uint32_t sampleRate=22050;
constexpr uint32_t frames=615439;
constexpr uint32_t loopBegin=0;
constexpr uint32_t loopEnd=frames;
constexpr uint32_t pcmFNV1a=302818443u;
extern const int16_t samples[frames];
}
