#include "coin_sampler.h"
#include "clean_coins.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32)
#include <esp32-hal-psram.h>
#endif

namespace coin_sampler {namespace {
constexpr unsigned sizes=maxSize-minSize+1,maps=sizes*rollCount;
Map* mapData=nullptr;Row* rowData=nullptr;uint32_t* texelData=nullptr;
Span* phaseData=nullptr;
Info stats;
void* allocate(size_t n){
#if defined(ARDUINO_ARCH_ESP32)
 return ps_malloc(n);
#else
 return std::malloc(n);
#endif
}
int radius(unsigned size){const float width=float(size),height=float(size);return int(std::ceil(std::sqrt(width*width+height*height)*.5f+1));}
uint32_t coordinates(int x,int y,const clean_coins::Frame& im,float width,float height,float cs,float sn){
 // Preserve the v0.2.62 expression order. These values describe a point
 //relative to an integer coin centre, exactly as in turnedSprite().
 const float dx=float(x),dy=float(y),anchorX=(im.w-1)*.5f,anchorY=(im.h-1)*.5f;
 const float u=(dx*cs+dy*sn)*im.w/width+anchorX;
 const float v=(-dx*sn+dy*cs)*im.h/height+anchorY;
 const int ix=int(std::floor(u)),iy=int(std::floor(v));
 if(ix<-1||iy<-1||ix>=int(im.w)||iy>=int(im.h))return 0;
 const unsigned fx=unsigned((u-ix)*256),fy=unsigned((v-iy)*256);
 return validBit|unsigned(ix+1)|(unsigned(iy+1)<<6)|(fx<<12)|(fy<<20);
}
bool support(uint32_t packed,const uint32_t* unionRows){
 if(!(packed&validBit))return false;
 const int ix=int(packed&63)-1,iy=int((packed>>6)&63)-1;
 const unsigned fx=(packed>>12)&255,fy=(packed>>20)&255;
 const unsigned weights[]={(256-fx)*(256-fy),fx*(256-fy),(256-fx)*fy,fx*fy};
 for(unsigned j=0;j<4;++j){const int x=ix+int(j&1),y=iy+int(j>>1);
  if(weights[j]&&x>=0&&x<32&&y>=0&&y<32&&(unionRows[y]&(1u<<x)))return true;
 }
 return false;
}
}
bool prepare(){
 if(stats.ready)return true;
 unsigned rows=0;for(unsigned size=minSize;size<=maxSize;++size)rows+=unsigned(radius(size))*2*rollCount;
 const size_t metaBytes=maps*sizeof(Map)+rows*sizeof(Row);
 auto* metadata=static_cast<uint8_t*>(allocate(metaBytes));if(!metadata)return false;
 auto* preparedMaps=reinterpret_cast<Map*>(metadata);auto* preparedRows=reinterpret_cast<Row*>(metadata+maps*sizeof(Map));
 std::memset(metadata,0,metaBytes);
 uint32_t unionRows[32]={};
 for(unsigned frame=0;frame<clean_coins::frameCount;++frame){const auto& im=clean_coins::frame(frame);
  if(im.w!=32||im.h!=32||!im.rgb||!im.alpha){std::free(metadata);return false;}
  for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x)if(im.alpha[y*32+x])unionRows[y]|=1u<<x;
 }
 const auto& im=clean_coins::frame(0);unsigned nextRow=0,entries=0;
 for(unsigned size=minSize;size<=maxSize;++size)for(unsigned roll=0;roll<rollCount;++roll){
  auto& map=preparedMaps[(size-minSize)*rollCount+roll];const int reach=radius(size);
  map={entries,uint16_t(nextRow),uint8_t(reach),0};
  const float angle=roll*(6.2831853f/32.f),cs=std::cos(angle),sn=std::sin(angle);
  unsigned mapEntries=0;
  for(int y=-reach;y<reach;++y){auto& row=preparedRows[nextRow++];int left=reach,right=-reach;
   for(int x=-reach;x<reach;++x)if(support(coordinates(x,y,im,float(size),float(size),cs,sn),unionRows)){left=std::min(left,x);right=x+1;}
   row.offset=uint16_t(mapEntries);
   if(right>left){row.left=int8_t(left);row.right=int8_t(right);mapEntries+=unsigned(right-left);}
  }
  entries+=mapEntries;
 }
 auto* preparedTexels=static_cast<uint32_t*>(allocate(entries*sizeof(uint32_t)));
 if(!preparedTexels){std::free(metadata);return false;}
 constexpr unsigned spans=clean_coins::frameCount*rollCount*canonicalRows;
 auto* preparedPhases=static_cast<Span*>(allocate(spans*sizeof(Span)));
 if(!preparedPhases){std::free(preparedTexels);std::free(metadata);return false;}
 for(unsigned i=0;i<spans;++i)preparedPhases[i]={127,-127};
 for(unsigned frame=0;frame<clean_coins::frameCount;++frame)for(unsigned roll=0;roll<rollCount;++roll){
  const auto& art=clean_coins::frame(frame);const float angle=roll*(6.2831853f/32.f),cs=std::cos(angle),sn=std::sin(angle);
  // Every contributing alpha texel has a continuous bilinear support square
  //of +/-1sourcepixel. Its rotated AABB spans these canonical row bands.
  //Padding and neighbouring-band union keep this conservative under FP
  //rounding; it only skips guaranteed-empty areas, never approximates alpha.
  const float extent=std::abs(cs)+std::abs(sn)+.005f;
  auto* phase=preparedPhases+(frame*rollCount+roll)*canonicalRows;
  for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x)if(art.alpha[y*32+x]){
   const float dx=float(x)-15.5f,dy=float(y)-15.5f;
   const float cx=dx*cs-dy*sn,cy=dx*sn+dy*cs;
   const int lo=std::max(canonicalTop,int(std::floor(cy-extent))),hi=std::min(canonicalTop+canonicalRows-1,int(std::floor(cy+extent)));
   const int8_t left=int8_t(std::floor(cx-extent)),right=int8_t(std::ceil(cx+extent));
   for(int row=lo;row<=hi;++row){auto& span=phase[row-canonicalTop];span.left=std::min(span.left,left);span.right=std::max(span.right,right);}
  }
 }
 for(unsigned size=minSize;size<=maxSize;++size)for(unsigned roll=0;roll<rollCount;++roll){
  const auto& map=preparedMaps[(size-minSize)*rollCount+roll];const int reach=map.reach;
  const float angle=roll*(6.2831853f/32.f),cs=std::cos(angle),sn=std::sin(angle);
  for(int y=-reach;y<reach;++y){const auto& row=preparedRows[map.row+unsigned(y+reach)];
   for(int x=row.left;x<row.right;++x)preparedTexels[map.base+row.offset+unsigned(x-row.left)]=coordinates(x,y,im,float(size),float(size),cs,sn);
  }
 }
 mapData=preparedMaps;rowData=preparedRows;texelData=preparedTexels;phaseData=preparedPhases;
 stats={true,uint32_t(metaBytes+entries*sizeof(uint32_t)+spans*sizeof(Span)),entries,rows};return true;
}
Info info(){return stats;}
View view(unsigned size,unsigned roll,unsigned frame){
 if(!stats.ready||size<minSize||size>maxSize||roll>=rollCount)return {};
 const auto* map=mapData+(size-minSize)*rollCount+roll;return {map,rowData+map->row,texelData+map->base,phaseData+((frame%clean_coins::frameCount)*rollCount+roll)*canonicalRows};
}
uint32_t verifyHash(){
 if(!stats.ready)return 0;
 uint32_t h=2166136261u;
 auto add=[&](const void* p,size_t n){const auto* b=static_cast<const uint8_t*>(p);while(n--)h=(h^*b++)*16777619u;};
 add(mapData,maps*sizeof(Map));add(rowData,stats.rows*sizeof(Row));add(texelData,stats.entries*sizeof(uint32_t));add(phaseData,clean_coins::frameCount*rollCount*canonicalRows*sizeof(Span));return h;
}
uint32_t decodedHash(){if(!stats.ready)return 0;static const uint32_t value=verifyHash();return value;}
}
