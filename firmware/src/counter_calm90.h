#pragma once
#include "counter_fire89.h"
// User-selected quieter adaptation for the small1888 number rim only.
// Original measured contour data and every interior/number pixel are retained;
// the centered collection WINNER keeps the independent original89 treatment.
namespace counter_calm90 {
constexpr uint32_t periodMs=4000;
inline uint16_t pigment(uint16_t color){
 // The actual reel moulding's shoulder, body and highlight golds.
 // Keep distinct flame tongues through their contrast, without red pigment.
 constexpr uint16_t shoulderGold=0xc3c1,bodyGold=0xe5a3,lightGold=0xff4b;
 const unsigned green=(color>>5)&63u;
 const unsigned heat=green>=58u?255u:(green*255u+29u)/58u;
 return heat<128u?counter_fire89::mix565(bodyGold,shoulderGold,heat*2u):
                  counter_fire89::mix565(lightGold,bodyGold,(heat-128u)*2u);
}
template<class Dot>
inline void draw(uint32_t now,int left,int right,int bottom,Dot&& dot){
 counter_fire89::draw((now%periodMs)/2,left,right,bottom,
  [&](int x,int y,uint16_t color,unsigned alpha){dot(x,y,pigment(color),alpha);});
}
} // namespace counter_calm90
