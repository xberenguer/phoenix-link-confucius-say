#include <emscripten.h>
#include <emscripten/fiber.h>
#include <M5Unified.h>
#include <esp_system.h>
#include "ui.h"
#include "audio.h"
#include "audio_music.h"
#include "build_version.h"
#include "math_runtime_identity.h"
#include "phoenix_art.h"
#include "game_black_tap.h"
#include "checkpoint_bundle.h"
#include <array>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits>

EM_JS(double,web_clock,(),{return Module.browserBridge.clock();});
EM_JS(uint32_t,web_random,(),{const a=new Uint32Array(1);crypto.getRandomValues(a);return a[0];});
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
EM_ASYNC_JS(int,web_commit,(const uint8_t* data,size_t size,const char* prefs,int selector),{
 try{return await Module.browserBridge.persist(HEAPU8.slice(data,data+size),UTF8ToString(prefs),!!selector)?1:0;}catch(error){Module.browserBridge.storageFailure(error);return 0;}
});


EM_JS(int,web_owned_preferences,(char* out,size_t capacity),{
 const value=Module.browserBridge.ownedPreferences();
 if(!value||lengthBytesUTF8(value)+1>capacity)return 0;
 stringToUTF8(value,out,capacity);return 1;
});
EM_ASYNC_JS(int,web_verify_owned,(const uint8_t* data,size_t size,const char* prefs),{
 try{return await Module.browserBridge.verifyOwned(HEAPU8.slice(data,data+size),UTF8ToString(prefs))?1:0;}catch(error){Module.browserBridge.storageFailure(error);return 0;}
});

