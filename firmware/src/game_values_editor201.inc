// Live-session value editing. No alternate game, outcome fixture or HUD mode.
// Engine setters own admission, entropy and atomic state changes.
#include "bounded_decimal_editor121.h"
#include "ticket_font142.h"
enum class CreditsPanel201:uint8_t {Credits,GameValues,CounterEditor,JackpotPicker,MoneyEditor,CreditVoucher};
CreditsPanel201 creditsPanel201=CreditsPanel201::Credits;
unsigned valuesItem201=0,jackpotItem201=0,editFocus201=0;
uint64_t editedValue201=0;
unsigned voucherFirstDigit142(){unsigned first=0;uint64_t place=100000000;while(first<6&&editedValue201<place){++first;place/=10;}return first;}
const char* const jackpotNames201[]={"MINI","MINOR","MAJOR","GRAND"};
void enterRandomFault125();
bool valuesAvailable201(){return clearBalanceAvailable()&&engine.canEditManualSettings()&&!autoPlay&&!cabinetMenuRequested;}
bool editorMoney201(){return creditsPanel201==CreditsPanel201::MoneyEditor;}
unsigned editorDigits201(){return creditsPanel201==CreditsPanel201::CreditVoucher?9:editorMoney201()?10:4;}
void valuesNotice201(const char* message){snprintf(statusLine,sizeof(statusLine),"%s",message);dirty=true;}
void valuesFinished201(){
 ++runtimeRevision;displayMeter=engine.state.meter;displayQueuedHolds=engine.state.pendingMeterHolds;
 autoPlay=false;awaitingBonusResume=false;view=View::Game;statusLine[0]=0;
 creditsPanel201=CreditsPanel201::Credits;dirty=true;
}
void valuesBack201(){
 if(creditsPanel201==CreditsPanel201::CreditVoucher){creditsPanel201=CreditsPanel201::Credits;creditItem=0;if(voucherFromGame160){voucherFromGame160=false;view=View::Game;}}
 else if(creditsPanel201==CreditsPanel201::MoneyEditor)creditsPanel201=CreditsPanel201::JackpotPicker;
 else if(creditsPanel201==CreditsPanel201::CounterEditor||creditsPanel201==CreditsPanel201::JackpotPicker)creditsPanel201=CreditsPanel201::GameValues;
 else {creditsPanel201=CreditsPanel201::Credits;menuItem=4;view=View::Menu;}
 statusLine[0]=0;dirty=true;
}
bool openValueEditor201(bool moneyEditor){
 if(!valuesAvailable201()){valuesNotice201("FINISH CURRENT PLAY FIRST");return false;}
 int64_t value=engine.state.meter;
 if(moneyEditor&&(!engine.jackpotAmountMinor(jackpotItem201,value)||value<1||value>phoenix::Engine::maximumCustomJackpotMinor)){
  valuesNotice201("AMOUNT OUTSIDE EDIT RANGE");return false;
 }
 editedValue201=uint64_t(value);editFocus201=0;
 creditsPanel201=moneyEditor?CreditsPanel201::MoneyEditor:CreditsPanel201::CounterEditor;
 statusLine[0]=0;dirty=true;return true;
}
void applyValueEditor201(){
 if(!valuesAvailable201()){valuesNotice201("FINISH CURRENT PLAY FIRST");return;}
 const bool isMoney=editorMoney201();
 if((isMoney&&(editedValue201<1||editedValue201>uint64_t(phoenix::Engine::maximumCustomJackpotMinor)))||(!isMoney&&editedValue201>=phoenix::local::meterThresholdMaximum)){
  valuesNotice201("VALUE OUT OF RANGE");return;
 }
 const bool ok=isMoney?engine.setCustomJackpotMinor(jackpotItem201,int64_t(editedValue201)):engine.setPracticeMeter(uint16_t(editedValue201));
 if(!ok){if(!engine.randomHealthy())enterRandomFault125();else valuesNotice201("VALUE CHANGE UNAVAILABLE");return;}
 valuesFinished201();
}
void restoreJackpots201(){
 if(!valuesAvailable201()){valuesNotice201("FINISH CURRENT PLAY FIRST");return;}
 const auto requested=effectiveMathChoice();const static_grand::Entry* row=nullptr;
 // Resolve before clearing anything. With the same closed/healthy context,
 // selecting this row after clear is a checked, nonrandom cache assignment.
 if(static_grand::resolve(*mathFamily,requested,static_grand::profile(engine),row)!=static_grand::Result::Applied){valuesNotice201("DEFAULTS UNAVAILABLE");return;}
 if(!engine.clearCustomJackpots()){valuesNotice201("VALUE CHANGE UNAVAILABLE");return;}
 auto active=requested;
 const auto result=static_grand::selectFuture(engine,active,requested,*mathFamily,valuesAvailable201());
 if(result!=static_grand::Result::Applied&&result!=static_grand::Result::NoChange){enterService();return;}
 valuesFinished201();
}
void valuesSelect201(){
 using Page=CreditsPanel201;
 if(creditsPanel201==Page::CreditVoucher){
  if(editFocus201==9){
   if(!editedValue201){valuesNotice201("ENTER AN AMOUNT ABOVE ZERO");return;}
   if(editedValue201>999999999||!uiAddCredits(uint32_t(editedValue201))){valuesNotice201("CREDIT ADDITION UNAVAILABLE");return;}
   creditsPanel201=Page::Credits;voucherFromGame160=false;view=View::Game;autoPlay=false;statusLine[0]=0;
  }else if(editFocus201==10)valuesBack201();
  else{uint64_t place=1;for(unsigned i=editFocus201+1;i<9;++i)place*=10;
   const unsigned digit=unsigned(editedValue201/place%10);
   editedValue201=editedValue201-uint64_t(digit)*place+uint64_t((digit+1)%10)*place;statusLine[0]=0;}
 }else if(creditsPanel201==Page::GameValues){
  if(valuesItem201==3){valuesBack201();return;}
  if(!valuesAvailable201()){valuesNotice201("FINISH CURRENT PLAY FIRST");return;}
  if(valuesItem201==0)openValueEditor201(false);
  else if(valuesItem201==1){jackpotItem201=0;creditsPanel201=Page::JackpotPicker;statusLine[0]=0;}
  else restoreJackpots201();
 }else if(creditsPanel201==Page::JackpotPicker){
  if(jackpotItem201==4)valuesBack201();else openValueEditor201(true);
 }else{
  const unsigned digits=editorDigits201();
  if(editFocus201==digits)applyValueEditor201();
  else if(editFocus201==digits+1)valuesBack201();
  else if(!editorMoney201()){
   editedValue201=bounded_decimal_editor::next(editedValue201,editFocus201,digits,phoenix::local::meterThresholdMaximum-1);
   statusLine[0]=0;
  }
  else {uint64_t place=1;for(unsigned i=editFocus201+1;i<digits;++i)place*=10;
   const unsigned old=unsigned(editedValue201/place%10);
   editedValue201=editedValue201-uint64_t(old)*place+uint64_t((old+1)%10)*place;statusLine[0]=0;}
 }
 dirty=true;
}
void valuesNext201(){
 if(creditsPanel201==CreditsPanel201::GameValues)valuesItem201=(valuesItem201+1)%4;
 else if(creditsPanel201==CreditsPanel201::JackpotPicker)jackpotItem201=(jackpotItem201+1)%5;
 else editFocus201=(editFocus201+1)%(editorDigits201()+2);
 dirty=true;
}
int ticketWidth142(const char* value,unsigned size){int w=0;for(const unsigned char* p=(const unsigned char*)value;*p;++p)w+=ticket_font142::glyph(size,*p).advance;return w;}
void ticketText142(const char* value,int x,int y,unsigned size,uint16_t color,int align=0){if(align==1)x-=ticketWidth142(value,size)/2;else if(align==2)x-=ticketWidth142(value,size);const auto* alpha=ticket_font142::pixels(size);for(const unsigned char* p=(const unsigned char*)value;*p;++p){const auto& g=ticket_font142::glyph(size,*p);for(unsigned r=0;r<g.h;++r)for(unsigned c=0;c<g.w;++c)if(auto opacity=alpha[g.offset+r*g.w+c])dot(x+int(c),y+int(r),color,opacity);x+=g.advance;}}
void ticketEdge142(bool right,uint16_t color){const char* caption="INSERT THIS SIDE UP";int pen=0;const int width=ticketWidth142(caption,6);const auto* alpha=ticket_font142::pixels(6);for(const unsigned char* p=(const unsigned char*)caption;*p;++p){const auto& g=ticket_font142::glyph(6,*p);for(unsigned r=0;r<g.h;++r)for(unsigned c=0;c<g.w;++c)if(auto opacity=alpha[g.offset+r*g.w+c])dot(right?227-int(r):12+int(r),right?62-width/2+pen+int(c):62+width/2-pen-int(c),color,opacity);pen+=g.advance;}}
std::string ticketInteger142(uint32_t n){
 static const char* small[]={"ZERO","ONE","TWO","THREE","FOUR","FIVE","SIX","SEVEN","EIGHT","NINE","TEN","ELEVEN","TWELVE","THIRTEEN","FOURTEEN","FIFTEEN","SIXTEEN","SEVENTEEN","EIGHTEEN","NINETEEN"};
 static const char* tens[]={"","","TWENTY","THIRTY","FORTY","FIFTY","SIXTY","SEVENTY","EIGHTY","NINETY"};
 if(n<20)return small[n];if(n<100)return std::string(tens[n/10])+(n%10?" "+ticketInteger142(n%10):"");
 const uint32_t unit=n>=1000000?1000000:n>=1000?1000:100;const char* label=unit==1000000?" MILLION":unit==1000?" THOUSAND":" HUNDRED";
 return ticketInteger142(n/unit)+label+(n%unit?" "+ticketInteger142(n%unit):"");
}
std::string ticketAmountWords142(uint64_t minor){const uint32_t dollars=uint32_t(minor/100),cents=uint32_t(minor%100);return ticketInteger142(dollars)+(dollars==1?" DOLLAR":" DOLLARS")+(cents?" AND "+ticketInteger142(cents)+(cents==1?" CENT":" CENTS"):" EXACTLY");}
uint32_t voucherOpened142=0,voucherSequence142=1;
char voucherCode143[19]="730148529674183026";
void openCreditVoucher143(bool fromGame=false){
 voucherFromGame160=fromGame;editedValue201=0;voucherOpened142=uiWallMillis();
 // Cosmetic identifiers use only already-public display information. They
 // never draw from, reveal, reseed or otherwise alter the outcome generator.
 uint32_t seed=0x8d2537abu^voucherOpened142^uint32_t(++voucherSequence142*0x9e3779b9u)^uint32_t(engine.state.meter);
 for(const auto& cell:visible)seed=(seed^uint32_t(cell.symbol))*16777619u;
 if(!seed)seed=1;
 for(unsigned i=0;i<18;++i)voucherCode143[i]=char('0'+cosmetic(seed)%10);
 voucherCode143[0]=char('1'+cosmetic(seed)%9);voucherCode143[18]=0;
 editFocus201=3;creditsPanel201=CreditsPanel201::CreditVoucher;statusLine[0]=0;
}
void drawCreditVoucher142(){
 shade();constexpr uint16_t paper=0xffde,ink=0x0000;
 fill(7,7,226,109,paper);rect(7,7,226,109,0xd6ba);
 ticketEdge142(false,ink);ticketEdge142(true,ink);
 ticketText142("STARDUST",120,12,11,ink,1);
 ticketText142("RESORT & CASINO - LAS VEGAS",120,23,6,ink,1);
 ticketText142("CASHOUT VOUCHER",120,31,12,ink,1);
 // Decorative stripes with no casino validation payload or RNG use.
 const char* patterns[]={"nnwwn","wnnnw","nwnnw","wwnnn","nnwnw","wnwnn","nwwnn","nnnww","wnnwn","nwnwn"};
 const char* code=voucherCode143;
 unsigned runs[97]={1,1,1,1};unsigned runCount=4;
 for(unsigned pair=0;pair<18;pair+=2)for(unsigned k=0;k<5;++k){runs[runCount++]=patterns[code[pair]-'0'][k]=='w'?3:1;runs[runCount++]=patterns[code[pair+1]-'0'][k]=='w'?3:1;}
 runs[runCount++]=3;runs[runCount++]=1;runs[runCount++]=1;
 unsigned total=0;for(auto run:runs)total+=run;unsigned at=0;
 for(unsigned i=0;i<runCount;++i){const int left=64+int(at*112/total);at+=runs[i];const int right=64+int(at*112/total);if(!(i%2)&&right>left)fill(left,43,right-left,21,ink);}
 char validation[48],meta[64];snprintf(validation,sizeof(validation),"VALIDATION %.2s-%.4s-%.4s-%.4s-%.4s",voucherCode143,voucherCode143+2,voucherCode143+6,voucherCode143+10,voucherCode143+14);
 ticketText142(validation,120,65,5,ink,1);
 const char* side=validation+11;int pen=0;const int sideWidth=ticketWidth142(side,5);const auto* sideAlpha=ticket_font142::pixels(5);
 for(const unsigned char* p=(const unsigned char*)side;*p;++p){const auto& g=ticket_font142::glyph(5,*p);for(unsigned r=0;r<g.h;++r)for(unsigned c=0;c<g.w;++c)if(auto opacity=sideAlpha[g.offset+r*g.w+c])dot(214-int(r),62-sideWidth/2+pen+int(c),ink,opacity);pen+=g.advance;}
 // Offline ticket uses the release date and elapsed session time, not a claim
 // of a synchronized casino clock. Identifiers are display-only.
 const uint32_t seconds=voucherOpened142/1000;
 snprintf(meta,sizeof(meta),"11/16/2026  %02u:%02u:%02u  TICKET # %04u",unsigned(seconds/3600%24),unsigned(seconds/60%60),unsigned(seconds%60),unsigned(strtoul(voucherCode143+14,nullptr,10)));ticketText142(meta,120,71,5,ink,1);
 const auto words=ticketAmountWords142(editedValue201);std::string first=words,second;
 if(ticketWidth142(words.c_str(),6)>192){size_t split=words.size()/2;auto at=words.find_last_of(' ',split);if(at==std::string::npos)at=split;first=words.substr(0,at);second=words.substr(at+1);}
 ticketText142(first.c_str(),120,76,6,ink,1);if(!second.empty())ticketText142(second.c_str(),120,82,6,ink,1);
 char digits[16];snprintf(digits,sizeof(digits),"%09llu",(unsigned long long)editedValue201);
 unsigned start=voucherFirstDigit142();if(editFocus201<start)start=editFocus201;
 std::string amount="$";int offsets[9]={};int width=ticketWidth142("$",18);
 for(unsigned i=start;i<9;++i){if(i==7){amount+='.';width+=ticketWidth142(".",18);}offsets[i]=width;const char glyph[2]={digits[i],0};amount+=digits[i];width+=ticketWidth142(glyph,18);if(i<6&&(6-i)%3==0){amount+=',';width+=ticketWidth142(",",18);}}
 const int left=(W-width)/2;ticketText142(amount.c_str(),left,88,18,ink);
 if(editFocus201<9){const char glyph[2]={digits[editFocus201],0};const int x=left+offsets[editFocus201],w=ticketWidth142(glyph,18);fill(x,87,w,20,0xe71c);ticketText142(glyph,x,88,18,ink);fill(x,107,w,1,ink);}
 ticketText142("Void after 180 days",31,110,5,ink);ticketText142("MACHINE # 0142",208,110,5,ink,2);
 fill(0,117,W,18,0x0923);
 if(editFocus201==9)rect(18,118,128,11,0xe585);if(editFocus201==10)rect(160,118,65,11,0xe585);
 text("INSERT CREDITS",82,120,8,editFocus201==9?BRIGHT:WHITE,1);text("CANCEL",192,120,8,editFocus201==10?BRIGHT:WHITE,1);
 ticketText142("GREY NEXT   BLUE +1   HOLD BLUE LEFT",120,130,5,WHITE,1);
 if(*statusLine){fill(8,74,224,12,paper);ticketText142(statusLine,120,77,6,ink,1);}
}
void drawValues201(){
 using Page=CreditsPanel201;
 if(creditsPanel201==Page::CreditVoucher){drawCreditVoucher142();return;}
 if(creditsPanel201==Page::GameValues){
  panel("COUNTER & JACKPOTS");
  const char* labels[]={"FEATURE COUNTER","JACKPOT AMOUNTS","RESTORE DEFAULT JACKPOTS","BACK TO MENU"};
  for(unsigned i=0;i<4;++i){const int y=45+int(i)*13;if(i==valuesItem201)fill(16,y-1,208,11,0x2a86);
   text(labels[i],23,y,8,i<3&&!valuesAvailable201()?0x7bef:i==valuesItem201?BRIGHT:WHITE);}
  text(engine.hasCustomJackpots()?"CUSTOM JACKPOTS; RETURN OPTIONS LOCKED":"CHANGES APPLY TO NORMAL PLAY",120,103,6,BRIGHT,1);
 }else if(creditsPanel201==Page::JackpotPicker){
  panel("JACKPOT AMOUNTS");
  for(unsigned i=0;i<5;++i){const int y=43+int(i)*12;if(i==jackpotItem201)fill(16,y-1,208,11,0x2a86);
   text(i<4?jackpotNames201[i]:"BACK",23,y,8,i==jackpotItem201?BRIGHT:WHITE);
   if(i<4){int64_t value=0;const auto label=engine.jackpotAmountMinor(i,value)?money(value):std::string("UNAVAILABLE");
    text(label.c_str(),214,y,8,i==jackpotItem201?BRIGHT:WHITE,2);}
  }
  text("AMOUNTS STAY FIXED ACROSS WAGERS",120,108,6,BRIGHT,1);
 }else{
  const bool isMoney=editorMoney201();char title[40],digits[16];
  if(isMoney)snprintf(title,sizeof(title),"SET %s AMOUNT",jackpotNames201[jackpotItem201]);else snprintf(title,sizeof(title),"SET FEATURE COUNTER");
  panel(title);const unsigned count=editorDigits201();snprintf(digits,sizeof(digits),"%0*llu",int(count),(unsigned long long)editedValue201);
  unsigned firstMoneyDigit=0;if(isMoney)while(firstMoneyDigit<7&&digits[firstMoneyDigit]=='0')++firstMoneyDigit;
  if(isMoney)cashText("$",38,56,20,BRIGHT);
  for(unsigned i=0;i<count;++i){const int x=isMoney?44+int(i)*14+(i>=8?8:0):88+int(i)*16;
   if(i==editFocus201)fill(x-1,54,14,24,0x2a86);char glyph[2]={digits[i],0};cashText(glyph,x+cashWidth(glyph,20),56,20,i==editFocus201?BRIGHT:isMoney&&i<firstMoneyDigit?0x7bef:WHITE);}
  if(isMoney)cashText(".",159,56,20,WHITE);
  text(isMoney?"$0.01 TO $30,000,000.00":"0 TO 1886; ACTIVE WAGER COUNTER",120,84,7,WHITE,1);
  const unsigned focus=editFocus201;
  if(focus==count)fill(28,96,78,12,0x2a86);if(focus==count+1)fill(134,96,78,12,0x2a86);
  text("APPLY",67,98,8,focus==count?BRIGHT:WHITE,1);text("CANCEL",173,98,8,focus==count+1?BRIGHT:WHITE,1);
 }
 if(*statusLine)text(statusLine,120,114,6,BRIGHT,1);
 text(creditsPanel201==Page::GameValues||creditsPanel201==Page::JackpotPicker?"GREY: NEXT  BLUE: SELECT  HOLD GREY: BACK":"GREY: NEXT  BLUE: CHANGE  HOLD GREY: BACK",120,122,6,WHITE,1);
}
