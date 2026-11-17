#pragma once
#include <cstdint>
#include "major_material153.h"
// Isolated original warm title fringe; same registered public-video excerpt as source_fire153.
// Four-frame local loop crossfade; not recovered animation ROM.
namespace source_word_fire153 {
constexpr unsigned width=192,height=54,frameMs=40,frames=30;
inline constexpr uint32_t offsets[]={0,12440,24752,37076,49432,61724,74092,86444,98728,111048,123344,135644,147960,160268,172528,184824,197104,209404,221720,234100,246460,258828,271148,283412,295684,307948,320212,333056,345800,358468,371164};

template<class Dot>void draw(uint32_t elapsed,Dot dot){
 const auto* raw=major_material153::bytes();if(!raw)return;
 const auto* data=raw+major_material153_data::wordOffset;
 const unsigned f=(elapsed/frameMs)%frames,end=offsets[f+1];unsigned at=offsets[f],pos=0;
 while(at+4<=end){const unsigned count=data[at++],a=data[at++];const uint16_t color=uint16_t(data[at]|unsigned(data[at+1])<<8);at+=2;
  const unsigned stop=pos+count;if(a)for(;pos<stop;++pos)dot(24+int(pos%width),16+int(pos/width),color,a);else pos=stop;
 }
}
}
