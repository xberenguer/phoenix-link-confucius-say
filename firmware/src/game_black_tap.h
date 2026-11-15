#pragma once
#include <cstdint>

// The larger side key opens the wager panel after one short release. Wait
// out the double-click window so a screen flip never opens that panel first.
// Menu navigation and the blue spin key remain immediate.
struct GameBlackTap {
  static constexpr uint32_t windowMs=400;
  uint32_t last=0;
  bool pending=false;
  void reset(){pending=false;}
  bool poll(uint32_t now,bool game,bool down=false){
    if(!game){reset();return false;}
    if(pending&&!down&&uint32_t(now-last)>windowMs){reset();return true;}
    return false;
  }
  bool released(uint32_t now){
    if(pending&&uint32_t(now-last)<=windowMs){reset();return true;}
    last=now;pending=true;return false;
  }
};
