// Last settled reel display belongs to the denomination, independently of
// the wager-specific Phoenix counter. This cache owns presentation only:
// no Engine state, outcome RNG, pending feature or paid result is restored.
struct DenominationScene144 {
 bool valid=false,hold=false,spinAwardFinished=false;
 int64_t denomination=100,win=0;uint8_t lines=5;
 uint32_t elapsed=0;uint16_t phoenixPhase[15]{};
 Cell cells[15]{},backdrop[15]{};
 phoenix::Presentation result{};
};
DenominationScene144* denominationScenes144=nullptr;
Cell initialDisplay144[15]{};
int archivedScene144=-1;bool boardContextPinned144=false;
int64_t boardDenomination144=100;uint8_t boardLines144=5;
void copy(Cell*,const Cell*);
uint32_t cosmetic(uint32_t&);
uint32_t phoenixClock(unsigned,uint32_t);
unsigned denominationSceneIndex144(int64_t denomination){
 for(unsigned i=0;i<phoenix::local::denominationCount;++i)if(phoenix::local::denominations[i]==denomination)return i;
 return phoenix::local::denominationCount;
}
const phoenix::Presentation& boardPresentation144(){
 return phase==Phase::Ready&&archivedScene144>=0&&denominationScenes144?
  denominationScenes144[archivedScene144].result:engine.presentation;
}
int64_t boardCoin144(){return phase==Phase::Ready&&boardContextPinned144?boardDenomination144:engine.state.coinValue;}
uint8_t boardLineCount144(){return phase==Phase::Ready&&boardContextPinned144?boardLines144:engine.state.lineCount;}
void initializeDenominationScenes144(){
 if(!denominationScenes144){
#ifdef UI_HOST_TEST
  void* block=std::malloc(sizeof(DenominationScene144)*phoenix::local::denominationCount);
#else
  void* block=ps_malloc(sizeof(DenominationScene144)*phoenix::local::denominationCount);
#endif
  if(block){denominationScenes144=static_cast<DenominationScene144*>(block);for(unsigned i=0;i<phoenix::local::denominationCount;++i)new(&denominationScenes144[i])DenominationScene144;}
 }
 if(denominationScenes144)for(unsigned i=0;i<phoenix::local::denominationCount;++i)denominationScenes144[i].valid=false;
 copy(initialDisplay144,visible);archivedScene144=-1;boardContextPinned144=false;
 boardDenomination144=engine.state.coinValue;boardLines144=engine.state.lineCount;
}
bool rememberDenominationScene144(){
 const unsigned index=denominationSceneIndex144(engine.state.coinValue);
 if(!denominationScenes144||index>=phoenix::local::denominationCount)return false;
 auto& scene=denominationScenes144[index];
 // Read the current source before assigning the same cached scene on a
 // later visit; same-object Presentation assignment is intentional.
 scene.result=boardPresentation144();scene.denomination=boardCoin144();scene.lines=boardLineCount144();
 scene.hold=holdView;scene.win=displayWin;scene.spinAwardFinished=spinAward.finished;
 scene.elapsed=millis()-(lineClockValid?lineDisplayAt:readyAt);
 copy(scene.cells,visible);copy(scene.backdrop,holdBase);
 for(unsigned i=0;i<15;++i)scene.phoenixPhase[i]=uint16_t(phoenixClock(i,millis()));
 scene.valid=true;return true;
}
void initializeUnplayedDenomination144(DenominationScene144& scene){
 // Fresh sessions have no imported cabinet history. Rearrange the already
 // public, unpaid opening board once for an unvisited denomination. This
 // cosmetic layout is never an Engine result, paid win or future spin.
 uint32_t seed=2166136261u^uint32_t(engine.state.coinValue);
 for(const auto& c:initialDisplay144)seed=(seed^uint32_t(c.symbol))*16777619u;
 if(!seed)seed=1;
 const unsigned shift=cosmetic(seed)%4;
 for(unsigned c=0;c<5;++c){const unsigned from=c?1+(c-1+shift)%4:0,roll=cosmetic(seed)%3;
  for(unsigned r=0;r<3;++r){Cell cell=initialDisplay144[from*3+(r+roll)%3];cell.highlight=false;scene.cells[c*3+r]=cell;scene.backdrop[c*3+r]=cell;}
 }
 scene.result=phoenix::Presentation{};scene.denomination=engine.state.coinValue;scene.lines=engine.state.lineCount;
 scene.hold=scene.spinAwardFinished=false;scene.win=0;scene.elapsed=0;
 for(unsigned i=0;i<15;++i)scene.phoenixPhase[i]=uint16_t(cosmetic(seed)%2500);
 scene.valid=true;
}
void restoreDenominationScene144(int64_t previousDenomination){
 if(previousDenomination==engine.state.coinValue){boardContextPinned144=true;return;}
 const unsigned index=denominationSceneIndex144(engine.state.coinValue);
 if(!denominationScenes144||index>=phoenix::local::denominationCount)return;
 auto& scene=denominationScenes144[index];if(!scene.valid)initializeUnplayedDenomination144(scene);
 copy(visible,scene.cells);copy(target,visible);copy(previous,visible);copy(holdBase,scene.backdrop);
 archivedScene144=int(index);boardContextPinned144=true;boardDenomination144=scene.denomination;boardLines144=scene.lines;
 holdView=scene.hold;holdBackdropCaptured=false;spinAward.finished=scene.spinAwardFinished;
 displayWin=winFrom=winTo=scene.win;displayBank=engine.state.balance;
 readyAt=millis()-scene.elapsed;lineDisplayAt=readyAt;lineClockValid=true;
 for(unsigned i=0;i<15;++i)phoenixPhases[i]=scene.phoenixPhase[i];phoenixClockAt=millis();
}
void beginLiveBoard144(){archivedScene144=-1;boardContextPinned144=false;boardDenomination144=engine.state.coinValue;boardLines144=engine.state.lineCount;}
