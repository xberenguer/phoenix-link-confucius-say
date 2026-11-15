#pragma once
#include <cstdint>
#include "audio.h"
struct MusicDiagnostics {bool ready,wanted,playing;uint32_t fileBytes,queuedFrames,loops,retries,underruns,loopBeginFrame,loopEndFrame;bool loopSeekReady;uint32_t maxPumpUs,maxWrapUs;FeatureMusicPart part;bool collectedSourceReady,sourceFinished,completionCacheReady;uint32_t completionFrames,completionBytes,completionFNV;bool awardSourceReady=false,awardCacheReady=false,finiteDraining=false;uint32_t awardCacheBytes=0,awardCacheFNV=0,awardStarts=0,awardLastStartDelayMs=0,maxConfigureUs=0;
// Appended only: Main/JSON publication is intentionally outside this adapter.
bool twinRootAcceptedForDevice=false,twinSourceReady=false,twinActive=false,twinHostReview=false;
uint32_t twinSampleRate=0,twinFrames=0,twinPCMBytes=0,twinLoopBeginFrame=0,twinLoopEndFrame=0,twinFNV=0,twinCacheBytes=0;
};
bool audioMusicInit();
// M5 squares channel gain. The quiet87s award passage needs255 versus180
// for active respin music:6.05dB more gain, with unchanged source samples.
inline uint8_t audioMusicProgramLevel(FeatureMusicPart part){return part==FeatureMusicPart::MajorAward||part==FeatureMusicPart::BigAward?255:180;}
void audioMusicRequest(FeatureMusicPart part);
// Explicit new award event. Repeated audioMusicRequest is idempotent; only
// this call can restart the same award part for a distinct action.
void audioMusicBeginAward(FeatureMusicPart part);
// Explicit mute/stop cancels a protected finite phrase. Ordinary UI changes
// let its already-triggered tail drain before another program takes the lane.
void audioMusicStop();
// Nonblocking exclusive handoff: the worker acknowledges only after lane6
// has released its borrowed buffers. This is not an acoustic/DMA timestamp.
uint32_t audioMusicStopFence();
bool audioMusicStopFenceReady(uint32_t ticket);
void audioMusicVolume(uint8_t value);
MusicDiagnostics audioMusicDiagnostics();
