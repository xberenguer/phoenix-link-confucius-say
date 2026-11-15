#include <M5Unified.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp32-hal-psram.h>
#include <driver/usb_serial_jtag.h>
#include <hal/usb_serial_jtag_ll.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "ui.h"
#include "phoenix_random.h"
#include "platform_entropy125.h"
#include "sage_qa_api.h"
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "build_version.h"
#include "audio.h"
#include "audio_music.h"
#include "game_black_tap.h"
#include "entry_art.h"
#include "collect_art.h"

namespace {
constexpr uint32_t kHoldMs = 850;
constexpr size_t kFrameBytes = 240u * 135u * sizeof(uint16_t);
constexpr size_t kUsbPacketBytes = 64;
constexpr size_t kUsbBudgetBytes = 512;
constexpr size_t kPullChunkBytes = 512;
constexpr uint32_t kFrameIdleMs = 2000;
constexpr uint32_t kFrameDeadlineMs = 15000;
bool statusLedDisabled = false;
bool powerButtonDefault=false;
uint8_t powerButtonConfigBefore=0,powerButtonConfigAfter=0;
uint8_t statusLedCfgBefore = 0, statusLedCfgAfter = 0;
bool aLongUsed = false;
bool bLongUsed = false;
bool chordSeen = false;
bool qaInputFence=false;
// Native USB batches may outlive the UI release frame. No physical-input dependency.
bool returnUsbDrain86=false;
UiSageSceneTest lastSageTest;
uint32_t lastSageJob=0,sageLargestBefore=0,sageLargestAfter=0,sageEntryHeadroom=0,sageMinimumBefore=0,sageMinimumAfter=0;
GameBlackTap blackTap;
uint32_t aStarted = 0;
uint32_t bStarted = 0;
String usbOutput;
size_t usbOutputSent = 0;
char usbInput[32];
size_t usbInputLength = 0;
bool usbInputOverflow = false;
bool usbDriverReady = false;
int usbInstallError = -1;
bool usbTxNeedsKick = false;
uint32_t usbTxProgressAt = 0;
uint32_t usbTxKickAt = 0;
bool usbStatusPending = false;
bool usbBusyPending = false;
UiEntryBench lastEffectBench;uint32_t lastEffectBenchJob=0;bool effectBenchReportPending=false;
uint32_t usbQueueDrops = 0;
uint8_t* frameSnapshot = nullptr;
bool frameActive = false;
size_t frameSent = 0;
size_t frameEnd = 0;
uint32_t frameId = 0;
uint32_t frameHash = 0;
uint32_t frameStartedAt = 0;
uint32_t frameProgressAt = 0;
uint32_t framesCompleted = 0;
uint32_t framesAborted = 0;
size_t lastFrameSent = 0;
bool rebootPending = false;
uint32_t rebootRequestedAt = 0;
uint32_t rebootDrainedAt = 0;

// A new process is a new, nonpersistent game. No NVS bank/scene reads,
// writes or imports exist here; the RNG service never enters a snapshot.
phoenix::random::Service outcomeRandom125;
platform_entropy125::Source entropySource125(platform_entropy125::nativeHooks(),platform_entropy125::pinnedSdk);
bool sessionReady125=false;
void saveOptions(){} // Deliberately session-only, including cosmetic options.
void dispatchUiInput59(void(*input)()){input();}
bool sessionOutcome125(){
 if(uiReturnSelectionCheckpointPending())returnUsbDrain86=true;
 return sessionReady125&&outcomeRandom125.healthy()&&uiRandomHealthy();
}

void queueUsb(const String& line) {
  if (!usbDriverReady) return;
  if (usbOutput.length() + line.length() + 1 > 4096) { ++usbQueueDrops; return; }
  usbOutput += line;
  usbOutput += '\n';
}

void reportStatus() {
  String line = String("{\"firmware\":\"phoenix-confucius\",\"version\":\"")+build_version::firmware+"\",\"board\":" + unsigned(M5.getBoard());
  line += ",\"boardMatch\":";
  line += M5.getBoard() == m5::board_t::board_M5StickS3 ? "true" : "false";
  line += ",\"statusLedDisabled\":"; line += statusLedDisabled ? "true" : "false";
  line += String(",\"powerButtonDefault\":")+(powerButtonDefault?"true":"false");
  line += String(",\"statusLedCfgBefore\":") + unsigned(statusLedCfgBefore);
  line += String(",\"statusLedCfgAfter\":") + unsigned(statusLedCfgAfter);
  line += String(",\"displayWidth\":") + M5.Display.width();
  line += String(",\"displayHeight\":") + M5.Display.height();
  line += String(",\"displayRotation\":") + unsigned(M5.Display.getRotation());
  line += String(",\"psramBytes\":") + ESP.getPsramSize();
  line += String(",\"freePsramBytes\":") + ESP.getFreePsram();
  line += String(",\"heapBytes\":") + ESP.getFreeHeap();
  line += String(",\"minFreeHeapBytes\":") + ESP.getMinFreeHeap();
  line += String(",\"uptimeMs\":")+millis()+",\"resetReason\":"+unsigned(esp_reset_reason())+",\"lastBenchJob\":"+lastEffectBenchJob+",\"lastBenchFrames\":"+lastEffectBench.frames;
  line += String(",\"loopStackHighWaterMark\":") + unsigned(uxTaskGetStackHighWaterMark(nullptr));
  line += ",\"statePersistence\":\"session-only\",\"freshCredits\":1000,\"restoredAtBoot\":false,\"saveAvailable\":false";
  line += String(",\"rngHealthy\":")+(uiRandomHealthy()?"true":"false")+",\"rngFault\":"+unsigned(uiRandomFault())+",\"entropyLeaseFault\":"+unsigned(entropySource125.fault());
  char leases125[64];snprintf(leases125,sizeof(leases125),",\"entropyCaptures\":%llu",(unsigned long long)entropySource125.captures());line+=leases125;
  line += ",\"overlay\":"; line += uiOverlayVisible() ? "true" : "false";
  line += ",\"audioReady\":"; line += audioReady() ? "true" : "false";
  line += String(",\"audioConfiguredCues\":") + unsigned(audioConfiguredCount());
  line += ",\"muted\":"; line += audioMuted() ? "true" : "false";
  line += String(",\"audioVolume\":") + unsigned(audioVolume());
  line += String(",\"audioGainMagnification\":") + unsigned(audioGainMagnification());
  line += String(",\"testJingleMs\":") + audioCueDurationMs(AudioCue::WisdomThreeStart);
  const auto output=audioOutputConfig();line+=String(",\"audioOutputRate\":")+output.sampleRate;line+=String(",\"audioDmaFrames\":")+output.bufferFrames;line+=String(",\"audioDmaBuffers\":")+output.bufferCount;
  const auto audio = audioDiagnostics();
  line += String(",\"audioAttempts\":") + audio.attempts;
  line += String(",\"audioAccepted\":") + audio.accepted;
  line += String(",\"audioFailed\":") + audio.failed;
  line += String(",\"audioHardwareQueued\":") + audio.hardwareQueued;
  const auto landing=audioLandingTiming();
  line += String(",\"landingEvents\":") + landing.events;
  line += String(",\"landingMaxQueueMs\":") + landing.maxQueueMs;
  line += String(",\"landingMaxLateMs\":") + landing.maxLateMs;
  line += String(",\"landingMaxSubmitUs\":") + landing.maxSubmitUs;
  const auto effect=uiEffectCache();
  line += String(",\"effectCacheReady\":") + (effect.ready?"true":"false");
  line += String(",\"effectCacheBytes\":") + effect.bytes;
  line += String(",\"effectCacheRate\":") + effect.rate;
  line += String(",\"effectCacheBuildUs\":") + effect.buildUs;
  line += String(",\"fireAlphaReady\":") + (effect.alphaReady?"true":"false");
  line += String(",\"fireAlphaChecks\":") + effect.alphaChecks;
  line += String(",\"fireAlphaBoundaryFNV\":") + effect.alphaBoundaryFNV;
  line+=String(",\"pickupCacheReady\":")+(effect.pickupReady?"true":"false")+",\"pickupCacheBytes\":"+effect.pickupBytes+",\"pickupCacheHits\":"+effect.pickupHits+",\"pickupCacheMisses\":"+effect.pickupMisses;
  line += String(",\"audioDeferred\":") + audio.deferred;
  line += String(",\"audioPending\":") + audio.pending;
  line += String(",\"audioReelFades\":") + audio.reelFades;
  line += String(",\"audioLastCue\":") + unsigned(audio.lastCue);
  line += ",\"audioLastResult\":\""; line += audioResultName(audio.lastResult); line += '"';
  const auto music=audioMusicDiagnostics();line+=String(",\"musicReady\":")+(music.ready?"true":"false");line+=String(",\"musicPlaying\":")+(music.playing?"true":"false");line+=String(",\"musicFramesQueued\":")+music.queuedFrames;line+=String(",\"musicUnderruns\":")+music.underruns;line+=String(",\"musicLoops\":")+music.loops;line+=String(",\"musicLoopBeginFrame\":")+music.loopBeginFrame;line+=String(",\"musicLoopEndFrame\":")+music.loopEndFrame;
  line+=String(",\"musicLoopSeekReady\":")+(music.loopSeekReady?"true":"false");line+=String(",\"musicMaxPumpUs\":")+music.maxPumpUs;line+=String(",\"musicMaxWrapUs\":")+music.maxWrapUs;
  line+=String(",\"musicAwardSourceReady\":")+(music.awardSourceReady?"true":"false");
  line+=String(",\"awardCacheReady\":")+(music.awardCacheReady?"true":"false")+",\"awardCacheBytes\":"+music.awardCacheBytes+",\"awardCacheFNV\":"+music.awardCacheFNV+",\"awardStarts\":"+music.awardStarts+",\"awardLastStartDelayMs\":"+music.awardLastStartDelayMs+",\"musicMaxConfigureUs\":"+music.maxConfigureUs+",\"musicFiniteDraining\":"+(music.finiteDraining?"true":"false");
  line+=String(",\"musicPart\":")+unsigned(music.part);line+=String(",\"musicCollectedSourceReady\":")+(music.collectedSourceReady?"true":"false");line+=String(",\"musicSourceFinished\":")+(music.sourceFinished?"true":"false");
  line+=String(",\"completionCacheReady\":")+(music.completionCacheReady?"true":"false");line+=String(",\"completionCacheFrames\":")+music.completionFrames;line+=String(",\"completionCacheBytes\":")+music.completionBytes;line+=String(",\"completionCacheFNV\":")+music.completionFNV;
  line += ",\"usbTransport\":\"idf-usb-serial-jtag\"";
  line += String(",\"usbInitError\":") + usbInstallError;
  line += ",\"frameActive\":"; line += frameActive ? "true" : "false";
  line += String(",\"frameId\":") + frameId;
  line += String(",\"frameSent\":") + unsigned(frameActive ? frameSent : lastFrameSent);
  line += String(",\"framesCompleted\":") + framesCompleted;
  line += String(",\"framesAborted\":") + framesAborted;
  line += String(",\"usbQueueDrops\":") + usbQueueDrops;
  line += ",\"entryArtReady\":"; line += entry_art::ready() ? "true" : "false";
  line += ",\"entryArtBytes\":"; line += entry_art::storageBytes();
   line += ",\"collectArtReady\":"; line += collect_art::ready() ? "true" : "false";
   line += ",\"collectArtBytes\":"; line += collect_art::storageBytes();
   line += ",\"effectBenchFrames\":"; line += kUiEffectBenchmarkFrames;
   line += ",\"effectParityFrames\":"; line += kUiEffectParityFrames;
  line += ",\"entryArtDecodedFNV\":"; line += entry_art::decodedHash();
  line += ",\"legacyStateImports\":false,\"rngStateExport\":false";
  line += ",\"game\":"; line += uiSnapshot(); line += '}';
  queueUsb(line);
}

uint32_t frameChecksum(const uint8_t* source, size_t size) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; ++i) { hash ^= source[i]; hash *= 16777619u; }
  return hash;
}

