#include "winner_wordmark89.h"
#include "hold_center_body89.h"
#include "counter_fire89.h"
// Display-only center adaptation. The foreground Phoenix/ribbons retain
// their existing clocks; no game/counter state, new cache or clock is added.
bool holdCenterWinnerActive89(uint32_t now){
 if(!collectCount())return false;
 const uint32_t elapsed=now-phaseAt;
 return phase==Phase::Collect?elapsed>=collectionLead():
        phase==Phase::WinTransfer||
        (phase==Phase::ReturnWipe&&ordinaryCollectionTransfer()&&elapsed<collectionReturnLead());
}
#include "center_render49.inc"
void centeredWinnerLabel89(uint32_t elapsed){center_render49::word(48,39,elapsed);}
bool drawHoldCenterWinner89(uint32_t now){
 if(!holdCenterWinnerActive89(now))return false;
 centeredWinnerLabel89(now-lastAward.beganAt);
 // displayWin owns the pickup group, including its locked Mini/Minor
 // cells. Major/Grand intros and prior Base/Free winnings stay separate.
 const auto value=money(phase==Phase::Collect?displayWin:collectCashTotal());
 unsigned size=20;
 for(unsigned candidate:{20u,16u,10u}){size=candidate;if(cashWidth(value.c_str(),size)<=164)break;}
 fill(31,71,178,27,BLACK,220);
 // Reuse the sealed, source-measured fire motion with authored pigment.
 // Its geometry stays fixed; a two-pixel glow surrounds this amount box.
 counter_fire89::draw(now,31,208,26,[](int x,int y,uint16_t color,unsigned){
  y+=71;
  for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)
   if(dx||dy)dot(x+dx,y+dy,color,44);
  dot(x-2,y,color,18);dot(x+2,y,color,18);
  dot(x,y-2,color,18);dot(x,y+2,color,18);
 });
 counter_fire89::draw(now,31,208,26,[](int x,int y,uint16_t color,unsigned a){dot(x,y+71,color,a);});
 const int right=(W+cashWidth(value.c_str(),size))/2,y=74+(20-int(size))/2;
 center_render49::outlinedCash(value.c_str(),right,y,size);
 return true;
}
