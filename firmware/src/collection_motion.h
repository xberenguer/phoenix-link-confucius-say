#pragma once
#include <algorithm>
#include <cstdint>
namespace collection_motion {
// Independent collection contact sequence shows flat fiery rise, clearing
// field, then the stationary gold bird, then pickups. Approximate clock fit;
// these are presentation constants, never original game-math parameters.
constexpr uint32_t fireDelay=120,fireDuration=780;
constexpr uint32_t goldStart=700,goldFull=900,goldBurn=1400,goldGone=2100;
constexpr uint32_t backdropGone=2350;
// The original gold figure has a black upper display behind it. On the Stick
// it overlays the reels: dim that backdrop temporarily, then restore it before
// individual prizes fly. Header/footer and held results remain untouched.
inline unsigned backdropOpacity(uint32_t elapsed){
 if(elapsed<600||elapsed>=backdropGone)return 0;
 if(elapsed<900)return (elapsed-600)*192/300;
 if(elapsed<=goldGone)return 192;
 return (backdropGone-elapsed)*192/(backdropGone-goldGone);
}
inline unsigned goldOpacity(uint32_t elapsed){
 if(elapsed<goldStart||elapsed>=goldGone)return 0;
 return std::min(255u,(elapsed-goldStart)*255/(goldFull-goldStart));
}
inline unsigned goldDissolve(uint32_t elapsed){return elapsed<=goldBurn?0:std::min(255u,(elapsed-goldBurn)*255/(goldGone-goldBurn));}
}
