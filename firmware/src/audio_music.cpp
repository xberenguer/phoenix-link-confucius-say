#include "audio_music.h"
#ifdef ARDUINO
#include <M5Unified.h>
#include <SPIFFS.h>
#include <esp_heap_caps.h>
#include <mbedtls/sha256.h>
#include <atomic>
#include <algorithm>
#include <cstring>
#include "music_asset.h"
#include "music_seek_data.h"
#include "collected_music_seek.h"
#include "award_music_seek.h"
#include "music_stream.h"
#include "feature_music_timeline.h"
#include "completion_cache.h"
#include "award_cache.h"
#include "immutable_pcm_loop89.h"
#include "twin_music_bank39.h"
namespace {
constexpr uint8_t kChannel=6;
MusicStream stream;
// Worker-owned scalar cursor only; immutable PCM stays in flash, never copied.
ImmutablePcmLoop89 twinStream;
// One atomic snapshot binds the requested part to its event identity.
// Equality handles29-bit wrap; no chronological ordering comparison is used.
constexpr uint32_t requestPartBits=3,requestPartMask=7,requestEpochMask=UINT32_MAX>>requestPartBits;
static_assert(unsigned(FeatureMusicPart::TwinFree)<=requestPartMask,"Music parts must fit the command word");
static_assert(std::atomic<uint32_t>::is_always_lock_free,"Music request publication must remain lock-free");
std::atomic<uint32_t> requestWord{0};
std::atomic<FeatureMusicPart> currentPart{FeatureMusicPart::Off};
FeatureMusicPart requestPart(uint32_t word){return FeatureMusicPart(word&requestPartMask);}
bool isAwardPart(FeatureMusicPart part){return part==FeatureMusicPart::MajorAward||part==FeatureMusicPart::BigAward;}
void publishRequest(FeatureMusicPart part,bool newAward=false);
std::atomic<bool> ready{false},playing{false},fastPoint{false},collectedSourceReady{false},sourceFinished{false};
std::atomic<bool> awardSourceReady{false};
std::atomic<bool> finiteDraining{false};
std::atomic<uint32_t> stopAcknowledged{0};
std::atomic<uint32_t> stopGeneration{0},awardRequestedAt{0},awardStarts{0},awardLastStartDelayMs{0},maxConfigureUs{0};
std::atomic<uint32_t> queuedFrames{0},loops{0},retries{0},underruns{0};
std::atomic<uint32_t> maxPumpUs{0},maxWrapUs{0};
std::atomic<uint32_t> rangeBegin{0},rangeEnd{0};
uint8_t* compressed=nullptr;uint32_t fileBytes=0;
bool holdIndexMatches=false;
int16_t* completionBlock=nullptr;uint32_t completionFNV=0;
int16_t* awardBlock=nullptr;uint32_t awardFNV=0;
bool prepareAwardCache(){
 if(awardBlock)return true;
 if(!awardSourceReady)return false;
 auto* block=static_cast<int16_t*>(heap_caps_malloc(award_cache::storageBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));if(!block)return false;
 if(!award_cache::decode(stream,compressed,fileBytes,block,awardFNV)){heap_caps_free(block);return false;}
 awardBlock=block;return true;
}
bool prepareCompletionCache(){
 if(completionBlock)return audioRegisterCompletion(completionBlock,completion_cache::frames,completionBlock+completion_cache::alignedFrames,completion_cache::alignedFrames);
 if(!collectedSourceReady)return false;
 auto* block=static_cast<int16_t*>(heap_caps_malloc(completion_cache::storageBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));if(!block)return false;
 if(!completion_cache::decode(stream,compressed,fileBytes,block,completionFNV)||!audioRegisterCompletion(block,completion_cache::frames,block+completion_cache::alignedFrames,completion_cache::alignedFrames)){heap_caps_free(block);return false;}
 completionBlock=block;return true;
}
bool twinSourceAvailable39(){
 if constexpr(!twin_music_bank39::enabled)return false;
 else return twin_music_bank39::metadataValid&&!(reinterpret_cast<uintptr_t>(twin_music_bank39::data())&1u);
}
bool configure(FeatureMusicPart part){
 if(part==FeatureMusicPart::TwinFree){
  if constexpr(!twin_music_bank39::enabled)return false;
  else{
   if(!twinSourceAvailable39()||!twinStream.bind(twin_music_bank39::data(),twin_music_bank39::frames,twin_music_bank39::sampleRate,twin_music_bank39::loopBegin,twin_music_bank39::loopEnd))return false;
   rangeBegin=twin_music_bank39::loopBegin;rangeEnd=twin_music_bank39::loopEnd;
   fastPoint=false;sourceFinished=false;return true;
  }
 }
 if(!stream.init(compressed,fileBytes))return false;
 const bool collected=part==FeatureMusicPart::Collected;
 const bool award=part==FeatureMusicPart::MajorAward||part==FeatureMusicPart::BigAward;
 if(award){
  const uint32_t end=part==FeatureMusicPart::MajorAward?feature_music::majorAwardEndFrame:feature_music::bigAwardEndFrame;
  if(!awardSourceReady||!stream.setOneShotRegion(feature_music::awardBeginFrame,end)||!stream.setLoopSeekPoint(award_music_seek::point))return false;
 }else if(collected){
  if(!collectedSourceReady||!stream.setOneShotRegion(feature_music::collectedBeginFrame,feature_music::collectedEndFrame)||!stream.setLoopSeekPoint(collected_music_seek::point))return false;
 }else{
  if(!stream.setLoopRegion(feature_music::loopBeginFrame,feature_music::loopEndFrame))return false;
  if(holdIndexMatches)stream.setLoopSeekPoint(music_seek::loopPoint);
 }
 rangeBegin=uint32_t(stream.loopBegin());rangeEnd=uint32_t(stream.loopEnd());fastPoint=stream.loopSeekReady();sourceFinished=false;return true;
}
class Sink final:public MusicSink {
 public:unsigned queued()const override{return unsigned(M5.Speaker.isPlaying(kChannel));}
 bool play(const int16_t* data,size_t count,uint32_t rate)override{return M5.Speaker.playRaw(data,count,rate,false,1,kChannel,false);}
} sink;
void worker(void*){
 FeatureMusicPart active=FeatureMusicPart::Off,configured=FeatureMusicPart::Hold;bool everQueued=false;uint32_t mayRestartAt=0;
 bool cached=false;size_t cachedAt=0,cachedEnd=0;uint32_t seenStop=stopGeneration.load();
 uint64_t beforeFrames=0;uint32_t beforeRetries=0;
 uint32_t seenAwardEpoch=requestWord.load(std::memory_order_acquire)>>requestPartBits;
 if(!M5.Speaker.isPlaying(kChannel))stopAcknowledged.store(seenStop,std::memory_order_release);
 for(;;){
  const uint32_t command=requestWord.load(std::memory_order_acquire);
  const auto wanted=requestPart(command);const uint32_t epoch=command>>requestPartBits;
  const uint32_t stop=stopGeneration.load(std::memory_order_acquire);
  if(stop!=seenStop){M5.Speaker.stop(kChannel);seenStop=stop;active=FeatureMusicPart::Off;currentPart=active;playing=finiteDraining=false;everQueued=cached=false;mayRestartAt=millis()+40;}
  // stop() publishes a mixer marker; it need not consume it synchronously.
  // Never acknowledge while a stopped buffer is still borrowed by lane6.
  if(!M5.Speaker.isPlaying(kChannel))stopAcknowledged.store(seenStop,std::memory_order_release);
  const bool newAward=epoch!=seenAwardEpoch;seenAwardEpoch=epoch;
  // Consume every epoch, including one cancelled by Off/Hold. An unrelated
  // later scope must never resurrect a stale event. Only a new award event
  // deliberately replaces a currently sounding phrase for a new action.
  if(newAward&&isAwardPart(wanted)&&active!=FeatureMusicPart::Off){
   M5.Speaker.stop(kChannel);active=FeatureMusicPart::Off;currentPart=active;
   playing=finiteDraining=false;everQueued=cached=false;mayRestartAt=millis()+40;
  }
  // Finite streaming awards have the same UI-scope tail contract as the
  // PSRAM cache. Explicit stop and a new award epoch still cancel above.
  const bool keepTail=cached?(cachedAt<cachedEnd||sink.queued()):
   (isAwardPart(active)&&(!stream.finished()||sink.queued()));
  if(active!=FeatureMusicPart::Off&&active!=wanted&&!keepTail){M5.Speaker.stop(kChannel);active=FeatureMusicPart::Off;currentPart=active;playing=finiteDraining=false;everQueued=cached=false;mayRestartAt=millis()+40;}
  if(wanted==FeatureMusicPart::Off&&active==FeatureMusicPart::Off){
   vTaskDelay(pdMS_TO_TICKS(4));continue;
  }
  if(active==FeatureMusicPart::Off){
   if(M5.Speaker.isPlaying(kChannel)||int32_t(millis()-mayRestartAt)<0){vTaskDelay(1);continue;}
   cached=awardBlock&&(wanted==FeatureMusicPart::MajorAward||wanted==FeatureMusicPart::BigAward);
   if(!cached&&configured!=wanted){
    const uint32_t began=micros();
    if(!configure(wanted)){ready=false;publishRequest(FeatureMusicPart::Off);vTaskDelay(1);continue;}
    const uint32_t used=micros()-began;if(used>maxConfigureUs)maxConfigureUs=used;
    configured=wanted;beforeFrames=beforeRetries=0;
   }
   if(requestWord.load(std::memory_order_acquire)!=command)continue;
   if(cached){
    cachedAt=0;cachedEnd=(wanted==FeatureMusicPart::MajorAward?feature_music::majorAwardEndFrame:feature_music::bigAwardEndFrame)-feature_music::awardBeginFrame;
    rangeBegin=feature_music::awardBeginFrame;rangeEnd=feature_music::awardBeginFrame+cachedEnd;finiteDraining=true;
   }else{
    if(!(wanted==FeatureMusicPart::TwinFree?twinStream.restart():stream.restart())){ready=false;publishRequest(FeatureMusicPart::Off);vTaskDelay(1);continue;}
    if(wanted==FeatureMusicPart::TwinFree){
     rangeBegin=twin_music_bank39::loopBegin;rangeEnd=twin_music_bank39::loopEnd;fastPoint=false;
    }
    beforeFrames=beforeRetries=0;finiteDraining=isAwardPart(wanted);
   }
   active=wanted;currentPart=active;sourceFinished=false;
  }
  if(cached){
   if(everQueued&&!sink.queued()){if(cachedAt<cachedEnd)++underruns;everQueued=false;}
   if(cachedAt<cachedEnd&&sink.queued()<2){
    const size_t count=std::min(size_t(MusicStream::kFrames),cachedEnd-cachedAt);
    if(sink.play(awardBlock+cachedAt,count,44100)){
     if(!cachedAt){++awardStarts;awardLastStartDelayMs=millis()-awardRequestedAt.load();}
     cachedAt+=count;queuedFrames+=count;everQueued=true;
    }else ++retries;
   }
   sourceFinished=cachedAt==cachedEnd;playing=bool(sink.queued());finiteDraining=cachedAt<cachedEnd||sink.queued();
   vTaskDelay(pdMS_TO_TICKS(2));continue;
  }
  if(active==FeatureMusicPart::TwinFree){
   if(everQueued&&!sink.queued()){++underruns;everQueued=false;}
   const uint32_t beforeLoops=twinStream.loops(),started=micros();
   const bool pumped=twinStream.pump(sink);const uint32_t elapsed=micros()-started;
   if(elapsed>maxPumpUs.load(std::memory_order_relaxed))maxPumpUs.store(elapsed,std::memory_order_relaxed);
   if(twinStream.loops()!=beforeLoops&&elapsed>maxWrapUs.load(std::memory_order_relaxed))maxWrapUs.store(elapsed,std::memory_order_relaxed);
   queuedFrames+=uint32_t(twinStream.framesQueued()-beforeFrames);retries+=twinStream.retries()-beforeRetries;loops+=twinStream.loops()-beforeLoops;
   beforeFrames=twinStream.framesQueued();beforeRetries=twinStream.retries();
   if(pumped)everQueued=true;
   playing=bool(sink.queued());sourceFinished=false;finiteDraining=false;
   vTaskDelay(pdMS_TO_TICKS(2));continue;
  }
  if(everQueued&&!sink.queued()){if(!stream.finished())++underruns;everQueued=false;}
  const uint32_t beforeLoops=stream.loops(),started=micros();const bool pumped=stream.pump(sink);const uint32_t elapsed=micros()-started;
  // Only this worker writes the peaks; main reads atomics for diagnostics.
  if(elapsed>maxPumpUs.load(std::memory_order_relaxed))maxPumpUs.store(elapsed,std::memory_order_relaxed);
  if(stream.loops()!=beforeLoops&&elapsed>maxWrapUs.load(std::memory_order_relaxed))maxWrapUs.store(elapsed,std::memory_order_relaxed);
  // Counters remain cumulative even when a new program reinitializes the
  // decoder. A finite source draining normally is not an underrun.
  queuedFrames+=uint32_t(stream.framesQueued()-beforeFrames);retries+=stream.retries()-beforeRetries;loops+=stream.loops()-beforeLoops;
  beforeFrames=stream.framesQueued();beforeRetries=stream.retries();
  if(pumped){playing=true;everQueued=true;}
  sourceFinished=stream.finished();finiteDraining=isAwardPart(active)&&(!stream.finished()||sink.queued());if(stream.finished()&&!sink.queued())playing=false;
  vTaskDelay(pdMS_TO_TICKS(2));
 }
}
}
bool audioMusicInit(){
 if(ready)return true;
 // Never format storage on boot. A missing/mismatched asset disables music
 // while the local game and financial checkpoint continue normally.
 if(!SPIFFS.begin(false))return false;auto file=SPIFFS.open("/feature_music.mp3","r");if(!file||file.size()!=music_asset::bytes)return false;
 compressed=static_cast<uint8_t*>(heap_caps_malloc(music_asset::bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));if(!compressed)return false;
 size_t read=0;while(read<music_asset::bytes){const size_t n=file.read(compressed+read,music_asset::bytes-read);if(!n)break;read+=n;}file.close();
 uint8_t hash[32];if(read!=music_asset::bytes||mbedtls_sha256_ret(compressed,read,hash,0)||memcmp(hash,music_asset::sha256,32)||!stream.init(compressed,read)||!stream.setLoopRegion(feature_music::loopBeginFrame,feature_music::loopEndFrame)){stream.close();heap_caps_free(compressed);compressed=nullptr;return false;}
 // A stale generated index must never seek into a different encoded file.
 // The verified legacy path remains available if its identity does not match.
 holdIndexMatches=read==music_seek::sourceBytes&&!memcmp(hash,music_seek::sha256,32);if(holdIndexMatches)stream.setLoopSeekPoint(music_seek::loopPoint);
 collectedSourceReady=read==collected_music_seek::sourceBytes&&!memcmp(hash,collected_music_seek::sha256,32);
 awardSourceReady=read==award_music_seek::sourceBytes&&!memcmp(hash,award_music_seek::sha256,32);
 fileBytes=read;
 // Decode once, before this stream has a worker/speaker consumer. The
 // independent finite voice can keep its tail while Hold music restarts.
 prepareCompletionCache();
 prepareAwardCache();
 if(!configure(FeatureMusicPart::Hold)){stream.close();heap_caps_free(compressed);compressed=nullptr;fileBytes=0;return false;}
 M5.Speaker.setChannelVolume(kChannel,0);
 if(xTaskCreatePinnedToCore(worker,"phoenix-music",8192,nullptr,2,nullptr,0)!=pdPASS){stream.close();heap_caps_free(compressed);compressed=nullptr;fileBytes=0;return false;}
 ready=true;return true;
}
namespace {
void publishRequest(FeatureMusicPart part,bool newAward){
 uint32_t previous=requestWord.load(std::memory_order_relaxed);
 for(;;){
  const uint32_t epoch=((previous>>requestPartBits)+unsigned(newAward))&requestEpochMask;
  const uint32_t next=(epoch<<requestPartBits)|uint32_t(part);
  if(isAwardPart(part)&&(newAward||part!=requestPart(previous)))awardRequestedAt=millis();
  if(requestWord.compare_exchange_weak(previous,next,std::memory_order_release,std::memory_order_relaxed))return;
 }
}
}
void audioMusicRequest(FeatureMusicPart part){
 const bool award=isAwardPart(part);
 const auto wanted=ready&&(part!=FeatureMusicPart::Collected||collectedSourceReady)&&(!award||awardSourceReady)&&(part!=FeatureMusicPart::TwinFree||twinSourceAvailable39())?part:FeatureMusicPart::Off;
 publishRequest(wanted);
}
void audioMusicBeginAward(FeatureMusicPart part){
 if(!isAwardPart(part)){audioMusicRequest(part);return;}
 if(!ready||!awardSourceReady){publishRequest(FeatureMusicPart::Off);return;}
 publishRequest(part,true);
}
uint32_t audioMusicStopFence(){publishRequest(FeatureMusicPart::Off);return stopGeneration.fetch_add(1,std::memory_order_release)+1;}
bool audioMusicStopFenceReady(uint32_t ticket){return !M5.Speaker.isPlaying(kChannel)&&(!ready.load()||stopAcknowledged.load(std::memory_order_acquire)==ticket);}
void audioMusicStop(){(void)audioMusicStopFence();}
void audioMusicVolume(uint8_t value){M5.Speaker.setChannelVolume(kChannel,value);}
MusicDiagnostics audioMusicDiagnostics(){return {ready.load(),requestPart(requestWord.load())!=FeatureMusicPart::Off,playing.load(),fileBytes,queuedFrames.load(),loops.load(),retries.load(),underruns.load(),rangeBegin.load(),rangeEnd.load(),fastPoint.load(),maxPumpUs.load(),maxWrapUs.load(),currentPart.load(),collectedSourceReady.load(),sourceFinished.load(),audioCompletionReady(),completionBlock?uint32_t(completion_cache::frames):0,completionBlock?uint32_t(completion_cache::storageBytes):0,completionFNV,awardSourceReady.load(),awardBlock!=nullptr,finiteDraining.load(),awardBlock?uint32_t(award_cache::storageBytes):0,awardFNV,awardStarts.load(),awardLastStartDelayMs.load(),maxConfigureUs.load(),
twin_music_bank39::rootAcceptedForDevice,ready.load()&&twinSourceAvailable39(),currentPart.load()==FeatureMusicPart::TwinFree,twin_music_bank39::hostReviewEnabled,
twin_music_bank39::sampleRate,twin_music_bank39::frames,twin_music_bank39::pcmBytes,twin_music_bank39::loopBegin,twin_music_bank39::loopEnd,twin_music_bank39::pcmFNV1a,0};}
#else
bool audioMusicInit(){return false;}
void audioMusicRequest(FeatureMusicPart){}
void audioMusicBeginAward(FeatureMusicPart){}
void audioMusicStop(){}
uint32_t audioMusicStopFence(){return 0;}
bool audioMusicStopFenceReady(uint32_t){return true;}
void audioMusicVolume(uint8_t){}
MusicDiagnostics audioMusicDiagnostics(){return {};}
#endif
