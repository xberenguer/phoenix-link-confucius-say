#include <emscripten.h>
#include <emscripten/fiber.h>
#include <M5Unified.h>
#include <esp_system.h>
#include "ui.h"
#include "phoenix_random.h"
#include "entropy_lease125.h"
#include "audio.h"
#include "audio_music.h"
#include "build_version.h"
#include "math_runtime_identity.h"
#include "phoenix_art.h"
#include "game_black_tap.h"
#include <array>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits>

EM_JS(double,web_clock,(),{return Module.browserBridge.clock();});
EM_JS(int,web_entropy125,(uint8_t* out,size_t size),{
 try{
  if(size!==48||!globalThis.isSecureContext||!globalThis.crypto||typeof crypto.getRandomValues!=='function')return 0;
  const bytes=new Uint8Array(size);crypto.getRandomValues(bytes);HEAPU8.set(bytes,out);bytes.fill(0);return 1;
 }catch{if(out&&size===48)HEAPU8.fill(0,out,out+size);return 0;}
});
EM_JS(int,web_button_pressed,(int index),{return (Module.browserBridge.heldButtons&(1<<index))?1:0;});
EM_JS(void,web_present,(const uint16_t* data,int w,int h,int rotation,int brightness),{Module.browserBridge.present(HEAPU8.buffer,data,w,h,rotation,brightness);});
EM_JS(int,web_audio_ready,(),{return Module.browserBridge.audio.available()?1:0;});
EM_JS(void,web_audio_master,(int value,int magnification,int stereo),{Module.browserBridge.audio.master(value,magnification,!!stereo);});
EM_JS(void,web_audio_gain,(int channel,int value),{Module.browserBridge.audio.gain(channel,value);});
EM_JS(int,web_audio_queued,(int channel),{return Module.browserBridge.audio.queued(channel);});
EM_JS(int,web_audio_play,(const int16_t* data,int count,int rate,int stereo,uint32_t repeat,int channel,int replace),{return Module.browserBridge.audio.play(HEAP16.subarray(data>>>1,(data>>>1)+count),count,rate,!!stereo,repeat>>>0,channel,!!replace)?1:0;});
EM_JS(void,web_audio_stop,(int channel),{Module.browserBridge.audio.stop(channel);});
EM_ASYNC_JS(int,web_sha256,(const unsigned char* data,size_t size,unsigned char* out),{
 try{const hash=new Uint8Array(await crypto.subtle.digest('SHA-256',HEAPU8.slice(data,data+size)));HEAPU8.set(hash,out);return 0;}catch{return -1;}
});
uint32_t millis(){return uint32_t(uint64_t(web_clock()));}
uint32_t micros(){return uint32_t(uint64_t(web_clock()*1000.0));}
void delay(uint32_t ms){emscripten_sleep(ms);}
void browserPresent(const uint16_t*p,int w,int h,uint8_t r,uint8_t b){web_present(p,w,h,r,b);}
bool browserAudioAvailable(){return web_audio_ready()!=0;}
bool browserButtonPressed(unsigned i){return web_button_pressed(i)!=0;}
void browserAudioMaster(uint8_t v,unsigned m,bool s){web_audio_master(v,m,s);}
void browserAudioGain(uint8_t c,uint8_t v){web_audio_gain(c,v);}
unsigned browserAudioQueued(unsigned c){return unsigned(web_audio_queued(c));}
bool browserAudioPlay(const int16_t*p,size_t n,uint32_t r,bool s,uint32_t count,unsigned c,bool replace){return web_audio_play(p,int(n),int(r),s,count,int(c),replace)!=0;}
void browserAudioStop(int c){web_audio_stop(c);}
int browserSha256(const unsigned char*p,size_t n,unsigned char*out,int sha224){return sha224?-1:web_sha256(p,n,out);}

namespace {
emscripten_fiber_t mainFiber{},musicFiber{};
alignas(16) std::array<uint8_t,262144> mainAsyncStack{},musicAsyncStack{};
alignas(16) std::array<uint8_t,131072> musicCStack{};
bool musicCreated=false,insideMusic=false;uint32_t musicWake=0;
GameBlackTap blackTap;
void(*musicEntry)(void*)=nullptr;void*musicArgument=nullptr;
void musicMain(void*){insideMusic=true;musicEntry(musicArgument);insideMusic=false;emscripten_fiber_swap(&musicFiber,&mainFiber);std::abort();}
void serviceMusic(){if(musicCreated&&int32_t(millis()-musicWake)>=0){insideMusic=true;emscripten_fiber_swap(&mainFiber,&musicFiber);insideMusic=false;}}
phoenix::random::Service outcomeRandom125;
bool initialized=false;
bool acquireEntropy125(void*){return true;}
bool fillEntropy125(void*,uint8_t*out,size_t size){return web_entropy125(out,size)!=0;}
void releaseEntropy125(void*){}
platform_entropy125::Source entropySource125({acquireEntropy125,fillEntropy125,releaseEntropy125,nullptr},true);
bool commitSession125(){if(uiReturnSelectionCheckpointPending())blackTap.reset();return initialized&&outcomeRandom125.healthy()&&uiRandomHealthy();}
std::array<char,1024> info{};

}
int xTaskCreatePinnedToCore(void(*fn)(void*),const char*,unsigned,void*arg,unsigned,void*,unsigned){
 if(musicCreated||!fn)return 0;
 musicEntry=fn;musicArgument=arg;musicWake=millis();
 emscripten_fiber_init_from_current_context(&mainFiber,mainAsyncStack.data(),mainAsyncStack.size());
 emscripten_fiber_init(&musicFiber,musicMain,nullptr,musicCStack.data(),musicCStack.size(),musicAsyncStack.data(),musicAsyncStack.size());
 musicCreated=true;return pdPASS;
}
void vTaskDelay(uint32_t ms){if(!insideMusic){emscripten_sleep(ms);return;}musicWake=millis()+ms;insideMusic=false;emscripten_fiber_swap(&musicFiber,&mainFiber);insideMusic=true;}

