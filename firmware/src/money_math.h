#pragma once
#include <cstdint>
#include <limits>

// Display arithmetic only. No award, stake, meter, prize or RNG is changed.
namespace money_math {
inline uint64_t magnitude(int64_t value){return value<0?uint64_t(-(value+1))+1:uint64_t(value);}
// Exact floor(value*elapsed/duration), with0<=elapsed<=duration. Quotient
// multiplication is bounded by value; remainder*elapsed fits uint64 because
// both factors are at most UINT32_MAX. No target __int128 is required.
inline uint64_t portionU64(uint64_t value,uint32_t elapsed,uint32_t duration){
 if(!duration||elapsed>=duration)return value;
 const uint64_t whole=value/duration,remainder=value%duration;
 return whole*elapsed+remainder*elapsed/duration;
}
inline int64_t portion(int64_t value,uint32_t elapsed,uint32_t duration){
 return value<=0?0:int64_t(portionU64(uint64_t(value),elapsed,duration));
}
inline uint64_t ordered(int64_t value){return uint64_t(value)^(uint64_t(1)<<63);}
inline int64_t signedValue(uint64_t value){
 const uint64_t bias=uint64_t(1)<<63;
 return value>=bias?int64_t(value-bias):std::numeric_limits<int64_t>::min()+int64_t(value);
}
// Exact from+trunc((to-from)*elapsed/duration), without overflowing either
// subtraction or multiplication. This also handles signed display origins.
// A zero duration is an already-complete transition; elapsed is clamped.
inline int64_t lerp(int64_t from,int64_t to,uint32_t elapsed,uint32_t duration){
 const uint64_t a=ordered(from),b=ordered(to);
 return b>=a?signedValue(a+portionU64(b-a,elapsed,duration)):
             signedValue(a-portionU64(a-b,elapsed,duration));
}
struct DecimalParts{uint64_t whole=0;uint32_t hundredths=0;};
// Exact displayed hundredths without representing value*100. For cents,
// divisor100 gives dollars; a denomination divisor gives credit units.
inline DecimalParts split(int64_t value,uint32_t divisor){
 if(!divisor)return {};
 const uint64_t n=magnitude(value);
 return {n/divisor,uint32_t((n%divisor)*100/divisor)};
}
// Exact terminating credit fraction. All supported denominations divide10000.
// Keep at least two places for a nonzero fraction to preserve existing labels;
// retain up to four places when required instead of flooring it.
struct ExactParts{uint64_t whole=0;uint32_t fraction=0;unsigned places=0;};
inline bool splitExact(int64_t value,uint32_t divisor,ExactParts& out){
 if(!divisor)return false;
 uint32_t scale=1;unsigned places=0;
 while(scale%divisor&&places<4){scale*=10;++places;}
 if(scale%divisor)return false;
 const uint64_t n=magnitude(value);uint32_t fraction=uint32_t((n%divisor)*scale/divisor);
 if(!fraction){out={n/divisor,0,0};return true;}
 while(places<2){fraction*=10;++places;}
 while(places>2&&fraction%10==0){fraction/=10;--places;}
 out={n/divisor,fraction,places};return true;
}
struct CompactParts{bool abbreviated=false;uint64_t whole=0;uint32_t tenth=0;char unit=0;};
inline CompactParts compact(uint64_t whole){
 const uint64_t scales[]={1000000000000ULL,1000000000ULL,1000000ULL,1000ULL};
 const char units[]={'T','B','M','K'};
 for(unsigned i=0;i<4;++i)if(whole>=scales[i])return {true,whole/scales[i],uint32_t((whole%scales[i])*10/scales[i]),units[i]};
 return {false,whole,0,0};
}
} // namespace money_math