void abortFrame(const char* reason) {
  if (!frameActive) return;
  frameActive = false;
  lastFrameSent = frameSent;
  ++framesAborted;
  // An interrupted raw transfer is explicitly invalid. A leading newline lets
  // a reconnecting host resynchronize after any already queued binary bytes.
  usbOutput = "";
  usbOutputSent = 0;
  char message[160];
  snprintf(message, sizeof(message), "\n{\"type\":\"frame-aborted\",\"id\":%u,\"sent\":%u,\"reason\":\"%s\"}", frameId, unsigned(frameSent), reason);
  queueUsb(message);
}

void beginFrameTransfer(bool pull = false) {
  const uint16_t* source = uiFramePixels();
  if (!source || usbOutput.length() > 3072) {
    queueUsb("{\"error\":\"frame unavailable or USB busy\"}");
    return;
  }
  if (!frameSnapshot) frameSnapshot = static_cast<uint8_t*>(ps_malloc(kFrameBytes));
  if (!frameSnapshot) {
    queueUsb("{\"error\":\"frame snapshot allocation failed\"}");
    return;
  }
  // UI returns native RGB565 words. ESP32-S3 stores them little-endian.
  // Copy synchronously so subsequent animations cannot alter this transfer.
  memcpy(frameSnapshot, source, kFrameBytes);
  frameHash = frameChecksum(frameSnapshot, kFrameBytes);
  char header[256];
  snprintf(header, sizeof(header), "{\"type\":\"frame\",\"id\":%u,\"width\":240,\"height\":135,\"format\":\"RGB565LE\",\"bytes\":%u,\"fnv1a32\":%u,\"actions\":%llu,\"transfer\":\"%s\",\"chunkBytes\":%u}",
           ++frameId, unsigned(kFrameBytes), frameHash, (unsigned long long)uiActionCount(), pull ? "pull" : "stream", unsigned(kPullChunkBytes));
  queueUsb(header);
  frameSent = 0;
  frameEnd = kFrameBytes;
  frameStartedAt = frameProgressAt = millis();
  frameActive = !pull;
}

