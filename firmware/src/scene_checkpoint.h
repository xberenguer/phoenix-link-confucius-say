#pragma once
#include "phoenix_engine.h"

// Optional presentation sidecar. The financial/RNG snapshot is unchanged.
// A64-bit hash binds this scene to the exact independently saved game bytes.
// An interrupted two-key write yields a rejected scene, never a stale board.
namespace phoenix_scene {
constexpr size_t kLegacyBytes=200,kBytes=230;
constexpr unsigned kPhoenixPeriodMs=2500;
struct Scene {
  bool holdGrid=false,backdropCaptured=false;
  phoenix::Cell backdrop[phoenix::kCells]{};
  uint16_t phoenixPhases[phoenix::kCells]{};
  bool hasPhoenixPhases=true;
};
inline uint64_t gameHash(const void* p,size_t n){
  const auto* b=static_cast<const uint8_t*>(p);uint64_t h=14695981039346656037ULL;
  for(size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ULL;}return h;
}
inline uint32_t checksum(const uint8_t* p,size_t n){
  uint32_t h=2166136261u;for(size_t i=0;i<n;++i){h^=p[i];h*=16777619u;}return h;
}
inline void put(uint8_t*& p,uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)*p++=uint8_t(v>>(i*8));}
inline uint64_t get(const uint8_t*& p,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;++i)v|=uint64_t(*p++)<<(i*8);return v;}
inline bool valid(const phoenix::Cell& c){
  using namespace phoenix;
  if(uint8_t(c.symbol)>=uint8_t(Symbol::Count)||uint8_t(c.kind)>uint8_t(PrizeKind::Major)||c.multiplier<1||c.multiplier>3||c.prize<0||c.prize>local::maximumCreditPrize)return false;
  return c.symbol==Symbol::Phoenix?c.prize>0&&(c.kind!=PrizeKind::Credits||c.multiplier==1):c.prize==0&&c.kind==PrizeKind::Credits&&c.multiplier==1;
}
inline size_t encode(const Scene& scene,const void* game,size_t gameLength,void* target,size_t capacity){
  if(!target||capacity<kBytes)return kBytes;
  if(!game||!gameLength||gameLength>phoenix::kSaveCapacity)return 0;
  for(const auto& c:scene.backdrop)if(!valid(c))return 0;
  for(auto phase:scene.phoenixPhases)if(phase>=kPhoenixPeriodMs)return 0;
  auto* begin=static_cast<uint8_t*>(target);auto* p=begin;
  put(p,0x43534c50u,4);put(p,2,1);put(p,unsigned(scene.holdGrid)|(unsigned(scene.backdropCaptured)<<1),1);put(p,gameLength,2);put(p,gameHash(game,gameLength),8);
  for(const auto& c:scene.backdrop){put(p,unsigned(c.symbol),1);put(p,unsigned(c.kind),1);put(p,c.multiplier,1);put(p,c.highlight,1);put(p,uint64_t(c.prize),8);}
  for(auto phase:scene.phoenixPhases)put(p,phase,2);
  put(p,checksum(begin,kBytes-4),4);return kBytes;
}
inline bool decode(const void* source,size_t length,const void* game,size_t gameLength,Scene& target){
  if(!source||(length!=kBytes&&length!=kLegacyBytes)||!game||!gameLength||gameLength>phoenix::kSaveCapacity)return false;
  const auto* begin=static_cast<const uint8_t*>(source);const auto* p=begin;const auto* end=begin+length-4;
  if(get(end,4)!=checksum(begin,length-4)||get(p,4)!=0x43534c50u)return false;
  const auto version=get(p,1);if((version!=1||length!=kLegacyBytes)&&(version!=2||length!=kBytes))return false;
  const auto flags=get(p,1);if(flags>3||get(p,2)!=gameLength||get(p,8)!=gameHash(game,gameLength))return false;
  Scene result;result.holdGrid=flags&1;result.backdropCaptured=flags&2;
  for(auto& c:result.backdrop){c.symbol=phoenix::Symbol(get(p,1));c.kind=phoenix::PrizeKind(get(p,1));c.multiplier=uint8_t(get(p,1));const auto highlight=get(p,1);if(highlight>1)return false;c.highlight=highlight;const auto prize=get(p,8);if(prize>uint64_t(phoenix::local::maximumCreditPrize))return false;c.prize=int64_t(prize);if(!valid(c))return false;}
  result.hasPhoenixPhases=version==2;
  if(result.hasPhoenixPhases)for(auto& phase:result.phoenixPhases){phase=uint16_t(get(p,2));if(phase>=kPhoenixPeriodMs)return false;}
  target=result;return true;
}
}
