#pragma once
#include "source_major153.h"
#include "major_title112_data.h"
#include "source_word_fire153.h"
#include "hold_winner.h"
#include "counter_fire89.h"
namespace authored_major112 {
constexpr int left=24,top=16;
template<class Dot>void title(uint32_t elapsed,Dot dot){
 for(unsigned y=0;y<source_major153::height;++y)for(unsigned x=0;x<source_major153::width;++x){
  const unsigned at=y*source_major153::width+x,a=source_major153::hollowAlpha[at];if(!a)continue;
  const int d=major_title112_data::distanceQ16[at];
  uint16_t color=source_major153::hollowRgb[at];
  if(d>=-8)color=0x0000;
  else if(d>=-28)color=hold_winner::shade(d>=-18?0xfde6:0xfd85,x/2,y/2,elapsed);
  dot(left+int(x),top+int(y),color,a);
 }
 source_word_fire153::draw(elapsed,dot);
}
template<class Dot>void amountFrame(uint32_t elapsed,Dot dot){
 for(int y=71;y<98;++y)for(int x=31;x<209;++x)dot(x,y,0x0000,220);
 counter_fire89::draw(elapsed,31,208,26,[&](int x,int y,uint16_t color,unsigned){
  y+=71;for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)dot(x+dx,y+dy,color,44);
  dot(x-2,y,color,18);dot(x+2,y,color,18);dot(x,y-2,color,18);dot(x,y+2,color,18);
 });
 counter_fire89::draw(elapsed,31,208,26,[&](int x,int y,uint16_t color,unsigned a){dot(x,y+71,color,a);});
}
}