void beginFrameChunk(const char* command) {
  if (usbOutput.length() > 3072) { queueUsb("{\"error\":\"USB busy; retry chunk\"}"); return; }
  unsigned id = 0, offset = 0;
  if (sscanf(command, "frame_chunk %u %u", &id, &offset) != 2 || !frameSnapshot || !frameId || id != frameId || offset >= kFrameBytes) {
    queueUsb("{\"error\":\"use frame_begin then frame_chunk ID OFFSET\"}");
    return;
  }
  const size_t count = std::min(kPullChunkBytes, kFrameBytes - size_t(offset));
  char header[160];
  snprintf(header, sizeof(header), "{\"type\":\"frame-chunk\",\"id\":%u,\"offset\":%u,\"bytes\":%u,\"fnv1a32\":%u}", id, offset, unsigned(count), frameChecksum(frameSnapshot + offset, count));
  queueUsb(header);
  frameSent = offset;
  frameEnd = offset + count;
  frameStartedAt = frameProgressAt = millis();
  frameActive = true;
}

// Cold diagnostic append; not on active game/input paths.
__attribute__((noinline)) void appendBackdropABReport103(String& report,const UiEntryBench::BackdropAB& ab){
 char part[512];
 const int used=snprintf(part,sizeof(part),",\"backdropAB\":{\"ran\":%s,\"oom\":%s,\"pairs\":%u,\"fullFirstPairs\":%u,\"fastFirstPairs\":%u,\"comparedPixels\":%u,\"mismatchPixels\":%u,\"mismatchPairs\":%u,\"full\":{\"totalUs\":%llu,\"maxUs\":%u},\"fast\":{\"totalUs\":%llu,\"maxUs\":%u},\"panel\":{\"totalUs\":%llu,\"maxUs\":%u},\"firstMismatchSample\":%d,\"firstMismatchAt\":%d}",ab.ran?"true":"false",ab.oom?"true":"false",unsigned(ab.pairs),unsigned(ab.fullFirstPairs),unsigned(ab.fastFirstPairs),unsigned(ab.comparedPixels),unsigned(ab.mismatchPixels),unsigned(ab.mismatchPairs),(unsigned long long)ab.fullTotalUs,unsigned(ab.fullMaxUs),(unsigned long long)ab.fastTotalUs,unsigned(ab.fastMaxUs),(unsigned long long)ab.panelTotalUs,unsigned(ab.panelMaxUs),int(ab.firstMismatchSample),int(ab.firstMismatchAt));
 if(used>0&&size_t(used)<sizeof(part))report+=part;
 else report+=",\"backdropAB\":{\"ran\":false,\"formatError\":true}";
}

// Major timing is independent from the old fixture/AB counters.
__attribute__((noinline)) void appendMajorTimingReport119(String& report,const UiEntryBench::MajorTiming& timing){
 char part[240];
 const int used=snprintf(part,sizeof(part),",\"majorTiming\":{\"ran\":%s,\"frames\":%u,\"renderer\":{\"totalUs\":%llu,\"maxUs\":%u},\"panel\":{\"totalUs\":%llu,\"maxUs\":%u}}",timing.ran?"true":"false",unsigned(timing.frames),(unsigned long long)timing.rendererTotalUs,unsigned(timing.rendererMaxUs),(unsigned long long)timing.panelTotalUs,unsigned(timing.panelMaxUs));
 if(used>0&&size_t(used)<sizeof(part))report+=part;
 else report+=",\"majorTiming\":{\"ran\":false,\"formatError\":true}";
}

