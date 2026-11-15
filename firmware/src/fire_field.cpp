#include "fire_field.h"
#include "fire_field_data.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace fire_field {
namespace {
// Integer channel interpolation keeps every sample inside the source gamut.
uint16_t mix(uint16_t a,uint16_t b,unsigned weight){
 const unsigned inv=256-weight;
 const unsigned r=(((a>>11)*inv+(b>>11)*weight+128)>>8);
 const unsigned g=(((a>>5)&63)*inv+((b>>5)&63)*weight+128)>>8;
 const unsigned blue=(((a&31)*inv+(b&31)*weight+128)>>8);
 return uint16_t((r<<11)|(g<<5)|blue);
}
int32_t lerp(int32_t a,int32_t b,unsigned weight){return a+(b-a)*int32_t(weight)/256;}
struct Axis {uint16_t cell,fraction;};
template<unsigned size,unsigned segments>constexpr std::array<Axis,size> axis(){
 std::array<Axis,size> result{};
 for(unsigned n=0;n<size;++n){const unsigned grid=n*segments*256/(size-1),cell=grid/256<segments?grid/256:segments-1;result[n]={uint16_t(cell),uint16_t(grid-cell*256)};}
 return result;
}
constexpr auto xAxis=axis<width,16>();
constexpr auto yAxis=axis<height,verticalSegments>();
uint16_t material(Point p){
 // Analytic node bounds are u15..209 and v29..215, including the
 // full0..1400ms range. Convex interpolation stays inside the256px map.
 // No clamp or edge branch is needed at every texture lookup.
 const unsigned u=unsigned(p.u),v=unsigned(p.v),ix=u>>8,iy=v>>8,fx=u&255,fy=v&255;
 const auto &indices=fire_data::indices;const auto &palette=fire_data::palette;
 return mix(mix(palette[indices[iy*256+ix]],palette[indices[iy*256+ix+1]],fx),
            mix(palette[indices[(iy+1)*256+ix]],palette[indices[(iy+1)*256+ix+1]],fx),fy);
}
}

Flow::Flow(uint32_t elapsed,Workspace* workspace){
 // The effect ends by 1400ms. Clamp an out-of-range caller instead of ever
 // wrapping the texture or introducing a time reset inside a visible wipe.
 const float t=float(std::min(elapsed,1400u));
 for(unsigned r=0;r<=rows;++r)for(unsigned c=0;c<=columns;++c){
  const float x=float(c)*float(width-1)/columns,y=float(r)*float(height-1)/rows;
  // Rising fire: positive material-v velocity moves visible features UP.
  // Slow independent curl terms deform the filaments instead of translating
  // a rigid photograph. The source map is sampled away from all four edges.
  const float u=30.f+x*.70f+10.f*std::sin(y*.036f+t*.0021f)+5.f*std::sin(x*.027f-y*.020f-t*.0037f);
  const float v=40.f+y*.70f+t*.065f+7.f*std::sin(x*.025f+t*.0017f)+4.f*std::sin(x*.019f+y*.031f-t*.0031f);
  nodes[r*(columns+1)+c]={int32_t(std::lround(u*256.f)),int32_t(std::lround(v*256.f))};
 }
 if(workspace){
  // Preserve the original interpolation order and signed rounding exactly.
  // Each horizontal interpolation is shared by every scanline in its band.
  for(unsigned r=0;r<=rows;++r)for(unsigned x=0;x<width;++x){
   const auto axis=xAxis[x];const auto &a=nodes[r*(columns+1)+axis.cell],&b=nodes[r*(columns+1)+axis.cell+1];
   workspace->horizontal[r*width+x]={lerp(a.u,b.u,axis.fraction),lerp(a.v,b.v,axis.fraction)};
  }
  horizontal=workspace->horizontal;
 }
}

Flow::Row Flow::row(unsigned y) const{
 const auto axis=yAxis[std::min(y,height-1)];
 if(horizontal)return {horizontal+axis.cell*width,horizontal+(axis.cell+1)*width,axis.fraction,true};
 return {nodes+axis.cell*(columns+1),nodes+(axis.cell+1)*(columns+1),axis.fraction,false};
}
Point Flow::Row::uv(unsigned x) const{
 x=std::min(x,width-1);
 if(cached)return {lerp(a[x].u,b[x].u,fy),lerp(a[x].v,b[x].v,fy)};
 const auto axis=xAxis[x];const unsigned c=axis.cell,fx=axis.fraction;
 return {lerp(lerp(a[c].u,a[c+1].u,fx),lerp(b[c].u,b[c+1].u,fx),fy),
         lerp(lerp(a[c].v,a[c+1].v,fx),lerp(b[c].v,b[c+1].v,fx),fy)};
}
Point Flow::uv(unsigned x,unsigned y) const{return row(y).uv(x);}
uint16_t Flow::Row::pixel(unsigned x) const{return material(uv(x));}
uint16_t Flow::pixel(unsigned x,unsigned y) const{return row(y).pixel(x);}
unsigned storageBytes(){return fire_data::storageBytes;}
uint32_t decodedHash(){
 uint32_t hash=2166136261u;
 for(auto i:fire_data::indices){const auto color=fire_data::palette[i];hash=(hash^uint8_t(color))*16777619u;hash=(hash^uint8_t(color>>8))*16777619u;}
 return hash;
}
}
