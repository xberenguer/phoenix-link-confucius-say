// Capture actual pre-input rest and an entire natural paid spin/feature.
// The original recorder stays byte-identical for existing published receipts.
#define main original_capture_gameplay_main
#include "capture_gameplay.cpp"
#undef main

int main(int argc,char**argv){
 if(argc<2){std::fprintf(stderr,"usage: capture-loop-gameplay OUTPUT [PAID_SPINS=30]\n");return 2;}
 const unsigned paidSpins=argc>2?unsigned(std::strtoul(argv[2],nullptr,10)):30;
 Recorder record(argv[1]);phoenix::random::Service random;
 if(!random.initialize(freshEntropy,nullptr)||!uiInitFresh(random))return 3;
 uiSetOutcomeCheckpointHandler(owner);audioInit(false);uiPrepareEffects();uiRedraw();record.tick();record.log("fresh-start");record.still("startup-denomination");
 uiButtonA();record.tick();record.log("blue-select-denomination");
 for(unsigned i=0;i<30&&field(uiSnapshot(),"view")!="game";++i){
  uiButtonA();record.tick();record.log("blue-select-option");
  if(field(uiSnapshot(),"view")=="game")break;
  uiButtonB();record.tick();record.log("grey-next-option");
 }
 if(field(uiSnapshot(),"view")!="game")return 4;
 uiButtonA();record.tick();record.log("blue-open-credit-voucher");
 uiButtonA();record.tick();record.log("blue-increase-voucher-digit-to-1000-dollars");
 for(unsigned i=0;i<6;++i){uiButtonB();record.tick();record.log("grey-next-voucher-field");}
 uiButtonA();record.tick();record.log("blue-insert-play-money-voucher");record.still("funded-reels");
 if(number(uiSnapshot(),"balanceMinor")==0||field(uiSnapshot(),"view")!="game")return 5;
 unsigned ordinaryWins=0;
 for(unsigned spin=0;spin<paidSpins&&uiRandomHealthy();++spin){
  if(!uiReady()||field(uiSnapshot(),"mode")!="base")return 6;
  if(number(uiSnapshot(),"balanceMinor")<10000){
   uiMenu();record.tick();record.log("hold-grey-open-balance-menu");
   uiButtonA();record.tick();record.log("blue-open-balance");
   uiButtonB();record.tick();record.log("grey-select-first-credit-preset");
   uiButtonA();record.tick();record.log("blue-add-play-money-credit-preset");
  }
  record.log("pre-spin-rest-capture-anchor");
  record.start({"whole-paid-spin","pre-input-rest","continuous-40ms-frames"});
  for(unsigned frame=0;frame<25;++frame)record.tick();
  const auto before=uiActionCount();
  uiButtonA();record.log("blue-spin");
  uint32_t phaseAt=clockMs;std::string lastPhase;bool feature=false;
  while(uiRandomHealthy()){
   record.tick();
   const auto snapshot=std::string(uiSnapshot());
   const auto phase=field(snapshot,"animation");
   if(phase!=lastPhase){phaseAt=clockMs;lastPhase=phase;}
   feature|=field(snapshot,"mode")!="base";
   if((phase=="free-intro"||phase=="hold-intro")&&clockMs-phaseAt>9000){
    uiButtonA();record.log("blue-resume-earned-feature");
   }
   if(uiReady()&&field(snapshot,"mode")=="base")break;
   if(clockMs-record.activeAt>6000000){std::fprintf(stderr,"Natural round exceeded capture watchdog\n");return 7;}
  }
  if(!uiRandomHealthy())return 8;
  const auto result=std::string(uiSnapshot());
  if(!feature&&uiActionCount()==before+1){
   record.labels.push_back("ordinary-paid-spin");
   if(number(result,"winMinor")>0){record.labels.push_back("natural-ordinary-winning-spin");++ordinaryWins;}
  }else record.labels.push_back("complete-natural-feature-round");
  // Keep the real running result presentation, rather than freezing a frame.
  for(unsigned frame=0;frame<163;++frame)record.tick();
  record.labels.push_back("completed-base-ready-result");record.finish();
  record.log("complete-paid-round-recorded");
  std::fprintf(stderr,"paid_spins=%u engine_actions=%llu ordinary_wins=%u\n",spin+1,(unsigned long long)uiActionCount(),ordinaryWins);
 }
 record.still("last-real-reel-board");record.log("recording-stop-at-base-ready-boundary");
 std::ofstream summary(record.out/"summary.json");
 summary<<"{\"source\":\"unchanged production C++ UI, Engine and CSPRNG\",\"entropy\":\"fresh OS arc4random_buf entropy per production callback; no seed or replay\",\"time\":\"virtual monotonic ms; every captured frame is 40ms after the preceding frame\",\"audio\":\"silent native output adapter; registered short-cue duration tracking; native music backend unavailable\",\"paid_spins\":"<<paidSpins<<",\"actions\":"<<uiActionCount()<<",\"ordinary_wins\":"<<ordinaryWins<<",\"clips\":"<<record.clips<<",\"rng_healthy\":"<<(uiRandomHealthy()?"true":"false")<<"}\n";
 return uiRandomHealthy()?0:9;
}
