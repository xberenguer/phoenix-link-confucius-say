#pragma once
#include "generated_assets.h"
#include <cstdint>
// The existing clean reconstructed master supplies this cap. Camera/user
// pixels and alpha are NEVER production inputs. Four-pixel internal overhang
// is an authored compact-display adaptation, not recovered vendor alpha.
namespace sage_crown122 {
constexpr unsigned outsideRows=4,width=47;
inline constexpr uint16_t outsideRgb[]={
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8420,21066,23211,27372,21066,16872,6211,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14726,18985,25260,18953,25292,23146,23179,25260,21033,14694,16774,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14628,16806,12613,10532,18952,14759,18920,14759,18953,14759,14726,10533,14693,10532,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,18887,16872,10532,12613,12645,16872,12645,18985,12645,16872,12645,10532,16872,14693,8452,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
inline constexpr uint8_t outsideAlpha[]={
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,41,145,197,124,11,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,81,188,212,251,252,252,252,232,209,136,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,121,247,253,253,253,253,253,253,253,253,252,185,42,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,251,252,253,253,253,253,253,253,253,253,253,253,200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
// Keep the canonical current three inside rows exactly; no eye/body shift.
// weight(y) permits the same upward-reveal mask as the existing merge bank.
template<class Weight,class Dot> inline void draw(const pa::Image& canonical,int x,int y,Weight weight,Dot paint){
 if(canonical.w!=47||canonical.h!=5||!canonical.rgb||!canonical.alpha)return;
 for(int r=-4;r<3;++r)for(unsigned c=0;c<47;++c){
  const unsigned i=unsigned(r+4)*47+c,j=unsigned(r<0?0:r+2)*47+c;
  const unsigned alpha=r<0?outsideAlpha[i]:canonical.alpha[j];
  const unsigned w=weight(y+r),factor=w>255?255:w;const unsigned a=(alpha*factor+127)/255;
  if(a)paint(x+int(c),y+r,r<0?outsideRgb[i]:canonical.rgb[j],a);
 }
}
struct Texel {uint16_t rgb;unsigned alpha;};
inline Texel sample(const pa::Image& im,unsigned x,int y){
 if(x>=47||y< -4||y>=3)return {0,0};
 const unsigned i=unsigned(y<0?y+4:y+2)*47+x;
 return y<0?Texel{outsideRgb[i],outsideAlpha[i]}:Texel{im.rgb[i],im.alpha[i]};
}
// Existing merge is a LINEAR old/new reveal. Combine both contributions
// before one framebuffer blend; sequential source-over would darken overlap.
template<class Mix,class Dot> inline void transition(const pa::Image& oldCap,const pa::Image& newCap,int x,int rootY,unsigned rows,int movingY,int clipTop,Mix mix,Dot paint){
 for(unsigned row=0;row<rows;++row)for(int r=-4;r<3;++r)for(unsigned c=0;c<47;++c){const int yy=rootY+int(row)*35+r;if(yy<clipTop)continue;
  const auto a=sample(oldCap,c,r),b=sample(newCap,c,yy-movingY);if(b.alpha)continue;
  const unsigned alpha=(a.alpha*(255u-mix(yy))+127)/255;if(alpha)paint(x+int(c),yy,a.rgb,alpha);
 }
 for(int r=-4;r<3;++r)for(unsigned c=0;c<47;++c){const int yy=movingY+r;if(yy<clipTop)continue;const auto b=sample(newCap,c,r);if(!b.alpha)continue;
  Texel a{0,0};for(unsigned row=0;row<rows;++row){const auto q=sample(oldCap,c,yy-rootY-int(row)*35);if(q.alpha){a=q;break;}}
  const unsigned m=mix(yy),ao=(a.alpha*(255u-m)+127)/255,an=(b.alpha*m+127)/255,sum=ao+an;if(!sum)continue;
  // The independently rounded complementary terms can sum256 by at most1.
  const unsigned alpha=sum>255?255:sum;
  const auto channel=[&](unsigned shift,unsigned mask){return ((((a.rgb>>shift)&mask)*ao+((b.rgb>>shift)&mask)*an+sum/2)/sum);};
  const uint16_t rgb=uint16_t((channel(11,31)<<11)|(channel(5,63)<<5)|channel(0,31));paint(x+int(c),yy,rgb,alpha);
 }
}
}
