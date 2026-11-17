// Source-scoped drawing-only central-figure composition candidate. Caller owns confirmed decks and policy.
// Requires cabinetDenom, cabinetLines, cabinetFocus, cabinetAvailable and the
// existing ui.cpp helpers. Deck arrays/counts are passed by the caller.
// No game-state mutation, fallback deck or availability policy occurs here.
// Geometry contract: 1..7 caller-confirmed denominations (cents below $1, whole dollars above),
// one or two line choices <=100, 0..5 multipliers <=10. An empty row has
// no paid action; caller supplies its read-only unavailable-row caption.
// The caller validates choices, focus range and safe cash arithmetic.
// Accepted original publisher CG, deterministically cropped/resampled.
#include "cabinet_selection146.h"
#include "cabinet_type27.h"
struct CabinetWagerArtDeck {
 const int64_t* denoms;unsigned denomCount;
 const uint8_t* lines;unsigned lineCount;
 const uint8_t* bets;unsigned betCount;
};
bool cabinetRoundedInside(int x,int y,int w,int h,unsigned radius){
 if(x<0||y<0||x>=w||y>=h)return false;
 const int r=int(radius),dx=x<r?2*r-1-2*x:x>=w-r?2*x-2*(w-r)+1:0,dy=y<r?2*r-1-2*y:y>=h-r?2*y-2*(h-r)+1:0;
 return !dx||!dy||dx*dx+dy*dy<=4*r*r;
}
std::string cabinetDenomLabel(int64_t denomination);
void cabinetNumber(const char*value,int center,int y,unsigned size,uint16_t color);
uint16_t cabinetRgb27(int r,int g,int b){return uint16_t((unsigned(std::clamp(r,0,255))>>3)<<11|(unsigned(std::clamp(g,0,255))>>2)<<5|(unsigned(std::clamp(b,0,255))>>3));}
void cabinetButtonFace(int x,int y,int w,int h,bool red,bool selected,bool enabled=true){
 for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){
  if(!cabinetRoundedInside(xx,yy,w,h,5))continue;
  const bool inner1=cabinetRoundedInside(xx-1,yy-1,w-2,h-2,4);
  const bool inner2=cabinetRoundedInside(xx-2,yy-2,w-4,h-4,3);
  const bool face=cabinetRoundedInside(xx-3,yy-3,w-6,h-6,2);
  const int north=(h-1-yy)*255/std::max(1,h-1),west=(w-1-xx)*255/std::max(1,w-1);
  const int light=(north*3+west)/4;
  uint16_t color;
  if(!inner1)color=cabinetRgb27(54+light/3,25+light/5,3+light/25);
  else if(!inner2){const int shine=std::max(0,200-std::abs(2*yy-3)*15);color=cabinetRgb27(128+light/3+shine/7+(selected?12:0),68+light/3+shine/5+(selected?12:0),9+light/10+shine/4);}
  else if(!face)color=yy<h/2?cabinetRgb27(43,24,7):cabinetRgb27(169,102,17);
  else{
   const int edge=std::abs(2*xx-w)*255/std::max(1,w),depth=yy*255/std::max(1,h-1);
   if(red){const int shine=std::max(0,60-std::abs(yy-6)*12);color=cabinetRgb27(247-edge/5-depth/9+shine/5,7+shine/7,6);}
   else{const int shine=std::max(0,40-std::abs(yy-6)*7);color=cabinetRgb27(34-edge/18-depth/30+shine/5,41-edge/17-depth/24+shine/5,53-edge/15-depth/19+shine/4);}
  }
  if(!enabled)color=blend(0x39c7,color,120);
  dot(x+xx,y+yy,color);
 }
}
int cabinetTypeWidth27(const char* value,unsigned size){int width=0;for(const unsigned char ch:std::string(value))width+=cabinet_type27::glyph(size,ch).advance;return width;}
void cabinetTypeText27(const char* value,int x,int y,unsigned size,uint16_t color){
 const auto* alpha=cabinet_type27::pixels(size);
 for(const unsigned char ch:std::string(value)){
  const auto& g=cabinet_type27::glyph(size,ch);
  for(unsigned yy=0;yy<g.h;++yy)for(unsigned xx=0;xx<g.w;++xx)if(alpha[g.offset+yy*g.w+xx])dot(x+int(xx),y+int(yy),color,alpha[g.offset+yy*g.w+xx]);
  x+=g.advance;
 }
}
void cabinetMachineCaption27(const char* value,int center,int y,unsigned size,uint16_t color=WHITE){const int left=center-cabinetTypeWidth27(value,size)/2;cabinetTypeText27(value,left+1,y+1,size,BLACK);cabinetTypeText27(value,left,y,size,color);}
int cabinetCoinWidth27(int64_t denomination,unsigned size){if(denomination>=100)return cashWidth(cabinetDenomLabel(denomination).c_str(),size);const auto& cent=size==16?cabinet_type27::centGlyph16:cabinet_type27::centGlyph10;return cashWidth(integer(denomination).c_str(),size)+1+cent.advance;}
void cabinetCoinNumber27(int64_t denomination,int center,int y,unsigned size){
 if(denomination>=100){cabinetNumber(cabinetDenomLabel(denomination).c_str(),center,y,size,WHITE);return;}
 const auto digits=integer(denomination);const auto& cent=size==16?cabinet_type27::centGlyph16:cabinet_type27::centGlyph10;const auto* alpha=size==16?cabinet_type27::cent16:cabinet_type27::cent10;
 const int left=center-cabinetCoinWidth27(denomination,size)/2,numberWidth=cashWidth(digits.c_str(),size);
 cashText(digits.c_str(),left+numberWidth+1,y+1,size,BLACK);cashText(digits.c_str(),left+numberWidth,y,size,WHITE);
 for(unsigned shadow=0;shadow<2;++shadow)for(unsigned yy=0;yy<cent.h;++yy)for(unsigned xx=0;xx<cent.w;++xx)if(alpha[yy*cent.w+xx])dot(left+numberWidth+1+int(xx)+(shadow?0:1),y+int(yy)+(shadow?0:1),shadow?WHITE:BLACK,alpha[yy*cent.w+xx]);
}
void cabinetCaption(const char*value,int x,int y,unsigned size=6,uint16_t color=WHITE){text(value,x,y+1,size,BLACK,1);text(value,x,y,size,color,1);}
void cabinetNumber(const char*value,int center,int y,unsigned size,uint16_t color=WHITE){
 const int right=center+cashWidth(value,size)/2;cashText(value,right+1,y+1,size,BLACK);cashText(value,right,y,size,color);
}
void cabinetFocusFrame(int x,int y,int w,int h){
 // Discrete white/gold navigation corners, separate from the selected bevel.
 for(int side=0;side<2;++side)for(int end=0;end<2;++end){
  const int xx=side?x+w:x-1,yy=end?y+h:y-1,dx=side?-1:1,dy=end?-1:1;
  line(xx+dx*3,yy,xx+dx*8,yy,WHITE);line(xx,yy+dy*3,xx,yy+dy*7,WHITE);
  dot(xx+dx*2,yy+dy,0xffe0);dot(xx+dx,yy+dy*2,0xffe0);
 }
}
std::string cabinetDenomLabel(int64_t denomination){return denomination<100?integer(denomination)+"c":"$"+integer(denomination/100);}
void cabinetDenomFace23(int x,int y,int w,int h,bool cyan,bool selected,bool green=false){
 for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){
  if(!cabinetRoundedInside(xx,yy,w,h,4))continue;
  const bool inner1=cabinetRoundedInside(xx-1,yy-1,w-2,h-2,3),inner3=cabinetRoundedInside(xx-3,yy-3,w-6,h-6,2);
  uint16_t color;
  if(!inner1)color=cyan?(yy<h/2?cabinetRgb27(18,193,255):cabinetRgb27(1,62,161)):(yy<h/2?0xff6b:0x7220);
  else if(!inner3)color=cyan?(yy<h/2?cabinetRgb27(94,235,255):cabinetRgb27(7,109,223)):(yy<h/2?0xffee:0xb402);
  else{const int edge=std::abs(2*xx-w)*3/w;const unsigned t=unsigned(yy)*255/unsigned(h-1);color=cyan?blend(cabinetRgb27(2,111,229),cabinetRgb27(8,185,247),t):blend(0xb2e0,0xfe42,t);if(edge)color=blend(cyan?0x0396:0xaa00,color,unsigned(edge)*20);}
  if(green){if(!inner1)color=yy<h/2?cabinetRgb27(16,122,6):cabinetRgb27(0,55,0);else if(!inner3)color=yy<h/2?cabinetRgb27(91,245,35):cabinetRgb27(7,116,4);else color=blend(cabinetRgb27(1,112,2),cabinetRgb27(35,232,12),unsigned(yy)*255/unsigned(h-1));}
  dot(x+xx,y+yy,color);
 }
 if(selected){line(x+4,y+2,x+w-5,y+2,WHITE);line(x+2,y+4,x+2,y+h-5,cyan?0xafff:0xfff5);}
}
uint8_t cabinetDraftMultiplier24(const CabinetWagerArtDeck&deck){
 for(unsigned i=0;i<deck.betCount;++i)if(deck.bets[i]==engine.state.betMultiplier)return deck.bets[i];
 return deck.betCount?deck.bets[0]:0; // Never invent a multiplier for an unknown row.
}
void cabinetTempleBackdrop28(){image(cabinet_scene145::image,0,0);}

