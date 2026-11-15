#pragma once
#include <cstdint>

// A power-key press is a menu tap only after a clean, short release. Long
// holds belong to the PMIC download mechanism. Failed reads never invent edges.
class GreyButton {
 public:
  void reset(){primed_=candidate_=stable_=armed_=false;}
  bool sample(bool ok,bool down,uint32_t now){
    if(!ok){reset();return false;}
    if(!primed_){primed_=true;candidate_=stable_=down;changedAt_=now;armed_=false;return false;}
    if(down!=candidate_){candidate_=down;changedAt_=now;}
    if(candidate_==stable_||uint32_t(now-changedAt_)<20)return false;
    stable_=candidate_;
    if(stable_){pressedAt_=changedAt_;armed_=true;return false;}
    const bool tap=armed_&&uint32_t(changedAt_-pressedAt_)<700;
    armed_=false;return tap;
  }
 private:
  bool primed_=false,candidate_=false,stable_=false,armed_=false;
  uint32_t changedAt_=0,pressedAt_=0;
};
