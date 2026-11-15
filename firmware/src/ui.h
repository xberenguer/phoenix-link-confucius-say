#pragma once
#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// Native Confucius Say UI / engine contract. The renderer owns presentation;
// uiSave returns the required length when destination is null or too small.
// uiRestore validates its own game magic/version/size/hash before mutation.
// Main detects action-count changes for autosave; no radio is initialized.
// Exact startup seeds include zero; the legacy zero shorthand stays supported.
void uiInit(uint64_t seed = 0, bool seedIsExact = false);
struct UiEffectCache {bool ready;uint32_t bytes,fireFrames,birdFrames,rate,buildUs;bool alphaReady=false;uint32_t alphaChecks=0,alphaBoundaryFNV=0;bool pickupReady=false;uint32_t pickupBytes=0,pickupHits=0,pickupMisses=0;};
// Prime immutable PSRAM once, after the restored game is visible and the
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
// Clone-only diagnostic of the observed wager and stale imported prize cache.
// The live game, wager, every progress bank and PCG remain untouched.
UiMathProfileTest uiTestMathProfile();
uint64_t uiActionCount();
const uint16_t* uiFramePixels();
size_t uiSave(void* destination, size_t capacity);
bool uiRestore(const void* source, size_t length);
// Optional source-board/flame-phase sidecar, bound to the game checkpoint.
// Reads the prior200-byte format as well; the financial snapshot is unchanged.
constexpr size_t kUiSceneCapacity=230;
size_t uiSaveScene(void* target,size_t capacity,const void* game,size_t gameLength);
bool uiRestoreScene(const void* source,size_t length,const void* game,size_t gameLength);
String uiSnapshot();

// Presentation preference: stored separately from the game/RNG snapshot.
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
// Funding can change a save without advancing the game's action/RNG count.
uint32_t uiCreditRevision();

// Atomic canonical bank/scene/custom/service checkpoint. Legacy uiRestore stays supported.
constexpr size_t kUiCheckpointCapacity=3106;
size_t uiSaveCheckpoint(void*target,size_t capacity,uint64_t generation);
bool uiRestoreCheckpoint(const void*source,size_t length,uint64_t&generation);
uint32_t uiRuntimeRevision();
bool uiCheckpointBlocked();
void uiBlockCheckpoint();
String uiMathDiagnostic();

// Main owns durable-storage authority; pausing never changes the game bank.
enum class UiCheckpointPause:uint8_t {None,UnknownBank,UndurableRecord};
void uiSetCheckpointPause(UiCheckpointPause reason);
bool uiCheckpointPaused();
// Main installs this after uiInit. A real action must be checkpointed before
// its presentation can be exposed. Missing ownership fails before a draw.
using UiOutcomeCheckpointHandler=bool(*)();
void uiSetOutcomeCheckpointHandler(UiOutcomeCheckpointHandler handler);
bool uiRestoreLegacyCheckpoint(const void*game,size_t length,const void*scene,size_t sceneLength,bool scenePresent);

// Separate bounded eligibility/geometry report; not a pixel measurement.
String uiSageDiagnostic();

// Adapter must discard/reset input while fenced; uiFrame acknowledges release.
bool uiReturnSelectionInputFenced();
// Ownership signal only: the real Main callback still compares/commits tuples.
bool uiReturnSelectionCheckpointPending();
