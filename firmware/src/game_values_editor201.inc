// Live-session value editing. No alternate game, outcome fixture or HUD mode.
// Engine setters own admission, entropy and atomic state changes.
enum class CreditsPanel201:uint8_t {Credits,GameValues,CounterEditor,JackpotPicker,MoneyEditor};
CreditsPanel201 creditsPanel201=CreditsPanel201::Credits;
unsigned valuesItem201=0,jackpotItem201=0,editFocus201=0;
uint64_t editedValue201=0;
const char* const jackpotNames201[]={"MINI","MINOR","MAJOR","GRAND"};
void enterRandomFault125();
bool valuesAvailable201(){return clearBalanceAvailable()&&engine.canEditManualSettings()&&!autoPlay&&!cabinetMenuRequested;}
bool editorMoney201(){return creditsPanel201==CreditsPanel201::MoneyEditor;}
unsigned editorDigits201(){return editorMoney201()?10:4;}
void valuesNotice201(const char* message){snprintf(statusLine,sizeof(statusLine),"%s",message);dirty=true;}
void valuesFinished201(){
 ++runtimeRevision;displayMeter=engine.state.meter;displayQueuedHolds=engine.state.pendingMeterHolds;
 autoPlay=false;awaitingBonusResume=false;view=View::Game;statusLine[0]=0;
 creditsPanel201=CreditsPanel201::Credits;dirty=true;
}
void valuesBack201(){
 if(creditsPanel201==CreditsPanel201::MoneyEditor)creditsPanel201=CreditsPanel201::JackpotPicker;
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
 if((isMoney&&(editedValue201<1||editedValue201>uint64_t(phoenix::Engine::maximumCustomJackpotMinor)))||(!isMoney&&editedValue201>1886)){
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
 if(creditsPanel201==Page::GameValues){
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
void drawValues201(){
 using Page=CreditsPanel201;
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