// Keep the cold768-byte report formatter off active command/USB stack frames.
__attribute__((noinline)) String effectBenchReport(const UiEntryBench& bench) {
  char result[768];
    snprintf(result,sizeof(result),"{\"command\":\"fx_bench\",\"requested\":%s,\"frames\":%u,\"maxUs\":%u,\"totalUs\":%u,\"engineUnchanged\":%s,\"beforeHash\":%u,\"afterHash\":%u,\"parityFrames\":%u,\"parityMismatchPixels\":%u,\"parityLegacyUs\":%u,\"parityOptimizedUs\":%u,\"mismatchEffect\":%d,\"mismatchMs\":%d,\"mismatchAt\":%d,\"fixtures\":[",bench.ran?"true":"false",unsigned(bench.frames),unsigned(bench.maxUs),unsigned(bench.totalUs),bench.unchanged?"true":"false",unsigned(bench.beforeHash),unsigned(bench.afterHash),unsigned(bench.parityFrames),unsigned(bench.parityMismatchPixels),unsigned(bench.parityLegacyUs),unsigned(bench.parityOptimizedUs),int(bench.mismatchEffect),int(bench.mismatchMs),int(bench.mismatchAt));
    String report(result);for(unsigned i=0;i<kUiEffectBenchmarkFixtures;++i){const auto& f=bench.fixtures[i];char part[128];snprintf(part,sizeof(part),"%s{\"frames\":%u,\"maxUs\":%u,\"totalUs\":%u,\"maxElapsedMs\":%u}",i?",":"",unsigned(f.frames),unsigned(f.maxUs),unsigned(f.totalUs),unsigned(f.maxElapsedMs));report+=part;}report+=String("],\"holdParityFrames\":")+bench.holdParityFrames+",\"holdParityMismatchPixels\":"+bench.holdParityMismatchPixels+",\"holdParityLegacyUs\":"+bench.holdParityLegacyUs+",\"holdParityOptimizedUs\":"+bench.holdParityOptimizedUs;report+=String(",\"cacheParityFrames\":")+bench.cacheParityFrames+",\"cacheParityMismatchPixels\":"+bench.cacheParityMismatchPixels+",\"cacheReferenceUs\":"+bench.cacheReferenceUs+",\"cachePreparedUs\":"+bench.cachePreparedUs+",\"entryPoseRateHz\":30,\"entryLegacyParityReelInteriorOnly\":true";report+=String(",\"sceneParityFrames\":")+bench.sceneParityFrames+",\"sceneParityMismatchPixels\":"+bench.sceneParityMismatchPixels+",\"sceneReferenceUs\":"+bench.sceneReferenceUs+",\"sceneOptimizedUs\":"+bench.sceneOptimizedUs;report+=String(",\"coinParityFrames\":")+bench.coinParityFrames+",\"coinParityMismatchPixels\":"+bench.coinParityMismatchPixels+",\"coinReferenceUs\":"+bench.coinReferenceUs+",\"coinPreparedUs\":"+bench.coinPreparedUs;report+=String(",\"goldParityFrames\":")+bench.goldParityFrames+",\"goldParityMismatchPixels\":"+bench.goldParityMismatchPixels+",\"goldReferenceUs\":"+bench.goldReferenceUs+",\"goldPreparedUs\":"+bench.goldPreparedUs;report+=String(",\"pickupParityFrames\":")+bench.pickupParityFrames+",\"pickupParityMismatchPixels\":"+bench.pickupParityMismatchPixels+",\"pickupReferenceUs\":"+bench.pickupReferenceUs+",\"pickupPreparedUs\":"+bench.pickupPreparedUs;report+=String(",\"alphaParityFrames\":")+bench.alphaParityFrames+",\"alphaParityMismatchPixels\":"+bench.alphaParityMismatchPixels+",\"alphaReferenceUs\":"+bench.alphaReferenceUs+",\"alphaPreparedUs\":"+bench.alphaPreparedUs;report+=String(",\"bookRecords\":")+bench.bookRecords+",\"bookPixelPairs\":"+bench.bookPixelPairs+",\"bookFNV\":"+bench.bookFNV+",\"bookInvalidRecords\":"+bench.bookInvalidRecords;report+=String(",\"rendererUnchanged\":")+(bench.rendererUnchanged?"true":"false")+",\"rendererBeforeHash\":"+unsigned(bench.rendererBeforeHash)+",\"rendererAfterHash\":"+unsigned(bench.rendererAfterHash);appendBackdropABReport103(report,bench.backdropAB);appendMajorTimingReport119(report,bench.majorTiming);report+=String(",\"job\":")+lastEffectBenchJob+"}";return report;

}


