#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>

// One opaque durable record: financial bank, hash-bound scene, immutable
// custom preference and optional service latch. Each section also validates
// its own format before the UI commits any restored state.
namespace checkpoint_bundle {
constexpr size_t headerBytes=40,maxGame=2560,maxScene=230,prefBytes=48,maxService=128;
constexpr size_t maxBytes=headerBytes+maxGame+maxScene+prefBytes+maxService;
struct Part{const uint8_t*bytes=nullptr;size_t length=0;};
struct View{Part game,scene,preference,service;uint64_t generation=0;};
inline uint32_t checksum(const uint8_t*p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i){h^=p[i];h*=16777619u;}return h;}
inline void put(uint8_t*p,uint64_t x,unsigned n){for(unsigned i=0;i<n;++i)p[i]=uint8_t(x>>(8*i));}
inline uint64_t get(const uint8_t*p,unsigned n){uint64_t x=0;for(unsigned i=0;i<n;++i)x|=uint64_t(p[i])<<(8*i);return x;}
inline size_t encode(const View&v,void*destination,size_t capacity){
 if(!v.game.bytes||!v.game.length||v.game.length>maxGame||!v.preference.bytes||v.preference.length!=prefBytes||!v.generation||v.scene.length>maxScene||v.service.length>maxService||(v.scene.length&&!v.scene.bytes)||(v.service.length&&!v.service.bytes))return 0;
 const size_t n=headerBytes+v.game.length+v.scene.length+v.preference.length+v.service.length;
 if(!destination||capacity<n)return n;
 auto*b=static_cast<uint8_t*>(destination);std::memset(b,0,headerBytes);
 put(b,0x314b4350u,4);put(b+4,1,2);put(b+6,headerBytes,2);put(b+8,n,4);
 put(b+12,v.game.length,2);put(b+14,v.scene.length,2);put(b+16,v.preference.length,2);put(b+18,v.service.length,2);put(b+20,v.generation,8);
 size_t at=headerBytes;for(const auto&p:{v.game,v.scene,v.preference,v.service})if(p.length){std::memcpy(b+at,p.bytes,p.length);at+=p.length;}
 put(b+32,checksum(b+headerBytes,n-headerBytes),4);put(b+36,checksum(b,36),4);return n;
}
inline bool decode(const void*source,size_t n,View&destination){
 if(!source||n<headerBytes||n>maxBytes)return false;const auto*b=static_cast<const uint8_t*>(source);
 if(get(b,4)!=0x314b4350u||get(b+4,2)!=1||get(b+6,2)!=headerBytes||get(b+8,4)!=n||get(b+28,4)!=0||get(b+36,4)!=checksum(b,36)||get(b+32,4)!=checksum(b+headerBytes,n-headerBytes))return false;
 View v;v.generation=get(b+20,8);const size_t lengths[]={size_t(get(b+12,2)),size_t(get(b+14,2)),size_t(get(b+16,2)),size_t(get(b+18,2))};
 if(!v.generation||!lengths[0]||lengths[0]>maxGame||lengths[1]>maxScene||lengths[2]!=prefBytes||lengths[3]>maxService||headerBytes+lengths[0]+lengths[1]+lengths[2]+lengths[3]!=n)return false;
 size_t at=headerBytes;Part*parts[]={&v.game,&v.scene,&v.preference,&v.service};for(unsigned i=0;i<4;++i){*parts[i]={b+at,lengths[i]};at+=lengths[i];}
 destination=v;return true;
}
} // namespace checkpoint_bundle
