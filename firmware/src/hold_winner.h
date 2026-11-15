#pragma once
#include <stdint.h>
// Restrained temperature/luminance flicker for the existing clean WINNER rim.
// Its contour/alpha and red/blue channels stay fixed. No source RGB/alpha,
// asset fields, scratch, heap or RNG. Local2048ms loop, not a vendor period.
namespace hold_winner {
constexpr uint32_t periodMs=2048;
inline unsigned triangle(unsigned phase){phase&=255;return phase<128?phase*2:(255-phase)*2;}
inline unsigned gain(unsigned x,unsigned y,uint32_t elapsed){
 const unsigned phase=(elapsed>>3)&255;
 const unsigned a=triangle(phase+x*11+y*7);
 const unsigned b=triangle(phase*3+x*23-y*13);
 return 224+((a+b)*81>>9); //224..304/256; preserve the readable orange rim.
}
inline uint16_t shade(uint16_t color,unsigned x,unsigned y,uint32_t elapsed){
 const unsigned g=(((color>>5)&63)*gain(x,y,elapsed)+128)>>8;
 return uint16_t((color&0xf81f)|((g>63?63:g)<<5));
}
}