// Current SP headroom is distinct from the historic minimum-unused watermark.
// The pinned ESP32 FreeRTOS exposes the lowest stack address via this API.
uint32_t sageCurrentStackHeadroom(){
 const uintptr_t base=reinterpret_cast<uintptr_t>(pxTaskGetStackStart(nullptr));
 if(!base)return 0;
 uintptr_t current=0;
#if defined(__XTENSA__)
 __asm__ __volatile__("mov %0, a1":"=a"(current));
#else
 current=reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
#endif
 return current>base&&current-base<=UINT32_MAX?uint32_t(current-base):0;
}
void consumeQaInput(){
 aStarted=bStarted=0;aLongUsed=bLongUsed=true;chordSeen=false;blackTap.reset();
}
String sageTestReport(const char* command){
 const auto& q=lastSageTest;char part[256];
 int n=snprintf(part,sizeof(part),"{\"command\":\"%s\",\"requested\":%s,\"job\":%u,\"rows\":%u,\"protectedStateUnchanged\":%s,\"joined\":%s,\"pixelWitness\":%s",command,q.ran?"true":"false",unsigned(lastSageJob),unsigned(q.rows),q.unchanged?"true":"false",q.joined?"true":"false",q.pixelWitness?"true":"false");
 if(n<0||size_t(n)>=sizeof(part))return "{\"error\":\"sage report overflow\"}";
 String report(part);
 n=snprintf(part,sizeof(part),",\"winMask\":%u,\"expandedMask\":%u,\"beforeHash\":%u,\"afterHash\":%u,\"frames\":%u,\"expandFrames\":%u,\"backupBytes\":%u",unsigned(q.winMask),unsigned(q.expandedMask),unsigned(q.beforeHash),unsigned(q.afterHash),unsigned(q.frames),unsigned(q.expandFrames),unsigned(q.backupBytes));
 if(n<0||size_t(n)>=sizeof(part))return "{\"error\":\"sage report overflow\"}";report+=part;
 n=snprintf(part,sizeof(part),",\"bodyFrames\":%u,\"bodyChecked\":%u,\"bodyMatched\":%u,\"bodyMismatched\":%u,\"bodyInvalidFrames\":%u,\"bodyDifferentBlue\":%u,\"bodyFirstClock\":%u",unsigned(q.bodyFrames),unsigned(q.bodyChecked),unsigned(q.bodyMatched),unsigned(q.bodyMismatched),unsigned(q.bodyInvalidFrames),unsigned(q.bodyDifferentBlue),unsigned(q.bodyFirstClock));
 if(n<0||size_t(n)>=sizeof(part))return "{\"error\":\"sage report overflow\"}";report+=part;
 n=snprintf(part,sizeof(part),",\"largestPsramBefore\":%u,\"largestPsramAfter\":%u,\"entryStackHeadroom\":%u,\"stackMinimumBefore\":%u,\"stackMinimumAfter\":%u}",unsigned(sageLargestBefore),unsigned(sageLargestAfter),unsigned(sageEntryHeadroom),unsigned(sageMinimumBefore),unsigned(sageMinimumAfter));
 if(n<0||size_t(n)>=sizeof(part))return "{\"error\":\"sage report overflow\"}";report+=part;return report;
}
void requestSageTest(const char* command,unsigned rows){
 const char* rejected=nullptr;
 if(!sessionReady125||!uiRandomHealthy()||uiCheckpointPaused())rejected="healthy idle session required";
 else if(!uiReady()||blackTap.pending||qaInputFence||M5.BtnA.isPressed()||M5.BtnB.isPressed())rejected="release buttons in idle game";
 else if(frameActive||!usbOutput.isEmpty())rejected="USB output busy";
 else if(M5.Speaker.isPlaying()||audioFeatureBusy()||audioLandingsBusy()||audioDiagnostics().pending)rejected="audio must be idle";
 else if(lastSageJob==UINT32_MAX)rejected="diagnostic job counter exhausted";
 const uint32_t largest=uint32_t(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
 const uint32_t headroom=sageCurrentStackHeadroom();
 // Actual payload is97187B before subsequent UI additions. Keep allocation
 // overhead and caller/deeper SDK frames separate from this coarse admission.
 if(!rejected&&largest<104096)rejected="contiguous PSRAM unavailable";
 if(!rejected&&headroom<6144)rejected="current stack headroom insufficient";
 if(rejected){queueUsb(String("{\"command\":\"")+command+"\",\"requested\":false,\"reason\":\""+rejected+"\",\"largestPsram\":"+largest+",\"entryStackHeadroom\":"+headroom+"}");return;}
 const bool priorMute=audioMuted();const uint32_t lowBefore=unsigned(uxTaskGetStackHighWaterMark(nullptr));
 consumeQaInput();audioSetMuted(true);
 const UiSageSceneTest result=uiTestSageScene(rows);
 audioSetMuted(priorMute);M5.update();consumeQaInput();qaInputFence=true;
 if(!result.ran){queueUsb(String("{\"command\":\"")+command+"\",\"requested\":false,\"reason\":\"UI guard or allocation rejected\"}");return;}
 lastSageTest=result;++lastSageJob;sageLargestBefore=largest;sageLargestAfter=uint32_t(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));sageEntryHeadroom=headroom;sageMinimumBefore=lowBefore;sageMinimumAfter=unsigned(uxTaskGetStackHighWaterMark(nullptr));
 queueUsb(sageTestReport(command));
}

