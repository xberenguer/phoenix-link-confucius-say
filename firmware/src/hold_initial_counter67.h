#pragma once
#include <cstdint>

// Display-only retarget of four natural Hold entries in 3pfZnZUWGhI.
// First1->first2 is about133ms; first1->first3 is300..334ms;
// first3->first motion is about200ms. Physical START is not visible.
// Place that roughly500ms visual window at the end of the EXISTING nominal
// Ready delay. The650ms path therefore carries START for150ms; longer mixed
// win/award delays carry it longer. This placement is a LOCAL adaptation,
// not measured button latency, a vendor timer or a new action delay.
namespace hold_initial_counter67 {
constexpr uint32_t rampMs=500,secondAfterOneMs=133,thirdAfterOneMs=300;
inline uint32_t carry(uint32_t readyDelay){return readyDelay>rampMs?readyDelay-rampMs:0;}
inline bool prompt(uint32_t elapsed,uint32_t readyDelay){return elapsed<carry(readyDelay);}
inline unsigned count(uint32_t elapsed,uint32_t readyDelay){
 const uint32_t prefix=carry(readyDelay);
 if(elapsed<prefix+secondAfterOneMs)return 1;
 return elapsed<prefix+thirdAfterOneMs?2u:3u;
}
}
