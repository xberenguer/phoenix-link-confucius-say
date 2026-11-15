#pragma once
#include <cstdint>

// Compact reconstruction of the 1888-counter entry in3pfZnZUWGhI1329.9–1332.8.
// Key positions were read from independently sourced, perspective-rectified
// frames. Geometry/timing is reconstructed, not the manufacturer's effect bank.
// The trajectory stays independent of financial state and engine RNG.
namespace feature_entry {
struct Point {float x,y;};
struct Key {uint16_t ms;Point p;};
constexpr Key ribbonKeys[]={
 {0,{195,7}}, {133,{202,47}}, {267,{217,85}},
 {400,{190,108}}, {534,{132,97}}, {667,{101,118}},
 {801,{112,153}}, {934,{120,170}}
};
constexpr uint32_t ribbonMs=1260,birdLeadMs=1400,birdRiseMs=430,fireDelayMs=120;
constexpr uint32_t fireRevealMs=560,fireMs=1260;
// Original29.97fps independent entries put the hot wing ridge about24native
// pixels higher on each camera frame. This is a fitted retarget, not proof of
// the manufacturer's clock. Collection/return keep their separate old timing.
constexpr uint32_t returnDurationMs=1520,returnRevealMs=820;
inline float birdTop(uint32_t elapsed){return 135.f-.720f*float(elapsed);}
// Observed final no-win Twin result1560.83s -> FEATURE COMPLETED1560.97s
// -> credit transfer about1561.5s. Retarget to the native40ms frame cadence.
// This is an observed presentation interval, not a certified vendor clock.
constexpr uint32_t freeLastMissLandMs=160,freeCompletionMs=520;
constexpr uint32_t lead(bool meter){return meter?birdLeadMs:0;}
constexpr uint32_t duration(bool meter){return lead(meter)+fireDelayMs+fireMs;}
constexpr uint32_t reveal(bool meter){return lead(meter)+fireDelayMs+fireRevealMs;}
inline Point ribbon(float ms){
 if(ms<=0)return ribbonKeys[0].p;
 unsigned i=0;while(i+1<8&&ms>ribbonKeys[i+1].ms)++i;
 if(i>=7)return ribbonKeys[7].p;
 const float t=(ms-ribbonKeys[i].ms)/(ribbonKeys[i+1].ms-ribbonKeys[i].ms),t2=t*t,t3=t2*t;
 const Point a=ribbonKeys[i?i-1:i].p,b=ribbonKeys[i].p,c=ribbonKeys[i+1].p,d=ribbonKeys[i<6?i+2:i+1].p;
 return {.5f*((2*b.x)+(-a.x+c.x)*t+(2*a.x-5*b.x+4*c.x-d.x)*t2+(-a.x+3*b.x-3*c.x+d.x)*t3),
         .5f*((2*b.y)+(-a.y+c.y)*t+(2*a.y-5*b.y+4*c.y-d.y)*t2+(-a.y+3*b.y-3*c.y+d.y)*t3)};
}
}