void usbCommand(const char* command) {
  if (!strcmp(command, "sync") || !strcmp(command, "frame_abort")) {
    abortFrame("cancelled by host");
    usbOutput = "";
    usbOutputSent = 0;
    usbStatusPending = usbBusyPending = false;
    queueUsb(String("\n{\"command\":\"") + command + "\",\"requested\":true}");
    return;
  }
  if (frameActive) {
    // RX is always serviced. Read-only requests wait behind a valid binary
    // payload; they are never interleaved or silently lost. Abort/sync above
    // remain usable even when a host stops reading halfway through a frame.
    if (!strcmp(command, "status")) usbStatusPending = true;
    else if (!strcmp(command, "reboot")) abortFrame("reboot requested");
    else { usbBusyPending = true; return; }
    if (frameActive) return;
  }
  if (rebootPending) return;
  // During post-QA input rearming, permit only reads/cancellation. This also
  // stops a queued host spin from escaping the physical-release fence before
  // the normal action/save observer resumes.
  if(qaInputFence||uiReturnSelectionInputFenced()||returnUsbDrain86){
    const bool readOnly=(!qaInputFence&&!strcmp(command,"storage_retry"))||!strcmp(command,"status")||!strcmp(command,"storage_status")||!strcmp(command,"math_status")||
      !strcmp(command,"ckpt_begin")||!strncmp(command,"ckpt_chunk ",11)||!strcmp(command,"frame")||!strcmp(command,"frame_begin")||!strncmp(command,"frame_chunk ",12)||
      !strcmp(command,"sage_status")||!strcmp(command,"sage_result")||!strcmp(command,"bench_result")||!strcmp(command,"award_status")||!strcmp(command,"help");
    if(!readOnly){queueUsb(String("{\"command\":\"")+command+"\",\"requested\":false,\"reason\":\""+((uiReturnSelectionInputFenced()||returnUsbDrain86)?"release buttons after return selection":"release buttons after diagnostic")+"\"}");return;}
  }
  if (!strcmp(command, "status")) usbStatusPending=true;
  else if (!strcmp(command, "frame")) beginFrameTransfer();
  else if (!strcmp(command, "frame_begin")) beginFrameTransfer(true);
  else if (!strncmp(command, "frame_chunk ", 12)) beginFrameChunk(command);
  else if (!strcmp(command, "a")) {
    dispatchUiInput59(uiButtonA); queueUsb("{\"command\":\"a\",\"requested\":true}");
  } else if (!strcmp(command, "b")) {
    dispatchUiInput59(uiButtonB); queueUsb("{\"command\":\"b\",\"requested\":true}");
  } else if (!strcmp(command, "menu")) {
    uiMenu(); queueUsb("{\"command\":\"menu\",\"requested\":true}");
  } else if (!strcmp(command, "flip")) {
    uiFlipScreen();saveOptions();queueUsb("{\"command\":\"flip\",\"requested\":true}");
  } else if (!strcmp(command, "auto")) {
    uiToggleAuto(); queueUsb("{\"command\":\"auto\",\"requested\":true}");
  } else if (!strcmp(command, "twin_music")) {
    const bool queued=audioTestTwinMusic();queueUsb(String("{\"command\":\"twin_music\",\"requested\":")+(queued?"true":"false")+"}");
  } else if (!strcmp(command, "feature_music")) {
    const bool queued=audioTestFeatureMusic();queueUsb(String("{\"command\":\"feature_music\",\"requested\":")+(queued?"true":"false")+"}");
  } else if (!strcmp(command, "sage_status")) {
    queueUsb(String("{\"command\":\"sage_status\",\"state\":")+uiSageDiagnostic()+"}");
  } else if (!strcmp(command, "sage_scene_test") || !strcmp(command, "sage_two_scene_test")) {
    requestSageTest(command,!strcmp(command,"sage_two_scene_test")?2:3);
  } else if (!strcmp(command, "sage_result")) {
    queueUsb(sageTestReport(command));
  } else if (!strcmp(command, "award_status")) {
    const auto music=audioMusicDiagnostics();String report=String("{\"command\":\"award_status\",\"lastCollection\":")+uiAwardDiagnostic();
    report+=String(",\"backendStarts\":")+music.awardStarts+",\"musicLevel\":"+unsigned(audioMusicOutputLevel())+",\"volume\":"+unsigned(audioVolume())+",\"muted\":"+(audioMuted()?"true":"false");report+=String(",\"twinReady\":")+(music.twinSourceReady?"true":"false")+",\"twinRootAccepted\":"+(music.twinRootAcceptedForDevice?"true":"false")+",\"twinFrames\":"+music.twinFrames+",\"twinRate\":"+music.twinSampleRate+",\"twinFNV\":"+music.twinFNV+"}";
    queueUsb(report);
  } else if(!strcmp(command,"storage_status")||!strcmp(command,"storage_retry")){
    queueUsb("{\"type\":\"storage-status\",\"policy\":\"session-only\",\"persistentGame\":false,\"retryAvailable\":false}");
  } else if(!strcmp(command,"math_status")){queueUsb(uiMathDiagnostic());
  } else if(!strcmp(command,"wager_menu")){uiCabinetMenu();queueUsb("{\"command\":\"wager_menu\",\"requested\":true}");
  } else if(!strcmp(command,"ckpt_begin")||!strncmp(command,"ckpt_chunk ",11)){
    queueUsb("{\"type\":\"checkpoint-export\",\"ok\":false,\"reason\":\"session-only; game import/export disabled\"}");
  } else if (!strcmp(command, "award_scene_test")||!strcmp(command,"spin_award_scene_test")||!strcmp(command,"award_auto_scene_test")) {
    const bool automatic=!strcmp(command,"award_auto_scene_test");
    const auto before=audioMusicDiagnostics();const auto scene=uiTestAwardScene(!strcmp(command,"spin_award_scene_test"),automatic);const auto after=audioMusicDiagnostics();
    String report=String("{\"command\":\"")+command+"\",\"requested\":"+(scene.ran?"true":"false")+",\"engineUnchanged\":"+(scene.unchanged?"true":"false");
    report+=String(",\"automaticFixture\":")+(automatic?"true":"false")+",\"beforeHash\":"+scene.beforeHash+",\"afterHash\":"+scene.afterHash+",\"frames\":"+scene.frames+",\"cueRequested\":"+(scene.cueRequested?"true":"false")+",\"fountainTriggered\":"+(scene.fountainTriggered?"true":"false");
    char counted[32];snprintf(counted,sizeof(counted),"%lld",(long long)scene.countedMinor);
    report+=String(",\"cueFirstMs\":")+scene.cueFirstMs+",\"fountainFirstMs\":"+scene.fountainFirstMs+",\"maxMusicLevel\":"+scene.maxMusicLevel+",\"counterMask\":"+scene.counterMask+",\"countedMinor\":"+counted+",\"awardStartsDelta\":"+uint32_t(after.awardStarts-before.awardStarts)+",\"queuedFramesDelta\":"+uint32_t(after.queuedFrames-before.queuedFrames)+",\"underrunsDelta\":"+uint32_t(after.underruns-before.underruns)+"}";queueUsb(report);
  } else if (!strcmp(command,"profile_scene_test")) {
    queueUsb("{\"command\":\"profile_scene_test\",\"requested\":false,\"reason\":\"state-only diagnostic required with external RNG service\"}");
  } else if (!strcmp(command,"arrival_scene_test")) {
    const auto scene=uiTestArrivalScene();
    String report=String("{\"command\":\"arrival_scene_test\",\"requested\":")+(scene.ran?"true":"false")+",\"engineUnchanged\":"+(scene.unchanged?"true":"false");
    report+=String(",\"beforeHash\":")+scene.beforeHash+",\"afterHash\":"+scene.afterHash+",\"accepted\":"+scene.accepted+",\"hardwareQueued\":"+scene.hardwareQueued+",\"failures\":"+scene.failures+",\"pending\":"+scene.pending+",\"durationMs\":"+scene.durationMs+",\"clips\":[";
    for(unsigned i=0;i<3;++i){if(i)report+=",";report+=String("{\"frames\":")+scene.frames[i]+",\"rate\":"+scene.rates[i]+",\"fnv\":"+scene.pcmFNV[i]+"}";}
    queueUsb(report+"]}");
  } else if (!strcmp(command, "award_music")) {
    const bool queued=audioTestAwardMusic();queueUsb(String("{\"command\":\"award_music\",\"requested\":")+(queued?"true":"false")+"}");
  } else if (!strcmp(command, "jingle")) {
    const bool queued = audioTestJingle();
    queueUsb(String("{\"command\":\"jingle\",\"requested\":") + (queued ? "true" : "false") + "}");
  }
  else if (!strcmp(command, "reboot")) {
    if (usbOutput.length() > 3072) {
      queueUsb("{\"command\":\"reboot\",\"saved\":false,\"reason\":\"USB busy\"}");
      return;
    }
    audioStop();
    queueUsb("{\"command\":\"reboot\",\"saved\":false,\"freshOnRestart\":true}");
    rebootPending = true;
    rebootRequestedAt = millis();
    rebootDrainedAt = 0;
  }
  else if (!strcmp(command, "spin")) {
    if (uiOverlayVisible() || !uiReady()) {
      queueUsb("{\"command\":\"spin\",\"requested\":false,\"reason\":\"not ready\"}");
    } else {
      dispatchUiInput59(uiButtonA);
      queueUsb("{\"command\":\"spin\",\"requested\":true}");
    }
  } else if (!strcmp(command, "fx_bench")) {
    if(effectBenchReportPending){queueUsb("{\"command\":\"fx_bench\",\"requested\":false,\"reason\":\"result pending; use bench_result\"}");return;}
    lastEffectBench=uiBenchmarkEntry();++lastEffectBenchJob;effectBenchReportPending=true;
  } else if (!strcmp(command, "bench_result")) {
    if(!lastEffectBenchJob)queueUsb("{\"command\":\"bench_result\",\"requested\":false,\"reason\":\"no completed benchmark\"}");
    else effectBenchReportPending=true;
  } else if (!strcmp(command, "help")) {
    queueUsb("Commands: status, storage_status, math_status, frame, frame_begin, frame_chunk ID OFFSET, frame_abort, sync, spin, a, b, menu, wager_menu, auto, flip, fx_bench, bench_result, feature_music, twin_music, award_music, award_status, sage_status, sage_scene_test, sage_two_scene_test, sage_result, award_scene_test, award_auto_scene_test, spin_award_scene_test, arrival_scene_test, profile_scene_test, jingle, reboot, help. Pull chunks are retryable reads of one immutable snapshot.");
  } else queueUsb("{\"error\":\"unknown command\"}");
}

