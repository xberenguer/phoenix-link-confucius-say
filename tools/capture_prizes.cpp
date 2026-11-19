// Observational supplement: play the unchanged game until each named-prize
// label is actually visible. Templates classify existing framebuffer pixels;
// they are NEVER drawn, composited, injected, or assigned to game cells.
#define main capture_gameplay_original_main
#include "capture_gameplay.cpp"
#undef main
#include "pdf_assets.h"

struct BadgeSample {unsigned x,y;uint16_t rgb;};
struct BadgeTemplate {unsigned tier,multiplier;std::vector<BadgeSample> pixels;};
static std::array<BadgeTemplate,9> badgeTemplates(){
 std::array<BadgeTemplate,9> templates{};
 for(unsigned kind=1;kind<=3;++kind)for(unsigned multiplier=1;multiplier<=3;++multiplier){
  auto& target=templates[(kind-1)*3+multiplier-1];target.tier=kind;target.multiplier=multiplier;
  const auto& asset=pdfa::prize(kind,multiplier);
  for(unsigned y=3;y<32;++y)for(unsigned x=4;x<43;++x){const unsigned at=y*47+x;if(asset.alpha[at]==255&&asset.image.rgb[at])target.pixels.push_back({x,y,asset.image.rgb[at]});}
  if(target.pixels.size()<20)std::abort();
 }
 return templates;
}

int main(int argc,char**argv){
 if(argc<2){std::fprintf(stderr,"usage: capture-prizes OUTPUT [MAX_ACTIONS=1000000]\n");return 2;}
 const uint64_t limit=argc>2?std::strtoull(argv[2],nullptr,10):1000000;
 Recorder record(argv[1]);std::ofstream observations(record.out/"prize-observations.ndjson");
 const auto templates=badgeTemplates();std::set<std::pair<unsigned,unsigned>> found;
 phoenix::random::Service random;
 if(!random.initialize(freshEntropy,nullptr)||!uiInitFresh(random))return 3;
 uiSetOutcomeCheckpointHandler(owner);audioInit(false);uiPrepareEffects();uiRedraw();record.tick();record.log("fresh-start");
 uiButtonA();record.tick();record.log("blue-select-denomination");
 for(unsigned i=0;i<30&&field(uiSnapshot(),"view")!="game";++i){uiButtonA();record.tick();record.log("blue-select-option");if(field(uiSnapshot(),"view")=="game")break;uiButtonB();record.tick();record.log("grey-next-option");}
 if(field(uiSnapshot(),"view")!="game")return 4;
 uiButtonA();record.tick();record.log("blue-open-credit-voucher");uiButtonA();record.tick();record.log("blue-increase-voucher-digit-to-1000-dollars");
 for(unsigned i=0;i<6;++i){uiButtonB();record.tick();record.log("grey-next-voucher-field");}uiButtonA();record.tick();record.log("blue-insert-play-money-voucher");
 uint64_t lastAction=uiActionCount();uint32_t watchdog=clockMs,promptAt=0;std::string lastPhase;
 auto observe=[&](){
  const std::string snapshot=uiSnapshot();if(field(snapshot,"view")!="game")return;
  const auto phase=field(snapshot,"animation");if(phase=="spinning"||phase=="hold-fire"||phase=="return-fire")return;
  const uint16_t*pixels=uiFramePixels();
  for(unsigned cell=0;cell<15;++cell)for(const auto& badge:templates){
   if(found.count({badge.tier,badge.multiplier}))continue;
   const unsigned x=2+(cell/3)*47,y=16+(cell%3)*35;bool match=true;
   for(const auto& pixel:badge.pixels)if(pixels[(y+pixel.y)*240+x+pixel.x]!=pixel.rgb){match=false;break;}
   if(!match)continue;
   found.insert({badge.tier,badge.multiplier});const char*tier=badge.tier==1?"mini":badge.tier==2?"minor":"major";
   const std::string tag=std::string("prize-")+tier+"-"+std::to_string(badge.multiplier)+"x";
   record.seen.insert(tag);record.still(tag);
   if(!record.capture)record.start({tag});else record.labels.push_back(tag);
   observations<<"{\"id\":"<<quoted(tag)<<",\"action_index\":"<<record.inputIndex<<",\"engine_action_count\":"<<uiActionCount()<<",\"time_ms\":"<<clockMs<<",\"cell_index\":"<<cell<<",\"tier\":"<<badge.tier<<",\"multiplier\":"<<badge.multiplier<<",\"matched_opaque_pixels\":"<<badge.pixels.size()<<",\"crop\":["<<x<<','<<y<<",47,35],\"snapshot\":"<<snapshot<<"}\n";observations.flush();
   std::fprintf(stderr,"FOUND %s action=%llu cell=%u matched=%zu\n",tag.c_str(),(unsigned long long)uiActionCount(),cell,badge.pixels.size());
  }
 };
 while(uiActionCount()<limit&&uiRandomHealthy()&&found.size()<9){
  const auto phase=field(uiSnapshot(),"animation");if(phase!=lastPhase){lastPhase=phase;promptAt=clockMs;}
  if((phase=="free-intro"||phase=="hold-intro")&&clockMs-promptAt>9000){uiButtonA();record.log("blue-resume-earned-feature");}
  if(uiReady()){
   if(record.capture){if(!record.readyAt)record.readyAt=clockMs;if(clockMs-record.readyAt<6500){record.tick();observe();continue;}record.finish();}
   if(number(uiSnapshot(),"balanceMinor")<10000&&field(uiSnapshot(),"mode")=="base"){
    uiMenu();record.tick();record.log("hold-grey-open-balance-menu");uiButtonA();record.tick();record.log("blue-open-balance");uiButtonB();record.tick();record.log("grey-select-first-credit-preset");uiButtonA();record.tick();record.log("blue-add-play-money-credit-preset");
   }
   uiButtonA();record.log("blue-spin-or-bonus-resume");
  }
  record.tick(record.capture?40:1000);observe();
  if(uiActionCount()!=lastAction){lastAction=uiActionCount();watchdog=clockMs;if(lastAction%1000==0)std::fprintf(stderr,"actions=%llu variants=%zu\n",(unsigned long long)lastAction,found.size());}
  if(clockMs-watchdog>600000){std::fprintf(stderr,"No action for 10 simulated minutes\n");return 6;}
 }
 // Finish the real in-flight action/feature; never reset or restore a state.
 while(!uiReady()&&uiRandomHealthy()){
  const auto phase=field(uiSnapshot(),"animation");if(phase=="free-intro"||phase=="hold-intro"){uiButtonA();record.log("blue-resume-earned-feature-at-session-end");}record.tick(record.capture?40:1000);observe();
 }
 if(record.capture)for(unsigned i=0;i<163;++i)record.tick();record.finish();record.log("recording-stop-at-ready-boundary");
 std::ofstream summary(record.out/"summary.json");summary<<"{\"kind\":\"actual-gameplay-prize-observer\",\"source\":\"unchanged production UI, Engine and CSPRNG\",\"capture_fps\":25,\"ordinary_tick_ms\":1000,\"template_scope\":\"read-only exact opaque pixel match, no template rendered into footage\",\"actions\":"<<uiActionCount()<<",\"clips\":"<<record.clips<<",\"variants\":"<<found.size()<<",\"rng_healthy\":"<<(uiRandomHealthy()?"true":"false")<<"}\n";
 std::fprintf(stderr,"Complete: %llu actual actions, %zu actual named variants\n",(unsigned long long)uiActionCount(),found.size());return uiRandomHealthy()?0:7;
}