void drawCabinetWager(const CabinetWagerArtDeck&deck){
 fill(0,0,W,H,BLACK);
 if(cabinetDenominationPage146){
  image(cabinet_selection146::denomination,0,0);
  cabinetButtonFace(184,4,50,16,false,false);cabinetCaption("RETURN",209,8,8,WHITE);
  fill(3,103,234,31,0x1800);rect(3,103,234,31,GOLD);rect(4,104,232,29,0xb341);
  for(int y=105;y<132;++y)for(int x=5;x<235;++x)if(((x+y)%8)==0)dot(x,y,0x4800);
  cabinetMachineCaption27("CHOOSE YOUR DENOMINATION",120,105,6,0xff8a);
  const int gap=2,coinW=(226-int(deck.denomCount-1)*gap)/int(deck.denomCount),coinY=112,coinH=20;
  const int start=(240-int(deck.denomCount)*coinW-int(deck.denomCount-1)*gap)/2;
  for(unsigned i=0;i<deck.denomCount;++i){
   const int x=start+int(i)*(coinW+gap);const auto denomination=deck.denoms[i];
   cabinetDenomFace23(x,coinY,coinW,coinH,denomination<100,denomination==cabinetDenom,highLimit146&&denomination<2500);
   const unsigned size=cabinetCoinWidth27(denomination,16)<=coinW-6?16:10;
   cabinetCoinNumber27(denomination,x+coinW/2,coinY+(coinH-int(size))/2,size);
   if(i==cabinetFocus)cabinetFocusFrame(x,coinY,coinW,coinH);
  }
  if(cabinetFocus==cabinetReturnFocus())cabinetFocusFrame(184,4,50,16);
  return;
 }
 cabinetTempleBackdrop28();rect(1,1,W-2,H-2,0x8321);rect(2,2,W-4,H-4,0x3182);
 int focusX=0,focusY=0,focusW=0,focusH=0;
 const unsigned betFocusBase=deck.lineCount,denomFocus=betFocusBase+deck.betCount,returnFocus=denomFocus+1;
 cabinetDenomFace23(6,6,48,21,cabinetDenom<100,true);cabinetCoinNumber27(cabinetDenom,30,8,cabinetCoinWidth27(cabinetDenom,16)<=42?16:10);
 for(unsigned i=0;i<deck.lineCount;++i){
  const int x=235-int(deck.lineCount-i)*46,y=36;const auto lines=deck.lines[i];
  cabinetButtonFace(x,y,45,37,false,lines==cabinetLines);
  cabinetMachineCaption27("PLAY",x+22,y+2,6,WHITE);cabinetNumber(integer(lines).c_str(),x+22,y+9,16);
  cabinetMachineCaption27("LINES",x+22,y+26,5,WHITE);
  if(i==cabinetFocus){focusX=x;focusY=y;focusW=45;focusH=37;}
 }
 for(unsigned i=0;i<deck.betCount;++i){
  const int x=(W-int(deck.betCount)*45-int(deck.betCount-1))/2+int(i)*46,y=75;const auto multiplier=deck.bets[i];
  const bool selected=cabinetDenom==engine.state.coinValue&&cabinetLines==engine.state.lineCount&&multiplier==engine.state.betMultiplier;
  cabinetButtonFace(x,y,45,39,true,selected,cabinetAvailable);
  const auto multiplierText=integer(multiplier);const int prefixWidth=cabinetTypeWidth27("BET x",6),labelWidth=prefixWidth+cabinetTypeWidth27(multiplierText.c_str(),6),labelX=x+22-labelWidth/2;
  cabinetTypeText27("BET x",labelX+1,y+3,6,BLACK);cabinetTypeText27(multiplierText.c_str(),labelX+prefixWidth+1,y+3,6,BLACK);
  cabinetTypeText27("BET x",labelX,y+2,6,cabinetAvailable?WHITE:0x9cd3);cabinetTypeText27(multiplierText.c_str(),labelX+prefixWidth,y+2,6,cabinetAvailable?0xffe0:0xad40);
  const auto amount=integer(int64_t(cabinetLines)*multiplier);const unsigned size=cashWidth(amount.c_str(),20)<=39?20:16;
  cabinetNumber(amount.c_str(),x+22,y+9,size,cabinetAvailable?WHITE:0x9cd3);
  cabinetMachineCaption27("CREDITS",x+22,y+30,5,cabinetAvailable?WHITE:0x9cd3);
  if(i+betFocusBase==cabinetFocus){focusX=x;focusY=y;focusW=45;focusH=39;}
 }
 fill(5,117,173,16,BLACK);line(6,116,177,116,0x72e2);
 cabinetButtonFace(6,117,51,16,false,false);cabinetCaption("DENOM",31,121,8,WHITE);
 const unsigned previewMultiplier=cabinetFocus>=betFocusBase&&cabinetFocus<denomFocus?deck.bets[cabinetFocus-betFocusBase]:cabinetDraftMultiplier24(deck);
 const int64_t credits=int64_t(cabinetLines)*previewMultiplier,cash=cabinetDenom*credits;
 text(money(cash).c_str(),120,120,10,WHITE,1);
 cabinetButtonFace(184,117,50,16,false,false);cabinetCaption("RETURN",209,121,8,WHITE);
 if(cabinetFocus==denomFocus){focusX=6;focusY=117;focusW=51;focusH=16;}
 if(cabinetFocus==returnFocus){focusX=184;focusY=117;focusW=50;focusH=16;}
 if(focusW)cabinetFocusFrame(focusX,focusY,focusW,focusH);
}
