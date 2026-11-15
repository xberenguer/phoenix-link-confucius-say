#pragma once
#include <stdint.h>
#include <algorithm>
// Presentation-only fit to 5BUJOWxIBN4's first Free spin, not recovered vendor
// constants: motion n521, first fully readable 1/1 n524, then n534..n574.
// Keep the existing, independently fitted retrigger counter clock separate.
namespace wisdom_entry_timing29 {
constexpr uint32_t firstCounterLeadMs=50;
inline unsigned shownInitialTotal(unsigned awarded,uint32_t firstSpinElapsed){
 if(!awarded||firstSpinElapsed<firstCounterLeadMs)return 0;
 const uint64_t age=firstSpinElapsed-firstCounterLeadMs;
 return std::min(awarded,unsigned(1+age*6/1000));
}
}
