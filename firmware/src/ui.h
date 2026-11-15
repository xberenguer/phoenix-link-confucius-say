#pragma once
#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// Fresh RAM session only. RNG state is owned externally and never serialized.
namespace phoenix {namespace random {class Service;}}
bool uiInitFresh(phoenix::random::Service& source);
bool uiRandomHealthy();
uint8_t uiRandomFault();
struct UiEffectCache {bool ready;uint32_t bytes,fireFrames,birdFrames,rate,buildUs;bool alphaReady=false;uint32_t alphaChecks=0,alphaBoundaryFNV=0;bool pickupReady=false;uint32_t pickupBytes=0,pickupHits=0,pickupMisses=0;};
// Prime immutable PSRAM once, after the fresh game is visible and the
// audio source has its memory. No game/audio action or LCD transfer occurs.
bool uiPrepareEffects();
UiEffectCache uiEffectCache();
void uiFrame();
void uiRedraw();
void uiStatus(const char* text);
bool uiReady();
void uiAction();
void uiNextBet();
void uiNewBank();
void uiButtonA();
void uiButtonB();
void uiButtonBHold();
void uiMenu();
void uiCabinetMenu();
void uiToggleAuto();
bool uiOverlayVisible();
bool uiIsGameView();
void uiSetScreenFlipped(bool flipped);
void uiFlipScreen();
bool uiScreenFlipped();
constexpr uint32_t kUiEffectBenchmarkFrames=1015,kUiEffectBenchmarkFixtures=16;
struct UiEntryBench {
 bool ran=false,unchanged=false;uint32_t frames=0,maxUs=0,totalUs=0,beforeHash=0,afterHash=0;
 struct Fixture {uint32_t frames=0,maxUs=0,totalUs=0,maxElapsedMs=0;};Fixture fixtures[kUiEffectBenchmarkFixtures];
 uint32_t parityFrames=0,parityMismatchPixels=0,parityLegacyUs=0,parityOptimizedUs=0;
 uint32_t holdParityFrames=0,holdParityMismatchPixels=0,holdParityLegacyUs=0,holdParityOptimizedUs=0;
 uint32_t cacheParityFrames=0,cacheParityMismatchPixels=0,cacheReferenceUs=0,cachePreparedUs=0;
 uint32_t sceneParityFrames=0,sceneParityMismatchPixels=0,sceneReferenceUs=0,sceneOptimizedUs=0;
 uint32_t coinParityFrames=0,coinParityMismatchPixels=0,coinReferenceUs=0,coinPreparedUs=0;
 uint32_t goldParityFrames=0,goldParityMismatchPixels=0,goldReferenceUs=0,goldPreparedUs=0;
 uint32_t pickupParityFrames=0,pickupParityMismatchPixels=0,pickupReferenceUs=0,pickupPreparedUs=0;
 uint32_t alphaParityFrames=0,alphaParityMismatchPixels=0,alphaReferenceUs=0,alphaPreparedUs=0;
 uint32_t bookRecords=0,bookPixelPairs=0,bookFNV=2166136261u,bookInvalidRecords=0;
 // Measurement-only append; the original1015frames/16fixtures stay separate.
 struct BackdropAB {
  bool ran=false,oom=false;
  uint32_t pairs=0,fullFirstPairs=0,fastFirstPairs=0,comparedPixels=0,mismatchPixels=0,mismatchPairs=0;
  uint64_t fullTotalUs=0,fastTotalUs=0,panelTotalUs=0;
  uint32_t fullMaxUs=0,fastMaxUs=0,panelMaxUs=0;
  int32_t firstMismatchSample=-1,firstMismatchAt=-1;
 } backdropAB;
 // Separate Major full-raster measurement; not an original fixture or AB pair.
 struct MajorTiming {
  bool ran=false;uint32_t frames=0;
  uint64_t rendererTotalUs=0,panelTotalUs=0;
  uint32_t rendererMaxUs=0,panelMaxUs=0;
 } majorTiming;
 bool rendererUnchanged=false;uint32_t rendererBeforeHash=0,rendererAfterHash=0;
 int32_t mismatchEffect=-1,mismatchMs=-1,mismatchAt=-1;
};
constexpr uint32_t kUiEffectParityFrames=42;
constexpr uint32_t kUiHoldParityFrames=44;
// Explicit USB diagnostics: render entry/collection and Hold reels offscreen,
// transfer only the unchanged real LCD image, and never advance game/audio.
UiEntryBench uiBenchmarkEntry();
// Read-only sticky diagnostics for the most recent actual collection.
String uiAwardDiagnostic();
struct UiAwardSceneTest {
 bool ran=false,unchanged=false,cueRequested=false,fountainTriggered=false;
 uint32_t beforeHash=0,afterHash=0,frames=0,cueFirstMs=0,fountainFirstMs=0,maxMusicLevel=0;
 uint32_t counterMask=0;int64_t countedMinor=0;
};
// Explicit USB QA only: replay the actual large-award UI/coin/music path,
// then restore the complete engine and the original ready game. No save,
// spin or RNG action occurs; a power loss still restores the original NVS.
// spinOnly replays FreeIntro/+3-counter/individual award/continuous-coin tail.
// automatic uses the same presentation with autoplay enabled; it performs no paid action.
UiAwardSceneTest uiTestAwardScene(bool spinOnly=false,bool automatic=false);
struct UiArrivalSceneTest {
 bool ran=false,unchanged=false;
 uint32_t beforeHash=0,afterHash=0,accepted=0,hardwareQueued=0,failures=0,pending=0,durationMs=0;
 uint32_t frames[3]{},rates[3]{},pcmFNV[3]{};
};
// Explicit quiet-idle USB audition only. Uses real registered landing voices
// at the measured A07 intervals; does not spin, save or alter any preference.
UiArrivalSceneTest uiTestArrivalScene();
struct UiMathProfileTest {
 bool ran=false,unchanged=false;uint32_t beforeHash=0,afterHash=0;
 uint32_t model=0,miniCredits=0,minorCredits=0,betMinor=0,holdMinor=0,roundMinor=0;
 uint32_t baseKing3=0,freeKing5=0,baseCandle5=0,baseJack4=0,linePathFNV=0;
 uint32_t baseQueen4=0,freeBell3=0,freeBook3=0,freeJack3=0,freeJack5=0,freeTen5=0,freeCandle5=0;
};
// Unsupported legacy profile diagnostic: returns ran=false; it cannot clone
// or rewind the external RNG service. Use the independent pure-math audit.
UiMathProfileTest uiTestMathProfile();
uint64_t uiActionCount();
const uint16_t* uiFramePixels();
String uiSnapshot();

