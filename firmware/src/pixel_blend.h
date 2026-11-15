#pragma once
#include <cstdint>

namespace pixel_blend {
// Exact rounded8-bit-alpha RGB565 blend. Red and blue occupy independent
// 16-bit lanes (each weighted sum is <=7905); green is evaluated separately.
// Division by255 uses x+128, then (x+(x>>8))>>8, exact in these bounds.
inline uint16_t over(uint16_t src,uint16_t dst,unsigned alpha){
 const unsigned inverse=255-alpha;
 const uint32_t source=(src&31u)|((uint32_t(src)&0xf800u)<<5);
 const uint32_t target=(dst&31u)|((uint32_t(dst)&0xf800u)<<5);
 uint32_t rb=source*alpha+target*inverse+0x00800080u;
 rb+=(rb>>8)&0x00ff00ffu;
 rb=(rb>>8)&0x001f001fu;
 uint32_t green=((src>>5)&63u)*alpha+((dst>>5)&63u)*inverse+128u;
 green+=green>>8;
 return uint16_t(((rb>>5)&0xf800u)|((green>>3)&0x07e0u)|(rb&31u));
}
}
