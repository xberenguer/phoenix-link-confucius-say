// Owned source-informed header candidate. Requires the existing UI primitives.
// The source lower-status count and upper prompt share the compact header.
// The source has two separate display areas. This is not an exact cabinet
// layout or a recovered manufacturer font. No game, RNG or input mutation.
#include "wisdom_entry29.h"
void drawWisdomPlaque29(){
 // Use the same matte as the compact game header.
 fill(0,0,186,14,native_title::background);
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
