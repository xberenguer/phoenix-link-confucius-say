#pragma once
#include "source_major153.h"
#include "source_fire153.h"
#include "source_word_fire153.h"
#include <algorithm>
#include <cstdint>
// Isolated original lettering material and warm animation from public footage.
// The excerpt loop is a local fit, not animation ROM. No outcome RNG or heap.
namespace authored_major112 {
constexpr int left=24,top=16;
template<class Dot>void vignette(Dot dot){
 // The cabinet title occupies a dark upper display. Give its small-screen
 // adaptation an opaque stage so reel flames do not muddy the lettering.
 for(int y=15;y<119;++y)for(int x=2;x<238;++x){const int edge=std::min({x-2,237-x,y-15,118-y});const unsigned a=unsigned(std::min(255,(edge+1)*128));dot(x,y,source_major153::background,a);}
}
template<class Dot>void title(uint32_t elapsed,bool filled,Dot dot){
 const auto* rgb=filled?source_major153::filledRgb:source_major153::hollowRgb;
 const auto* alpha=filled?source_major153::filledAlpha:source_major153::hollowAlpha;
 for(unsigned y=0;y<source_major153::height;++y)for(unsigned x=0;x<source_major153::width;++x){const unsigned at=y*source_major153::width+x;const unsigned a=alpha[at];
  if(a)dot(left+int(x),top+int(y),rgb[at],a);
 }
 source_word_fire153::draw(elapsed,dot);
}
template<class Dot>void amountFrame(uint32_t elapsed,Dot dot){
 for(int y=69;y<108;++y)for(int x=21;x<219;++x)dot(x,y,source_major153::background,255);
 source_fire153::draw(elapsed,dot);
}
}