bool kickUsbTx() {
  if (!usbDriverReady || !usb_serial_jtag_ll_txfifo_writable()) return false;
  // IDF4.4 install clears SERIAL_IN_EMPTY. Without an initial empty packet
  // there may be no TX interrupt to consume the first ring-buffer write.
  // Prime that event (also used by Arduino HWCDC's connection bootstrap).
  // Only the IDF ISR writes FIFO bytes; this task never writes its data FIFO.
  usb_serial_jtag_ll_ena_intr_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);
  usb_serial_jtag_ll_txfifo_flush();
  usbTxKickAt = millis();
  return true;
}

void serviceUsb() {
  if(uiReturnSelectionInputFenced())returnUsbDrain86=true;
  if (!usbDriverReady) {
    if(!uiReturnSelectionInputFenced()){returnUsbDrain86=false;usbInputLength=0;usbInputOverflow=false;}
    return;
  }
  const uint32_t now = millis();
  if (frameActive && (uint32_t(now - frameProgressAt) >= kFrameIdleMs || uint32_t(now - frameStartedAt) >= kFrameDeadlineMs)) abortFrame("transfer timeout");
  uint8_t received[64];
  const int length = usb_serial_jtag_read_bytes(received, sizeof(received), 0);
  // Only an observed empty RX after UI release rearms host mutations. Drop a
  // pre-save partial line too; a later suffix must not complete an old spin.
  // A read error keeps USB fenced, while physical controls stay independent.
  if(returnUsbDrain86&&length==0&&!uiReturnSelectionInputFenced()){
    usbInputLength=0;usbInputOverflow=false;returnUsbDrain86=false;
  }
  for (int at = 0; at < length; ++at) {
    const char c = char(received[at]);
    if (c == '\r') continue;
    if (c == '\n') {
      usbInput[usbInputLength] = 0;
      if (usbInputOverflow) { if (frameActive) usbBusyPending = true; else queueUsb("{\"error\":\"command too long\"}"); }
      else if (usbInputLength) usbCommand(usbInput);
      usbInputLength = 0;
      usbInputOverflow = false;
      continue;
    }
    if (usbInputLength < sizeof(usbInput) - 1 && !usbInputOverflow) usbInput[usbInputLength++] = c;
    else usbInputOverflow = true;
  }
  // A host-open reset can clear the pending empty event after driver setup.
  // Re-prime only outstanding/stalled TX, at most once per100ms, and only
  // when the hardware says FIFO space is available. This also ends a final
  // exact64-byte USB transaction with an empty packet for CDC host drivers.
  if ((frameActive || !usbOutput.isEmpty() || usbTxNeedsKick) &&
      uint32_t(now - usbTxProgressAt) >= 100 && uint32_t(now - usbTxKickAt) >= 100 && kickUsbTx()) {
    if (!frameActive && usbOutput.isEmpty()) usbTxNeedsKick = false;
  }
  // The IDF driver preserves partial hardware-FIFO writes in its ISR and
  // returns 0 when its ring is full. Arduino2 HWCDC can discard those bytes;
  // its zero-timeout retry counter can also underflow. Never call that path.
  size_t budget = kUsbBudgetBytes;
  for (size_t packets = 0; budget && packets < kUsbBudgetBytes / kUsbPacketBytes; ++packets) {
    if (!frameActive&&usbOutput.isEmpty()) {
      if(effectBenchReportPending){effectBenchReportPending=false;queueUsb(effectBenchReport(lastEffectBench));}
      else if (usbStatusPending) { usbStatusPending = false; reportStatus(); }
      else if (usbBusyPending) { usbBusyPending = false; queueUsb("{\"error\":\"frame busy; retry after frame or use frame_abort\"}"); }
    }
    const bool text = usbOutputSent < usbOutput.length();
    if (!text && !frameActive) break;
    const size_t remaining = text ? usbOutput.length() - usbOutputSent : frameEnd - frameSent;
    const size_t count = std::min({remaining, budget, kUsbPacketBytes});
    const uint8_t* source = text ? reinterpret_cast<const uint8_t*>(usbOutput.c_str() + usbOutputSent) : frameSnapshot + frameSent;
    const int accepted = usb_serial_jtag_write_bytes(source, count, 0);
    if (accepted <= 0) break;
    const size_t written = std::min(count, size_t(accepted));
    usbTxProgressAt = millis();
    usbTxNeedsKick = true;
    budget -= written;
    if (text) {
      usbOutputSent += written;
      if (usbOutputSent == usbOutput.length()) { usbOutput = ""; usbOutputSent = 0; }
    } else {
      frameSent += written;
      frameProgressAt = millis();
      if (frameSent == frameEnd) { frameActive = false; lastFrameSent = frameSent; ++framesCompleted; }
    }
  }
}
}  // namespace

void disableStatusLed() {
  // StickS3's green status LED is on M5PM1 LED_EN, not an ESP GPIO.
  // PWR_CFG0x06 bit4 is the dedicated LED level. Preserve every power/charge
  // bit and verify its readback; the PM1's download indicator remains separate.
  statusLedDisabled = false;
  if (M5.getBoard() != m5::board_t::board_M5StickS3) return;
  if (!M5.Power.M5pm1.readRegister(0x06, &statusLedCfgBefore, 1)) return;
  if (!M5.Power.M5pm1.setLedEnLevel(false)) return;
  if (!M5.Power.M5pm1.readRegister(0x06, &statusLedCfgAfter, 1)) return;
  statusLedDisabled = !(statusLedCfgAfter & 0x10)
      && (statusLedCfgBefore & ~0x10) == (statusLedCfgAfter & ~0x10);
}

