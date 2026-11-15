#pragma once
// Native upper display: Lq6DOzfNIe0 at70s/80s shows an advancing
// "N COLLECTED / 15 CAN WIN GRAND" while the remaining positions spin.
// Retarget that information into the existing77x14 title slot. The49px
// remaining/START slot,1888 meter, reel field and financial footer stay intact.
// This helper owns no new state and never reads the future Engine hold bank.
bool liveHoldCountVisible48(uint32_t now){
 if(!holdView)return false;
 switch(phase){
  case Phase::Spin:case Phase::Land:case Phase::HoldIntro:return true;
  case Phase::Ready:return engine.state.mode==Mode::Hold;
  case Phase::HoldWipe:return holdWipeRevealed;
  case Phase::Revive:return uint32_t(now-phaseAt)>=revival_timing::promptAt;
  default:return false;
 }
}
uint16_t shownHoldMask48(){
 if(phase!=Phase::Spin)return phoenix::Engine::phoenixMask(visible);
 // Existing locks stay in place. New locks become eligible only after the
 // UI has processed their actual impact, exactly as the meter/respins do.
 // Passing cosmetic strip symbols and committed later columns never count.
 uint16_t mask=phoenix::Engine::phoenixMask(previous)&heldMask;
 for(unsigned i=0;i<15;++i)
  if(int(i/3)<=lastArrived&&phoenixCell(target[i]))mask|=uint16_t(1u<<i);
 return mask;
}
unsigned shownHoldCount48(){
 unsigned n=0;for(uint16_t mask=shownHoldMask48();mask;mask>>=1)n+=mask&1u;
 return n;
}
bool drawLiveHoldCount48(uint32_t now){
 if(!liveHoldCountVisible48(now))return false;
 char number[3];snprintf(number,sizeof(number),"%u",shownHoldCount48());
 text(number,12,0,12,BRIGHT,1,1,24);
 text("COLLECTED",25,0,7,BRIGHT,0,25,77);
 // Retain CAN: the source wording is a condition, not an invented promise
 // about the amount or progressive settings of this prototype model.
 text("15 CAN WIN GRAND",25,7,6,WHITE,0,25,77);
 return true;
}
