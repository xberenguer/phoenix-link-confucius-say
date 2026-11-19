// Observe a real nonwinning board long enough for complete HUD cycles.
// The original recorder stays byte-identical for existing published receipts.
#define main original_capture_gameplay_main
#include "capture_gameplay.cpp"
#undef main

int main(int argc,char**argv){
 if(argc<2){std::fprintf(stderr,"usage: capture-hud-gameplay OUTPUT [MAX_PAID_SPINS=100]\n");return 2;}
 const unsigned limit=argc>2?unsigned(std::strtoul(argv[2],nullptr,10)):100;
 Recorder record(argv[1]);phoenix::random::Service random;
 if(!random.initialize(freshEntropy,nullptr)||!uiInitFresh(random))return 3;
 uiSetOutcomeCheckpointHandler(owner);audioInit(false);uiPrepareEffects();uiRedraw();record.tick();record.log("fresh-start");
 uiButtonA();record.tick();record.log("blue-select-denomination");
 for(unsigned i=0;i<30&&field(uiSnapshot(),"view")!="game";++i){
  uiButtonA();record.tick();record.log("blue-select-option");if(field(uiSnapshot(),"view")=="game")break;
  uiButtonB();record.tick();record.log("grey-next-option");
 }
 if(field(uiSnapshot(),"view")!="game")return 4;
 uiButtonA();record.tick();record.log("blue-open-credit-voucher");uiButtonA();record.tick();record.log("blue-increase-voucher-digit-to-1000-dollars");
 for(unsigned i=0;i<6;++i){uiButtonB();record.tick();record.log("grey-next-voucher-field");}
 uiButtonA();record.tick();record.log("blue-insert-play-money-voucher");
 if(number(uiSnapshot(),"balanceMinor")==0||field(uiSnapshot(),"view")!="game")return 5;
 unsigned paidSpins=0;bool found=false;
 while(paidSpins<limit&&uiRandomHealthy()){
  const auto before=uiActionCount();
  uiButtonA();record.log("blue-spin");++paidSpins;
  uint32_t phaseAt=clockMs,roundAt=clockMs;std::string lastPhase;bool feature=false;
  while(uiRandomHealthy()){
   record.tick(200);const auto snapshot=std::string(uiSnapshot());const auto phase=field(snapshot,"animation");
   if(phase!=lastPhase){phaseAt=clockMs;lastPhase=phase;}
   feature|=field(snapshot,"mode")!="base";
   if((phase=="free-intro"||phase=="hold-intro")&&clockMs-phaseAt>9000){uiButtonA();record.log("blue-resume-earned-feature");}
   if(uiReady()&&field(snapshot,"mode")=="base")break;
   if(clockMs-roundAt>6000000)return 6;
  }
  if(!uiRandomHealthy())return 7;
  const auto snapshot=std::string(uiSnapshot());
  if(!feature&&uiActionCount()==before+1&&number(snapshot,"winMinor")==0){found=true;break;}
  record.log("complete-natural-paid-round");
 }
 if(!found){std::fprintf(stderr,"No ordinary nonwinning board occurred within requested play limit\n");return 8;}
 const auto action=uiActionCount();record.log("stationary-nonwinning-ready-capture-anchor");
 record.still("hud-ready-poster");record.start({"ordinary-nonwinning-ready","long-stationary-hud-cycle","continuous-40ms-frames"});
 for(unsigned frame=0;frame<3110;++frame){record.tick();if(uiActionCount()!=action||!uiReady()||field(uiSnapshot(),"mode")!="base")return 9;}
 record.finish();record.log("recording-stop-at-base-ready-boundary");
 std::ofstream summary(record.out/"summary.json");
 summary<<"{\"source\":\"unchanged production C++ UI, Engine and CSPRNG\",\"entropy\":\"fresh OS arc4random_buf entropy per production callback; no seed or replay\",\"paid_spins\":"<<paidSpins<<",\"actions\":"<<uiActionCount()<<",\"clips\":1,\"frames\":3110,\"frame_ms\":40,\"duration_ms\":124400,\"selection\":\"First naturally reached ordinary Base-ready board with winMinor0\",\"rng_healthy\":"<<(uiRandomHealthy()?"true":"false")<<"}\n";
 return uiRandomHealthy()?0:10;
}
