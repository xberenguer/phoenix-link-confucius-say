#include <M5Unified.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_random.h>
#include <bootloader_random.h>
#include <esp_timer.h>
#include <esp32-hal-psram.h>
#include <driver/usb_serial_jtag.h>
#include <hal/usb_serial_jtag_ll.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "ui.h"
#include "sage_qa_api.h"
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "checkpoint_store.h"
#include "startup_nvs_guard.h"
#include "build_version.h"
#include "audio.h"
#include "audio_music.h"
#include "game_black_tap.h"
#include "entry_art.h"
#include "collect_art.h"

namespace {
constexpr uint32_t kHoldMs = 850;
constexpr uint32_t kSaveDelayMs = 1000;
constexpr uint32_t kCheckpointMs = 30000;
constexpr size_t kMaxSnapshot = 3072;
constexpr size_t kFrameBytes = 240u * 135u * sizeof(uint16_t);
constexpr size_t kUsbPacketBytes = 64;
constexpr size_t kUsbBudgetBytes = 512;
constexpr size_t kPullChunkBytes = 512;
constexpr uint32_t kFrameIdleMs = 2000;
constexpr uint32_t kFrameDeadlineMs = 15000;
Preferences preferences;
uint8_t savedState[kMaxSnapshot];
uint8_t savedCheckpoint[kUiCheckpointCapacity],nextCheckpoint[kUiCheckpointCapacity],checkpointReadback[kUiCheckpointCapacity];
size_t savedCheckpointLength=0;uint64_t checkpointGeneration=0;
bool checkpointRecordPresent=false,checkpointSaveBlocked=false;
uint32_t seenRuntimeRevision=0;
uint8_t checkpointExport[kUiCheckpointCapacity];size_t checkpointExportLength=0;uint32_t checkpointExportId=0;
constexpr size_t kSceneBytes=kUiSceneCapacity;
uint8_t savedScene[kSceneBytes];
size_t savedSceneLength=0,storedSceneBytes=0;
bool sceneRestoredAtBoot=false,sceneSaveFailed=false;
size_t savedLength = 0;
size_t storedSnapshotBytes = 0;
bool restoredAtBoot = false;
bool statusLedDisabled = false;
bool powerButtonDefault=false;
uint8_t powerButtonConfigBefore=0,powerButtonConfigAfter=0;
uint8_t statusLedCfgBefore = 0, statusLedCfgAfter = 0;
bool preferencesReady = false;
bool pendingSave = false;
bool saveFailed = false;
bool aLongUsed = false;
bool bLongUsed = false;
bool chordSeen = false;
bool qaInputFence=false;
// Native USB batches may outlive the UI release frame. No physical-input dependency.
bool returnUsbDrain86=false;
UiSageSceneTest lastSageTest;
uint32_t lastSageJob=0,sageLargestBefore=0,sageLargestAfter=0,sageEntryHeadroom=0,sageMinimumBefore=0,sageMinimumAfter=0;
GameBlackTap blackTap;
uint8_t savedBrightness = 100;
bool savedMuted = false;
bool savedCreditsDisplay = false;
bool savedScreenFlipped = false;
bool savedSpinSounds = true, savedLandingSounds = true;
uint8_t savedVolume = 192;
bool savedAutoRefill=false;
uint32_t savedLastAddedMinor=1000000,seenCreditRevision=0;
uint32_t aStarted = 0;
uint32_t bStarted = 0;
uint32_t changedAt = 0;
uint32_t lastSaveAt = 0;
uint32_t lastSaveAttemptAt = 0;
uint32_t lastOptionsAt = 0;
uint64_t seenActionCount = 0;
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

// Stored state is trusted only after error-distinguishing read and validation.
// The existing nextCheckpoint/readback buffers are the immutable pending record.
enum class StoragePhase:uint8_t {StartupRead,StartupWriter,Ready,Pending,InvalidStored,Conflict,Exhausted};
StoragePhase storagePhase=StoragePhase::StartupRead;
nvs_handle_t financialHandle=0;
bool bankTrusted=false,pendingRecordReady=false,sawCanonical=false,sawStoredRecord=false,legacySceneMissing=false;
size_t pendingCheckpointLength=0;
esp_err_t lastStorageError=ESP_OK;
uint32_t lastStorageRetryAt=0;
const char*storageDetail="startup read required";

void storageHold(StoragePhase phase,esp_err_t error,const char*detail){
 storagePhase=phase;lastStorageError=error;storageDetail=detail;
 uiSetCheckpointPause(bankTrusted?UiCheckpointPause::UndurableRecord:UiCheckpointPause::UnknownBank);
}
void saveState();
void applyStoredOptions(){
 const uint8_t brightness=preferences.getUChar("brightness",100);if(brightness>=20)savedBrightness=brightness;
 savedMuted=preferences.getBool("muted",false);savedCreditsDisplay=preferences.getBool("credits",false);savedScreenFlipped=preferences.getBool("flipped",false);
 savedSpinSounds=preferences.getBool("spin-sfx",true);savedLandingSounds=preferences.getBool("land-sfx",true);savedVolume=preferences.getUChar("volume",192);
 savedAutoRefill=preferences.getBool("auto-refill",false);savedLastAddedMinor=preferences.getUInt("topup-amt",1000000);
 M5.Display.setBrightness(savedBrightness);audioSetMuted(savedMuted);uiSetScreenFlipped(savedScreenFlipped);uiSetCreditPreferences(savedAutoRefill,savedLastAddedMinor);uiSetCreditsDisplay(savedCreditsDisplay);uiSetSoundPreferences(savedSpinSounds,savedLandingSounds,savedVolume);
}
void observeLoadedCounters(){seenActionCount=uiActionCount();seenCreditRevision=uiCreditRevision();seenRuntimeRevision=uiRuntimeRevision();}

bool recoverStoredBank(){
 // Once actions have produced a pending record, never reload an older bank.
 if(storagePhase==StoragePhase::Pending||storagePhase==StoragePhase::Conflict||storagePhase==StoragePhase::Exhausted)return false;
 lastStorageRetryAt=millis();nvs_handle_t reader=0;const auto opened=nvs_open("pl_confucius",NVS_READONLY,&reader);
 bool canonicalAbsent=false,fresh=false,legacy=false;size_t canonicalLength=0,legacyLength=0,sceneLength=0;bool scenePresent=false;
 checkpoint_store::Result canonical;
 if(opened==ESP_ERR_NVS_NOT_FOUND){canonicalAbsent=true;fresh=true;}
 else if(opened!=ESP_OK){storageHold(StoragePhase::StartupRead,opened,"storage open/read unavailable");return false;}
 else{
  canonical=checkpoint_store::read(reader,checkpointReadback,sizeof(checkpointReadback));canonicalLength=canonical.length;
  sawCanonical=sawCanonical||canonical.observedPresent;sawStoredRecord=sawStoredRecord||canonical.observedPresent;
  if(canonical.state==checkpoint_store::Read::Error){nvs_close(reader);storageHold(StoragePhase::StartupRead,canonical.error,"canonical read unavailable");return false;}
  if(canonical.state==checkpoint_store::Read::Invalid){nvs_close(reader);checkpointRecordPresent=true;checkpointSaveBlocked=true;storageHold(StoragePhase::InvalidStored,canonical.error,"invalid present canonical");return false;}
  canonicalAbsent=canonical.state==checkpoint_store::Read::Absent;
  if(canonicalAbsent){
   if(sawCanonical){nvs_close(reader);checkpointSaveBlocked=true;storageHold(StoragePhase::InvalidStored,ESP_OK,"observed canonical disappeared");return false;}
   const auto old=checkpoint_store::readBlob(reader,"state",checkpointReadback,sizeof(savedState));sawStoredRecord=sawStoredRecord||old.observedPresent;
   if(old.state==checkpoint_store::Read::Error||old.state==checkpoint_store::Read::Invalid){nvs_close(reader);storageHold(old.state==checkpoint_store::Read::Error?StoragePhase::StartupRead:StoragePhase::InvalidStored,old.error,"legacy bank unreadable/invalid");return false;}
   const auto visual=checkpoint_store::readBlob(reader,"scene",nextCheckpoint,sizeof(savedScene));sawStoredRecord=sawStoredRecord||visual.observedPresent;
   if(visual.state==checkpoint_store::Read::Error||visual.state==checkpoint_store::Read::Invalid){nvs_close(reader);storageHold(visual.state==checkpoint_store::Read::Error?StoragePhase::StartupRead:StoragePhase::InvalidStored,visual.error,"legacy scene unreadable/invalid");return false;}
   scenePresent=visual.state==checkpoint_store::Read::Valid;sceneLength=scenePresent?visual.length:0;
   if(old.state==checkpoint_store::Read::Absent){if(scenePresent||sawStoredRecord){nvs_close(reader);storageHold(StoragePhase::InvalidStored,ESP_OK,"stored material without bank");return false;}fresh=true;}
   else{legacy=true;legacyLength=old.length;}
  }
  nvs_close(reader);
 }
 if(fresh&&sawStoredRecord){storageHold(StoragePhase::InvalidStored,ESP_OK,"known stored bank cannot become fresh");return false;}
 if(!canonicalAbsent){
  if(!uiRestoreCheckpoint(checkpointReadback,canonicalLength,checkpointGeneration)){checkpointSaveBlocked=true;checkpointRecordPresent=true;storageHold(StoragePhase::InvalidStored,ESP_OK,"canonical financial binding invalid");return false;}
  memcpy(savedCheckpoint,checkpointReadback,canonicalLength);checkpoint_bundle::View record;checkpoint_bundle::decode(savedCheckpoint,canonicalLength,record);
  savedCheckpointLength=canonicalLength;memcpy(savedState,record.game.bytes,record.game.length);savedLength=storedSnapshotBytes=record.game.length;
  savedSceneLength=storedSceneBytes=record.scene.length;if(record.scene.length)memcpy(savedScene,record.scene.bytes,record.scene.length);
  sceneRestoredAtBoot=record.scene.length!=0;restoredAtBoot=true;checkpointRecordPresent=true;legacySceneMissing=false;
 }else if(legacy){
  if(!uiRestoreLegacyCheckpoint(checkpointReadback,legacyLength,nextCheckpoint,sceneLength,scenePresent)){checkpointSaveBlocked=true;storageHold(StoragePhase::InvalidStored,ESP_OK,"legacy bank/scene binding invalid");return false;}
  memcpy(savedState,checkpointReadback,legacyLength);if(sceneLength)memcpy(savedScene,nextCheckpoint,sceneLength);savedLength=storedSnapshotBytes=legacyLength;savedSceneLength=storedSceneBytes=sceneLength;sceneRestoredAtBoot=scenePresent;legacySceneMissing=!scenePresent;restoredAtBoot=true;checkpointRecordPresent=false;savedCheckpointLength=0;checkpointGeneration=0;
 }else{restoredAtBoot=false;sceneRestoredAtBoot=false;checkpointRecordPresent=false;savedCheckpointLength=savedLength=savedSceneLength=0;checkpointGeneration=0;legacySceneMissing=false;}
 bankTrusted=true;checkpointSaveBlocked=false;observeLoadedCounters();
 if(checkpointGeneration==UINT64_MAX){checkpointSaveBlocked=true;uiBlockCheckpoint();storageHold(StoragePhase::Exhausted,ESP_OK,"checkpoint generation exhausted");return false;}
 if(!preferencesReady)preferencesReady=preferences.begin("pl_confucius",false);
 if(!preferencesReady){storageHold(StoragePhase::StartupWriter,ESP_FAIL,"preferences writer unavailable");return false;}
 if(!financialHandle){lastStorageError=nvs_open("pl_confucius",NVS_READWRITE,&financialHandle);if(lastStorageError!=ESP_OK){financialHandle=0;storageHold(StoragePhase::StartupWriter,lastStorageError,"financial writer unavailable");return false;}}
 applyStoredOptions();observeLoadedCounters();
 if(canonicalAbsent){pendingSave=true;pendingRecordReady=false;pendingCheckpointLength=0;saveFailed=false;storagePhase=StoragePhase::Pending;saveState();return storagePhase==StoragePhase::Ready;}
 pendingSave=saveFailed=false;storagePhase=StoragePhase::Ready;lastStorageError=ESP_OK;storageDetail="validated stored bank";uiSetCheckpointPause(UiCheckpointPause::None);return true;
}

uint64_t bootSeed() {
  // Capture offline entropy before application ADC/RF peripherals start.
  // PCG continuation is restored from a valid saved bank independently.
  uint64_t seed = 0;
  bootloader_random_enable();
  esp_fill_random(&seed, sizeof(seed));
  bootloader_random_disable();
  return seed;
}

void markChanged() {
  if(!bankTrusted||pendingRecordReady)return;
  pendingSave=true;changedAt=millis();
}

void saveOptions();
void noticeEngineActions() {
  const auto runtime=uiRuntimeRevision();if(runtime!=seenRuntimeRevision){seenRuntimeRevision=runtime;markChanged();}
  const auto credit=uiCreditRevision();
  if(credit!=seenCreditRevision){seenCreditRevision=credit;saveOptions();markChanged();}
  const uint64_t actions = uiActionCount();
  if (actions != seenActionCount) {
    seenActionCount = actions;
    markChanged();
  }
}

void dispatchUiInput59(void(*input)()){
 input();
 // A completed outcome has already been acknowledged by the synchronous
 // barrier. Cosmetic skips/menu navigation must not queue a duplicate write.
 noticeEngineActions();
}

void saveState() {
 if(!bankTrusted||!preferencesReady||!pendingSave||checkpointSaveBlocked)return;
 if(!financialHandle){lastStorageError=nvs_open("pl_confucius",NVS_READWRITE,&financialHandle);if(lastStorageError!=ESP_OK){financialHandle=0;saveFailed=true;storageHold(StoragePhase::Pending,lastStorageError,"pending financial writer unavailable");return;}}
 if(saveFailed&&uint32_t(millis()-lastSaveAttemptAt)<5000)return;
 if(checkpointGeneration==UINT64_MAX){checkpointSaveBlocked=true;uiBlockCheckpoint();storageHold(StoragePhase::Exhausted,ESP_OK,"checkpoint generation exhausted");return;}
 lastSaveAttemptAt=millis();
 if(!pendingRecordReady){
  pendingCheckpointLength=uiSaveCheckpoint(nextCheckpoint,sizeof(nextCheckpoint),checkpointGeneration+1);
  if(!pendingCheckpointLength||pendingCheckpointLength>sizeof(nextCheckpoint)){saveFailed=true;storageHold(StoragePhase::Pending,ESP_ERR_INVALID_ARG,"cannot encode trusted RAM bank");return;}
  pendingRecordReady=true;
 }
 // A valid unexpected canonical is preserved alongside the pending RAM bank.
 const auto onDisk=checkpoint_store::read(financialHandle,checkpointReadback,sizeof(checkpointReadback));
 if(onDisk.state==checkpoint_store::Read::Valid){
  const bool isPending=onDisk.length==pendingCheckpointLength&&!memcmp(checkpointReadback,nextCheckpoint,onDisk.length);
  const bool isPrior=onDisk.length==savedCheckpointLength&&savedCheckpointLength&&!memcmp(checkpointReadback,savedCheckpoint,onDisk.length);
  if(!isPending&&!isPrior){checkpointSaveBlocked=true;saveFailed=true;storageHold(StoragePhase::Conflict,ESP_OK,"conflicting canonical preserved");return;}
 }
 if(!checkpoint_store::commit(financialHandle,nextCheckpoint,pendingCheckpointLength,checkpointReadback,sizeof(checkpointReadback),lastStorageError)){
  saveFailed=true;if(lastStorageError==ESP_ERR_NVS_INVALID_HANDLE||lastStorageError==ESP_ERR_NVS_NOT_INITIALIZED){nvs_close(financialHandle);financialHandle=0;}storageHold(StoragePhase::Pending,lastStorageError,"exact pending record undurable");return;
 }
 memcpy(savedCheckpoint,nextCheckpoint,pendingCheckpointLength);savedCheckpointLength=pendingCheckpointLength;
 checkpoint_bundle::View record;checkpoint_bundle::decode(savedCheckpoint,savedCheckpointLength,record);checkpointGeneration=record.generation;checkpointRecordPresent=sawCanonical=sawStoredRecord=true;
 memcpy(savedState,record.game.bytes,record.game.length);savedLength=storedSnapshotBytes=record.game.length;
 savedSceneLength=storedSceneBytes=record.scene.length;if(record.scene.length)memcpy(savedScene,record.scene.bytes,record.scene.length);
 pendingSave=saveFailed=sceneSaveFailed=pendingRecordReady=false;pendingCheckpointLength=0;lastSaveAt=millis();storagePhase=StoragePhase::Ready;lastStorageError=ESP_OK;storageDetail="exact record committed/read back";uiSetCheckpointPause(UiCheckpointPause::None);
 if(checkpointGeneration==UINT64_MAX){checkpointSaveBlocked=true;uiBlockCheckpoint();storageHold(StoragePhase::Exhausted,ESP_OK,"checkpoint generation exhausted");}
}

bool currentReturnTupleOwned86(){
 if(!savedCheckpointLength||!checkpointRecordPresent||pendingSave||pendingRecordReady||!financialHandle)return false;
 const auto disk=checkpoint_store::read(financialHandle,checkpointReadback,sizeof(checkpointReadback));
 if(disk.state!=checkpoint_store::Read::Valid||disk.length!=savedCheckpointLength||memcmp(checkpointReadback,savedCheckpoint,savedCheckpointLength))return false;
 checkpoint_bundle::View saved,current;
 if(!checkpoint_bundle::decode(savedCheckpoint,savedCheckpointLength,saved))return false;
 const auto n=uiSaveCheckpoint(nextCheckpoint,sizeof(nextCheckpoint),checkpointGeneration);
 if(!n||!checkpoint_bundle::decode(nextCheckpoint,n,current))return false;
 auto same=[](checkpoint_bundle::Part a,checkpoint_bundle::Part b){return a.length==b.length&&(!a.length||!memcmp(a.bytes,b.bytes,a.length));};
 // Cosmetic phases may advance; the owned financial/raw-intent/service tuple
 // must already match exactly, with no pending funding or immutable record.
 return same(saved.game,current.game)&&same(saved.preference,current.preference)&&same(saved.service,current.service);
}
bool checkpointOutcome59(){
 if(uiReturnSelectionCheckpointPending())returnUsbDrain86=true;
 noticeEngineActions();
 if(!bankTrusted||!preferencesReady||checkpointSaveBlocked)return false;
 if(uiReturnSelectionCheckpointPending()&&storagePhase==StoragePhase::Ready&&currentReturnTupleOwned86())return true;
 pendingSave=true;saveState();
 return storagePhase==StoragePhase::Ready&&!pendingSave&&!pendingRecordReady&&
        !checkpointSaveBlocked&&!uiCheckpointPaused();
}

void saveOptions() {
  if(!preferencesReady||!bankTrusted||uiCheckpointPaused())return;
  const uint8_t level = M5.Display.getBrightness();
  if (level != savedBrightness && level >= 20) {
    if (preferences.putUChar("brightness", level) == sizeof(level)) savedBrightness = level;
  }
  if(uiAutoRefillEnabled()!=savedAutoRefill){const bool value=uiAutoRefillEnabled();if(preferences.putBool("auto-refill",value)==sizeof(value))savedAutoRefill=value;}
  if(uiLastAddedMinor()!=savedLastAddedMinor){const auto value=uiLastAddedMinor();if(preferences.putUInt("topup-amt",value)==sizeof(value))savedLastAddedMinor=value;}
  if(uiSpinSoundEnabled()!=savedSpinSounds){bool value=uiSpinSoundEnabled();if(preferences.putBool("spin-sfx",value)==sizeof(value))savedSpinSounds=value;}
  if(uiLandingSoundEnabled()!=savedLandingSounds){bool value=uiLandingSoundEnabled();if(preferences.putBool("land-sfx",value)==sizeof(value))savedLandingSounds=value;}
  if(uiSoundVolume()!=savedVolume){uint8_t value=uiSoundVolume();if(preferences.putUChar("volume",value)==sizeof(value))savedVolume=value;}
  if(uiScreenFlipped()!=savedScreenFlipped){const bool value=uiScreenFlipped();if(preferences.putBool("flipped",value)==sizeof(value))savedScreenFlipped=value;}
  if (uiCreditsDisplay() != savedCreditsDisplay) {
    const bool value=uiCreditsDisplay();
    if(preferences.putBool("credits",value)==sizeof(value))savedCreditsDisplay=value;
  }
  if (audioMuted() != savedMuted) {
    const bool muted = audioMuted();
    if (preferences.putBool("muted", muted) == sizeof(muted)) savedMuted = muted;
  }
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
  line += String(",\"snapshotBytes\":") + uiSave(nullptr, 0);
  line += String(",\"storedSnapshotBytes\":") + storedSnapshotBytes;
  line += ",\"restoredAtBoot\":"; line += restoredAtBoot ? "true" : "false";
  line += ",\"savePending\":"; line += pendingSave ? "true" : "false";
  line += ",\"saveAvailable\":"; line += preferencesReady&&bankTrusted&&!uiCheckpointPaused()&&!saveFailed&&!checkpointSaveBlocked?"true":"false";
  line += String(",\"bankTrusted\":")+(bankTrusted?"true":"false")+",\"checkpointPaused\":"+(uiCheckpointPaused()?"true":"false");
  line += ",\"startupNvsFullEraseBlocked\":"; line += startupNvsFullEraseBlocked() ? "true" : "false";
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
  line += String(",\"sceneBytes\":") + unsigned(savedSceneLength);
  line += ",\"sceneRestoredAtBoot\":";line += sceneRestoredAtBoot?"true":"false";
  line += ",\"sceneSaveFailed\":";line += sceneSaveFailed?"true":"false";
  line += ",\"entryArtReady\":"; line += entry_art::ready() ? "true" : "false";
  line += ",\"entryArtBytes\":"; line += entry_art::storageBytes();
   line += ",\"collectArtReady\":"; line += collect_art::ready() ? "true" : "false";
   line += ",\"collectArtBytes\":"; line += collect_art::storageBytes();
   line += ",\"effectBenchFrames\":"; line += kUiEffectBenchmarkFrames;
   line += ",\"effectParityFrames\":"; line += kUiEffectParityFrames;
  line += ",\"entryArtDecodedFNV\":"; line += entry_art::decodedHash();
  char checkpointInfo[180];snprintf(checkpointInfo,sizeof(checkpointInfo),",\"checkpointBytes\":%u,\"checkpointGeneration\":%llu,\"checkpointBlocked\":%s",unsigned(savedCheckpointLength),(unsigned long long)checkpointGeneration,checkpointSaveBlocked?"true":"false");line+=checkpointInfo;
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

// Keep the cold768-byte report formatter off active command/USB stack frames.
__attribute__((noinline)) String effectBenchReport(const UiEntryBench& bench) {
  char result[768];
    snprintf(result,sizeof(result),"{\"command\":\"fx_bench\",\"requested\":%s,\"frames\":%u,\"maxUs\":%u,\"totalUs\":%u,\"engineUnchanged\":%s,\"beforeHash\":%u,\"afterHash\":%u,\"parityFrames\":%u,\"parityMismatchPixels\":%u,\"parityLegacyUs\":%u,\"parityOptimizedUs\":%u,\"mismatchEffect\":%d,\"mismatchMs\":%d,\"mismatchAt\":%d,\"fixtures\":[",bench.ran?"true":"false",unsigned(bench.frames),unsigned(bench.maxUs),unsigned(bench.totalUs),bench.unchanged?"true":"false",unsigned(bench.beforeHash),unsigned(bench.afterHash),unsigned(bench.parityFrames),unsigned(bench.parityMismatchPixels),unsigned(bench.parityLegacyUs),unsigned(bench.parityOptimizedUs),int(bench.mismatchEffect),int(bench.mismatchMs),int(bench.mismatchAt));
    String report(result);for(unsigned i=0;i<kUiEffectBenchmarkFixtures;++i){const auto& f=bench.fixtures[i];char part[128];snprintf(part,sizeof(part),"%s{\"frames\":%u,\"maxUs\":%u,\"totalUs\":%u,\"maxElapsedMs\":%u}",i?",":"",unsigned(f.frames),unsigned(f.maxUs),unsigned(f.totalUs),unsigned(f.maxElapsedMs));report+=part;}report+=String("],\"holdParityFrames\":")+bench.holdParityFrames+",\"holdParityMismatchPixels\":"+bench.holdParityMismatchPixels+",\"holdParityLegacyUs\":"+bench.holdParityLegacyUs+",\"holdParityOptimizedUs\":"+bench.holdParityOptimizedUs;report+=String(",\"cacheParityFrames\":")+bench.cacheParityFrames+",\"cacheParityMismatchPixels\":"+bench.cacheParityMismatchPixels+",\"cacheReferenceUs\":"+bench.cacheReferenceUs+",\"cachePreparedUs\":"+bench.cachePreparedUs+",\"entryPoseRateHz\":30,\"entryLegacyParityReelInteriorOnly\":true";report+=String(",\"sceneParityFrames\":")+bench.sceneParityFrames+",\"sceneParityMismatchPixels\":"+bench.sceneParityMismatchPixels+",\"sceneReferenceUs\":"+bench.sceneReferenceUs+",\"sceneOptimizedUs\":"+bench.sceneOptimizedUs;report+=String(",\"coinParityFrames\":")+bench.coinParityFrames+",\"coinParityMismatchPixels\":"+bench.coinParityMismatchPixels+",\"coinReferenceUs\":"+bench.coinReferenceUs+",\"coinPreparedUs\":"+bench.coinPreparedUs;report+=String(",\"goldParityFrames\":")+bench.goldParityFrames+",\"goldParityMismatchPixels\":"+bench.goldParityMismatchPixels+",\"goldReferenceUs\":"+bench.goldReferenceUs+",\"goldPreparedUs\":"+bench.goldPreparedUs;report+=String(",\"pickupParityFrames\":")+bench.pickupParityFrames+",\"pickupParityMismatchPixels\":"+bench.pickupParityMismatchPixels+",\"pickupReferenceUs\":"+bench.pickupReferenceUs+",\"pickupPreparedUs\":"+bench.pickupPreparedUs;report+=String(",\"alphaParityFrames\":")+bench.alphaParityFrames+",\"alphaParityMismatchPixels\":"+bench.alphaParityMismatchPixels+",\"alphaReferenceUs\":"+bench.alphaReferenceUs+",\"alphaPreparedUs\":"+bench.alphaPreparedUs;report+=String(",\"bookRecords\":")+bench.bookRecords+",\"bookPixelPairs\":"+bench.bookPixelPairs+",\"bookFNV\":"+bench.bookFNV+",\"bookInvalidRecords\":"+bench.bookInvalidRecords;report+=String(",\"rendererUnchanged\":")+(bench.rendererUnchanged?"true":"false")+",\"rendererBeforeHash\":"+unsigned(bench.rendererBeforeHash)+",\"rendererAfterHash\":"+unsigned(bench.rendererAfterHash);report+=String(",\"job\":")+lastEffectBenchJob+"}";return report;

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
 if(!bankTrusted||storagePhase!=StoragePhase::Ready||pendingSave||saveFailed||checkpointSaveBlocked||uiCheckpointPaused())rejected="durable idle bank required";
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
    if(!strcmp(command,"storage_retry")){
      if(storagePhase==StoragePhase::Pending){lastSaveAttemptAt=millis()-5000;saveState();}
      else if(storagePhase!=StoragePhase::Ready&&storagePhase!=StoragePhase::Conflict&&storagePhase!=StoragePhase::Exhausted)recoverStoredBank();
    }
    char reply[480];snprintf(reply,sizeof(reply),"{\"type\":\"storage-status\",\"phase\":%u,\"bankTrusted\":%s,\"paused\":%s,\"detail\":\"%s\",\"nvsError\":%d,\"pendingBytes\":%u,\"generation\":%llu,\"legacySceneMissingCompatible\":%s,\"lastCommitAttemptWallMs\":%u,\"lastCommitFinishedWallMs\":%u}",unsigned(storagePhase),bankTrusted?"true":"false",uiCheckpointPaused()?"true":"false",storageDetail,int(lastStorageError),unsigned(pendingCheckpointLength),(unsigned long long)checkpointGeneration,legacySceneMissing?"true":"false",unsigned(lastSaveAttemptAt),unsigned(lastSaveAt));queueUsb(reply);
  } else if(!strcmp(command,"math_status")){queueUsb(uiMathDiagnostic());
  } else if(!strcmp(command,"wager_menu")){uiCabinetMenu();queueUsb("{\"command\":\"wager_menu\",\"requested\":true}");
  } else if(!strcmp(command,"ckpt_begin")){
    checkpointExportLength=0;
    if(!bankTrusted){queueUsb("{\"type\":\"checkpoint-export\",\"ok\":false,\"bankTrusted\":false,\"reason\":\"stored bank not validated; read-only storage retry required\"}");return;}
    noticeEngineActions();if(storagePhase==StoragePhase::Ready&&pendingSave)saveState();
    bool durable=false;const char*key="ram";size_t length=0;
    if(pendingRecordReady){length=pendingCheckpointLength;memcpy(checkpointExport,nextCheckpoint,length);}
    else if(savedCheckpointLength){length=savedCheckpointLength;memcpy(checkpointExport,savedCheckpoint,length);durable=true;key="checkpoint";}
    else if(storagePhase==StoragePhase::StartupWriter&&savedLength){length=savedLength;memcpy(checkpointExport,savedState,length);durable=true;key="state";}
    if(!length||length>sizeof(checkpointExport)){queueUsb("{\"type\":\"checkpoint-export\",\"ok\":false,\"reason\":\"no validated canonical export available\"}");return;}
    checkpointExportLength=length;++checkpointExportId;if(!checkpointExportId)++checkpointExportId;
    char reply[230];snprintf(reply,sizeof(reply),"{\"type\":\"checkpoint-export\",\"ok\":true,\"id\":%u,\"bytes\":%u,\"fnv1a32\":%u,\"key\":\"%s\",\"durable\":%s,\"bankTrusted\":true}",checkpointExportId,unsigned(length),checkpoint_bundle::checksum(checkpointExport,length),key,durable?"true":"false");queueUsb(reply);
  } else if(!strncmp(command,"ckpt_chunk ",11)){
    unsigned id=0,offset=0;if(sscanf(command,"ckpt_chunk %u %u",&id,&offset)!=2||id!=checkpointExportId||!checkpointExportLength||offset>=checkpointExportLength)queueUsb("{\"type\":\"checkpoint-chunk\",\"ok\":false}");
    else {const size_t n=std::min<size_t>(256,checkpointExportLength-offset);char head[150];snprintf(head,sizeof(head),"{\"type\":\"checkpoint-chunk\",\"id\":%u,\"offset\":%u,\"bytes\":%u,\"hex\":\"",id,offset,unsigned(n));String reply(head);const char*digits="0123456789abcdef";for(size_t i=0;i<n;++i){const auto b=checkpointExport[offset+i];reply+=digits[b>>4];reply+=digits[b&15];}reply+="\"}";queueUsb(reply);}
  } else if (!strcmp(command, "award_scene_test")||!strcmp(command,"spin_award_scene_test")||!strcmp(command,"award_auto_scene_test")) {
    const bool automatic=!strcmp(command,"award_auto_scene_test");
    const auto before=audioMusicDiagnostics();const auto scene=uiTestAwardScene(!strcmp(command,"spin_award_scene_test"),automatic);const auto after=audioMusicDiagnostics();
    String report=String("{\"command\":\"")+command+"\",\"requested\":"+(scene.ran?"true":"false")+",\"engineUnchanged\":"+(scene.unchanged?"true":"false");
    report+=String(",\"automaticFixture\":")+(automatic?"true":"false")+",\"beforeHash\":"+scene.beforeHash+",\"afterHash\":"+scene.afterHash+",\"frames\":"+scene.frames+",\"cueRequested\":"+(scene.cueRequested?"true":"false")+",\"fountainTriggered\":"+(scene.fountainTriggered?"true":"false");
    char counted[32];snprintf(counted,sizeof(counted),"%lld",(long long)scene.countedMinor);
    report+=String(",\"cueFirstMs\":")+scene.cueFirstMs+",\"fountainFirstMs\":"+scene.fountainFirstMs+",\"maxMusicLevel\":"+scene.maxMusicLevel+",\"counterMask\":"+scene.counterMask+",\"countedMinor\":"+counted+",\"awardStartsDelta\":"+uint32_t(after.awardStarts-before.awardStarts)+",\"queuedFramesDelta\":"+uint32_t(after.queuedFrames-before.queuedFrames)+",\"underrunsDelta\":"+uint32_t(after.underruns-before.underruns)+"}";queueUsb(report);
  } else if (!strcmp(command,"profile_scene_test")) {
    const auto p=uiTestMathProfile();
    String report=String("{\"command\":\"profile_scene_test\",\"requested\":")+(p.ran?"true":"false")+",\"engineUnchanged\":"+(p.unchanged?"true":"false");
    report+=String(",\"beforeHash\":")+p.beforeHash+",\"afterHash\":"+p.afterHash+",\"model\":"+p.model+",\"miniCredits\":"+p.miniCredits+",\"minorCredits\":"+p.minorCredits+",\"betMinor\":"+p.betMinor+",\"holdMinor\":"+p.holdMinor+",\"roundMinor\":"+p.roundMinor;
    report+=String(",\"baseKing3\":")+p.baseKing3+",\"freeKing5\":"+p.freeKing5+",\"baseCandle5\":"+p.baseCandle5+",\"baseJack4\":"+p.baseJack4+",\"linePathFNV\":"+p.linePathFNV+",\"baseQueen4\":"+p.baseQueen4+",\"freeBell3\":"+p.freeBell3+",\"freeBook3\":"+p.freeBook3+",\"freeJack3\":"+p.freeJack3+",\"freeJack5\":"+p.freeJack5+",\"freeTen5\":"+p.freeTen5+",\"freeCandle5\":"+p.freeCandle5+"}";queueUsb(report);
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
    markChanged();
    saveState();
    saveOptions();
    if (!preferencesReady || !bankTrusted || uiCheckpointPaused() || pendingSave || saveFailed) {
      queueUsb("{\"command\":\"reboot\",\"saved\":false,\"reason\":\"bank save failed\"}");
      return;
    }
    audioStop();
    queueUsb("{\"command\":\"reboot\",\"saved\":true}");
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
    queueUsb("Commands: status, storage_status, storage_retry, math_status, ckpt_begin, ckpt_chunk ID OFFSET, frame, frame_begin, frame_chunk ID OFFSET, frame_abort, sync, spin, a, b, menu, wager_menu, auto, flip, fx_bench, bench_result, feature_music, twin_music, award_music, award_status, sage_status, sage_scene_test, sage_two_scene_test, sage_result, award_scene_test, award_auto_scene_test, spin_award_scene_test, arrival_scene_test, profile_scene_test, jingle, reboot, help. Pull chunks are retryable reads of one immutable snapshot.");
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
  const uint64_t initialSeed = bootSeed();
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
  // Separate game namespace: preserve the installed Money Gong bank. Phoenix
  // owns its own versioned snapshot schema; uiRestore validates that schema.
  uiInit(initialSeed, true);
  uiSetOutcomeCheckpointHandler(checkpointOutcome59);
  uiSetCheckpointPause(UiCheckpointPause::UnknownBank);
  recoverStoredBank();

  // The first screen is gameplay. There is no splash, custom intro or delay.
  seenActionCount = uiActionCount();
  seenCreditRevision=uiCreditRevision();
  seenRuntimeRevision=uiRuntimeRevision();
  uiRedraw();
  uiFrame();
  noticeEngineActions();
  // Bring up the audio mixer after the saved game has reached the LCD.
  audioInit(savedMuted);
  uiPrepareEffects();
  // Initial fresh/legacy records commit in recoverStoredBank before play.
  // An unknown or invalid startup bank is never written from fresh RAM.
  reportStatus();
}

void loop() {
  M5.update();
  audioUpdate(millis());
  const uint32_t now = millis();
  if(storagePhase!=StoragePhase::Ready&&storagePhase!=StoragePhase::Pending&&storagePhase!=StoragePhase::Conflict&&storagePhase!=StoragePhase::Exhausted&&uint32_t(now-lastStorageRetryAt)>=5000)recoverStoredBank();
  if(storagePhase==StoragePhase::Pending)saveState();
  if (rebootPending) {
    // The saved engine is frozen during acknowledgment/flush. In particular,
    // automatic bonus play cannot advance after the checkpoint being tested.
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
  const uint32_t saveNow = millis();
  if (pendingSave && (saveNow - changedAt >= kSaveDelayMs || saveNow - lastSaveAt >= kCheckpointMs)) saveState();
  // UI owns all scene transitions and source-art overlays.
  uiFrame();
  noticeEngineActions();
  serviceUsb();
  if (saveNow - lastOptionsAt >= 1000) { lastOptionsAt = saveNow; saveOptions(); }
  delay(2);
}
