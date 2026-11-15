#pragma once
#include "generated_assets.h"
#include "upper_win_data.h"
#include "pixel_blend.h"
#include <algorithm>
// Clean30x14 lettering and a bounded attached rim/halo. Period/phase are a
// local fit, not a recovered vendor flame bank. No RNG, heap or PSRAM.
// Header-only so all existing native and host UI targets consume one derived
// cache without changing their linked source lists. Draw each returned image
// immediately: its RGB buffer is reused by the next frame() query.
namespace upper_win {
constexpr uint32_t periodMs=1600,rightOffsetMs=periodMs/2;
namespace detail {
constexpr unsigned width=30,height=14;
constexpr uint16_t matte=0x18c3,bodyInk=0x1000,rimGold=0xfe8c;
constexpr uint16_t firePalette[]={0x9800,0xc8c0,0xea00,0xfba0,0xfca0,0xfe48,0xff4e,0xffd9};
inline uint16_t rgb[width*height];inline unsigned cached=64;
inline const pa::Image image={width,height,rgb};
}
inline const pa::Image& frame(uint32_t elapsedMs){
 using namespace detail;
 const unsigned phase=((elapsedMs%periodMs)*64)/periodMs;
 if(phase==cached)return image;
 for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
  const unsigned at=y*width+x,angle=upper_win_data::angle[at];
  const unsigned a=upper_win_data::wave[(angle*5+phase)&63];
  const unsigned b=upper_win_data::wave[(angle*9+128-phase*2)&63];
  // Flare stays attached within about two native pixels of the letter body.
  // Source warm activity changes around the whole word; no distant sparks
  // or copied flame texture are introduced in the compact14-row header.
  const unsigned reach=12+a*15/255+b*7/255;
  const int depth=int(reach)-upper_win_data::distanceQ16[at];
  unsigned alpha=depth>0?std::min(152u,unsigned(depth)*9):0;
  if(x==0||x+1==width||y==0||y+1==height)alpha=0;
  uint16_t color=pixel_blend::over(firePalette[std::min(7u,(a+b)/64)],matte,alpha);
  color=pixel_blend::over(rimGold,color,upper_win_data::rim[at]);
  rgb[at]=pixel_blend::over(bodyInk,color,upper_win_data::body[at]);
 }
 cached=phase;return image;
}
}
