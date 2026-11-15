#pragma once
#include <stdint.h>
#include <algorithm>
// Research candidate: camera-fitted sequencing, not original manufacturer
// clocks/rules. Independent Yfbaa239s and5BU2054s prefixes are~0.85bet/s.
namespace spin_award {
constexpr uint32_t counterTickMs=200,settleMs=400,readyDelayMs=1300;
constexpr uint32_t jingleMs=5247,localAutoCollectMs=jingleMs+1500;
// Fitted to last-visible pixels (~3.47s after the filmed count jump), not
// merely the2s abstract particle lifetime, which outlasts the viewport.
constexpr uint32_t emissionAfterCountMs=1700;
// Ordinary-Free source controls show header-only16/18/23.8x and coins34.2x.
// Local10/25 gates lie inside conditional observed brackets; neither is a
// recovered vendor threshold. Initial Base->Free and Hold keep their policy.
enum class Style : uint8_t {None,Header,Fountain};
inline Style style(int64_t cash,int64_t bet,bool freeSpin){
 if(cash<=0||bet<=0)return Style::None;
 const int64_t multiple=cash/bet;
 if(freeSpin){if(multiple>=25)return Style::Fountain;if(multiple>=10)return Style::Header;}
 return multiple>=50?Style::Fountain:Style::None;
}
// Saturating positive multiply/divide, without a product that can overflow.
inline int64_t boundedScale(int64_t amount,uint64_t units,uint64_t divisor,int64_t limit){
 if(amount<=0||limit<=0)return 0;
 const uint64_t whole=uint64_t(amount)/divisor,remainder=uint64_t(amount)%divisor;
 if(whole&&units>uint64_t(limit)/whole)return limit;
 const uint64_t base=whole*units;
 const uint64_t tail=remainder*units/divisor;
 return int64_t(base+std::min(tail,uint64_t(limit)-base));
}
// Local upper-award fit, approximately1.7% of the CURRENT action persecond,
// with a minimum5credits/second. Seven measured prefixes differ by0..12%;
// exact ramp/duration law is unrecovered. Normal lower-WIN uses its own clock.
inline int64_t countedUpper(int64_t total,int64_t denominationMinor,uint32_t elapsed){
 if(total<=0||denominationMinor<=0)return 0;
 return std::max(boundedScale(total,uint64_t(elapsed)*17,1000000,total),
                 boundedScale(denominationMinor,elapsed,200,total));
}

inline int64_t counted(int64_t total,int64_t bet,uint32_t elapsed){
 if(total<=0||bet<=0)return 0;
 const uint64_t units=uint64_t(elapsed)*17,whole=uint64_t(bet)/20000;
 if(whole&&units>uint64_t(total)/whole)return total;
 const uint64_t base=whole*units;
 const uint64_t tail=(uint64_t(bet)%20000)*units/20000;
 return int64_t(base+std::min(tail,uint64_t(total)-base));
}
}
