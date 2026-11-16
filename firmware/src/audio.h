#pragma once
#include <stddef.h>
#include <stdint.h>

// Theme-specific PCM is registered by the asset/UI layer. Provenance records
// distinguish restored recordings from explicitly labeled reconstructions;
// this hardware backend does not generate or substitute game sounds.
enum class AudioCue : uint8_t {
  SpinClick, ReelSpin, ReelStop, Button, Win, PhoenixLand,
  FeatureStart, FeatureEnd, FeatureAnticipation,
  CountUp, MeterTrigger, FreeStart, HoldStart, BigWin, FreeEnd, CoinRain,
  PhoenixFollow, CollectedComplete, PhoenixThird, CashIn, WisdomThreeStart, Count
};

enum class AudioResult : uint8_t {
  None, Accepted, NotReady, Muted, Disabled, MissingClip, InvalidCue, QueueRejected, Deferred
};
enum class FeatureMusicPart : uint8_t {Off=0,Hold=1,Collected=2,MajorAward=3,BigAward=4,TwinFree=5};
// Backend acceptance is not proof of audible output. hardwareQueued counts
// actual M5 acceptances; deferred counts bounded backend FIFO acceptances.
struct AudioDiagnostics {
  uint32_t attempts;
  uint32_t accepted;
  uint32_t failed;
  AudioCue lastCue;
  AudioResult lastResult;
  uint32_t hardwareQueued;
  uint32_t deferred;
  uint32_t reelFades;
  uint32_t pending;
};
struct AudioOutputConfig {uint32_t sampleRate,bufferFrames,bufferCount;};
AudioOutputConfig audioOutputConfig();
// Software submission timing only: these do not measure speaker/DMA onset.
struct AudioLandingTiming {uint32_t events,maxQueueMs,maxLateMs,maxSubmitUs;};
AudioLandingTiming audioLandingTiming();

void audioInit(bool muted = false);
// Call once per app loop. Drives protected finite cue queues; no delays,
// allocation, or render/game callbacks.
void audioUpdate(uint32_t nowMs);
void audioSetMuted(bool muted);
void audioSetGameplayPaused(bool paused);
bool audioMuted();
bool audioReady();
uint32_t audioCueDurationMs(AudioCue cue);
// Finite feature music, including deferred stingers; used by autoplay intros.
bool audioFeatureBusy();
// Pending stops or finite landing tails; payout/feature transitions wait for
// these instead of cutting into a delayed reel-stop sequence.
bool audioLandingsBusy();
// Defaults: landing sounds ON, volume192/255. Set before audioInit;
// initialization preserves those selections. Muting never discards volume.
// Retained spin preference accessors are inert; no reel-bed cue is registered.
void audioSetSpinEnabled(bool enabled);
bool audioSpinEnabled();
void audioSetLandingsEnabled(bool enabled);
bool audioLandingsEnabled();
void audioSetVolume(uint8_t volume);
uint8_t audioVolume();
uint8_t audioMusicOutputLevel();
// Actual selected M5 mixer gain; the default StickS3 profile uses3.
uint8_t audioGainMagnification();
// Continuous user-specified Hold & Spin music uses its own streamed channel.
// beginAward identifies a NEW action, not a repeated scope assertion.
// Its explicit event may replace an older award; default requests retain tails.
void audioSetFeatureMusic(FeatureMusicPart part,bool beginAward=false);
// Boot-decoded clean completion sound and independent tail scratch, both
// immutable while borrowed by the speaker. No financial/game state involved.
bool audioRegisterCompletion(const int16_t* pcm,size_t frames,int16_t* tailScratch,size_t scratchFrames);
bool audioCompletionReady();
// Repeated UI requests identify one timed cue window. Mute/unmute resumes
// its remaining samples with a short antipop fade, never the full cue again.
// finishHoldMusic keeps the already-selected terminal Hold bed until this
// cue window, then waits for lane6's stop fence before queuing its live tail.
void audioSetCompletionCue(bool active,uint32_t elapsedMs,bool finishHoldMusic=false);
bool audioTestFeatureMusic();
// Audition one complete reconstructed Twin phrase; no game-state change.
bool audioTestTwinMusic();
// Audition the exact clean87s coin-celebration passage, with the same worker
// and finite-tail policy as its actual feature event. No game-state mutation.
bool audioTestAwardMusic();
AudioDiagnostics audioDiagnostics();
const char* audioResultName(AudioResult result);
// Stops overlapping cues and auditions the registered7.12s three-Wisdom entry.
// Respects master mute; legacy FreeStart remains for other feature routes.
bool audioTestJingle();
size_t audioConfiguredCount();
struct AudioCueInfo {uint32_t frames=0,rate=0,fnv=0;bool registered=false;};
// Read-only explicit diagnostics; hashes immutable registered PCM, never plays.
AudioCueInfo audioCueInfo(AudioCue cue);
bool audioRegister(AudioCue cue, const int16_t* samples, size_t sampleCount,
                   uint32_t sampleRate = 16000);
bool audioPlay(AudioCue cue);
// Ordered finite reel events. A late UI frame may submit several stops;
// preserve their order and tails instead of collapsing or replacing them.
bool audioQueueLanding(AudioCue cue);
// Timestamped physical arrivals enter an available voice immediately; the
// UI already controls their spacing. Busy voices retain the finite FIFO.
bool audioQueueLanding(AudioCue cue,uint32_t nowMs);
bool audioQueueLanding(AudioCue cue,uint32_t nowMs,uint32_t impactMs);
// Stops1/2; Phoenix arrivals5/7; menu2; wins/countup3;
// bonus stingers4; independent full feature stream6.
// Starts an action's landing scope; no continuous reel sound is submitted.
bool audioStartReels();
void audioStopReels();
void audioStopCue(AudioCue cue);
void audioStop();
