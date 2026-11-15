#pragma once
#include <cstddef>
#include <cstdint>

// Display-rate samples of the existing reconstructed shaders, not camera
// frames or a recovered manufacturer bank. Opacity envelopes retain their
// live clock; immutable PSRAM holds the expensive material/filtered poses.
namespace effect_frames {
constexpr unsigned width=240,height=135,pixels=width*height,rate=30;
constexpr unsigned fireCount=43,birdCount=14;
struct Span {uint8_t left,right;};
constexpr size_t materialBytes=size_t(pixels)*(2*fireCount+3*birdCount);
constexpr size_t spanBytes=size_t(height)*birdCount*sizeof(Span);
constexpr size_t bytes=materialBytes+spanBytes;
inline unsigned frame(unsigned elapsed){return (uint64_t(elapsed)*rate+500)/1000;}
inline unsigned time(unsigned index){return (index*1000+rate/2)/rate;}
inline unsigned sample(unsigned elapsed){return time(frame(elapsed));}
struct Bank {
 uint16_t* fire=nullptr;
 uint16_t* bird=nullptr;
 uint8_t* opacity=nullptr;
 Span* spans=nullptr;
 void bind(void* block){fire=static_cast<uint16_t*>(block);bird=fire+size_t(pixels)*fireCount;opacity=reinterpret_cast<uint8_t*>(bird+size_t(pixels)*birdCount);spans=reinterpret_cast<Span*>(opacity+size_t(pixels)*birdCount);}
 const uint16_t* fireAt(unsigned ms)const{return fire+size_t(frame(ms)<fireCount?frame(ms):fireCount-1)*pixels;}
 const uint16_t* birdAt(unsigned ms)const{return bird+size_t(frame(ms)<birdCount?frame(ms):birdCount-1)*pixels;}
 const uint8_t* opacityAt(unsigned ms)const{return opacity+size_t(frame(ms)<birdCount?frame(ms):birdCount-1)*pixels;}
 const Span* spansAt(unsigned ms)const{return spans+size_t(frame(ms)<birdCount?frame(ms):birdCount-1)*height;}
};
}
