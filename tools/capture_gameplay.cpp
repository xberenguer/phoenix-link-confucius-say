// Real-play gallery recorder. No fixture, state restoration, test RNG, or
// outcome injection. Only the normal public UI API admits player actions.
#include <M5Unified.h>
#include "ui.h"
#include "audio.h"
#include "phoenix_random.h"
#include <CommonCrypto/CommonDigest.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs=std::filesystem;
static uint32_t clockMs=1000;
static std::array<uint64_t,8> audioUntil{};
uint32_t millis(){return clockMs;}
uint32_t micros(){return uint32_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
void delay(uint32_t value){clockMs+=value;}
void browserPresent(const uint16_t*,int,int,uint8_t,uint8_t){}
bool browserAudioAvailable(){return true;}
void browserAudioMaster(uint8_t,unsigned,bool){}
void browserAudioGain(uint8_t,uint8_t){}
unsigned browserAudioQueued(unsigned channel){return channel<8&&audioUntil[channel]>clockMs?1:0;}
bool browserAudioPlay(const int16_t*,size_t count,uint32_t rate,bool stereo,uint32_t repeat,unsigned channel,bool replace){if(channel>=8||!rate)return false;uint64_t duration=count*1000ull/rate/(stereo?2:1);if(repeat==0xffffffffu)duration=3600000;else duration*=repeat;audioUntil[channel]=(replace?clockMs:std::max<uint64_t>(clockMs,audioUntil[channel]))+duration;return true;}
void browserAudioStop(int channel){if(channel<0)audioUntil.fill(0);else if(channel<8)audioUntil[channel]=0;}
bool browserButtonPressed(unsigned){return false;}
int browserSha256(const unsigned char*p,size_t n,unsigned char*out,int sha224){if(sha224)return -1;CC_SHA256(p,CC_LONG(n),out);return 0;}
int xTaskCreatePinnedToCore(void(*)(void*),const char*,unsigned,void*,unsigned,void*,unsigned){return 0;}
void vTaskDelay(uint32_t value){clockMs+=value;}

static bool freshEntropy(void*,uint8_t*out,size_t count){arc4random_buf(out,count);return true;}
static bool owner(){return uiRandomHealthy();}
static std::string field(const std::string&s,const std::string&key){auto at=s.find("\""+key+"\":");if(at==s.npos)return "";at+=key.size()+3;if(s[at]=='\"'){auto end=s.find('"',at+1);return s.substr(at+1,end-at-1);}auto end=s.find_first_of(",}",at);return s.substr(at,end-at);}
static uint64_t number(const std::string&s,const std::string&key){auto value=field(s,key);return value.empty()?0:std::strtoull(value.c_str(),nullptr,10);}
static std::vector<unsigned> grid(const std::string&s){auto begin=s.find("\"grid\":[");std::vector<unsigned> out;if(begin==s.npos)return out;const char*p=s.c_str()+begin+8;while(*p&&*p!=']'){char*end=nullptr;out.push_back(unsigned(std::strtoul(p,&end,10)));p=end;if(*p==',')++p;else break;}return out;}
static std::string quoted(const std::string&s){std::string out="\"";for(char c:s){if(c=='"'||c=='\\')out+='\\';out+=c;}return out+'"';}

struct Recorder {
 fs::path out;
 std::ofstream actions,events;
 std::set<std::string> seen;
 unsigned clips=0;
 uint64_t inputIndex=0,activeInputIndex=0;
 uint64_t activeAction=0;
 uint32_t activeAt=0,readyAt=0;
 std::ofstream raw,frames;
 std::string activeId,initial;
 std::vector<std::string> labels;
 bool capture=false;
 unsigned frameCount=0;
 explicit Recorder(fs::path directory):out(std::move(directory)){fs::create_directories(out);actions.open(out/"actions.ndjson");events.open(out/"events.ndjson");}
 void tick(unsigned step=40){clockMs+=step;audioUpdate(clockMs);const auto before=uiActionCount();uiFrame();if(uiActionCount()!=before)log("automatic-feature-continuation");if(capture){const auto*p=uiFramePixels();raw.write(reinterpret_cast<const char*>(p),240*135*2);frames<<"{\"index\":"<<frameCount++<<",\"time_ms\":"<<clockMs<<",\"snapshot\":"<<uiSnapshot()<<",\"sage\":"<<uiSageDiagnostic()<<",\"award\":"<<uiAwardDiagnostic()<<"}\n";}}
 void log(const char*input){actions<<"{\"action_index\":"<<++inputIndex<<",\"engine_action_count\":"<<uiActionCount()<<",\"time_ms\":"<<clockMs<<",\"input\":"<<quoted(input)<<",\"snapshot\":"<<uiSnapshot()<<",\"sage\":"<<uiSageDiagnostic()<<",\"award\":"<<uiAwardDiagnostic()<<"}\n";actions.flush();}
 void start(const std::vector<std::string>&tags){
  if(capture)return;activeAction=uiActionCount();activeInputIndex=inputIndex;activeAt=clockMs;readyAt=0;frameCount=0;labels=tags;initial=uiSnapshot();
  char name[80];std::snprintf(name,sizeof(name),"action-%06llu-clip-%03u",(unsigned long long)activeAction,++clips);activeId=name;fs::create_directories(out/activeId);raw.open(out/activeId/"frames.rgb565",std::ios::binary);frames.open(out/activeId/"frames.ndjson");capture=true;
 }
 void finish(){if(!capture)return;raw.close();frames.close();events<<"{\"id\":"<<quoted(activeId)<<",\"kind\":\"actual-gameplay\",\"action_index\":"<<activeInputIndex<<",\"engine_action_count\":"<<activeAction<<",\"start_ms\":"<<activeAt<<",\"end_ms\":"<<clockMs<<",\"frame_count\":"<<frameCount<<",\"fps\":25,\"format\":\"rgb565le\",\"width\":240,\"height\":135,\"raw_file\":"<<quoted(activeId+"/frames.rgb565")<<",\"frame_journal\":"<<quoted(activeId+"/frames.ndjson")<<",\"tags\":[";for(size_t i=0;i<labels.size();++i){if(i)events<<',';events<<quoted(labels[i]);}events<<"],\"snapshot_start\":"<<initial<<",\"snapshot_end\":"<<uiSnapshot()<<"}\n";events.flush();capture=false;}
 void still(const std::string&name){std::ofstream file(out/(name+".rgb565"),std::ios::binary);file.write(reinterpret_cast<const char*>(uiFramePixels()),240*135*2);std::ofstream meta(out/(name+".json"));meta<<"{\"action_index\":"<<inputIndex<<",\"engine_action_count\":"<<uiActionCount()<<",\"time_ms\":"<<clockMs<<",\"snapshot\":"<<uiSnapshot()<<"}\n";}
 std::vector<std::string> interests(){
  std::vector<std::string> tags;auto s=std::string(uiSnapshot()),sage=std::string(uiSageDiagnostic()),award=std::string(uiAwardDiagnostic());
  auto add=[&](std::string t){if(seen.insert(t).second)tags.push_back(t);};
  auto mode=field(s,"mode"),phase=field(s,"animation");
  if(uiActionCount()<=4)add("opening-play-"+std::to_string(uiActionCount()));
  if(mode!="base")add("mode-"+mode);
  if(phase!="ready"&&phase!="spinning"&&phase!="landing")add("phase-"+phase);
  auto cells=grid(s);auto mask=number(sage,"winMask"),join=number(sage,"joinMask");
  for(unsigned i=0;i<cells.size();++i){if(mask&(1u<<i))add("win-symbol-"+std::to_string(cells[i]));if(cells[i]==10||cells[i]==11)add("present-symbol-"+std::to_string(cells[i]));}
  if(join){unsigned maxStack=0;for(unsigned col=0;col<5;++col){unsigned n=0;for(unsigned row=0;row<3;++row)n+=(join>>(col*3+row))&1u;maxStack=std::max(maxStack,n);}add("confucius-merge-"+std::to_string(maxStack));}
  if(field(sage,"free")=="true"&&mode=="hold")add("hold-inside-free-games");
  if(field(award,"valid")=="true"&&field(award,"test")=="false"&&number(award,"action")==uiActionCount()){
   if(field(award,"grand")=="true")add("grand-award");
   for(const char*name:{"miniCount","minorCount","majorCount"})if(number(award,name))add(std::string("named-")+name+"-"+std::to_string(number(award,name)));
   if(field(award,"fountainStarted")=="true")add("coin-fountain");
  }
  return tags;
 }
};

int main(int argc,char**argv){
 if(argc<2){std::fprintf(stderr,"usage: capture-gameplay OUTPUT [MAX_ACTIONS=5000]\n");return 2;}
 const uint64_t limit=argc>2?std::strtoull(argv[2],nullptr,10):5000;
 Recorder record(argv[1]);phoenix::random::Service random;
 if(!random.initialize(freshEntropy,nullptr)||!uiInitFresh(random)){std::fprintf(stderr,"Fresh game/RNG failed\n");return 3;}
 uiSetOutcomeCheckpointHandler(owner);audioInit(false);uiPrepareEffects();uiRedraw();record.tick();record.log("fresh-start");record.still("startup-denomination");
 uiButtonA();record.tick();record.log("blue-select-denomination");record.still("startup-lines-and-bet");
 // The initial focus is the first line option. Advance to the first red BET.
 for(unsigned i=0;i<30&&field(uiSnapshot(),"view")!="game";++i){uiButtonA();record.tick();record.log("blue-select-option");if(field(uiSnapshot(),"view")=="game")break;uiButtonB();record.tick();record.log("grey-next-option");}
 if(field(uiSnapshot(),"view")!="game"){std::fprintf(stderr,"Could not choose a real offered wager: %s\n",uiSnapshot().c_str());return 4;}
 record.still("unfunded-reels");
 uiButtonA();record.tick();record.log("blue-open-credit-voucher");record.still("credit-voucher");
 uiButtonA();record.tick();record.log("blue-increase-voucher-digit-to-1000-dollars");
 for(unsigned i=0;i<6;++i){uiButtonB();record.tick();record.log("grey-next-voucher-field");}
 record.still("credit-voucher-filled");uiButtonA();record.tick();record.log("blue-insert-play-money-voucher");record.still("funded-reels");
 if(number(uiSnapshot(),"balanceMinor")==0||field(uiSnapshot(),"view")!="game"){std::fprintf(stderr,"Funding refused: %s\n",uiSnapshot().c_str());return 5;}
 uint64_t lastAction=uiActionCount();unsigned ordinary=0;uint32_t watchdog=clockMs,promptAt=0;std::string lastPhase;
 while(uiActionCount()<limit&&uiRandomHealthy()){
  const auto phase=field(uiSnapshot(),"animation");if(phase!=lastPhase){lastPhase=phase;promptAt=clockMs;}
  if((phase=="free-intro"||phase=="hold-intro")&&clockMs-promptAt>9000){uiButtonA();record.log("blue-resume-earned-feature");}
  if(uiReady()){
   if(record.capture){if(!record.readyAt)record.readyAt=clockMs;if(clockMs-record.readyAt<6500){record.tick();continue;}record.finish();}
   if(number(uiSnapshot(),"balanceMinor")<10000&&field(uiSnapshot(),"mode")=="base"){
    uiMenu();record.tick();record.log("hold-grey-open-balance-menu");uiButtonA();record.tick();record.log("blue-open-balance");uiButtonB();record.tick();record.log("grey-select-first-credit-preset");uiButtonA();record.tick();record.log("blue-add-play-money-credit-preset");
   }
   uiButtonA();record.log("blue-spin-or-bonus-resume");
  }
  auto interests=record.interests();
  if(!interests.empty()){
   if(!record.capture)record.start(interests);else for(auto&t:interests)record.labels.push_back(t);
  }
  record.tick(record.capture?40:200);
  if(uiActionCount()!=lastAction){lastAction=uiActionCount();watchdog=clockMs;if(lastAction%100==0){std::fprintf(stderr,"actions=%llu clips=%u simulated_ms=%u\n",(unsigned long long)lastAction,record.clips,clockMs);}}
  if(clockMs-watchdog>600000){std::fprintf(stderr,"No action for 10 simulated minutes: %s\n",uiSnapshot().c_str());return 6;}
  ++ordinary;
 }
 while(!uiReady()&&uiRandomHealthy()){const auto phase=field(uiSnapshot(),"animation");if(phase=="free-intro"||phase=="hold-intro"){uiButtonA();record.log("blue-resume-earned-feature-at-session-end");}record.tick(record.capture?40:200);}
 if(record.capture)for(unsigned i=0;i<163;++i)record.tick();
 record.finish();record.log("recording-stop-at-ready-boundary");record.still("last-real-reel-board");
 uiJackpots();record.tick();record.log("double-grey-jackpot-board");record.still("jackpot-board");record.start({"jackpot-board-menu"});for(unsigned i=0;i<125;++i)record.tick();record.finish();uiButtonA();record.tick();
 uiMenu();record.tick();record.log("hold-grey-menu");record.still("game-menu");
 // Inspect real menu screens with their ordinary buttons; no values are edited.
 uiButtonA();record.tick();record.log("blue-open-balance");record.still("balance-menu");uiMenu();record.tick();
 uiButtonB();record.tick();uiButtonA();record.tick();record.log("blue-open-game-rules");
 for(unsigned i=0;i<6;++i){record.still("rules-"+std::to_string(i+1));uiButtonB();record.tick();record.log("grey-next-rules-page");}uiMenu();record.tick();
 uiButtonB();record.tick();uiButtonA();record.tick();record.log("blue-open-paylines");
 for(unsigned layout=0;layout<5;++layout){for(unsigned page=0;page<20;++page){record.still("paylines-layout-"+std::to_string(layout+1)+"-page-"+std::to_string(page+1));uiButtonB();record.tick();record.log("grey-next-paylines-page");}uiButtonA();record.tick();record.log("blue-next-payline-layout");}uiMenu();record.tick();
 // Menu opens focused on PAYLINES; two next presses select SETTINGS.
 uiButtonB();record.tick();uiButtonB();record.tick();uiButtonA();record.tick();record.log("blue-open-settings");record.still("settings-menu");
 uiButtonA();record.tick();record.log("blue-open-sound");record.still("sound-menu");uiMenu();record.tick();
 uiButtonB();record.tick();uiButtonA();record.tick();record.log("blue-open-display");record.still("display-menu");
 std::ofstream summary(record.out/"summary.json");summary<<"{\"source\":\"unchanged production C++ UI, Engine and CSPRNG\",\"entropy\":\"fresh OS arc4random_buf entropy per production callback; no seed or replay\",\"time\":\"virtual monotonic ms; captures sampled every 40 ms (25 fps), noncaptured ordinary actions every 200 ms\",\"audio\":\"silent host output adapter; registered cue durations tracked, native music backend unavailable\",\"actions\":"<<uiActionCount()<<",\"clips\":"<<record.clips<<",\"rng_healthy\":"<<(uiRandomHealthy()?"true":"false")<<",\"coverage\":[";bool comma=false;for(auto&tag:record.seen){if(comma)summary<<',';comma=true;summary<<quoted(tag);}summary<<"]}\n";
 std::fprintf(stderr,"Complete: %llu actual actions, %u clips\n",(unsigned long long)uiActionCount(),record.clips);
 return uiRandomHealthy()?0:7;
}
