#pragma once
#include "static_grand.h"
#include <cstdint>
#include <cstdio>
#include <type_traits>

namespace return_selector86 {
// Private UI proposal. Root must accept/rebind the final composed runtime.
inline constexpr bool rootAcceptedForDevice=true;
#if defined(ARDUINO) && !defined(UI_HOST_TEST) && !defined(PHOENIX_PRIVATE_RETURN_SELECTOR86)
static_assert(rootAcceptedForDevice,"Private return-selector86 proposal; Root acceptance and runtime rebinding required");
#endif
inline constexpr uint8_t optionToSlot[6]={3,0,4,1,5,2};
inline constexpr uint8_t defaultOption=2; // zero-based; unchanged internal slot4
inline int optionFor(uint8_t slot){for(unsigned i=0;i<6;++i)if(optionToSlot[i]==slot)return int(i);return -1;}
enum class Page:uint8_t {List,Detail,Help,Locked};
struct Detail {
 static_grand::Profile profile{};
 static_grand::Fingerprint family{};
 int64_t grand=0;
 uint32_t low=0,point=0,high=0;
 uint8_t option=defaultOption;
 bool valid=false,returnQualified=false;
};
struct State {
 Page page=Page::List;
 uint8_t cursor=defaultOption;
 Detail detail{};
 bool pending=false,recoveryFence=false,contextChanged=false;
};
static_assert(std::is_trivially_copyable<State>::value,"Whole volatile selector state participates in UI QA backup");
inline bool same(const Detail&a,const Detail&b){return a.valid&&b.valid&&a.returnQualified==b.returnQualified&&a.option==b.option&&
 legacy_static_grand::same(a.profile,b.profile)&&a.family==b.family&&a.grand==b.grand&&a.low==b.low&&a.point==b.point&&a.high==b.high;}
inline void hundredths(uint32_t value,char*out,size_t n){std::snprintf(out,n,"%u.%02u",value/100,value%100);}
inline void range(uint32_t lowPpm,uint32_t highPpm,char*out,size_t n){
 char low[12],high[12];hundredths(lowPpm/100,low,sizeof(low));hundredths((highPpm+99)/100,high,sizeof(high));std::snprintf(out,n,"%s-%s%%",low,high);
}
inline void edge(uint32_t lowPpm,uint32_t highPpm,char*out,size_t n){range(1000000-highPpm,1000000-lowPpm,out,n);}
} // namespace return_selector86