// Session presentation preference; no game or RNG state is persisted.
void uiSetCreditsDisplay(bool credits);
bool uiCreditsDisplay();

void uiSetSoundPreferences(bool spin, bool landings, uint8_t volume);
bool uiSpinSoundEnabled();
bool uiLandingSoundEnabled();
uint8_t uiSoundVolume();

bool uiAddCredits(uint32_t amountMinor);
void uiSetCreditPreferences(bool autoRefill,uint32_t lastAddedMinor);
bool uiAutoRefillEnabled();
uint32_t uiLastAddedMinor();
// Funding changes the RAM balance independently of paid-action count.
uint32_t uiCreditRevision();

// RAM-session owner/stop diagnostics. Import/export/recovery APIs are removed.
uint32_t uiRuntimeRevision();
bool uiCheckpointBlocked();
void uiBlockCheckpoint();
String uiMathDiagnostic();

// Historical pause names now only block the current RAM session. No recovery
// or durable-storage guarantee exists; a restart begins a fresh game.
enum class UiCheckpointPause:uint8_t {None,UnknownBank,UndurableRecord};
void uiSetCheckpointPause(UiCheckpointPause reason);
bool uiCheckpointPaused();
// Main installs this after uiInitFresh. A real action requires a live session
// owner before its presentation is exposed. Missing ownership blocks draws.
using UiOutcomeCheckpointHandler=bool(*)();
void uiSetOutcomeCheckpointHandler(UiOutcomeCheckpointHandler handler);

// Separate bounded eligibility/geometry report; not a pixel measurement.
String uiSageDiagnostic();

// Adapter must discard/reset input while fenced; uiFrame acknowledges release.
bool uiReturnSelectionInputFenced();
// RAM-session ownership signal; no persisted tuple is read or written.
bool uiReturnSelectionCheckpointPending();
