#pragma once
#include <cstdint>
#include "major_material153.h"
// Warm-only original border footage XnCGvWkNlRg, registered on the currency field.
// Selected30-frame25fps excerpt with a four-frame premultiplied-alpha loop crossfade; not original animation ROM.
namespace source_fire153 {
constexpr unsigned width=208,height=49,frameMs=40,frames=30;
inline constexpr uint32_t offsets[]={0,10968,21884,32896,43680,54436,65176,75908,86644,97368,108048,118612,129236,140000,150976,162108,173060,184292,195508,206640,217568,228344,239172,250020,260892,271756,282460,293964,305280,316676,327956};

template<class Dot>void draw(uint32_t elapsed,Dot dot){
 const auto* raw=major_material153::bytes();if(!raw)return;
 const auto* data=raw;
 const unsigned f=(elapsed/frameMs)%frames,end=offsets[f+1];unsigned at=offsets[f],pos=0;
 while(at+4<=end){const unsigned count=data[at++],a=data[at++];const uint16_t color=uint16_t(data[at]|unsigned(data[at+1])<<8);at+=2;
  const unsigned stop=pos+count;if(a)for(;pos<stop;++pos)dot(16+int(pos%width),64+int(pos/width),color,a);else pos=stop;
 }
}
}