void restorePowerButton(){
  powerButtonDefault=false;
  // Undo 85's power-key takeover. The requested grey key is GPIO BtnB.
  // Clear only single-reset-disable; download, shutdown and timing stay intact.
  if(M5.getBoard()!=m5::board_t::board_M5StickS3||!M5.Power.M5pm1.readRegister(0x49,&powerButtonConfigBefore,1))return;
  const uint8_t configured=uint8_t(powerButtonConfigBefore&~1u);
  if(configured!=powerButtonConfigBefore&&!M5.Power.M5pm1.writeRegister8(0x49,configured))return;
  powerButtonDefault=M5.Power.M5pm1.readRegister(0x49,&powerButtonConfigAfter,1)&&powerButtonConfigAfter==configured;
}

void setup() {
  const bool seeded125=outcomeRandom125.initialize(platform_entropy125::Source::callback,&entropySource125);
  auto config = M5.config();
  // Keep Arduino HWCDC uninitialized: exactly one ISR owns USB Serial/JTAG.
  config.serial_baudrate = 0;
  config.clear_display = true;
  config.output_power = false;
  config.internal_imu = false;
  config.internal_rtc = false;
  config.internal_mic = false;
  config.internal_spk = true;
  config.external_imu = false;
  config.external_rtc = false;
  config.external_spk = false;
  config.fallback_board = m5::board_t::board_M5StickS3;
  M5.begin(config);
  restorePowerButton();
  disableStatusLed();
  usb_serial_jtag_driver_config_t usbConfig = {};
  usbConfig.tx_buffer_size = 1024;
  usbConfig.rx_buffer_size = 1024;
  usbInstallError = usb_serial_jtag_driver_install(&usbConfig);
  usbDriverReady = usbInstallError == ESP_OK;
  kickUsbTx();
  M5.Display.setRotation(1);
  M5.BtnA.setDebounceThresh(30);
  M5.BtnB.setDebounceThresh(30);
  const bool profile125=platform_entropy125::pinnedSdk&&M5.getBoard()==m5::board_t::board_M5StickS3&&
    !config.internal_mic&&!config.internal_imu&&!config.internal_rtc&&!config.external_imu&&!config.external_rtc&&!config.external_spk;
  const bool qualified125=entropySource125.qualifyRuntime(profile125);
  const bool fresh125=uiInitFresh(outcomeRandom125);
  sessionReady125=seeded125&&qualified125&&fresh125;
  uiSetOutcomeCheckpointHandler(sessionOutcome125);
  // No old bank is read and no RNG state is restored. Autoplay starts off.
  uiRedraw();
  uiFrame();
  // Bring up the audio mixer after the fresh session/error screen reaches LCD.
  audioInit(false);
  uiPrepareEffects();
  reportStatus();
}

void loop() {
  M5.update();
  audioUpdate(millis());
  const uint32_t now = millis();
  if (rebootPending) {
    // Do not advance the current session while reboot acknowledgment drains.
    serviceUsb();
    if (usbOutput.isEmpty() && !rebootDrainedAt) rebootDrainedAt = millis();
    if ((rebootDrainedAt && millis() - rebootDrainedAt >= 150) ||
        millis() - rebootRequestedAt >= 2000) {
      ESP.restart();
    }
    delay(2);
    return;
  }
  if(uiCheckpointPaused()||uiReturnSelectionInputFenced()){
    // Consume physical edges while paused. Holding a stale button through a
    // recovery must not restart autoplay or skip the pending scene afterward.
    aStarted=bStarted=0;aLongUsed=bLongUsed=true;chordSeen=false;blackTap.reset();
    uiFrame();serviceUsb();delay(2);return;
  }
  if(qaInputFence){
    consumeQaInput();
    if(!M5.BtnA.isPressed()&&!M5.BtnB.isPressed()&&!M5.BtnA.wasReleased()&&!M5.BtnB.wasReleased())qaInputFence=false;
    uiFrame();serviceUsb();delay(2);return;
  }
  if (M5.BtnA.wasPressed()) { aStarted = now; aLongUsed = false; blackTap.reset(); }
  if (M5.BtnB.wasPressed()) { bStarted = now; bLongUsed = false; }
  if (M5.BtnA.isPressed() && M5.BtnB.isPressed()) {chordSeen = true;blackTap.reset();}
  if (!chordSeen && aStarted && M5.BtnA.isPressed() && !aLongUsed && now - aStarted >= kHoldMs) {
    aLongUsed = true;
    uiToggleAuto();
  }
  if (!chordSeen && bStarted && M5.BtnB.isPressed() && !bLongUsed && now - bStarted >= kHoldMs) {
    bLongUsed = true;
    blackTap.reset();
    uiButtonBHold();
  }
  if (M5.BtnA.wasReleased() && aStarted && !aLongUsed && !chordSeen && now - aStarted < kHoldMs) {
    dispatchUiInput59(uiButtonA);
    saveOptions();
  }
  if (M5.BtnB.wasReleased() && bStarted && !bLongUsed && !chordSeen && now - bStarted < kHoldMs) {
    if(uiIsGameView()){
      if(blackTap.released(now)){uiFlipScreen();saveOptions();}
    }else {blackTap.reset();dispatchUiInput59(uiButtonB);}
  }
  // Resolve the single only after release handling. A second press, long hold,
  // blue press, chord or overlay must never leak an overdue cabinet action.
  if(blackTap.poll(now,uiIsGameView(),M5.BtnB.isPressed()||M5.BtnA.isPressed()))uiCabinetMenu();
  if (!M5.BtnA.isPressed() && !M5.BtnB.isPressed() && chordSeen) {
    chordSeen = false;
    aLongUsed = true;
    bLongUsed = true;
    aStarted = bStarted = 0;
    blackTap.reset();
  }
  // UI owns all scene transitions and source-art overlays.
  uiFrame();
  serviceUsb();
  delay(2);
}
