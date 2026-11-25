#include "audio.h"
#include "audio_music.h"
#include <M5Unified.h>
#include <algorithm>

namespace {
constexpr size_t kCueCount = static_cast<size_t>(AudioCue::Count);
struct Clip {
  const int16_t* samples = nullptr;
  size_t count = 0;
  uint32_t rate = 16000;
};
Clip clips[kCueCount];
bool muted = false, available = false,gameplayPaused141=false;
bool landingsEnabled = true;
uint8_t volume = 26;
constexpr uint8_t kVolumePreviewChannel = 0;
bool volumePreviewActive = false;
uint8_t gainMagnification = 0;
AudioDiagnostics diagnostics = {0, 0, 0, AudioCue::Count, AudioResult::None, 0, 0, 0, 0};
constexpr size_t kPendingCapacity=4;
struct Pending { AudioCue cue;uint32_t offset=0; };
Pending pending[2][kPendingCapacity];
uint8_t pendingCount[2]={};
uint32_t clockMs=0;
uint8_t stopVoice=0,phoenixVoice=0;
constexpr uint8_t kLandingCapacity=8;
constexpr uint32_t kLandingSpacingMs=140;
struct Landing {AudioCue cue;uint32_t at,impact;bool timed;};
Landing landingPending[kLandingCapacity]={};
AudioLandingTiming landingTiming={};
uint8_t landingPendingCount=0;
uint32_t lastLandingAt=0;
bool landingHasPlayed=false;
FeatureMusicPart featureMusicScope=FeatureMusicPart::Off;
FeatureMusicPart musicTestPart=FeatureMusicPart::Hold;
uint32_t musicTestUntil=0,musicSuppressUntil=0,musicClock=0;
uint8_t musicLevel=0;
uint8_t musicFadeLevel=180;
uint16_t musicGainQ8=0;
bool musicTesting=false,musicSuppressed=false;
int16_t* completionScratch=nullptr;size_t completionScratchFrames=0;
bool completionActive=false,completionDelivered=false;uint32_t completionElapsed=0;
bool completionFencePending=false;uint32_t completionFenceTicket=0,completionClock=0,completionFenceElapsed=0;
void clearLandings(){diagnostics.pending-=landingPendingCount;landingPendingCount=0;landingHasPlayed=false;}
void clearPending(){pendingCount[0]=pendingCount[1]=0;landingPendingCount=0;landingHasPlayed=false;diagnostics.pending=0;}

void applyOutputVolume() {
  M5.Speaker.setVolume(muted ? 0 : volumePreviewActive ? 255 : volume);
}

void finishVolumePreview() {
  if (!volumePreviewActive) return;
  volumePreviewActive = false;
  applyOutputVolume();
  if (M5.Speaker.isPlaying(kVolumePreviewChannel)) M5.Speaker.stop(kVolumePreviewChannel);
}

void interruptCuePlayback() {
  // The visual window still owns the completion. Its next update can resume
  // from the current offset after mute or an audition interrupts the speaker.
  completionDelivered = false;
  completionFencePending = false;
  finishVolumePreview();
  clearPending();
  if (available) for (uint8_t channel = 0; channel < 8; ++channel) {
    if (channel != 6) M5.Speaker.stop(channel);
  }
}

uint8_t channelFor(AudioCue cue) {
  switch (cue) {
    case AudioCue::SpinClick:
    case AudioCue::ReelSpin:
    case AudioCue::Selection: return 0;
    case AudioCue::VolumePreview: return kVolumePreviewChannel;
    case AudioCue::ReelStop:
    case AudioCue::PhoenixLand:
    case AudioCue::PhoenixFollow:
    case AudioCue::PhoenixThird: return 5;
    case AudioCue::Button: return 2;
    case AudioCue::Win:
    case AudioCue::CountUp: return 3;
    case AudioCue::CoinRain: return 3;
    default: return 4;
  }
}
bool isSpin(AudioCue cue) {
  return cue == AudioCue::ReelSpin || cue == AudioCue::SpinClick;
}
bool isLanding(AudioCue cue) {
  return cue == AudioCue::ReelStop || cue == AudioCue::PhoenixLand || cue == AudioCue::PhoenixFollow || cue == AudioCue::PhoenixThird;
}
bool finish(AudioResult result) {
  diagnostics.lastResult = result;
  if (result == AudioResult::Accepted || result == AudioResult::Deferred) { ++diagnostics.accepted; return true; }
  ++diagnostics.failed;
  return false;
}
bool queueNative(AudioCue cue,uint32_t repeat,bool replace,uint32_t offset=0) {
  const Clip&clip=clips[static_cast<size_t>(cue)];
  if(offset>=clip.count)return false;
  const int16_t* samples=clip.samples+offset;const size_t count=clip.count-offset;
  uint8_t channel=channelFor(cue);
  if(isLanding(cue)){
    // Longer clean Phoenix flourishes need more than two voices when
    // consecutive columns arrive170ms apart. Share the four finite landing
    // channels without replacing any audible tail or using the music lane.
    const uint8_t voices[4]={uint8_t(cue==AudioCue::ReelStop?1:5),uint8_t(cue==AudioCue::ReelStop?2:7),uint8_t(cue==AudioCue::ReelStop?5:1),uint8_t(cue==AudioCue::ReelStop?7:2)};
    const uint8_t count=4;
    uint8_t& next=cue==AudioCue::ReelStop?stopVoice:phoenixVoice;bool found=false;
    for(uint8_t i=0;i<count;++i){const uint8_t at=(next+i)%count,c=voices[at];if(!M5.Speaker.isPlaying(c)){channel=c;next=(at+1)%count;found=true;break;}}
    if(!found)return false; // coalesce excess arrivals, never truncate a voice.
    replace=false;
  }
  if(cue==AudioCue::CollectedComplete&&offset){
    if(!completionScratch||completionScratchFrames<count)return false;
    // Only entered with protected voice4 idle. A pending tail carries its
    // offset, so scratch is never rewritten while an old tail is sounding.
    std::copy(samples,samples+count,completionScratch);
    const size_t fade=std::min(count,size_t(clip.rate*4/1000));
    for(size_t i=0;i<fade;++i)completionScratch[i]=int16_t(int32_t(completionScratch[i])*int32_t(i)/int32_t(std::max(size_t(1),fade-1)));
    samples=completionScratch;
  }
  // Match the previous180/255 streamed-lane level without changing the
  // original PCM. Other protected stingers retain their full channel gain.
  if(channel==4)M5.Speaker.setChannelVolume(4,cue==AudioCue::CollectedComplete?180:255);
  const bool preview=cue==AudioCue::VolumePreview;
  if(preview){
    // The startup click uses fixed loudness. Never boost a voice already
    // playing on another channel when an audition is active.
    for(uint8_t other=1;other<8;++other){
      if(M5.Speaker.isPlaying(other)){finishVolumePreview();return false;}
    }
    volumePreviewActive=true;
    applyOutputVolume();
  }else finishVolumePreview();
  const uint32_t submittedAt=micros();
  const bool queued=M5.Speaker.playRaw(samples,count,clip.rate,false,repeat,channel,replace);
  if(preview&&!queued)finishVolumePreview();
  if(isLanding(cue))landingTiming.maxSubmitUs=std::max(landingTiming.maxSubmitUs,uint32_t(micros()-submittedAt));
  if(queued)++diagnostics.hardwareQueued;
  return queued;
}

void pumpProtectedCue(uint8_t queue, bool cashInOnly = false) {
  if (!pendingCount[queue] || M5.Speaker.isPlaying(queue + 3)) return;
  auto* const first = pending[queue];
  auto* const end = first + pendingCount[queue];
  auto* const next = cashInOnly
      ? std::find_if(first, end, [](const Pending& entry) { return entry.cue == AudioCue::CashIn; })
      : first;
  if (next == end || !queueNative(next->cue, 1, false, next->offset)) return;
  std::move(next + 1, end, next);
  --pendingCount[queue];
  --diagnostics.pending;
}

void pumpExclusiveCompletion(uint32_t nowMs);
void pumpLandings(uint32_t nowMs){
 if(!available||muted||!landingPendingCount||
    (!landingPending[0].timed&&landingHasPlayed&&uint32_t(nowMs-lastLandingAt)<kLandingSpacingMs))return;
 const auto next=landingPending[0];
 if(!queueNative(next.cue,1,false))return;
 ++landingTiming.events;
 landingTiming.maxQueueMs=std::max(landingTiming.maxQueueMs,uint32_t(nowMs-next.at));
 landingTiming.maxLateMs=std::max(landingTiming.maxLateMs,uint32_t(nowMs-next.impact));
 for(uint8_t i=1;i<landingPendingCount;++i)landingPending[i-1]=landingPending[i];
 --landingPendingCount;--diagnostics.pending;lastLandingAt=nowMs;landingHasPlayed=true;
}
bool play(AudioCue cue, uint32_t repeat, bool orderedLanding=false,uint32_t offset=0,
          bool timed=false,uint32_t at=0,uint32_t impact=0) {
  ++diagnostics.attempts;
  diagnostics.lastCue = cue;
  const size_t index = static_cast<size_t>(cue);
  if (index >= kCueCount) return finish(AudioResult::InvalidCue);
  if (!available) return finish(AudioResult::NotReady);
  if (muted) return finish(AudioResult::Muted);
  if (isSpin(cue) || (isLanding(cue) && !landingsEnabled))
    return finish(AudioResult::Disabled);
  const Clip& clip = clips[index];
  if (!clip.samples || !clip.count||offset>=clip.count) return finish(AudioResult::MissingClip);
  if(orderedLanding){
    if(!isLanding(cue))return finish(AudioResult::InvalidCue);
    if(landingPendingCount>=kLandingCapacity)return finish(AudioResult::QueueRejected);
    landingPending[landingPendingCount++]={cue,timed?at:clockMs,timed?impact:clockMs,timed};
    ++diagnostics.pending;++diagnostics.deferred;
    return finish(AudioResult::Deferred);
  }
  const auto channel=channelFor(cue);
  // Finite win/bonus cues finish naturally. A later event enters a
  // bounded FIFO, instead of always replacing the audible current stinger.
  if((channel==3||channel==4) && (M5.Speaker.isPlaying(channel)||pendingCount[channel-3])) {
    const auto q=channel-3;
    if(pendingCount[q]>=kPendingCapacity)return finish(AudioResult::QueueRejected);
    pending[q][pendingCount[q]++]={cue,offset};++diagnostics.deferred;++diagnostics.pending;
    return finish(AudioResult::Deferred);
  }
  return finish(queueNative(cue,repeat,channel<3,offset)?AudioResult::Accepted:AudioResult::QueueRejected);
}
void pumpExclusiveCompletion(uint32_t nowMs){
 if(!completionFencePending||!available||muted||musicSuppressed||!audioCompletionReady())return;
 const auto& clip=clips[static_cast<size_t>(AudioCue::CollectedComplete)];
 const uint64_t elapsed=uint64_t(completionFenceElapsed)+uint32_t(nowMs-completionClock);
 const uint64_t offset=elapsed*clip.rate/1000;
 if(offset>=clip.count){completionDelivered=true;completionFencePending=false;return;}
 if(!audioMusicStopFenceReady(completionFenceTicket)||M5.Speaker.isPlaying(4)||pendingCount[1])return;
 // Do not put this clocked tail behind a busy voice: its offset would become
 // stale. Retry without blocking; preserve every older protected stinger.
 if(play(AudioCue::CollectedComplete,1,false,uint32_t(offset))){completionDelivered=true;completionFencePending=false;}
}
}

