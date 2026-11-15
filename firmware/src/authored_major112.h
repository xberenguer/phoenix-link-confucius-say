#pragma once
#include "major_title112_data.h"
#include <algorithm>
#include <cstdint>
// Pure native-pixel material over hand-authored geometry. The2.4s loop is a
// local fit, not a recovered animation. No sampled photograph, RNG or heap.
namespace authored_major112 {
constexpr uint32_t periodMs=2400;
constexpr int left=24,top=22;
constexpr uint8_t wave[]={128,140,153,165,177,188,199,209,218,226,234,240,245,249,252,254,255,254,252,249,245,240,234,226,218,209,199,188,177,165,153,140,128,115,102,90,78,67,56,46,37,29,21,15,10,6,3,1,0,1,3,6,10,15,21,29,37,46,56,67,78,90,102,115};
constexpr uint16_t palette[]={0x7800,0xa040,0xc900,0xea20,0xfbc2,0xfd44,0xfe69,0xff4e};
inline unsigned opacity(int v){return unsigned(std::max(0,std::min(255,v)));}
template<class Dot>void vignette(Dot dot){
 for(int y=18;y<114;++y)for(int x=8;x<232;++x){const int edge=std::min({x-8,231-x,y-18,113-y});const unsigned a=unsigned(std::min(220,edge*28));if(a)dot(x,y,0x18a4,a);}
}
template<class Dot>void title(uint32_t elapsed,bool filled,Dot dot){
 const unsigned phase=(elapsed%periodMs)*64/periodMs;
 for(unsigned y=0;y<major_title112_data::height;++y)for(unsigned x=0;x<major_title112_data::width;++x){
  const int d=major_title112_data::distanceQ16[y*major_title112_data::width+x];if(d<=-80)continue;
  const unsigned materialY=unsigned(int(y)-2);
  const unsigned a=wave[(x*5+materialY*3+phase)&63],b=wave[(x*3+128-materialY*7-phase*2)&63];
  const int reach=16+int(a)*32/255+int(b)*16/255;
  if(d<8){const unsigned flame=opacity((reach+d)*5);if(flame)dot(left+int(x),top+int(y),palette[std::min(7u,(a+b)/64)],flame);}
  const unsigned rim=opacity((d+12)*16);if(rim)dot(left+int(x),top+int(y),0xfe8b,rim);
  const unsigned dark=opacity((d-5)*16);if(dark)dot(left+int(x),top+int(y),0x0842,dark);
  const unsigned face=opacity((d-18)*16);if(face){const uint16_t green=uint16_t((2u<<11)|((45u-materialY/9)<<5)|8u);dot(left+int(x),top+int(y),filled?green:0x18a4,face);}
 }
}
template<class Dot>void amountFrame(uint32_t elapsed,Dot dot){
 const unsigned phase=(elapsed%periodMs)*64/periodMs;
 for(int y=64;y<108;++y)for(int x=16;x<224;++x){
  const int outer=std::max({20-x,x-219,68-y,y-103,0});
  const int inner=std::min({x-20,219-x,y-68,103-y});
  if(!outer&&inner>2){dot(x,y,0x18a5,240);continue;}
  const unsigned a=wave[(unsigned(x*3+y*5)+phase)&63],b=wave[(unsigned(x*5-y*3)+128-phase*2)&63];
  const int reach=1+int(a)*2/255+int(b)/255;
  const unsigned glow=opacity((reach-outer)*66);if(glow)dot(x,y,palette[std::min(7u,(a+b)/64)],glow);
  if(!outer&&inner<=1)dot(x,y,inner?0xfdd0:0xea44,230);
 }
}
}
