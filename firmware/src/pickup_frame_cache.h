#pragma once
#include "collection_pickup.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
// Exact per-millisecond memoization of the existing reconstructed route.
// This caches calculations, not animation frames: no pose quantization,
// interpolation, clock change, particle RNG or persisted state is added.
namespace pickup_frames {
using Point=collection_pickup::Point;
constexpr unsigned count=collection_pickup::flightMs+1;
constexpr uint8_t positionBit=1,edgesBit=2,headingBit=4;
struct Key {float sx,sy,tx,ty;};
static_assert(sizeof(Key)==16,"Exact float cache key has no padding");
struct Sample {Point position,edges[2];float heading;};
static_assert(sizeof(Sample)==28,"Bounded pickup cache layout");
constexpr size_t sampleBytes=sizeof(Sample)*count,bytes=sampleBytes+count;
class Bank {
 public:
  bool bind(void* memory,size_t capacity){
   if(!memory||capacity<bytes||reinterpret_cast<uintptr_t>(memory)%alignof(Sample))return false;
   if(samples)return samples==memory;
   samples=static_cast<Sample*>(memory);flags=reinterpret_cast<uint8_t*>(memory)+sampleBytes;
   std::memset(flags,0,count);keyValid=false;return true;
  }
  bool ready()const{return samples!=nullptr;}
  bool select(Key next){
   if(!ready())return false;
   if(!keyValid||std::memcmp(&key,&next,sizeof(Key))){
    key=next;keyValid=true;std::memset(flags,0,count);
   }
   return true;
  }
  bool has(unsigned ms,uint8_t bit){const bool hit=ready()&&ms<count&&(flags[ms]&bit);if(hit)++hits;else ++misses;return hit;}
  Sample& at(unsigned ms){return samples[ms];}
  void mark(unsigned ms,uint8_t bit){flags[ms]|=bit;}
  uint32_t hitCount()const{return hits;}
  uint32_t missCount()const{return misses;}
 private:
  Sample* samples=nullptr;uint8_t* flags=nullptr;Key key{};bool keyValid=false;
  uint32_t hits=0,misses=0;
};
}