extern "C" {
EMSCRIPTEN_KEEPALIVE int pl_boot(){
 if(initialized)return 0;
 const bool seeded=outcomeRandom125.initialize(platform_entropy125::Source::callback,&entropySource125);
 const bool qualified=entropySource125.qualifyRuntime(true);
 const bool fresh=uiInitFresh(outcomeRandom125);
 uiSetOutcomeCheckpointHandler(commitSession125);
 audioInit(false);uiPrepareEffects();initialized=true;uiRedraw();uiFrame();
 return seeded&&qualified&&fresh?1:-3;
}
EMSCRIPTEN_KEEPALIVE void pl_preferences(int spin,int landings,int volume,int muted,int credits,int flipped,int refill,uint32_t lastAdded,int brightness){
 uiSetSoundPreferences(spin,landings,uint8_t(volume));audioSetMuted(muted);uiSetCreditsDisplay(credits);uiSetScreenFlipped(flipped);uiSetCreditPreferences(refill,lastAdded);M5.Display.setBrightness(uint8_t(brightness));
}
int pl_input(int code);
// Native Main: audioUpdate, physical input, deferred grey tap, then uiFrame.
// The RAM session owner acknowledges synchronously; no game state persists.
EMSCRIPTEN_KEEPALIVE void pl_frame(int code){if(!initialized)return;serviceMusic();audioUpdate(millis());const bool fenced=uiReturnSelectionInputFenced();if(fenced||M5.BtnA.isPressed())blackTap.reset();if(!fenced&&M5.BtnB.isPressed()&&uiIsGameView())uiPreparePanelInput();if(code>=0&&!fenced)pl_input(code);if(!uiReturnSelectionInputFenced()&&blackTap.poll(millis(),uiIsGameView(),M5.BtnA.isPressed()||M5.BtnB.isPressed()))uiCabinetMenu();uiFrame();}
EMSCRIPTEN_KEEPALIVE void pl_tick(){pl_frame(-1);}
EMSCRIPTEN_KEEPALIVE int pl_input(int code){
 if(!initialized||!uiRandomHealthy()||uiCheckpointPaused()||uiReturnSelectionInputFenced()){blackTap.reset();return 0;}
 switch(code){case 0:blackTap.reset();uiButtonA();break;case 1:if(uiIsGameView()){uiPreparePanelInput();if(blackTap.released(millis()))uiJackpots();else return 1;}else{blackTap.reset();uiButtonB();}break;case 2:blackTap.reset();uiButtonBHold();break;case 3:blackTap.reset();uiToggleAuto();break;case 4:blackTap.reset();uiFlipScreen();break;case 5:blackTap.reset();uiMenu();break;default:return 0;}
 return 1;
}
EMSCRIPTEN_KEEPALIVE const char*pl_snapshot(){static String s;s=uiSnapshot();return s.c_str();}
EMSCRIPTEN_KEEPALIVE const char*pl_info(){std::snprintf(info.data(),info.size(),"{\"version\":\"%s\",\"runtime\":\"%s\",\"mathFamily\":\"%s\",\"accepted\":%s,\"statePersistence\":\"session-only\",\"freshCredits\":0,\"rngHealthy\":%s,\"rngFault\":%u}",build_version::number,math_runtime_identity::fingerprint,math_runtime_identity::mathematicalFamily,math_runtime_identity::accepted?"true":"false",uiRandomHealthy()?"true":"false",unsigned(uiRandomFault()));return info.data();}
EMSCRIPTEN_KEEPALIVE int pl_random_healthy(){return uiRandomHealthy();}
EMSCRIPTEN_KEEPALIVE int pl_return_input_fenced(){return uiReturnSelectionInputFenced();}
EMSCRIPTEN_KEEPALIVE int pl_game_view(){return uiIsGameView();}
EMSCRIPTEN_KEEPALIVE const uint16_t*pl_pixels(){return uiFramePixels();}
}
