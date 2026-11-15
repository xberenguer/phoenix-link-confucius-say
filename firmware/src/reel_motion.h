#pragma once
#include <cstdint>
// Base, Twin and Hold targets run down quickly, overrun their resting
// position, then pull up almost linearly for~300ms in independent footage.
// This fit preserves the existing end/stagger times. It is not vendor code.
namespace reel_motion {
constexpr uint32_t recoilMs=300,overrunPx=15,nominalPxPerSecond=660;
inline uint32_t impactMs(uint32_t end){return end>recoilMs?end-recoilMs:0;}
inline int steps(uint32_t end,unsigned tileHeight){
 const uint64_t travel=uint64_t(impactMs(end))*nominalPxPerSecond;
 const uint64_t initial=travel>overrunPx*1000?travel-overrunPx*1000:0;
 const unsigned divisor=tileHeight*1000;
 const int count=int((initial+divisor/2)/divisor);return count<3?3:count;
}
inline int offset(uint32_t end,int steps,unsigned tileHeight,uint32_t elapsed){
 if(elapsed>=end)return 0;
 const uint32_t impact=impactMs(end);if(!impact)return 0;
 if(elapsed>=impact)return -int((uint64_t(overrunPx)*(end-elapsed)+recoilMs/2)/recoilMs);
 const unsigned start=unsigned(steps)*tileHeight;
 return int(start)-int((uint64_t(start+overrunPx)*elapsed+impact/2)/impact);
}
struct Tile {int first,remainder;};
inline Tile split(int offset,int height){
 const int first=offset>=0?offset/height:(offset-height+1)/height;
 return {first,offset-first*height};
}
}
