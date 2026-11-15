#pragma once
#include "generated_assets.h"
#include "fire_field.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

// Reconstructed from the native30fps collection sequence: a burning front
// travels UP from the talons, expands into a flame cloud, and leaves embers.
// No camera RGB, game RNG, persistent state or frame-index loop is used.
namespace gold_burn {
constexpr unsigned coreMs=600,durationMs=700,margin=8,maxWidth=128,embers=144;
struct Pixel {uint16_t color=0;unsigned alpha=0;};
struct Ember {float x=0,y=0;uint16_t color=0;unsigned alpha=0;};
inline float clamp(float v){return std::max(0.f,std::min(1.f,v));}
inline float front(float milliseconds,unsigned height){return height*(1.05f-1.15f*std::min(float(coreMs),milliseconds)/coreMs);}
inline uint32_t hash(uint32_t n){n^=n>>16;n*=0x7feb352du;n^=n>>15;n*=0x846ca68bu;n^=n>>16;return n;}
inline uint16_t mix(uint16_t a,uint16_t b,unsigned weight){
 const unsigned inv=255-weight;
 return uint16_t((((a>>11)*inv+(b>>11)*weight+127)/255)<<11|
                 (((((a>>5)&63)*inv+((b>>5)&63)*weight+127)/255)<<5)|
                 ((a&31)*inv+(b&31)*weight+127)/255);
}
class Frame {
 public:
  Frame(const pa::Image& source,unsigned dissolve,fire_field::Workspace* workspace=nullptr):im(source),ms(std::min(dissolve,255u)*durationMs/255),material(650+ms,workspace){
   usable=im.rgb&&im.w>1&&im.w<=maxWidth&&im.h>1;
   if(!usable)return;
   base=front(float(ms),im.h);const float flutter=clamp(float(ms)/70);
   unsigned columns[maxWidth]={};
   const int lo=std::max(0,int(std::floor(base))-12),hi=std::min(int(im.h)-1,int(std::ceil(base))+12);
   for(unsigned x=0;x<im.w;++x)for(int y=lo;y<=hi;++y)columns[x]=std::max(columns[x],alpha(int(x),y));
   float firstEdge=1e9f,lastEdge=-1e9f;
   for(int x=-int(margin);x<int(im.w+margin);++x){
    const unsigned at=unsigned(x+int(margin));
    sourceX[at]=uint8_t(unsigned(x+int(margin))*(fire_field::width-1)/(im.w+2*margin-1));
    edge[at]=base+flutter*(2.2f*std::sin(x*.10f+ms*.018f)+1.1f*std::sin(x*.27f-ms*.023f));
    for(int c=std::max(0,x-int(margin));c<=std::min(int(im.w)-1,x+int(margin));++c){
     const float falloff=1.f-float(std::abs(c-x))/float(margin+1);
     support[at]=std::max(support[at],unsigned(columns[c]*falloff*falloff));
    }
    if(support[at]){
     // Original fire() rejects anything farther than18.2px from edge+3.
     // Keep a wider20pxband so float rounding cannot remove a live pixel.
     firstEdge=std::min(firstEdge,edge[at]);lastEdge=std::max(lastEdge,edge[at]);
    }
   }
   if(firstEdge<=lastEdge){fireFirst=int(std::floor(firstEdge+3.f-20.f));fireAfter=int(std::ceil(lastEdge+3.f+20.f))+1;}
  }
  bool valid() const{return usable;}
  unsigned remaining(unsigned x,unsigned y) const{
   if(!usable||x>=im.w||y>=im.h||ms>=durationMs)return 0;
   return unsigned(clamp((edge[x+margin]-float(y)+2.f)/4.f)*255);
  }
  struct RemainingSpan {unsigned fullAfter,after;};
  RemainingSpan remainingSpan(unsigned x)const{
   if(!usable||x>=im.w||ms>=durationMs)return {0,0};
   // Keep a deliberately wide seven-row transition band. Outside it the
   // original expression is certainly255 or0, including float rounding.
   // Inside, the compositor still calls the exact original remaining().
   const float cut=edge[x+margin];
   const auto bound=[&](int y){return unsigned(std::max(0,std::min(int(im.h),y)));};
   return {bound(int(std::floor(cut))-3),bound(int(std::ceil(cut))+3)};
  }
  Pixel fire(int x,int y) const{
   if(!usable||ms>=durationMs||x<-int(margin)||x>=int(im.w+margin))return {};
   const unsigned at=unsigned(x+int(margin));
   const float coarse=(float(y)-edge[at]-3.f)/14.f;if(std::abs(coarse)>=1.3f||!support[at])return {};
   const unsigned sx=unsigned((x+int(margin)))*int(fire_field::width-1)/(im.w+2*margin-1);
   const unsigned sy=unsigned(std::max(0,std::min(y+int(margin),int(im.h+2*margin-1))))*(fire_field::height-1)/(im.h+2*margin-1);
   const uint16_t sample=material.pixel(sx,sy);const float heat=float((sample>>5)&63)/63;
   const float d=(float(y)-edge[at]-3.f-(heat-.5f)*6.f)/14.f;if(std::abs(d)>=1.f)return {};
   const float filaments=.2f+.8f*clamp((heat-.14f)/.30f),ignite=clamp(float(ms)/80),tail=clamp(float(durationMs-ms)/100);
   const unsigned opacity=unsigned(support[at]*(1-d*d)*filaments*ignite*tail);
   // The real burn is a luminous gold/white cloud, not translucent brown
   // smoke. Preserve flowing material detail while lifting its hot core.
   const unsigned hot=80+unsigned(heat*140);
   return {mix(sample,0xffee,hot),opacity};
  }
  // A scanline shares its material-y interpolation and source-x mapping.
  // It retains the original float/alpha/mix order and does no allocation.
  class FireRow {
   public:
    Pixel fire(int x)const{
     const auto& f=owner;
     if(!f.usable||f.ms>=durationMs||x<-int(margin)||x>=int(f.im.w+margin))return {};
     const unsigned at=unsigned(x+int(margin));
     const float coarse=(float(y)-f.edge[at]-3.f)/14.f;if(std::abs(coarse)>=1.3f||!f.support[at])return {};
     const uint16_t sample=material.pixel(f.sourceX[at]);const float heat=float((sample>>5)&63)/63;
     const float d=(float(y)-f.edge[at]-3.f-(heat-.5f)*6.f)/14.f;if(std::abs(d)>=1.f)return {};
     const float filaments=.2f+.8f*clamp((heat-.14f)/.30f),ignite=clamp(float(f.ms)/80),tail=clamp(float(durationMs-f.ms)/100);
     const unsigned opacity=unsigned(f.support[at]*(1-d*d)*filaments*ignite*tail);
     const unsigned hot=80+unsigned(heat*140);
     return {mix(sample,0xffee,hot),opacity};
    }
   private:
    friend class Frame;
    FireRow(const Frame& f,int row):owner(f),y(row),material(f.material.row(f.sourceY(row))){}
    const Frame& owner;int y;fire_field::Flow::Row material;
  };
  FireRow row(int y)const{return FireRow(*this,y);}
  int firstFireRow()const{return usable&&ms<durationMs?fireFirst:0;}
  int afterFireRow()const{return usable&&ms<durationMs?fireAfter:0;}
  Ember ember(unsigned index) const{
   if(!usable||index>=embers||ms>=durationMs)return {};
   const unsigned birth=index*4+12,life=260;if(ms<birth||ms-birth>=life)return {};
   uint32_t seed=hash(index+0x53414745u);const float originY=front(float(birth),im.h)+3.f;
   const int row=int(std::lround(originY));if(row<0||row>=im.h)return {};
   int column=-1;
   for(unsigned attempt=0;attempt<6;++attempt){seed=hash(seed+0x9e3779b9u);const int candidate=int(seed%im.w);const auto c=im.rgb[row*im.w+candidate];
    if(alpha(candidate,row)>64&&std::max(unsigned(c>>11)*2,unsigned((c>>5)&63))>28){column=candidate;break;}}
   if(column<0)return {};
   const float age=float(ms-birth)/1000,fade=1.f-float(ms-birth)/life;
   const float vx=(int((seed>>12)&31)-15)*.7f,vy=10.f+float((seed>>20)&15);
   const unsigned opacity=unsigned(220*fade*fade*clamp(float(durationMs-ms)/100));
   return {column+.5f+vx*age,originY+vy*age+32.5f*age*age,seed&1?uint16_t(0xffe7):uint16_t(0xffa5),opacity};
  }
 private:
  unsigned sourceY(int y)const{return unsigned(std::max(0,std::min(y+int(margin),int(im.h+2*margin-1))))*(fire_field::height-1)/(im.h+2*margin-1);}
  unsigned alpha(int x,int y) const{if(x<0||x>=im.w||y<0||y>=im.h)return 0;return im.alpha?im.alpha[y*im.w+x]:255;}
  const pa::Image& im;unsigned ms;fire_field::Flow material;bool usable=false;float base=0;
  float edge[maxWidth+2*margin]={};unsigned support[maxWidth+2*margin]={};
  uint8_t sourceX[maxWidth+2*margin]={};int fireFirst=0x7fffffff,fireAfter=-0x7fffffff;
};
}