void audioInit(bool value) {
  finishVolumePreview();
  muted = value;gameplayPaused141=false;
  landingTiming={};
  completionActive=completionDelivered=false;completionElapsed=0;completionFencePending=false;completionClock=clockMs;
  clearPending();
  auto config = M5.Speaker.config();
  // Pinned M5Unified StickS3 profile: BCK17, DOUT14, magnification1.
  // Gain8 could saturate overlapping finite voices at high master volume.
  // Gain3 has headroom for all8configured channels at their registered peaks,
  // including music plus2Phoenix tones. Preserve codec pins/rate/settings.
  if (config.pin_bck == 17 && config.pin_data_out == 14 && config.magnification == 1) {
    M5.Speaker.end();  // gain is captured at mixer-task start; restart safely.
    config.magnification = 3;
    config.sample_rate = 44100;
    M5.Speaker.config(config);
  }
  gainMagnification = config.magnification;
  applyOutputVolume();
  available = M5.Speaker.begin();
  // M5Unified squares both master and channel settings. The former160/255
  // landing gain attenuated Phoenix notes by8.1dB relative to feature stingers.
  // All four interchangeable landing voices now have equal full gain; cue
  // PCM envelopes determine their relative dynamics.
  M5.Speaker.setChannelVolume(0, 255);
  M5.Speaker.setChannelVolume(1, 255);
  M5.Speaker.setChannelVolume(2, 255);
  M5.Speaker.setChannelVolume(3, 255);
  M5.Speaker.setChannelVolume(4, 255);
  M5.Speaker.setChannelVolume(5,255);
  M5.Speaker.setChannelVolume(7,255);
  audioMusicInit();musicLevel=0;musicGainQ8=0;audioMusicVolume(0);musicClock=clockMs;
}
void audioSetMuted(bool value) {
  muted = value;
  applyOutputVolume();
  if(muted)interruptCuePlayback();
  audioMusicRoute(musicTesting?musicTestPart:featureMusicScope,muted||(gameplayPaused141&&!musicTesting),musicTesting);
}
void audioSetGameplayPaused(bool paused){
 gameplayPaused141=paused;
 if(!paused){finishVolumePreview();musicTesting=false;musicSuppressed=false;}
 audioMusicRoute(musicTesting?musicTestPart:featureMusicScope,muted||(paused&&!musicTesting),musicTesting);
}
bool audioMuted() { return muted; }
bool audioReady() { return available; }
uint32_t audioCueDurationMs(AudioCue cue) {
  const auto index=static_cast<size_t>(cue);
  if(index>=kCueCount||!clips[index].rate)return 0;
  const uint64_t ms=(uint64_t(clips[index].count)*1000+clips[index].rate-1)/clips[index].rate;
  return ms>UINT32_MAX?UINT32_MAX:uint32_t(ms);
}
bool audioFeatureBusy() { return available&&!muted&&(pendingCount[1]||landingPendingCount||M5.Speaker.isPlaying(4)); }
bool audioLandingsBusy(){return available&&!muted&&(landingPendingCount||M5.Speaker.isPlaying(1)||M5.Speaker.isPlaying(2)||M5.Speaker.isPlaying(5)||M5.Speaker.isPlaying(7));}
void audioSetSpinEnabled(bool) {}
bool audioSpinEnabled() { return false; }
void audioSetLandingsEnabled(bool value) {
  landingsEnabled = value;
  if(!value){clearLandings();if(available)for(uint8_t c:{1,2,5,7})M5.Speaker.stop(c);}
}
bool audioLandingsEnabled() { return landingsEnabled; }
void audioSetVolume(uint8_t value) {
  volume = value;
  applyOutputVolume();
}
uint8_t audioVolume() { return volume; }
uint8_t audioMusicOutputLevel(){return musicLevel;}
uint8_t audioGainMagnification() { return gainMagnification; }
AudioOutputConfig audioOutputConfig(){const auto c=M5.Speaker.config();return {c.sample_rate,uint32_t(c.dma_buf_len),uint32_t(c.dma_buf_count)};}
AudioDiagnostics audioDiagnostics() { return diagnostics; }
AudioLandingTiming audioLandingTiming(){return landingTiming;}
const char* audioResultName(AudioResult result) {
  switch (result) {
    case AudioResult::None: return "none";
    case AudioResult::Accepted: return "queued";
    case AudioResult::NotReady: return "not ready";
    case AudioResult::Muted: return "muted";
    case AudioResult::Disabled: return "disabled";
    case AudioResult::MissingClip: return "missing clip";
    case AudioResult::InvalidCue: return "invalid cue";
    case AudioResult::QueueRejected: return "queue rejected";
    case AudioResult::Deferred: return "deferred";
  }
  return "unknown";
}
bool audioTestJingle() {
  interruptCuePlayback();musicTesting=false;
  musicSuppressUntil=clockMs+audioCueDurationMs(AudioCue::WisdomThreeStart)+500;
  musicSuppressed=true;
  return audioPlay(AudioCue::WisdomThreeStart);
}
void audioSetFeatureMusic(FeatureMusicPart part,bool beginAward){
 if(part!=FeatureMusicPart::Off)finishVolumePreview();
 featureMusicScope=part;
 if(available&&!muted&&!gameplayPaused141&&!musicSuppressed&&!musicTesting&&!completionFencePending&&part!=FeatureMusicPart::Off){
  // Both programs begin with a scored attack. Preserve that opening and
  // submit the requested stage before an expensive transition render.
  const auto live=audioMusicDiagnostics();const auto level=audioMusicProgramLevel(live.finiteDraining?live.part:part);
  musicFadeLevel=level;musicGainQ8=uint16_t(level)*256;musicLevel=level;audioMusicVolume(level);
  if(beginAward)audioMusicBeginAward(part);else audioMusicRequest(part);
 }
}
bool audioRegisterCompletion(const int16_t* pcm,size_t frames,int16_t* scratch,size_t scratchFrames){
 if(!pcm&&!frames&&!scratch&&!scratchFrames){completionFencePending=false;completionScratch=nullptr;completionScratchFrames=0;completionDelivered=false;return audioRegister(AudioCue::CollectedComplete,nullptr,0,44100);}
 if(!pcm||!frames||!scratch||pcm==scratch||scratchFrames<frames)return false;
 if(!audioRegister(AudioCue::CollectedComplete,pcm,frames,44100))return false;
 completionScratch=scratch;completionScratchFrames=scratchFrames;completionDelivered=false;completionFencePending=false;return true;
}
bool audioCompletionReady(){const auto& c=clips[static_cast<size_t>(AudioCue::CollectedComplete)];return c.samples&&c.count&&completionScratch&&completionScratchFrames>=c.count;}
void audioSetCompletionCue(bool active,uint32_t elapsed,bool finishHoldMusic){
 // An unheard request belongs to this visual window. Closing it must not
 // delay new music or resurrect a completion after a manual phase skip.
 // Already-submitted voice4 audio retains its ordinary finite-tail policy.
 if(!active){completionActive=completionDelivered=false;completionElapsed=0;completionFencePending=false;return;}
 if(!completionActive||elapsed<completionElapsed){completionDelivered=false;completionFencePending=false;}
 completionActive=true;completionElapsed=elapsed;
 if(completionDelivered||!available||muted||gameplayPaused141||musicSuppressed||!audioCompletionReady())return;
 const auto& clip=clips[static_cast<size_t>(AudioCue::CollectedComplete)];
 const uint64_t offset=uint64_t(elapsed)*clip.rate/1000;
 if(offset>=clip.count){completionDelivered=true;completionFencePending=false;return;}
 // An already-triggered finite award keeps its existing protected tail.
 if(finishHoldMusic&&(completionFencePending||!audioMusicDiagnostics().finiteDraining)){
  if(!completionFencePending){
   completionFencePending=true;featureMusicScope=FeatureMusicPart::Off;musicTesting=false;
   musicGainQ8=0;musicLevel=0;audioMusicVolume(0);
   completionFenceTicket=audioMusicStopFence();
  }
  completionClock=clockMs;completionFenceElapsed=elapsed;
  pumpExclusiveCompletion(clockMs);return;
 }
 // Recheck the current visual offset on the next update if a stinger owns
 // this voice. A deferred FIFO entry would retain an outdated offset.
 if(M5.Speaker.isPlaying(4)||pendingCount[1])return;
 if(play(AudioCue::CollectedComplete,1,false,uint32_t(offset)))completionDelivered=true;
}
bool audioTestFeatureMusic(){if(!available||muted||!audioMusicDiagnostics().ready)return false;finishVolumePreview();musicTestPart=FeatureMusicPart::Hold;musicTestUntil=clockMs+15000;musicTesting=true;musicSuppressed=false;return true;}
bool audioTestTwinMusic(){if(!available||muted||!audioMusicDiagnostics().twinSourceReady)return false;interruptCuePlayback();musicTestPart=FeatureMusicPart::TwinFree;musicTestUntil=clockMs+29000;musicTesting=true;musicSuppressed=false;return true;}
bool audioTestAwardMusic(){if(!available||muted||!audioMusicDiagnostics().awardSourceReady)return false;interruptCuePlayback();musicTestPart=FeatureMusicPart::BigAward;musicTestUntil=clockMs+8000;musicTesting=true;musicSuppressed=false;return true;}
size_t audioConfiguredCount() {
  size_t count = 0;
  for (const Clip& clip : clips) count += clip.samples && clip.count;
  return count;
}
AudioCueInfo audioCueInfo(AudioCue cue){
 const size_t index=static_cast<size_t>(cue);if(index>=kCueCount)return {};
 const auto& clip=clips[index];if(!clip.samples||!clip.count)return {};
 const auto* bytes=reinterpret_cast<const uint8_t*>(clip.samples);uint32_t h=2166136261u;
 for(size_t i=0;i<clip.count*sizeof(int16_t);++i)h=(h^bytes[i])*16777619u;
 return {uint32_t(clip.count),clip.rate,h,true};
}
bool audioRegister(AudioCue cue, const int16_t* samples, size_t sampleCount,
                   uint32_t sampleRate) {
  const size_t index = static_cast<size_t>(cue);
  if (index >= kCueCount || isSpin(cue) || sampleRate < 8000 || sampleRate > 48000 ||
      (sampleCount && !samples)) return false;
  // Zero-copy registration: static generated PCM outlives asynchronous play.
  if(cue==AudioCue::CollectedComplete){completionFencePending=false;completionDelivered=false;}
  clips[index] = {samples, sampleCount, sampleRate};
  return true;
}
bool audioPlay(AudioCue cue) { return play(cue, 1); }
bool audioQueueLanding(AudioCue cue){return play(cue,1,true);}
bool audioQueueLanding(AudioCue cue,uint32_t nowMs){return audioQueueLanding(cue,nowMs,nowMs);}
bool audioQueueLanding(AudioCue cue,uint32_t nowMs,uint32_t impactMs){const bool accepted=play(cue,1,true,0,true,nowMs,impactMs);if(accepted)pumpLandings(nowMs);return accepted;}
bool audioStartReels() {
  // A new action cancels unheard stops while sounding finite voices finish.
  clearLandings();
  return false;
}
void audioStopReels() {}
void audioUpdate(uint32_t nowMs) {
  clockMs=nowMs;
  if(volumePreviewActive&&!M5.Speaker.isPlaying(kVolumePreviewChannel))finishVolumePreview();
  const uint32_t elapsed=nowMs-musicClock;musicClock=nowMs;
  if(musicTesting&&int32_t(nowMs-musicTestUntil)>=0)musicTesting=false;
  if(musicSuppressed&&int32_t(nowMs-musicSuppressUntil)>=0)musicSuppressed=false;
  const auto liveMusic=audioMusicDiagnostics();const bool keepTail=liveMusic.finiteDraining;
  const bool wants=available&&!muted&&(featureMusicScope!=FeatureMusicPart::Off||musicTesting||keepTail)&&!musicSuppressed&&!completionFencePending;
  const auto part=musicTesting?musicTestPart:featureMusicScope;
  if(wants){musicFadeLevel=audioMusicProgramLevel(keepTail?liveMusic.part:part);musicGainQ8=uint16_t(musicFadeLevel)*256;}
  else {const unsigned down=std::min(elapsed,250u)*musicFadeLevel*256/250;musicGainQ8=down>=musicGainQ8?0:uint16_t(musicGainQ8-down);}
  musicLevel=musicGainQ8/256;
  audioMusicVolume(musicLevel);
  const bool paused=muted||(gameplayPaused141&&!musicTesting);
  audioMusicRoute(paused?featureMusicScope:wants?part:FeatureMusicPart::Off,paused,musicTesting);
  if(!available||muted)return;
  if(gameplayPaused141&&!musicTesting){
    // Funding can finish on a menu or the startup chooser. Let it confirm
    // as soon as voice 4 is free while gameplay stingers stay paused.
    pumpProtectedCue(1, true);
    return;
  }
  pumpExclusiveCompletion(nowMs);
  pumpLandings(nowMs);
  pumpProtectedCue(0);
  pumpProtectedCue(1);
}
void audioStopCue(AudioCue cue) {
  if (available && static_cast<size_t>(cue) < kCueCount) {
    const auto channel=channelFor(cue);
    if(channel==kVolumePreviewChannel&&volumePreviewActive){finishVolumePreview();return;}
    if(cue==AudioCue::CollectedComplete){completionFencePending=false;completionDelivered=true;}
    if(isLanding(cue)){clearLandings();for(uint8_t c:{1,2,5,7})M5.Speaker.stop(c);return;}
    if(channel==3||channel==4){diagnostics.pending-=pendingCount[channel-3];pendingCount[channel-3]=0;}
    M5.Speaker.stop(channel);
  }
}
void audioStop() {
  finishVolumePreview();
  featureMusicScope=FeatureMusicPart::Off;
  completionDelivered=false;completionFencePending=false;
  audioMusicStop();musicTesting=false;musicLevel=0;musicGainQ8=0;audioMusicVolume(0); clearPending();if (available) M5.Speaker.stop(); }