uint32_t millis(){return uint32_t(uint64_t(web_clock()));}
uint32_t micros(){return uint32_t(uint64_t(web_clock()*1000.0));}
uint32_t esp_random(){return web_random();}
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
std::array<uint8_t,kUiCheckpointCapacity> checkpoint{};
std::array<char,512> preferences{};std::array<char,1024> info{};
uint64_t generation=0,pendingGeneration=0,ownerCalls=0;
bool pendingVerify=false,pendingSelector=false;
std::array<uint8_t,kUiCheckpointCapacity> comparison{};std::array<char,512> ownedPreferences{};size_t checkpointLength=0;bool initialized=false,writeFailed=false,pendingRecord=false;
const char* preferencesJson(){std::snprintf(preferences.data(),preferences.size(),"{\"spin\":%s,\"landings\":%s,\"volume\":%u,\"muted\":%s,\"credits\":%s,\"flipped\":%s,\"autoRefill\":%s,\"lastAddedMinor\":%u,\"brightness\":%u}",uiSpinSoundEnabled()?"true":"false",uiLandingSoundEnabled()?"true":"false",uiSoundVolume(),audioMuted()?"true":"false",uiCreditsDisplay()?"true":"false",uiScreenFlipped()?"true":"false",uiAutoRefillEnabled()?"true":"false",uiLastAddedMinor(),M5.Display.getBrightness());return preferences.data();}
bool sameOwnedReturnTuple(){
 if(!checkpointLength||writeFailed||!web_owned_preferences(ownedPreferences.data(),ownedPreferences.size())||std::strcmp(ownedPreferences.data(),preferencesJson()))return false;
 const size_t n=uiSaveCheckpoint(comparison.data(),comparison.size(),generation);
 checkpoint_bundle::View owned,current;
 if(!n||!checkpoint_bundle::decode(checkpoint.data(),checkpointLength,owned)||!checkpoint_bundle::decode(comparison.data(),n,current))return false;
 auto same=[](checkpoint_bundle::Part a,checkpoint_bundle::Part b){return a.length==b.length&&(!a.length||!std::memcmp(a.bytes,b.bytes,a.length));};
 return same(owned.game,current.game)&&same(owned.preference,current.preference)&&same(owned.service,current.service);
}
bool commitCurrent(){
 ++ownerCalls;
 if(!pendingRecord){
  pendingSelector=uiReturnSelectionCheckpointPending();
  if(pendingSelector)blackTap.reset();
  pendingVerify=pendingSelector&&sameOwnedReturnTuple();
  if(pendingVerify){pendingGeneration=generation;preferencesJson();pendingRecord=true;}
  else{
   if(generation==std::numeric_limits<uint64_t>::max()){writeFailed=true;return false;}
   pendingGeneration=generation+1;
   checkpointLength=uiSaveCheckpoint(checkpoint.data(),checkpoint.size(),pendingGeneration);
   if(!checkpointLength){std::fprintf(stderr,"Browser canonical checkpoint encoding failed\n");writeFailed=true;return false;}
   preferencesJson();pendingRecord=true;
  }
 }
 // Both modes await actual IDB completion/readback. Frozen bytes, options and
 // generation remain identical on retry, including a read-only owned no-op.
 const bool durable=pendingVerify?web_verify_owned(checkpoint.data(),checkpointLength,preferences.data())!=0:web_commit(checkpoint.data(),checkpointLength,preferences.data(),pendingSelector)!=0;
 if(durable){generation=pendingGeneration;writeFailed=false;pendingRecord=false;pendingVerify=false;pendingSelector=false;}else writeFailed=true;
 return durable;
}
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
EMSCRIPTEN_KEEPALIVE int pl_boot(const uint8_t*saved,size_t size){
 if(initialized)return 0;
 if(size>kUiCheckpointCapacity||(!saved&&size)||(saved&&!size))return -1;
 uiInit();uiSetOutcomeCheckpointHandler(commitCurrent);
 if(saved&&size){uint64_t restored=0;if(!uiRestoreCheckpoint(saved,size,restored))return -1;generation=restored;std::memcpy(checkpoint.data(),saved,size);checkpointLength=size;}
 audioInit(false);uiPrepareEffects();initialized=true;
 if(!saved&&!commitCurrent()){uiSetCheckpointPause(UiCheckpointPause::UndurableRecord);return -2;}
 uiRedraw();uiFrame();return 1;
}
EMSCRIPTEN_KEEPALIVE void pl_preferences(int spin,int landings,int volume,int muted,int credits,int flipped,int refill,uint32_t lastAdded,int brightness){
 uiSetSoundPreferences(spin,landings,uint8_t(volume));audioSetMuted(muted);uiSetCreditsDisplay(credits);uiSetScreenFlipped(flipped);uiSetCreditPreferences(refill,lastAdded);M5.Display.setBrightness(uint8_t(brightness));
}
int pl_input(int code);
// Native Main: audioUpdate, physical input, deferred grey tap, then uiFrame.
// The same frame stays synchronously occupied across an awaited durable write.
EMSCRIPTEN_KEEPALIVE void pl_frame(int code){if(!initialized)return;serviceMusic();audioUpdate(millis());const bool fenced=uiReturnSelectionInputFenced();if(fenced||M5.BtnA.isPressed())blackTap.reset();if(code>=0&&!fenced)pl_input(code);if(!uiReturnSelectionInputFenced()&&blackTap.poll(millis(),uiIsGameView(),M5.BtnA.isPressed()||M5.BtnB.isPressed()))uiCabinetMenu();uiFrame();}
EMSCRIPTEN_KEEPALIVE void pl_tick(){pl_frame(-1);}
EMSCRIPTEN_KEEPALIVE int pl_input(int code){
 if(!initialized||writeFailed||uiCheckpointPaused()||uiReturnSelectionInputFenced()){blackTap.reset();return 0;}
 const auto actions=uiActionCount();const auto priorOwnerCalls=ownerCalls;
 switch(code){case 0:blackTap.reset();uiButtonA();break;case 1:if(uiIsGameView()){if(blackTap.released(millis()))uiFlipScreen();else return 1;}else{blackTap.reset();uiButtonB();}break;case 2:blackTap.reset();uiButtonBHold();break;case 3:blackTap.reset();uiToggleAuto();break;case 4:blackTap.reset();uiFlipScreen();break;case 5:blackTap.reset();uiMenu();break;default:return 0;}
 // Engine outcomes already used the real ownership callback above. Non-action
 // preferences/funding/wagers are committed here before their next frame too.
 if(uiActionCount()==actions&&ownerCalls==priorOwnerCalls&&!commitCurrent()){uiSetCheckpointPause(UiCheckpointPause::UndurableRecord);return -1;}
 return 1;
}
EMSCRIPTEN_KEEPALIVE int pl_retry_storage(){if(!initialized||!writeFailed)return 0;if(!commitCurrent())return 0;uiSetCheckpointPause(UiCheckpointPause::None);return 1;}
EMSCRIPTEN_KEEPALIVE const char*pl_snapshot(){static String s;s=uiSnapshot();return s.c_str();}
EMSCRIPTEN_KEEPALIVE const char*pl_info(){const auto m=audioMusicDiagnostics();std::snprintf(info.data(),info.size(),"{\"version\":\"%s\",\"runtime\":\"%s\",\"mathFamily\":\"%s\",\"accepted\":%s,\"audioReady\":%s,\"musicReady\":%s,\"completionReady\":%s,\"completionFNV\":%u,\"awardReady\":%s,\"awardFNV\":%u,\"twinReady\":%s,\"atlasReady\":%s,\"effectsReady\":%s,\"checkpointLength\":%u,\"hasCanonicalStoreOwner\":true}",build_version::number,math_runtime_identity::fingerprint,math_runtime_identity::mathematicalFamily,math_runtime_identity::accepted?"true":"false",audioReady()?"true":"false",m.ready?"true":"false",m.completionCacheReady?"true":"false",m.completionFNV,m.awardCacheReady?"true":"false",m.awardCacheFNV,m.twinSourceReady?"true":"false",phoenix_art::ready()?"true":"false",uiEffectCache().ready?"true":"false",unsigned(checkpointLength));return info.data();}
EMSCRIPTEN_KEEPALIVE int pl_return_input_fenced(){return uiReturnSelectionInputFenced();}
EMSCRIPTEN_KEEPALIVE int pl_game_view(){return uiIsGameView();}
EMSCRIPTEN_KEEPALIVE const uint16_t*pl_pixels(){return uiFramePixels();}
EMSCRIPTEN_KEEPALIVE const uint8_t*pl_checkpoint_data(){return checkpoint.data();}
EMSCRIPTEN_KEEPALIVE size_t pl_checkpoint_size(){return checkpointLength;}
EMSCRIPTEN_KEEPALIVE size_t pl_checkpoint_capacity(){return kUiCheckpointCapacity;}
}
