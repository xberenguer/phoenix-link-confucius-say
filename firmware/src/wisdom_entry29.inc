// Owned source-informed header candidate. Requires the existing UI primitives.
// The source lower-status count and upper prompt share the compact header.
// The source has two separate display areas. This is not an exact cabinet
// layout or a recovered manufacturer font. No game, RNG or input mutation.
#include "wisdom_entry29.h"
void drawWisdomPlaque29(){
 fill(0,0,186,14,native_title::background);
 // Upper source plaque: gold bevel, black top and jade lower field.
 fill(0,0,186,14,0x6a41);
 fill(1,0,184,1,0xeed1);
 fill(0,1,1,12,0xc566);
 fill(185,1,1,12,0x6a41);
 fill(1,13,184,1,0x72a1);
 fill(1,1,184,12,0x0000);
 for(int y=2;y<12;++y){
  const unsigned g=unsigned(y-2)*3+6;
  const uint16_t shade=uint16_t((g>>2)<<5)|uint16_t((g/3)>>3);
  fill(2,y,182,1,shade);
 }
}
void drawWisdomEntry29(uint32_t elapsed,unsigned awarded){
 drawWisdomPlaque29();
 // Measured source samples support approximately 1s on / 1s off. Phase at
 // the actual feature start is not established by the camera-occluded cut.
 // Retarget the source's separate lower count into the blank interval instead
 // of shrinking both source fields into unreadable small header fragments.
 if((elapsed%2000)<1000){
  const std::string count=integer(awarded)+" FREE GAMES WON!";
  text(count.c_str(),93,2,10,WHITE,1,2,184);
 }else image(wisdom_entry29::prompt,2,2,2,2,184,12);
}
void drawWisdomCompletion29(){
 drawWisdomPlaque29();image(wisdom_entry29::completed,2,2,2,2,184,12);
}
