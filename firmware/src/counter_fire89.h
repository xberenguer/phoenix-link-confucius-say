#pragma once
#include <cstdint>
#include "counter_fire_motion89.h"
namespace counter_fire89 {
inline uint16_t mix565(uint16_t a,uint16_t b,unsigned alpha){
 const unsigned inv=255-alpha;
 const unsigned r=(((a>>11)&31)*alpha+((b>>11)&31)*inv+127)/255;
 const unsigned g=(((a>>5)&63)*alpha+((b>>5)&63)*inv+127)/255;
 const unsigned bl=((a&31)*alpha+(b&31)*inv+127)/255;
 return uint16_t((r<<11)|(g<<5)|bl);
}
inline unsigned contour(uint32_t now,unsigned u,unsigned offset=0){
 const uint32_t phase=((now%periodMs+offset)%periodMs)*(frameCount*256)/periodMs;
 const unsigned i=phase>>8,f=phase&255,j=(i+1)%frameCount;
 if(u>=width)u=width-1;
 return (heightQ112[i][u]*(256-f)+heightQ112[j][u]*f+128)>>8;
}
inline uint16_t flame(unsigned heat){
 // Clean authored fire pigment: red tips, orange body and lemon-gold cores.
 // Camera color pixels are deliberately absent.
 constexpr uint16_t ramp[]={0x8800,0xc100,0xf9a0,0xfc20,0xfe25,0xffd2};
 if(heat>=255)return ramp[5];
 const unsigned segment=heat*5,index=segment/255,alpha=segment%255;
 return mix565(ramp[index+1],ramp[index],alpha);
}
inline uint16_t rim(unsigned h,bool lit){
 const uint16_t rail=lit?0xd545:0xa381;
 const unsigned heat=h>112?255:h*255/112;
 const unsigned alpha=h>8?160+(h>95?95:h):h*20;
 return mix565(flame(heat),rail,alpha);
}
template<class Dot>
inline void draw(uint32_t now,int left,int right,int bottom,Dot&& dot){
 const unsigned length=unsigned(right-left),height=unsigned(bottom);
 for(unsigned x=0;x<=length;++x){
  const unsigned u=x*(width-1)/length;
  const unsigned top=contour(now,u),base=contour(now,u,750);
  dot(left+int(x),0,rim(top,true),255);
  dot(left+int(x),bottom,rim(base,false),255);
 }
 for(unsigned y=1;y<height;++y){
  const unsigned u=(height-y)*(width-1)/height;
  const unsigned a=contour(now,u,333),b=contour(now,u,1167);
  dot(left,int(y),rim(a,true),255);dot(right,int(y),rim(b,false),255);
 }
}
} // namespace counter_fire89
