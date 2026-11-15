#include "collect_art.h"
#include "collect_art_data.h"
#include "entry_art.h"
#include <algorithm>
#include <cstdlib>
#ifdef ARDUINO
#include <esp32-hal-psram.h>
#endif
namespace collect_art {
namespace {pa::Image images[collect_data::count]={};uint8_t* storage=nullptr;bool available=false;
uint32_t checksum(const uint16_t* rgb,const uint8_t* alpha,size_t count){
 uint32_t h=2166136261u;for(size_t i=0;i<count;++i){h=(h^uint8_t(rgb[i]))*16777619u;h=(h^uint8_t(rgb[i]>>8))*16777619u;}
 for(size_t i=0;i<count;++i)h=(h^alpha[i])*16777619u;return h;
}}
bool init(){
 if(available)return true;size_t bytes=0;for(auto p:collect_data::sprites)bytes+=p->w*p->h*3+1;
#ifdef ARDUINO
 storage=static_cast<uint8_t*>(ps_malloc(bytes));
#else
 storage=static_cast<uint8_t*>(std::malloc(bytes));
#endif
 if(!storage)return false;size_t at=0;
 for(unsigned i=0;i<collect_data::count;++i){
  if(at&1)++at;const auto& p=*collect_data::sprites[i];const size_t n=p.w*p.h;
  auto rgb=reinterpret_cast<uint16_t*>(storage+at);auto alpha=storage+at+n*2;
  if(!entry_art::decode(p.runs,p.bytes,p.palette,p.opacity,n,rgb,alpha)||checksum(rgb,alpha,n)!=p.fnv){
   std::free(storage);storage=nullptr;for(auto& im:images)im={0,0,nullptr};return false;
  }
  images[i]={uint16_t(p.w),uint16_t(p.h),rgb,alpha,0};at+=n*3;
 }
 available=true;return true;
}
bool ready(){return available;}unsigned storageBytes(){return collect_data::storageBytes;}
const pa::Image& emblem(){return images[0];}const pa::Image& collected(){return images[1];}
const pa::Image& grandCondition(){return images[2];}const pa::Image& number(unsigned n){return images[3+std::min(15u,n)];}
}
