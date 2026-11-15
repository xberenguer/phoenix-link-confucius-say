// Optional exact-word compact caption for the first-spin total ramp. Include
// only once wired to a caller; the root owns its epoch and line-caption
// placement. The normal candidate does not silently remove winning-line info.
void drawWisdomFreeCaption29(unsigned played,unsigned total){
 fill(77,0,49,14,native_title::background);
 headerCaption("FREE GAME",0,6,BRIGHT);
 const std::string count=integer(played)+" OF "+integer(total);
 headerCaption(count.c_str(),7,7,BRIGHT);
}
