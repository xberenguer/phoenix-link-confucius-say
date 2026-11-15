// Source-scoped drawing-only central-figure composition candidate. Caller owns confirmed decks and policy.
// Requires cabinetDenom, cabinetLines, cabinetFocus, cabinetAvailable and the
// existing ui.cpp helpers. Deck arrays/counts are passed by the caller.
// No game-state mutation, fallback deck or availability policy occurs here.
// Geometry contract: 1..6 caller-confirmed denominations (cents below $1, whole dollars above),
// one or two line choices <=100, 0..5 multipliers <=10. An empty row has
// no paid action; caller supplies its read-only unavailable-row caption.
// The caller validates choices, focus range and safe cash arithmetic.
// Accepted original publisher CG, deterministically cropped/resampled.
#include "cabinet_sage_large28.h"
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
void cabinetDenomFace23(int x,int y,int w,int h,bool cyan,bool selected){
 for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){
  if(!cabinetRoundedInside(xx,yy,w,h,4))continue;
  const bool inner1=cabinetRoundedInside(xx-1,yy-1,w-2,h-2,3),inner3=cabinetRoundedInside(xx-3,yy-3,w-6,h-6,2);
  uint16_t color;
  if(!inner1)color=cyan?(yy<h/2?cabinetRgb27(18,193,255):cabinetRgb27(1,62,161)):(yy<h/2?0xff6b:0x7220);
  else if(!inner3)color=cyan?(yy<h/2?cabinetRgb27(94,235,255):cabinetRgb27(7,109,223)):(yy<h/2?0xffee:0xb402);
  else{const int edge=std::abs(2*xx-w)*3/w;const unsigned t=unsigned(yy)*255/unsigned(h-1);color=cyan?blend(cabinetRgb27(2,111,229),cabinetRgb27(8,185,247),t):blend(0xb2e0,0xfe42,t);if(edge)color=blend(cyan?0x0396:0xaa00,color,unsigned(edge)*20);}
  dot(x+xx,y+yy,color);
 }
 if(selected){line(x+4,y+2,x+w-5,y+2,WHITE);line(x+2,y+4,x+2,y+h-5,cyan?0xafff:0xfff5);}
}
uint8_t cabinetDraftMultiplier24(const CabinetWagerArtDeck&deck){
 for(unsigned i=0;i<deck.betCount;++i)if(deck.bets[i]==engine.state.betMultiplier)return deck.bets[i];
 return deck.betCount?deck.bets[0]:0; // Never invent a multiplier for an unknown row.
}
void cabinetTempleBackdrop28(){
 // Authored low-resolution temple geometry. No camera RGB, text, or texture.
 for(int y=0;y<115;++y){const unsigned t=unsigned(y)*255/114;fill(0,y,W,1,blend(cabinetRgb27(80,96,82),cabinetRgb27(174,178,151),t));}
 // Receding wall and door bays under a red/gold temple eave.
 fill(7,34,226,76,cabinetRgb27(93,104,87));
 for(int x=14;x<240;x+=38){fill(x,37,25,73,cabinetRgb27(48,76,67));rect(x,37,25,73,cabinetRgb27(157,139,75));rect(x+2,39,21,69,cabinetRgb27(92,117,94));for(int yy=44;yy<107;yy+=7)line(x+5,yy,x+19,yy,cabinetRgb27(62,88,73));}
 for(int x=-18;x<250;x+=38){fill(x,30,8,84,cabinetRgb27(164,153,111));fill(x+1,31,3,82,cabinetRgb27(205,194,145));fill(x+6,31,2,82,cabinetRgb27(97,98,71));}
 fill(0,10,240,20,cabinetRgb27(122,32,21));
 for(int y=10;y<28;++y)for(int x=0;x<240;++x){const int slope=(x+y*2)%13;if(slope<2)dot(x,y,cabinetRgb27(176,59,32));else if(slope==3)dot(x,y,cabinetRgb27(71,26,23));}
 fill(0,27,240,3,cabinetRgb27(179,123,50));line(0,27,239,27,cabinetRgb27(229,184,92));
 for(int x=3;x<240;x+=13){fill(x,30,5,3,cabinetRgb27(129,39,29));dot(x+2,31,cabinetRgb27(227,175,65));}
 // Low shrubs and stone foreground are visible only between the controls.
 for(int x=0;x<240;++x){const int top=62+(x*7%11);for(int y=top;y<87;++y)dot(x,y,cabinetRgb27(42+(x+y)%13,82+(x*3+y)%21,35+(x+y*2)%9));}
 fill(0,90,240,25,cabinetRgb27(114,119,105));for(int y=92;y<115;y+=9)line(0,y,239,y,cabinetRgb27(71,83,75));
}
void drawCabinetWager(const CabinetWagerArtDeck&deck){
 // Existing clean game backdrop, never sampled footage or attachment pixels.
 fill(0,0,W,H,BLACK);cabinetTempleBackdrop28();
 const pa::Image sageFigure={cabinet_sage_large28::width,cabinet_sage_large28::height,cabinet_sage_large28::rgb,cabinet_sage_large28::alpha,0};
 image(sageFigure,56,3,3,3,W-3,115);
 rect(1,1,W-2,H-2,0x8321);rect(2,2,W-4,H-4,0x3182);
 int focusX=0,focusY=0,focusW=0,focusH=0;
 const unsigned lineFocusBase=deck.denomCount,betFocusBase=lineFocusBase+deck.lineCount,returnFocus=betFocusBase+deck.betCount;
 const unsigned coinCount=deck.denoms?deck.denomCount:0;
 const int coinW=34,coinH=21,coinY=6;
 const int coinStart=5;
 for(unsigned i=0;i<coinCount;++i){
  const int x=coinStart+int(i%2)*(coinW+3),y=coinY+int(i/2)*23;const auto denomination=deck.denoms[i];
  cabinetDenomFace23(x,y,coinW,coinH,denomination<100,denomination==cabinetDenom);
  const unsigned size=coinW>=34&&cabinetCoinWidth27(denomination,16)<=coinW-6?16:10;
  cabinetCoinNumber27(denomination,x+coinW/2,y+(coinH-int(size))/2,size);
  if(i==cabinetFocus){focusX=x;focusY=y;focusW=coinW;focusH=coinH;}
 }
 for(unsigned i=0;i<deck.lineCount;++i){
  const int x=235-int(deck.lineCount-i)*46,y=36;const auto lines=deck.lines[i];
  cabinetButtonFace(x,y,45,37,false,lines==cabinetLines);
  cabinetMachineCaption27("PLAY",x+22,y+2,6,WHITE);cabinetNumber(integer(lines).c_str(),x+22,y+9,16);
  cabinetMachineCaption27("LINES",x+22,y+26,5,WHITE);
  if(i+lineFocusBase==cabinetFocus){focusX=x;focusY=y;focusW=45;focusH=37;}
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
 fill(5,117,173,16,0x0000);line(6,116,177,116,0x72e2);
 const unsigned previewMultiplier=cabinetFocus>=betFocusBase&&cabinetFocus<returnFocus?deck.bets[cabinetFocus-betFocusBase]:cabinetDraftMultiplier24(deck);
 const int64_t credits=int64_t(cabinetLines)*previewMultiplier,cash=cabinetDenom*credits;
 const char* hint=cabinetAvailable?(cabinetFocus>=betFocusBase&&cabinetFocus<returnFocus?"GREY: NEXT  BLUE: BET":"GREY: NEXT  BLUE: SELECT"):engine.state.mode!=Mode::Base||engine.state.roundOpen?"FEATURE IN PLAY - RETURN":"BET CHANGE PAUSED - RETURN";
 text(hint,8,117,6,BRIGHT);text((integer(credits)+" CREDITS  "+money(cash)).c_str(),8,124,8,WHITE);
 cabinetButtonFace(184,117,50,16,false,false);cabinetCaption("RETURN",209,121,8,WHITE);
 if(cabinetFocus==returnFocus){focusX=184;focusY=117;focusW=50;focusH=16;}
 if(focusW)cabinetFocusFrame(focusX,focusY,focusW,focusH);
}
