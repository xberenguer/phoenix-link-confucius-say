#pragma once
#include <stdint.h>

// Immutable inverse-sampling coordinates for the existing integer-positioned
//16..30pxcoin poses. Art, alpha, animation clocks and particle laws are not
//changed. Allocation failure leaves the original renderer available.
namespace coin_sampler {
constexpr unsigned minSize=16,maxSize=30,rollCount=32;
constexpr uint32_t validBit=1u<<31;
constexpr int canonicalTop=-25,canonicalRows=50;
struct Row {uint16_t offset;int8_t left,right;};
struct Span {int8_t left,right;};
struct Map {uint32_t base;uint16_t row;uint8_t reach,reserved;};
struct View {const Map* map=nullptr;const Row* rows=nullptr;const uint32_t* texels=nullptr;const Span* phase=nullptr;};
struct Info {bool ready=false;uint32_t bytes=0,entries=0,rows=0;};
bool prepare();
Info info();
View view(unsigned size,unsigned roll,unsigned frame=0);
uint32_t decodedHash();
uint32_t verifyHash();
}
