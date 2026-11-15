#include "entry_art.h"
#include "entry_art_data.h"
#include <cstdlib>
#ifdef ARDUINO
#include <esp32-hal-psram.h>
#endif
namespace {
uint8_t* storage=nullptr;
pa::Image current={0,0,nullptr};
bool available=false;
uint32_t hash=0;
uint32_t checksum(const uint16_t* rgb,const uint8_t* alpha,size_t count){
 uint32_t h=2166136261u;
 for(size_t i=0;i<count;++i){h=(h^uint8_t(rgb[i]))*16777619u;h=(h^uint8_t(rgb[i]>>8))*16777619u;}
 for(size_t i=0;i<count;++i)h=(h^alpha[i])*16777619u;
 return h;
}
}
namespace entry_art {
bool decode(const uint8_t* source,size_t bytes,const uint16_t* palette,const uint8_t* opacity,size_t count,uint16_t* rgb,uint8_t* alpha){
 if(!source||!bytes||!palette||!opacity||!count||!rgb||!alpha)return false;
 size_t in=0,out=0;
 while(in<bytes&&out<count){
  const uint8_t control=source[in++];const size_t n=size_t(control&127)+1;
  if(n>count-out)return false;
  if(control&128){for(size_t i=0;i<n;++i){rgb[out]=palette[0];alpha[out++]=opacity[0];}}
  else {
   if(n>bytes-in)return false;
   for(size_t i=0;i<n;++i){const unsigned index=source[in++];rgb[out]=palette[index];alpha[out++]=opacity[index];}
  }
 }
 return in==bytes&&out==count;
}
bool init(){
 if(available)return true;
 constexpr size_t count=entry_data::width*entry_data::height;
#ifdef ARDUINO
 storage=static_cast<uint8_t*>(ps_malloc(count*3));
#else
 storage=static_cast<uint8_t*>(std::malloc(count*3));
#endif
 if(!storage)return false;
 auto* rgb=reinterpret_cast<uint16_t*>(storage);auto* alpha=storage+count*2;
 if(!decode(entry_data::runs,sizeof(entry_data::runs),entry_data::palette,entry_data::opacity,count,rgb,alpha)){
  std::free(storage);storage=nullptr;return false;
 }
 hash=checksum(rgb,alpha,count);
 if(hash!=entry_data::decodedFNV){std::free(storage);storage=nullptr;hash=0;return false;}
 current={entry_data::width,entry_data::height,rgb,alpha,0};available=true;return true;
}
bool ready(){return available;}
const pa::Image& sprite(){return current;}
unsigned storageBytes(){return entry_data::storageBytes;}
uint32_t decodedHash(){return hash;}
}
