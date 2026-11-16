#include "ui.h"
#include "build_version.h"
#include "phoenix_engine.h"
#include "cabinet_profile.h"
#include "static_grand_table.h"
#include "return_selector86.h"
#include "math_runtime_identity.h"
#include "capacity_service.h"
#include "money_math.h"
#include "generated_assets.h"
#include "sage_crown122.h"
#include "pdf_assets.h"
#include "phoenix_numerals.h"
#include "phoenix_effects.h"
#include "clean_coins.h"
#include "coin_motion.h"
#include "coin_sampler.h"
#include "winged_phoenix.h"
#include "landing_cues.h"
#include "reel_motion.h"
#include "ui_font.h"
#include "cash_font.h"
#include "upper_win.h"
#include "hold_winner.h"
#include "meter_font.h"
#include "native_title.h"
#include "counter_hud.h"
#include "denomination_minbet46.h"
#include "counter_fire89.h"
#include "counter_calm90.h"
#include "feature_entry_motion.h"
#include "feature_music_timeline.h"
#include "revival_timing.h"
#include "entry_art.h"
#include "phoenix_art.h"
#include "fire_field.h"
#include "effect_frame_cache.h"
#include "pixel_blend.h"
#include "fire_alpha.h"
#include "collect_art.h"
#include "collection_motion.h"
#include "collection_steps101.h"
#include "collection_pickup.h"
#include "pickup_frame_cache.h"
#include "pickup_arrival.h"
#include "feature_award_director.h"
#include "spin_award_timing.h"
#include "wisdom_entry_controller30.h"
#include "wisdom_lower_win88.h"
#include "wisdom_entry_timing29.h"
#include "award_art.h"
#include "authored_major112.h"
#include "gold_burn.h"
#include "reference_gold_v0265.h"
#include "audio.h"
#include "hold_opening31.h"
#include "hold_initial_counter67.h"
#include <M5Unified.h>
#include <esp_system.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <new>
#include <type_traits>
#include <string>
#ifndef UI_HOST_TEST
#include <esp32-hal-psram.h>
#include "audio_cues.h"
#include "audio_music.h"
#endif

// Presentation is independent of the engine RNG. A spin is committed once,
// then a cosmetic reel strip moves continuously onto that committed result.
namespace {
// Capture the incoming clock before the local macro, including advancing-clock
// test fixtures. Every UI deadline freezes together; Main retries use wall time.
uint32_t uiWallMillis(){return millis();}
uint32_t uiClockOffset=0,uiPauseWallAt=0,uiPauseGameAt=0;
UiCheckpointPause storagePause=UiCheckpointPause::None;
UiOutcomeCheckpointHandler outcomeCheckpointHandler=nullptr;
uint32_t uiGameMillis(){return storagePause==UiCheckpointPause::None?uiWallMillis()-uiClockOffset:uiPauseGameAt;}
#pragma push_macro("millis")
#undef millis
#define millis uiGameMillis
using phoenix::Cell; using phoenix::Symbol; using phoenix::Mode;
constexpr int W=240,H=135,RX=2,RY=16,TW=47,TH=35,RW=235,RH=105;
constexpr int kReelLeft=RX; // unambiguous outside this namespace on Arduino
// Share the outer-right reel edge: no one-pixel outboard counter panel.
constexpr int METER_RIGHT=RX+RW+1,METER_LEFT=METER_RIGHT-34,METER_CENTER=(METER_LEFT+METER_RIGHT)/2;
constexpr uint16_t GOLD=0xe585,BRIGHT=0xff6e,WHITE=0xffff,JADE=0x1267,BLACK=0x0000;
phoenix::Engine engine;
hold_opening31::State holdOpening31;
#ifdef UI_HOLD_OPENING_TEST_HOOK
hold_opening31::Observation holdOpeningTestAudio;
#endif
uint16_t pixels[W*H];
int dotClipTop=0,dotClipBottom=H;
constexpr unsigned kReelStripCells=128;
Cell visible[15],target[15],previous[15],holdBase[15],strips[5][kReelStripCells];
// A vacant bonus square is its own finite strip. Its artwork/prize is chosen
// once per action, so a moving tile cannot change identity between frames.
constexpr unsigned kHoldStripCells=64;
constexpr uint32_t kHoldFirstMs=1250,kHoldColumnMs=500;
Cell holdStrips[15][kHoldStripCells];
bool holdStripStartsBlank=false;
// The frozen ordinary Hold material is composited once into PSRAM. Keep the
// original path as an allocation-failure fallback and native pixel oracle.
constexpr unsigned kHoldMaterialCount=unsigned(Symbol::Phoenix);
constexpr size_t kHoldMaterialPixels=kHoldMaterialCount*TW*TH;
constexpr uint32_t kHoldMaterialExpectedFNV=1464298268u;
uint16_t* holdMaterialPixels=nullptr;
uint32_t holdMaterialHash=0;
enum class Phase {Ready,Spin,Land,Expand,Revive,FreeIntro,FreeCounter,SpinAward,SpinAwardSettle,HoldWipe,HoldIntro,Jackpot,Collect,WinTransfer,ReturnWipe,Outro,AwardIntro};
enum class View {Game,Menu,Bet,Rules,Options,Reset,Lucky,Sound,Volume,Math,Service,Jackpots,Paylines,Settings};
Phase phase=Phase::Ready; View view=View::Game;
uint32_t phaseAt=0,phaseMs=0,lastDraw=0,readyAt=0,autoAt=0,frameCount=0,lastDrawUs=0,maxDrawUs=0;
constexpr uint32_t kPaylinePeriodMs=1300,kPaylineBlinkMs=650,kPaylineOnMs=325;
uint32_t lineDisplayAt=0;bool lineClockValid=false;
uint32_t reelMs[5]={},phoenixClockAt=0;
uint16_t phoenixPhases[15]={};
bool phoenixPhasesRestored=false,spinQuickStopped=false,spinLandingCoalesced=false;
int displayRespins=0;
// The upper-screen collected announcement precedes the existing pickup
// director. On the Stick it temporarily occupies the centre of the reels.
// This changes presentation timing only; all awards are already committed.
constexpr uint32_t kCollectedHold=1400,kCollectedBurn=900,kCollectedIntro=kCollectedHold+kCollectedBurn;
constexpr uint32_t kCollectLead=5333,kCollectStep=1333,kCollectRepeatStep=1067,kCollectTail=1200,kCollectArrival=collection_pickup::arrivalMs;
// Two direct Confucius multiplier sequences support repeated same-cell flights.
// 1067ms is a local~80% cadence fit; it is not a recovered universal timer.
int lastCollectSound=-1;
bool collectionNamedDone=false,fountainActive=false;
uint32_t fountainAt=0;
// Continuous emission uses absolute birth times, never elapsed modulo a clip.
bool fountainContinuous=false,fountainAwardMusic=false;uint32_t fountainStopMs=0,fountainMusicAt=0;
struct SpinAward {
 bool pending=false,finished=false,withCoins=true;uint32_t visualAt=0;uint64_t action=0;
 int64_t amount=0,before=0,bet=0,shown=0;
};
SpinAward spinAward;
wisdom_ctrl30::State wisdomEntry30;
bool freeIntroIsRetrigger=false;uint16_t freeIntroOldTotal=0;
bool spinAwardPhase(){return phase==Phase::SpinAward||phase==Phase::SpinAwardSettle;}
bool freeRetriggerIntro(){return freeIntroIsRetrigger;}
void clearSpinAward(){spinAward=SpinAward{};fountainContinuous=false;fountainAwardMusic=false;fountainStopMs=fountainMusicAt=0;}
bool holdWipeRevealed=false;
bool holdEntryMeter=false;
int reelSteps[5]={},lastArrived=-1,menuItem=0,rulesPage=0,optionItem=0;
unsigned paylinesPage=0,paylinesLayout=0;int settingsItem=0;
int64_t cabinetDenom=100;uint8_t cabinetLines=5;unsigned cabinetFocus=0;bool cabinetAvailable=true,cabinetMenuRequested=false,jackpotBoardRequested198=false;
const cabinet_profile::Coin& cabinetCoin(){const auto* coin=cabinet_profile::find(cabinetDenom);return coin?*coin:cabinet_profile::coins[0];}
const cabinet_profile::BetRow& cabinetRow(){const auto& coin=cabinetCoin();const auto index=cabinet_profile::lineIndex(coin,cabinetLines);return coin.rows[index<coin.count?index:0];}
unsigned cabinetBetBase(){return cabinet_profile::denominationCount+cabinetCoin().count;}
unsigned cabinetReturnFocus(){return cabinetBetBase()+cabinetRow().count;}
void cabinetFocusBet(){cabinetFocus=cabinetBetBase();const auto& row=cabinetRow();for(unsigned i=0;i<row.count;++i)if(row.multipliers[i]==engine.state.betMultiplier)cabinetFocus+=i;}
void cabinetDraftDenom(unsigned index){const auto& coin=cabinet_profile::coins[index];cabinetDenom=coin.minor;if(cabinet_profile::lineIndex(coin,cabinetLines)==coin.count)cabinetLines=coin.lines[0];cabinetFocus=cabinet_profile::denominationCount+cabinet_profile::lineIndex(coin,cabinetLines);}
int creditItem=0;
bool autoRefill=false;
uint32_t lastAddedMinor=1000000,creditRevision=0;
constexpr uint32_t creditAmounts[]={100000,1000000,10000000};
bool randomFailureShown125=false;
bool initialized=false,dirty=true,autoPlay=false,holdView=false,pendingFree=false,pendingHold=false,pendingOutro=false,pendingChained=false,introChained=false;
uint16_t heldMask=0,landedMask=0,pendingFreeCount=0,displayMeter=0,displayQueuedHolds=0;
int64_t displayBank=0,displayWin=0,winFrom=0,winTo=0;
uint32_t winAt=0;
char statusLine[72]={};
constexpr bool spinSounds=false;bool landingSounds=true,hostMuted=false;uint8_t soundVolume=192;int soundItem=0;uint32_t testJingleAt=0;bool testJingleQueued=false,testAwardJingle=false;
bool holdBackdropCaptured=false;
bool creditsDisplay=false;uint64_t luckyOfferAction=UINT64_MAX;
bool winCuePending=false;
bool awaitingBonusResume=false;
static_grand::Preference mathSelection;
const static_grand::Family* mathFamily=&static_grand_table::family;
uint8_t mathPreference[static_grand::selectionBytes]{};
uint32_t mathStoredModel=phoenix::local::modelId,runtimeRevision=0;
phoenix_capacity::Latch capacityService;
bool amountFailureShown190=false;
void enterAmountFault190();
bool checkpointBlocked=false;
// Untuned SGR1 records from this known engine are legacy migration intent.
// Deriving pending status from the preserved wire makes it survive reboot.
// Normal menus cannot create new Untuned choices. Reference fixtures require
// this separate compile-time hook AND an explicit per-fixture opt-in.
#ifdef UI_REFERENCE_MATH_FIXTURE
bool untunedReferenceMathFixture=false;
void useUntunedReferenceMathFixture(){
 untunedReferenceMathFixture=true;mathSelection={};mathSelection.wire=static_grand::Wire::Legacy1;
 mathStoredModel=phoenix::local::previousEPriceModelId;
 memset(mathPreference,0,sizeof(mathPreference));static_grand::u32(mathPreference,0x31524753u);
 static_grand::u32(mathPreference+40,mathStoredModel);static_grand::u32(mathPreference+44,static_grand::checksum(mathPreference,44));
}
#endif
bool knownUntunedMathModel(){return static_grand::knownUntunedModel(mathStoredModel);}
bool referenceLocalMath(){
#ifdef UI_REFERENCE_MATH_FIXTURE
 return untunedReferenceMathFixture&&mathSelection.wire==static_grand::Wire::Legacy1&&mathSelection.legacy.mode==legacy_static_grand::Mode::UntunedLocal&&knownUntunedMathModel();
#else
 return false;
#endif
}
bool defaultMathPending(){return !referenceLocalMath()&&mathSelection.wire==static_grand::Wire::Legacy1&&mathSelection.legacy.mode==legacy_static_grand::Mode::UntunedLocal&&knownUntunedMathModel();}
static_grand::Selection defaultMathChoice(){return static_grand::retained(*mathFamily,4);}
bool legacyMathPending(){uint8_t slot=0;return !referenceLocalMath()&&mathSelection.wire==static_grand::Wire::Legacy1&&static_grand::legacySlot(mathSelection,mathStoredModel,slot);}
bool priorNeutralMathPending(){uint8_t slot=0;return !referenceLocalMath()&&static_grand::priorNeutralSlot(mathSelection,mathStoredModel,slot);}
static_grand::Selection effectiveMathChoice(){
 static_grand::Selection selection;
 if(!static_grand::effective(mathSelection,mathStoredModel,*mathFamily,selection))return {};
 return selection;
}
bool runtimeFinancialBlocked(){return amountFailureShown190||!engine.randomHealthy()||checkpointBlocked||capacityService.active()||storagePause!=UiCheckpointPause::None;}
// Volatile drafts are never persisted or normalized by browsing.
return_selector86::State returnSelector86;
bool returnChoiceRecognized86(){static_grand::Selection selected;return static_grand::effective(mathSelection,mathStoredModel,*mathFamily,selected);}
bool returnClosedBoundary86(){
 return initialized&&!runtimeFinancialBlocked()&&phase==Phase::Ready&&view==View::Math&&
  engine.canChangeBet()&&!engine.hasCustomJackpots()&&!engine.state.freeSession&&!engine.state.freeLeft&&!engine.state.respinsLeft&&!engine.state.pendingMeterHolds&&
  !pendingFree&&!pendingHold&&!pendingOutro&&!pendingChained&&!spinAward.pending&&!cabinetMenuRequested&&!jackpotBoardRequested198&&
  !returnSelector86.pending&&!returnSelector86.recoveryFence&&returnChoiceRecognized86();
}
bool returnDetail86(unsigned option,return_selector86::Detail&out){
 if(engine.hasCustomJackpots()||option>=6)return false;
 const auto selected=static_grand::retained(*mathFamily,return_selector86::optionToSlot[option]);
 const static_grand::Entry* row=nullptr;
 if(static_grand::resolve(*mathFamily,selected,static_grand::profile(engine),row)!=static_grand::Result::Applied)return false;
 out={row->profile,mathFamily->fingerprint,row->grandCredits,row->lowReturnPpm,row->estimateReturnPpm,row->highReturnPpm,uint8_t(option),true};return true;
}
void openReturnSelector86(){
 returnSelector86={};const auto selected=effectiveMathChoice();const int saved=selected.family!=static_grand::Fingerprint{}?return_selector86::optionFor(selected.slot):-1;
 if(saved>=0)returnSelector86.cursor=uint8_t(saved);
 view=View::Math;statusLine[0]=0;
 if(!returnClosedBoundary86())returnSelector86.page=return_selector86::Page::Locked;
 dirty=true;
}
void finishReturnSelection86(){
 returnSelector86.pending=false;returnSelector86.recoveryFence=true;returnSelector86.page=return_selector86::Page::List;
 autoPlay=false;awaitingBonusResume=false;view=View::Game;statusLine[0]=0;dirty=true;
}
void applyReturnSelection86(); // owner barrier is defined later, next to action()
void selectReturnOption86(){
 using Page=return_selector86::Page;
 if(returnSelector86.pending||returnSelector86.recoveryFence)return;
 if(returnSelector86.page==Page::Help||returnSelector86.page==Page::Locked){if(returnSelector86.page==Page::Help)returnSelector86.page=Page::List;else {menuItem=7;view=View::Menu;statusLine[0]=0;}dirty=true;return;}
 if(returnSelector86.page==Page::Detail){applyReturnSelection86();return;}
 if(returnSelector86.cursor==6){returnSelector86.page=Page::Help;dirty=true;return;}
 if(returnSelector86.cursor==7){menuItem=7;view=View::Menu;statusLine[0]=0;dirty=true;return;}
 if(!returnClosedBoundary86()||!returnDetail86(returnSelector86.cursor,returnSelector86.detail)){returnSelector86.page=Page::Locked;dirty=true;return;}
 returnSelector86.page=Page::Detail;returnSelector86.contextChanged=false;dirty=true;
}
void nextReturnOption86(){
 using Page=return_selector86::Page;
 if(returnSelector86.pending||returnSelector86.recoveryFence)return;
 if(returnSelector86.page==Page::List)returnSelector86.cursor=uint8_t((returnSelector86.cursor+1)%8);
 else if(returnSelector86.page==Page::Locked){menuItem=7;view=View::Menu;statusLine[0]=0;}
 else returnSelector86.page=Page::List;
 returnSelector86.contextChanged=false;dirty=true;
}

void initializeMathPreference(){
#ifdef UI_REFERENCE_MATH_FIXTURE
 untunedReferenceMathFixture=false;
#endif
 mathSelection=static_grand::current(defaultMathChoice());mathStoredModel=phoenix::local::modelId;
 static_grand::encode(mathSelection.neutral,mathPreference,sizeof(mathPreference));
}
void enterService(){autoPlay=false;awaitingBonusResume=true;view=View::Service;dirty=true;}
void noteCapacityFailure(phoenix_capacity::Attempt kind){if(engine.capacityBlocked()&&capacityService.capture(engine,kind)){++runtimeRevision;enterService();}}
bool customWagerUsable(){
 if(referenceLocalMath())return true;
 const static_grand::Entry*entry=nullptr;
 return static_grand::resolve(*mathFamily,effectiveMathChoice(),static_grand::profile(engine),entry)==static_grand::Result::Applied;
}
bool prepareCustomRound(){
 if(runtimeFinancialBlocked())return false;
 if(engine.state.mode!=Mode::Base)return true;
 if(referenceLocalMath()||engine.hasCustomJackpots())return true;
 const auto requested=effectiveMathChoice();
 uint8_t normalized[static_grand::selectionBytes];
 if(static_grand::encode(requested,normalized,sizeof(normalized))!=sizeof(normalized)){
  autoPlay=false;snprintf(statusLine,sizeof(statusLine),"CUSTOM CONFIG NEEDS SERVICE - GAME MENU");dirty=true;return false;
 }
 auto active=mathSelection.neutral;
 const auto result=static_grand::selectFuture(engine,active,requested,*mathFamily,uiReady());
 // Pending known SGR1/SGR2 is never re-encoded on import, owed actions or autosave.
 // Only an already-qualified Ready/Base selection commits current intent.
 if(result==static_grand::Result::Applied){
  mathSelection=static_grand::current(active);mathStoredModel=phoenix::local::modelId;
  memcpy(mathPreference,normalized,sizeof(mathPreference));++runtimeRevision;dirty=true;
 }
 if(result==static_grand::Result::Applied||result==static_grand::Result::NoChange)return true;
 autoPlay=false;snprintf(statusLine,sizeof(statusLine),"CUSTOM CONFIG UNAVAILABLE - GAME MENU");dirty=true;return false;
}
bool applyRuntimeWager(int64_t coin,uint8_t lines,uint8_t multiplier){
 if(runtimeFinancialBlocked())return false;
 const bool changed=coin!=engine.state.coinValue||lines!=engine.state.lineCount||multiplier!=engine.state.betMultiplier;
 if(changed&&engine.state.actions==UINT64_MAX){if(capacityService.capturePreflight(engine,phoenix_capacity::Attempt::Normal)){++runtimeRevision;enterService();}return false;}
 if(referenceLocalMath()){
  if(phase!=Phase::Ready||!engine.setWager(coin,lines,multiplier))return false;
 }else{
  // Resolve the target schedule before a wager change can consume randomness.
  const auto result=static_grand::setWager(engine,effectiveMathChoice(),*mathFamily,coin,lines,multiplier,phase==Phase::Ready);
  if(result!=static_grand::Result::Applied&&result!=static_grand::Result::NoChange){snprintf(statusLine,sizeof(statusLine),"WAGER UNAVAILABLE FOR CUSTOM CONFIG");dirty=true;return false;}
 }
 ++runtimeRevision;displayMeter=engine.state.meter;displayQueuedHolds=engine.state.pendingMeterHolds;dirty=true;return true;
}
struct AwardTrace {
 bool valid=false,test=false,fountainEligible=false,grand=false,cueRequested=false,fountainStarted=false;
 bool collection=false,autoPlayAtBegin=false,holdSettled=false,freeEnded=false,roundCompleted=false,combinedRoundBasis=false;
 uint8_t phoenixCount=0,cashCellCount=0,miniCount=0,minorCount=0,majorCount=0;
 uint64_t action=0;int64_t cash=0,bet=0,named=0,fountainBasis=0,roundPayout=0;
 uint32_t beganAt=0,cueFirstMs=0,cueFirstAt=0,fountainFirstMs=0,fountainFirstAt=0,backendStartsBefore=0;
 FeatureMusicPart part=FeatureMusicPart::Off;
};
AwardTrace lastAward;
bool screenFlipped=false;
#ifdef UI_LANDING_TEST_HOOK
void (*landingTestObserver)(landing_cues::Tone,uint32_t)=nullptr;
#endif
#ifdef UI_MUSIC_TEST_HOOK
void (*musicTestObserver)(FeatureMusicPart)=nullptr;
void (*awardEpochTestObserver)(FeatureMusicPart)=nullptr;
void (*completionTestObserver)(bool,uint32_t)=nullptr;
void (*completionHandoffTestObserver)(bool,uint32_t,bool)=nullptr;
void (*collectionTestObserver)(uint32_t)=nullptr;
bool completionTestReady=true;
bool featureBusyTest=false;
#endif
void requestCollectionCue(){
#ifndef UI_HOST_TEST
 audioPlay(AudioCue::CountUp);
#elif defined(UI_MUSIC_TEST_HOOK)
 if(collectionTestObserver)collectionTestObserver(millis());
#endif
}
bool completionReady(){
#ifndef UI_HOST_TEST
 return audioCompletionReady();
#elif defined(UI_MUSIC_TEST_HOOK)
 return completionTestReady;
#else
 return true;
#endif
}
void requestCompletionCue(bool active,uint32_t elapsed,bool finishHoldMusic=false){
#if !defined(UI_HOST_TEST) || defined(UI_TWIN_MUSIC_BACKEND_TEST)
 audioSetCompletionCue(active,elapsed,finishHoldMusic);
#elif defined(UI_MUSIC_TEST_HOOK)
 if(completionTestObserver)completionTestObserver(active,elapsed);
 if(completionHandoffTestObserver)completionHandoffTestObserver(active,elapsed,finishHoldMusic);
#else
 (void)active;(void)elapsed;(void)finishHoldMusic;
#endif
}
void requestFeatureMusic(FeatureMusicPart part,bool beginAward=false){
#if !defined(UI_HOST_TEST) || defined(UI_TWIN_MUSIC_BACKEND_TEST)
 audioSetFeatureMusic(part,beginAward);
#elif defined(UI_MUSIC_TEST_HOOK)
 if(musicTestObserver)musicTestObserver(part);
 if(beginAward&&awardEpochTestObserver)awardEpochTestObserver(part);
#else
 (void)part;(void)beginAward;
#endif
}

// Source cues use independent, persistent controls. Win/bonus music remains
// available when reel motion/landing sounds are disabled.
void startReelSound(){
#ifndef UI_HOST_TEST
 audioStartReels();
#endif
}
void stopReelSound(){
#ifndef UI_HOST_TEST
 audioStopReels();
#endif
}
void cueStop(uint32_t impact){
#if !defined(UI_HOST_TEST) || defined(UI_LANDING_BACKEND_TEST)
 audioQueueLanding(AudioCue::ReelStop,millis(),impact);
#elif defined(UI_LANDING_TEST_HOOK)
 (void)impact;
 if(landingTestObserver)landingTestObserver(landing_cues::Tone::Stop,millis());
#else
 (void)impact;
#endif
}
void cuePhoenix(landing_cues::Tone tone,uint32_t impact){
 if(tone!=landing_cues::Tone::Initial&&tone!=landing_cues::Tone::Follow&&tone!=landing_cues::Tone::Third)return;
#if !defined(UI_HOST_TEST) || defined(UI_LANDING_BACKEND_TEST)
 const AudioCue cue=tone==landing_cues::Tone::Third?AudioCue::PhoenixThird:tone==landing_cues::Tone::Follow?AudioCue::PhoenixFollow:AudioCue::PhoenixLand;
 audioQueueLanding(cue,millis(),impact);
#elif defined(UI_LANDING_TEST_HOOK)
 (void)impact;
 if(landingTestObserver)landingTestObserver(tone,millis());
#else
 (void)impact;
#endif
}
void cuePhoenix(bool follow,uint32_t impact){
 cuePhoenix(follow?landing_cues::Tone::Follow:landing_cues::Tone::Initial,impact);
}
landing_cues::Tone landingForGroup(unsigned first,unsigned last){
 uint8_t counts[5]={};bool anyVacant=false;
 for(unsigned c=0;c<5;++c)for(unsigned r=0;r<3;++r){
  const unsigned i=c*3+r;
  if(!(heldMask&(1u<<i))){
   if(target[i].symbol==Symbol::Phoenix)++counts[c];
   if(c>=first&&c<=last)anyVacant=true;
  }
 }
 const bool regularSingle=engine.presentation.paidSpin&&!holdView&&!engine.presentation.twinSpin&&!spinQuickStopped&&!spinLandingCoalesced&&first==last;
 return landing_cues::choose(counts,first,last,holdView,anyVacant,regularSingle);
}
void cueButton(){
#ifndef UI_HOST_TEST
  audioPlay(AudioCue::Button);
#endif
}
void cueCredit(){
#if !defined(UI_HOST_TEST) || defined(UI_CREDIT_BACKEND_TEST)
  // Finite protected stinger: the next menu click or reel spin cannot cut it.
  // Call only for user changes or successfully committed credits, never load.
  audioPlay(AudioCue::CashIn);
#endif
}
void cueWin(){
#ifndef UI_HOST_TEST
  audioPlay(AudioCue::Win);
#endif
}
hold_opening31::Observation holdOpeningAudio(){
#ifdef UI_HOLD_OPENING_TEST_HOOK
 return holdOpeningTestAudio;
#elif defined(UI_HOST_TEST)
 return {};
#else
 const auto music=audioMusicDiagnostics();
 return {audioReady()&&!audioMuted()&&audioVolume()>0,music.ready,
         music.part==FeatureMusicPart::Hold&&music.playing,music.queuedFrames};
#endif
}
void armHoldOpening(){
 const auto audio=holdOpeningAudio();holdOpening31.arm(millis(),audio.queuedFrames);
}
void cueFeature(bool free,bool meter=false){
 if(!free)armHoldOpening();
#if !defined(UI_HOST_TEST) || defined(UI_WISDOM_BACKEND_TEST)
  if(!free){audioSetFeatureMusic(FeatureMusicPart::Hold);return;}
  if(!audioPlay(meter?AudioCue::MeterTrigger:free?AudioCue::FreeStart:AudioCue::HoldStart))audioPlay(free?AudioCue::FreeStart:AudioCue::HoldStart);
#else
  (void)free; (void)meter;
#endif
}
bool ordinaryWinAudio(){
 const auto& p=engine.presentation;
 if(p.lineWinMinor<=0||p.freeAwarded||p.holdEntered||p.holdSettled||p.grandAwarded||p.freeEnded)return false;
 // The repeated measured flourish belongs to winning Sage appearances.
 // Do not replay an invented tune on every ordinary card/book/bell payline.
 for(unsigned i=0;i<15;++i)if(p.spinResult[i].symbol==Symbol::Wild&&(p.winMask&(1u<<i)))return true;
 return false;
}
bool featureAudioBusy(){
#if !defined(UI_HOST_TEST) || defined(UI_WISDOM_BACKEND_TEST)
 return audioFeatureBusy();
#elif defined(UI_MUSIC_TEST_HOOK)
 return featureBusyTest;
#else
 return false;
#endif
}
bool landingAudioBusy(){
#if !defined(UI_HOST_TEST) || defined(UI_LANDING_BACKEND_TEST)
 return audioLandingsBusy();
#else
 return false;
#endif
}
uint32_t featureIntroDuration(bool free){uint32_t ms=free?2500:2400;
#if !defined(UI_HOST_TEST) || defined(UI_WISDOM_BACKEND_TEST)
 ms=std::max(ms,audioCueDurationMs(free?AudioCue::FreeStart:AudioCue::HoldStart));
#endif
 return ms;}
uint16_t blend(uint16_t src,uint16_t dst,unsigned a){return pixel_blend::over(src,dst,a);}
void dot(int x,int y,uint16_t color,unsigned alpha=255){if(x>=0&&x<W&&y>=dotClipTop&&y<dotClipBottom&&y>=0&&y<H)pixels[y*W+x]=alpha==255?color:blend(color,pixels[y*W+x],alpha);}
void fill(int x,int y,int w,int h,uint16_t c,unsigned alpha=255){for(int yy=std::max(y,0);yy<std::min(y+h,H);++yy)for(int xx=std::max(x,0);xx<std::min(x+w,W);++xx)dot(xx,yy,c,alpha);}
void rect(int x,int y,int w,int h,uint16_t c){fill(x,y,w,1,c);fill(x,y+h-1,w,1,c);fill(x,y,1,h,c);fill(x+w-1,y,1,h,c);}
void line(int x0,int y0,int x1,int y1,uint16_t color,unsigned alpha=255){int dx=std::abs(x1-x0),sx=x0<x1?1:-1,dy=-std::abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;for(;;){dot(x0,y0,color,alpha);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}}
void image(const pa::Image& im,int x,int y,int left=0,int top=0,int right=W,int bottom=H){
 if(!im.rgb||!im.w||!im.h)return;
 const auto* spans=im.alpha&&!im.key?phoenix_art::alphaSpans(im):nullptr;
 for(int r=0;r<im.h;++r){
  const int yy=y+r;if(yy<top||yy>=bottom||yy<0||yy>=H)continue;
  int a=std::max({0,left-x,-x}),b=std::min({int(im.w),right-x,W-x});
  if(spans){a=std::max(a,int(spans[r].left));b=std::min(b,int(spans[r].right));}
  if(a>=b)continue;
  const auto* source=im.rgb+r*im.w;auto* dest=pixels+yy*W+x+a;
  if(!im.alpha&&!im.key){memcpy(dest,source+a,size_t(b-a)*2);continue;}
  // Bounds and the temporary vertical clip are constant across this row.
  // Avoid repeating dot()'s coordinate/clip checks for every alpha texel.
  if(yy<dotClipTop||yy>=dotClipBottom)continue;
  for(int c=a;c<b;++c,++dest){
   const uint16_t color=source[c];if(im.key&&color==im.key)continue;
   const unsigned alpha=im.alpha?im.alpha[r*im.w+c]:255;
   if(alpha)*dest=alpha==255?color:blend(color,*dest,alpha);
  }
 }
}
void overlay(const pdfa::Overlay& im,int x,int y){for(int r=0;r<im.image.h;++r)for(int c=0;c<im.image.w;++c){unsigned at=r*im.image.w+c;if(im.alpha[at])dot(x+c,y+r,im.image.rgb[at],im.alpha[at]);}}
int textWidth(const char* value,unsigned size){int width=0;for(auto s=(const unsigned char*)value;*s;++s)width+=pf::glyph(size,*s).advance;return width;}
void text(const char* value,int x,int y,unsigned size=8,uint16_t color=WHITE,int align=0,int clipLeft=0,int clipRight=W){if(align==1)x-=textWidth(value,size)/2;else if(align==2)x-=textWidth(value,size);const uint8_t* p=pf::pixels(size);for(auto s=(const unsigned char*)value;*s;++s){const auto& g=pf::glyph(size,*s);for(int r=0;r<g.h;++r)for(int c=0;c<g.w;++c){uint8_t a=p[g.offset+r*g.w+c];if(a&&x+c>=clipLeft&&x+c<clipRight)dot(x+c,y+r,color,a);}x+=g.advance;}}
int cashWidth(const char* value,unsigned size){int w=0;for(auto p=(const unsigned char*)value;*p;++p)w+=pmoney::glyph(size,*p).advance;return w;}
void cashText(const char* value,int right,int y,unsigned size,uint16_t color=WHITE){
 int x=right-cashWidth(value,size);const auto* alpha=pmoney::pixels(size);
 for(auto p=(const unsigned char*)value;*p;++p){const auto& g=pmoney::glyph(size,*p);
  for(unsigned r=0;r<g.h;++r)for(unsigned c=0;c<g.w;++c)if(alpha[g.offset+r*g.w+c])dot(x+c,y+r,color,alpha[g.offset+r*g.w+c]);
  x+=g.advance;
 }
}
void headerCaption(const char* value,int y,unsigned size,uint16_t color){if(textWidth(value,size)>47)size=6;text(value,101,y,size,color,1,77,126);}
void meterNumber(unsigned number){
  const auto value=std::to_string(number);int x=METER_CENTER-(int(value.size())*pmeter::advance-1)/2;
  for(char digit:value){
    const auto* alpha=pmeter::alpha[digit-'0'];
    for(unsigned y=0;y<pmeter::height;++y)for(unsigned c=0;c<pmeter::width;++c)if(alpha[y*pmeter::width+c])dot(x+c,(14-int(pmeter::height))/2+y,pmeter::color[y],alpha[y*pmeter::width+c]);
    x+=pmeter::advance;
  }
}
std::string money(int64_t cents,bool compact=false){char b[40];uint64_t n=cents<0?uint64_t(-(cents+1))+1:uint64_t(cents);if(compact&&n%100==0)snprintf(b,sizeof(b),"%s$%llu",cents<0?"-":"",(unsigned long long)(n/100));else snprintf(b,sizeof(b),"%s$%llu.%02u",cents<0?"-":"",(unsigned long long)(n/100),unsigned(n%100));return b;}
std::string integer(int64_t n){char b[24];snprintf(b,sizeof(b),"%lld",(long long)n);return b;}
std::string hudValue(int64_t cents){
 if(!creditsDisplay)return money(cents);
 money_math::ExactParts n;if(!money_math::splitExact(cents,uint32_t(engine.state.coinValue),n)){enterAmountFault190();return "INVALID";}
 char b[40];if(n.fraction)snprintf(b,sizeof(b),"%s%llu.%0*u",cents<0?"-":"",(unsigned long long)n.whole,int(n.places),n.fraction);
 else snprintf(b,sizeof(b),"%s%llu",cents<0?"-":"",(unsigned long long)n.whole);return b;
}
void amount(int64_t cents,int right,int y,int maxWidth){
  std::string value=hudValue(cents);unsigned size=10;
  for(unsigned candidate:{10u,8u,7u}){size=candidate;if(cashWidth(value.c_str(),size)<=maxWidth)break;}
  if(cashWidth(value.c_str(),size)>maxWidth){const auto parts=money_math::compact(money_math::split(cents,creditsDisplay?uint32_t(engine.state.coinValue):100).whole);if(parts.abbreviated){char b[32];snprintf(b,sizeof(b),"%s%s%llu.%u%c",cents<0?"-":"",creditsDisplay?"":"$",(unsigned long long)parts.whole,unsigned(parts.tenth),parts.unit);value=b;}}
  cashText(value.c_str(),right,y+(10-int(size))/2,size);
}
static_assert(upper_win::detail::matte==native_title::background,"Upper WIN shares the header matte");
struct SpinAmountLayout {char value[40]={};unsigned size=10;int width=0,left=0,y=2;};
SpinAmountLayout spinAmountLayout(int64_t cents){
 constexpr int center=93,maxWidth=116;
 SpinAmountLayout result;const uint64_t n=cents<0?uint64_t(-(cents+1))+1:uint64_t(cents);
 const uint64_t unit=creditsDisplay?uint64_t(engine.state.coinValue):100;
 // Divide before scaling: valid denominations are <=1000c. This also keeps
 // a stress value at either signed64-bit limit from wrapping in credits mode.
 const uint64_t whole=n/unit;const char* sign=cents<0?"-":"";
 const auto exact=hudValue(cents);snprintf(result.value,sizeof(result.value),"%s",exact.c_str());
 for(unsigned candidate:{10u,8u,7u}){result.size=candidate;if(cashWidth(result.value,result.size)<=maxWidth)break;}
 if(cashWidth(result.value,result.size)>maxWidth){
  const uint64_t scales[]={1000000000000ULL,1000000000ULL,1000000ULL,1000ULL};
  const char units[]={'T','B','M','K'};
  for(int i=0;i<4;++i)if(whole>=scales[i]){snprintf(result.value,sizeof(result.value),"%s%s%llu.%u%c",sign,creditsDisplay?"":"$",(unsigned long long)(whole/scales[i]),unsigned(whole%scales[i]*10/scales[i]),units[i]);break;}
 }
 result.width=cashWidth(result.value,result.size);result.left=center-result.width/2;result.y=2+(10-int(result.size))/2;return result;
}
void drawSpinAwardHeader(uint32_t now){
 const int oldTop=dotClipTop,oldBottom=dotClipBottom;dotClipTop=0;dotClipBottom=14;
 fill(0,0,186,14,native_title::background);
 // Keep the flame clock continuous across early collection/400ms settle.
 // Each frame() image is consumed before the shared scratch is reused.
 const uint32_t elapsed=now-(spinAward.withCoins?fountainAt:spinAward.visualAt);
 image(upper_win::frame(elapsed),2,0);
 image(upper_win::frame(elapsed%upper_win::periodMs+upper_win::rightOffsetMs),154,0);
 const auto layout=spinAmountLayout(spinAward.shown);int x=layout.left;
 const auto* alpha=pmoney::pixels(layout.size);
 // Hard116px amount bounds leave a clear gap to both complete WIN words.
 for(auto p=(const unsigned char*)layout.value;*p;++p){
  const auto& glyph=pmoney::glyph(layout.size,*p);
  for(unsigned y=0;y<glyph.h;++y)for(unsigned c=0;c<glyph.w;++c){
   const int xx=x+int(c),yy=layout.y+int(y);
   if(xx>=35&&xx<151&&yy>=0&&yy<14){const unsigned a=alpha[glyph.offset+y*glyph.w+c];if(a)dot(xx,yy,WHITE,a);}
  }
  x+=glyph.advance;
 }
 dotClipTop=oldTop;dotClipBottom=oldBottom;
}
void copy(Cell* d,const Cell* s){memcpy(d,s,sizeof(Cell)*15);}
uint32_t cosmetic(uint32_t& s){s^=s<<13;s^=s>>17;s^=s<<5;return s;}
bool phoenixCell(const Cell& c){return c.symbol==Symbol::Phoenix;}
uint32_t phoenixClock(unsigned i,uint32_t now){
 const uint32_t period=pa::phoenixPeriodMs();
 return (phoenixPhases[i]+uint32_t(now-phoenixClockAt)%period)%period;
}
uint32_t phoenixRenderClock(unsigned i,uint32_t now){
 if(phase!=Phase::Spin||(holdView&&(heldMask&(1u<<i)))||lastArrived>=int(i/3))return phoenixClock(i,now);
 if(spinQuickStopped)return 0;
 // The target's clock is anchored to its planned stop while it approaches
 // that impact, too. This avoids replacing one fire pose with another on land.
 const int32_t period=int32_t(pa::phoenixPeriodMs());
 int32_t clock=(int32_t(now-phaseAt)-int32_t(reel_motion::impactMs(reelMs[i/3])))%period;
 if(clock<0)clock+=period;return uint32_t(clock);
}
void rebasePhoenixClocks(uint32_t now){
 for(unsigned i=0;i<15;++i)phoenixPhases[i]=uint16_t(phoenixClock(i,now));
 phoenixClockAt=now;
}
const char* modeName(Mode m){return m==Mode::Hold?"hold":m==Mode::Free?"free":"base";}
const char* viewName(){switch(view){case View::Game:return "game";case View::Menu:return "menu";case View::Bet:return "bet";case View::Rules:return "rules";case View::Options:return "options";case View::Reset:return "reset";case View::Lucky:return "lucky-chance";case View::Sound:return "sound";case View::Volume:return "volume";case View::Math:return "game-math";case View::Service:return "service";case View::Jackpots:return "jackpots";case View::Paylines:return "paylines";case View::Settings:return "settings";}return "unknown";}
const char* phaseName(){switch(phase){case Phase::Ready:return "ready";case Phase::Spin:return "spinning";case Phase::Land:return "landing";case Phase::Expand:return "sage-expansion";case Phase::Revive:return "phoenix-revival";case Phase::FreeIntro:return "free-intro";case Phase::FreeCounter:return "free-counter";case Phase::SpinAward:return "spin-award";case Phase::SpinAwardSettle:return "spin-award-settle";case Phase::HoldWipe:return "hold-fire";case Phase::HoldIntro:return "hold-intro";case Phase::Jackpot:return "jackpot-award";case Phase::Collect:return "collect";case Phase::WinTransfer:return "win-transfer";case Phase::ReturnWipe:return "return-fire";case Phase::Outro:return "feature-end";case Phase::AwardIntro:return "award-intro";}return "unknown";}
bool lineDisplayPhase(){return phase==Phase::Ready||phase==Phase::Land||phase==Phase::Expand;}
uint32_t lineElapsed(uint32_t now){return now-(lineClockValid?lineDisplayAt:readyAt);}
int activeLineIndex(uint32_t now){const auto& p=engine.presentation;if(!lineDisplayPhase()||holdView||!p.lineWinCount)return -1;return int((lineElapsed(now)/kPaylinePeriodMs)%p.lineWinCount);}
bool lineMarkersVisible(uint32_t now){return activeLineIndex(now)>=0&&lineElapsed(now)%kPaylineBlinkMs<kPaylineOnMs;}
void startPhase(Phase p,uint32_t ms){phase=p;phaseAt=millis();phaseMs=ms;dirty=true;}
#ifdef UI_SAGE_REGISTRATION_TEST_HOOK
bool sageCrownTestEnabled=true;
#endif
bool freeView();
#include "wisdom_accepted29.inc"
#include "wisdom_entry29.inc"
#include "wisdom_free_caption29.inc"
bool pairedReelBorders(){return pairedReelsAccepted29();}
void frameBorder(){
 // Clean gold moulding: a warm outer shoulder, bright inner lip and rounded
 // corners. The source's collars/end caps survive as subtle native pixels;
 // no photographed border strip or extra per-symbol highlight is used.
 constexpr uint16_t shoulder=0xc3c1,body=0xe5a3,light=0xff4b,shadow=0x82a0;
 const int left=RX-2,right=RX+RW+1,top=RY-2,bottom=RY+RH;
 line(left+2,top,right-2,top,shoulder);line(left+2,bottom,right-2,bottom,shoulder);
 line(left,top+2,left,bottom-2,shoulder);line(right,top+2,right,bottom-2,shadow);
 dot(left+1,top+1,shoulder);dot(right-1,top+1,shoulder);
 dot(left+1,bottom-1,shoulder);dot(right-1,bottom-1,shadow);
 line(left+2,top+1,right-2,top+1,light);line(left+2,bottom-1,right-2,bottom-1,body);
 line(left+1,top+2,left+1,bottom-2,light);line(right-1,top+2,right-1,bottom-2,body);
 for(int c=1;c<5;++c){
  // Recorded Twin Spins use one wide opening for each synchronized pair.
  // Keep the two centre rails; Hold & Spin retains its full fifteen-cell grid.
  if((c==1||c==4)&&pairedReelBorders())continue;
  const int x=RX+c*TW;
  line(x-2,RY,x-2,RY+RH-1,shoulder);line(x-1,RY,x-1,RY+RH-1,light);line(x,RY,x,RY+RH-1,body);
  for(int y=RY+4;y<RY+RH-2;y+=5){dot(x-1,y,shadow,96);dot(x,y,shadow,112);dot(x-1,y+1,0xffad,80);}
  // Tapered capitals join the outer frame instead of a raw square T-junction.
  line(x-3,RY-1,x+1,RY-1,light);line(x-2,RY,x,RY,0xff8e);dot(x-1,RY+1,0xff8e);
  line(x-3,RY+RH-1,x+1,RY+RH-1,body);dot(x-1,RY+RH-2,light);
 }
 // Row dividers belong only to the held bonus grid.
 if(holdView)for(int r=1;r<3;++r){const int y=RY+r*TH;line(RX,y-1,RX+RW-1,y-1,shoulder);line(RX,y,RX+RW-1,y,light);}
}
std::string cashPrizeDigits(const Cell& cell){int64_t minor=0;if(cell.kind!=phoenix::PrizeKind::Credits||cell.prizeUnit!=phoenix::money::Unit::Credits||!phoenix::Engine::cellBaseAmountMinor(cell,engine.state.coinValue,minor)){enterAmountFault190();return "INVALID";}return integer(engine.state.coinValue>=100?minor/100:cell.prize);}
void prizeText(const Cell& cell,int x,int y,unsigned alpha=255){
  if(!phoenixCell(cell))return;
  if(cell.kind!=phoenix::PrizeKind::Credits){overlay(pdfa::prize(unsigned(cell.kind),cell.multiplier),x,y);return;}
  std::string value=cashPrizeDigits(cell);const bool cash=engine.state.coinValue>=100;
  if(pnum::supports(value.c_str())){
    unsigned height=10,width=pnum::width(value.c_str(),height)+(cash?textWidth("$",8)+1:0);
    if(width>TW-6){height=8;width=pnum::width(value.c_str(),height)+(cash?textWidth("$",8)+1:0);}
    if(width<=TW-6){
      int xx=x+(TW-int(width))/2,yy=y+(TH-int(height))/2;
      if(cash){text("$",xx-1,yy,8,BLACK);text("$",xx+1,yy,8,BLACK);text("$",xx,yy-1,8,0xfdc4);xx+=textWidth("$",8)+1;}
      for(char ch:value){const auto* glyph=pnum::glyph(ch,height);for(unsigned r=0;r<glyph->h;++r)for(unsigned c=0;c<glyph->w;++c){unsigned at=r*glyph->w+c;if(glyph->alpha[at])dot(xx+c,yy+r,glyph->rgb[at],glyph->alpha[at]);}xx+=glyph->advance;}
      return;
    }
  }
  if(cash)value="$"+value;
  const unsigned size=value.size()>5?7:10;
  int xx=x+TW/2;
  text(value.c_str(),xx-1,y+TH/2-4,size,0x0000,1);text(value.c_str(),xx+1,y+TH/2-4,size,0x0000,1);
  text(value.c_str(),xx,y+TH/2-5,size,0xfdc4,1);
  if(cell.multiplier>1){std::string m=integer(cell.multiplier)+"X";text(m.c_str(),x+TW-3,y+3,6,0xfff0,2);}
  (void)alpha;
}
int symbolOffsetY(Symbol symbol){
  // The compact card/abacus masks have more clear pixels above their ink.
  // Move their complete masks, including every winning pose, onto the reel
  // centre. Full-panel Sage/Wisdom and oversized Phoenix art keep their own
  // anchors; neither the background texture nor the row pitch is translated.
  switch(symbol){
    case Symbol::Nine:case Symbol::Ten:case Symbol::Queen:case Symbol::King:return -2;
    case Symbol::Jack:case Symbol::Bell:case Symbol::Abacus:return -1;
    default:return 0;
  }
}
void drawCell(const Cell& cell,int x,int y,uint32_t now,bool winning=false,int clipTop=RY,int clipBottom=RY+RH,bool empty=false,bool deferPhoenix=false){
  if(phoenixCell(cell)&&!empty&&(deferPhoenix||phase!=Phase::Spin)){image(pa::background,x,y,RX,clipTop,RX+RW,clipBottom);return;}
  if(!empty)image(pa::background,x,y,RX,clipTop,RX+RW,clipBottom);
  if(cell.symbol==Symbol::Wild&&phase!=Phase::Spin)winning=cell.highlight&&(phase!=Phase::Expand||now-phaseAt>=650);
  uint32_t animationClock=winning?((phase==Phase::FreeIntro||phase==Phase::FreeCounter||spinAwardPhase()||(phase==Phase::Ready&&spinAward.finished))?now-lineDisplayAt:activeLineIndex(now)>=0?lineElapsed(now)%kPaylinePeriodMs:now-readyAt):now;
  // Abacus beads keep moving through successive payline displays in the
  // source. Resetting at1600ms truncates its measured4000ms cycle.
  if(winning&&(cell.symbol==Symbol::Abacus||cell.symbol==Symbol::Book||cell.symbol==Symbol::Bell||cell.symbol==Symbol::Wild)&&lineDisplayPhase())animationClock=lineElapsed(now);
  // Candle flame and vapor are ambient across changing winning-line highlights.
  // Keep the existing bank speed; this only removes presentation-clock resets.
  if(cell.symbol==Symbol::Candle)animationClock=now;
  if(cell.symbol==Symbol::Wild&&holdView)animationClock=0; // frozen held-bonus backdrop
  const pa::Image& im=empty?pa::holdBackground:pa::symbol(unsigned(cell.symbol),animationClock,winning);
  // 9/10 blank win frames are the full opaque background, not a glyph. Keep
  // that texture exactly on its tile while translating the visible masks.
  const int offset=empty||im.rgb==pa::background.rgb?0:symbolOffsetY(cell.symbol);
  image(im,x+(TW-int(im.w))/2,y+(TH-int(im.h))/2+offset,phoenixCell(cell)?std::max(RX,x):RX,clipTop,phoenixCell(cell)?std::min(RX+RW,x+TW):RX+RW,clipBottom);
  // Prize numerals are a separate overlay so each landed Phoenix has the exact
  // value computed by the engine rather than a number baked into video art.
  if(phoenixCell(cell)&&!empty){dotClipTop=clipTop;dotClipBottom=clipBottom;prizeText(cell,x,y);dotClipTop=0;dotClipBottom=H;}
}
void drawHoldOrdinaryReference(const Cell& cell,int x,int y,int clipTop,int clipBottom){
 drawCell(cell,x,y,0,false,clipTop,clipBottom);
 const int top=std::max(y,clipTop),bottom=std::min(y+TH,clipBottom);
 if(bottom>top)fill(x,top,TW,bottom-top,0x0005,220);
}
void drawHoldCell(const Cell& cell,int x,int y,uint32_t now,int clipTop,int clipBottom,bool landed=false,bool initialBlank=false){
  // The mini reel's virtual tile must enter the viewport before its artwork
  // does. In particular, the following Phoenix's oversized flame tips may
  // not leak back over a stopped ordinary tile at either strip endpoint.
  if(!landed){clipTop=std::max(clipTop,y);clipBottom=std::min(clipBottom,y+TH);}
  if(clipBottom<=clipTop)return;
  if(phoenixCell(cell)){drawCell(cell,x,y,now,false,clipTop,clipBottom,false,landed);return;}
  if(initialBlank){drawCell(cell,x,y,now,false,clipTop,clipBottom,true);return;}
  // Ordinary symbols belong to the moving mini reel too. Use exactly the
  // same idle art/material while moving, braking and resting; a black tile
  // followed by the trigger board's old grey symbol creates a visible pop.
  const unsigned id=unsigned(cell.symbol);
  if(holdMaterialPixels&&id<kHoldMaterialCount&&x>=RX&&x+TW<=RX+RW&&dotClipTop==0&&dotClipBottom==H&&
     !(cell.symbol==Symbol::Wild&&cell.highlight&&phase!=Phase::Spin)){
   const pa::Image material={TW,TH,holdMaterialPixels+id*TW*TH,nullptr,0};
   image(material,x,y,RX,clipTop,RX+RW,clipBottom);return;
  }
  drawHoldOrdinaryReference(cell,x,y,clipTop,clipBottom);
}
void initHoldMaterial(){
 if(holdMaterialPixels)return;
 // Opaque full-tile backing makes the material independent of old pixels.
 // Reject an incompatible future bank instead of caching a clipped sprite.
 if(pa::background.w!=TW||pa::background.h!=TH||pa::background.alpha||pa::background.key)return;
 for(unsigned id=0;id<kHoldMaterialCount;++id){const auto& im=pa::symbol(id,0,false);if(im.w>TW||im.h>TH)return;}
#ifdef UI_HOST_TEST
 auto* memory=static_cast<uint16_t*>(std::malloc((kHoldMaterialPixels+TW*TH)*2));
#else
 auto* memory=static_cast<uint16_t*>(ps_malloc((kHoldMaterialPixels+TW*TH)*2));
#endif
 if(!memory)return;
 auto* kept=memory+kHoldMaterialPixels;
 for(int y=0;y<TH;++y)memcpy(kept+y*TW,pixels+(RY+y)*W+RX,TW*2);
 const int savedTop=dotClipTop,savedBottom=dotClipBottom;dotClipTop=0;dotClipBottom=H;
 for(unsigned id=0;id<kHoldMaterialCount;++id){
  Cell cell;cell.symbol=Symbol(id);drawHoldOrdinaryReference(cell,RX,RY,RY,RY+TH);
  for(int y=0;y<TH;++y)memcpy(memory+id*TW*TH+y*TW,pixels+(RY+y)*W+RX,TW*2);
 }
 for(int y=0;y<TH;++y)memcpy(pixels+(RY+y)*W+RX,kept+y*TW,TW*2);
 dotClipTop=savedTop;dotClipBottom=savedBottom;
 uint32_t hash=2166136261u;
 for(size_t i=0;i<kHoldMaterialPixels;++i){hash=(hash^uint8_t(memory[i]))*16777619u;hash=(hash^uint8_t(memory[i]>>8))*16777619u;}
 if(hash!=kHoldMaterialExpectedFNV){std::free(memory);return;}
 holdMaterialPixels=memory;holdMaterialHash=hash;
}
bool freeView(){if(phase==Phase::Spin||phase==Phase::Land||phase==Phase::Expand)return engine.presentation.valid&&engine.presentation.freeSpin;return engine.state.freeSession||(phase!=Phase::Ready&&engine.presentation.valid&&engine.presentation.freeSpin);}
int shownRespins(){if(phase==Phase::AwardIntro||phase==Phase::Collect||phase==Phase::Jackpot||phase==Phase::WinTransfer||phase==Phase::ReturnWipe)return 0;if(phase==Phase::Revive&&uint32_t(millis()-phaseAt)>=revival_timing::promptAt)return engine.state.respinsLeft;return (phase==Phase::Spin||phase==Phase::Land||phase==Phase::Revive)?displayRespins:engine.state.respinsLeft;}
// Only freshly entered natural Holds are source-qualified. Restored/ongoing,
// revival and meter/queued entries keep their existing display behavior.
bool initialNaturalHoldCounter67(){
 const auto& p=engine.presentation;
 return phase==Phase::Ready&&holdView&&engine.state.mode==Mode::Hold&&
        engine.state.respinsLeft==3&&p.valid&&p.holdEntered&&!p.holdFromMeter&&
        !p.holdRespin&&!p.holdSettled;
}
uint32_t initialHoldReadyDelay67(){
 // Mirror ready()'s existing nominal delay, without changing its clock reads.
 // Deriving from stable presentation state avoids reversing the ramp when a
 // later manual/menu/autoplay action changes autoAt or skips the wait.
 return spinAward.finished?spin_award::readyDelayMs:engine.presentation.actionWinMinor?2600u:650u;
}
int shownRespins(uint32_t now){
 return initialNaturalHoldCounter67()?int(hold_initial_counter67::count(uint32_t(now-readyAt),initialHoldReadyDelay67())):shownRespins();
}

void counterFrame(uint32_t now){
 counter_calm90::draw(now,METER_LEFT,METER_RIGHT,13,
  [](int x,int y,uint16_t color,unsigned alpha){dot(x,y,color,alpha);});
}

int shownFreeTotal(uint32_t now){
 const auto& p=engine.presentation;
 const int oldTotal=int(engine.state.freePlayed+p.freeBefore-1);
 if(phase==Phase::FreeIntro&&freeRetriggerIntro())return freeIntroOldTotal;
 if(phase==Phase::FreeCounter)return freeIntroOldTotal+std::min(int(pendingFreeCount),int((now-phaseAt)/spin_award::counterTickMs)+1);
 const bool prior=(phase==Phase::Spin||phase==Phase::Land||phase==Phase::Expand||phase==Phase::FreeIntro)&&p.freeSpin;
 return prior?oldTotal:int(engine.state.freePlayed+engine.state.freeLeft);
}
bool initialFreeCounter30(){
 const auto& p=engine.presentation;
 return wisdomEntry30.stage==wisdom_ctrl30::Stage::FirstSpin&&phase==Phase::Spin&&!holdView&&p.valid&&p.freeSpin&&p.actionNumber==wisdomEntry30.triggerAction+1;
}
#include "live_hold_count48.inc"
// The source keeps Free Game n of total visible during a nested Hold,
// including its fire. Retarget that context into the existing condition slot;
// the meter/Phoenix, collected count and remaining-respins slots keep ownership.
// Pure presentation: no clocks, new counters, state or random draws.
bool nestedFreeProgress138(unsigned& played,unsigned& total){
 if(view!=View::Game||storagePause!=UiCheckpointPause::None||!engine.presentation.valid)return false;
 switch(phase){
  case Phase::Ready:if(engine.state.mode==Mode::Base)return false;break;
  case Phase::Spin:case Phase::Land:case Phase::Revive:case Phase::HoldWipe:
  case Phase::HoldIntro:case Phase::AwardIntro:case Phase::Jackpot:
  case Phase::Collect:case Phase::WinTransfer:case Phase::ReturnWipe:break;
  default:return false;
 }
 if(!holdView&&phase!=Phase::HoldWipe&&phase!=Phase::HoldIntro)return false;
 const auto& s=engine.state;
 if(!s.freeSession&&!engine.presentation.freeEnded)return false;
 played=unsigned(s.freePlayed);total=played+unsigned(s.freeLeft);
 // A retrigger queued behind this Hold keeps the already-announced total
 // until the existing FreeCounter animation presents its added games.
 if(freeRetriggerIntro()&&freeIntroOldTotal)total=freeIntroOldTotal;
 return played&&played<=total;
}
bool nestedFreeProgress138(){unsigned played=0,total=0;return nestedFreeProgress138(played,total);}
void drawNestedFreeProgress138(){
 unsigned played=0,total=0;if(!nestedFreeProgress138(played,total))return;
 const std::string count=integer(played)+" OF "+integer(total);
 // Source captions have a dark edge. One offset shadow keeps the authentic
 // white status legible on fire without a black panel or doubled foreground.
 const int bottom=dotClipBottom;dotClipBottom=std::min(bottom,14);
 text("FREE GAME",157,1,7,0x0002,1,126,186);
 text(count.c_str(),157,8,7,0x0002,1,126,186);
 text("FREE GAME",156,0,7,WHITE,1,126,186);
 text(count.c_str(),156,7,7,WHITE,1,126,186);
 dotClipBottom=bottom;
}
void drawHeader(uint32_t now){
  // One footer-matched charcoal matte under every panel and antialiased edge.
  // Do not reintroduce separately coloured logo or translucent phase blocks.
  fill(0,0,W,14,native_title::background);
  if(!drawLiveHoldCount48(now))image(holdView||bonusReelsActive29()?native_title::holdTitle:native_title::image,0,0);
  // Progress is painted once after the scene/fire, not alpha-blended twice.
  if(!nestedFreeProgress138())image(native_title::condition,126,0);
  image(counter_hud::phoenix(now),186,0);
  if(displayQueuedHolds){text(integer(displayQueuedHolds).c_str(),METER_CENTER,1,6,BRIGHT,1,METER_LEFT+1,METER_RIGHT);text("FEATURES",METER_CENTER,7,6,BRIGHT,1,METER_LEFT+1,METER_RIGHT);}
  else meterNumber(displayMeter);
  counterFrame(now);
  const int lineIndex=activeLineIndex(now);
  std::string lineCaption;if(lineIndex>=0){const auto& win=engine.presentation.lineWins[lineIndex];lineCaption="LINE "+integer(win.line+1)+" WINS "+integer(win.credits);if(textWidth(lineCaption.c_str(),6)>47)lineCaption="L"+integer(win.line+1)+" +"+integer(win.credits);}

  if(holdView){
    if(phase==Phase::Ready&&engine.state.mode!=Mode::Hold){
      const std::string value=engine.state.freeSession?"FREE "+integer(engine.state.freePlayed)+"/"+integer(engine.state.freePlayed+engine.state.freeLeft):"GAME OVER";
      headerCaption(value.c_str(),2,7,BRIGHT);
    }else if(initialNaturalHoldCounter67()&&hold_initial_counter67::prompt(uint32_t(now-readyAt),initialHoldReadyDelay67())){
      headerCaption("PRESS START",0,6,BRIGHT);headerCaption("FEATURE",7,6,BRIGHT);
    }else{
      const int spins=shownRespins(now);
      const bool singular=initialNaturalHoldCounter67()&&spins==1;
      headerCaption((integer(spins)+(singular?" FREE SPIN":" FREE SPINS")).c_str(),0,7,BRIGHT);
      headerCaption("REMAINING",7,6,BRIGHT);
    }
  }
  else if(initialFreeCounter30()){
   fill(77,0,49,14,native_title::background);
   const unsigned total=wisdom_entry_timing29::shownInitialTotal(wisdomEntry30.awarded,now-wisdomEntry30.motionAt);
   if(total)drawWisdomFreeCaption29(std::max(1u,unsigned(engine.state.freePlayed)),total);
  }
  else if(freeView()){const int total=shownFreeTotal(now);std::string value="FREE "+integer(std::max(1,int(engine.state.freePlayed)))+"/"+integer(total);if(lineIndex>=0){headerCaption(lineCaption.c_str(),0,6,BRIGHT);headerCaption(value.c_str(),7,6,WHITE);}else headerCaption(value.c_str(),2,7,BRIGHT);}
  else {headerCaption(lineIndex>=0?lineCaption.c_str():(integer(engine.state.lineCount)+" LINES").c_str(),0,6,BRIGHT);headerCaption(("BET X"+integer(engine.state.betMultiplier)).c_str(),7,6,WHITE);}

 if(phase==Phase::Ready&&completedFreeBoard29())drawWisdomCompletion29();
}
void drawFooter(){
  image(pa::footer,0,122);fill(0,122,240,13,0x18c3,225);
  image(pa::creditLabel,3,123);image(pa::betLabel,96,123);image(pa::winLabel,158,123);
  // One shared baseline and four clear pixels between adjacent groups.
  // WIN receives the same 31px value slot as BET, retaining the full-size
  // cash face for common three-digit wins instead of shrinking it early.
  amount(displayBank,92,123,44);
  amount(engine.betMinor(),154,123,31);
  amount((phase==Phase::Collect||phase==Phase::Jackpot)?engine.state.roundPayout-engine.presentation.phoenixWinMinor:displayWin,215,123,31);
  // Dedicated original-style condensed gold lettering and a true cent sign.
  // This occupies its existing slot without shrinking CREDIT/BET/WIN values.
  if(const auto* badge=denomination_minbet46::forWager(engine.state.coinValue,engine.state.lineCount,engine.state.betMultiplier))image(*badge,219,122);
  else image(counter_hud::denomination(unsigned(engine.state.coinValue)),219,122);
}

int reelOffset(int c,uint32_t elapsed){return reel_motion::offset(reelMs[c],reelSteps[c],TH,elapsed);}
constexpr uint32_t holdReelDuration(int c){return kHoldFirstMs+unsigned(c)*kHoldColumnMs;}
int holdReelSteps(int c){return reel_motion::steps(holdReelDuration(c),TH);}
int holdReelOffset(int c,uint32_t elapsed){return reel_motion::offset(reelMs[c],holdReelSteps(c),TH,elapsed);}
void paintSageCrown133(int x,int y,uint16_t color,unsigned alpha){
 if(x<RX||x>=RX+RW||y<RY-4||y>=RY+RH)return;
 // Preserve bright/coloured header ink; permit overhang over neutral
 // charcoal matte/grain. This palette guard is not a recovered glyph mask.
 if(y<14){const auto under=pixels[y*W+x];const int r=(under>>11)&31,g=(under>>5)&63,b=under&31;
  if(r>7||g>14||b>7||std::abs(r-b)>2||std::abs(g-r-b)>4)return;
 }
 dot(x,y,color,alpha);
}
void drawTopSageOutside133(int x,unsigned rows,uint32_t clock){
 const auto paint=[](int xx,int yy,uint16_t color,unsigned alpha){if(yy<RY)paintSageCrown133(xx,yy,color,alpha);};
 // Outside RGBA is immutable and independent of canonical profile. One
 // reference suffices; ignored inside pixels cannot alias the crown cache.
 const auto& cap=pa::sageCrown(1,0,false);
 if(rows<2||clock>=667){sage_crown122::draw(cap,x,RY,[](int){return 255u;},paint);return;}
 constexpr unsigned times[]={0,167,292,333,375,417,458,500,542,583,625,667};unsigned frame=0;while(frame+1<12&&clock>=times[frame+1])++frame;
 const unsigned sample=times[frame],motion=sample>(rows==2?42u:0u)?sample-(rows==2?42u:0u):0u;
 constexpr unsigned motionTimes[]={292,333,375,417,458,500,542,583,625};constexpr float positions[]={70,70,62,49,37,20,7,0,0};
 float position=70;if(motion>=625)position=0;else for(unsigned k=0;k+1<9;++k)if(motion>=motionTimes[k]&&motion<motionTimes[k+1]){const float t=float(motion-motionTimes[k])/float(motionTimes[k+1]-motionTimes[k]);position=positions[k]+t*(positions[k+1]-positions[k]);break;}
 position*=float(rows-1)/2.f;
 const auto mix=[&](int yy){if(motion<=310)return 0u;if(motion>=583)return 255u;const float v=(float(yy-RY)-position+7.f)/14.f;return unsigned(std::lround(std::max(0.f,std::min(1.f,v))*255.f));};
 if(motion<=310)sage_crown122::draw(cap,x,RY,[](int){return 255u;},paint);
 else sage_crown122::transition(cap,cap,x,RY,rows,RY+int(std::lround(position)),RY-4,mix,paint);
}
// Camera-derived hue categories from the scoped original-US reference facts.
// Unobserved line/count pairs retain the existing palette.
uint16_t lineMarkerColor(unsigned index,unsigned count){
 if((count==50||count==100)&&index==11)return 0x35df;
 if(count==50){if(index==14)return 0xfcb5;if(index==28)return 0x07a9;if(index==48)return 0x04df;if(index==49)return 0xb81f;}
 return index==0?0x35df:index==1?0xd85a:index==3?0x07a9:index==7?0x04df:index==10?0xb81f:BRIGHT;
}
void drawReels(uint32_t now){
  uint32_t e=now-phaseAt;
  if(phase==Phase::Spin){
    for(int c=0;c<5;++c){const auto split=reel_motion::split(reelOffset(c,e),TH);const int first=split.first,rem=split.remainder;
      for(int r=0;r<4;++r){int stripIndex=first+r;int yy=RY+r*TH-int(rem);if(stripIndex < -1||stripIndex>=int(kReelStripCells))continue;
        // Hold mode's occupied cells never move. Vacant cells each spin in
        // their own square and land new prizes at the same stop time.
        if(!holdView){const auto& cell=strips[c][stripIndex<0?kReelStripCells-1:unsigned(stripIndex)];uint32_t artClock=now;
          if(phoenixCell(cell)){const int oldRow=stripIndex-reelSteps[c];artClock=oldRow>=0&&oldRow<3?phoenixClock(unsigned(c*3+oldRow),now):phoenixRenderClock(unsigned(c*3),now);}
          drawCell(cell,RX+c*TW,yy,artClock,false,RY,RY+RH,false,e>=reelMs[c]);}
      }
      if(holdView)for(int r=0;r<3;++r){int i=c*3+r;int yy=RY+r*TH;
        if(heldMask&(1u<<i))drawCell(previous[i],RX+c*TW,yy,now,false,yy,yy+TH,false,true);
        else if(e>=reelMs[c])drawHoldCell(holdStrips[i][0],RX+c*TW,yy,phoenixRenderClock(unsigned(i),now),yy,yy+TH,true);
        else {
          const auto split=reel_motion::split(holdReelOffset(c,e),TH);const int first=split.first,rem=split.remainder;
          for(int tile=0;tile<2;++tile){
            const int index=first+tile;const auto& cell=holdStrips[i][index<0?kHoldStripCells-1:unsigned(index)];
            drawHoldCell(cell,RX+c*TW,yy+tile*TH-int(rem),phoenixRenderClock(unsigned(i),now),yy,yy+TH,false,
                         holdStripStartsBlank&&first+tile==holdReelSteps(c));
          }
        }
      }
    }
  }else{
    for(int c=0;c<5;++c)for(int r=0;r<3;++r){int i=c*3+r;
      const int active=activeLineIndex(now);bool winner=(active>=0?bool(engine.presentation.lineWins[active].mask&(1u<<i)):((phase==Phase::Ready&&visible[i].highlight&&(now-readyAt)%2200<1450)||(spinAwardPhase()&&visible[i].highlight)))||((phase==Phase::FreeIntro||phase==Phase::FreeCounter)&&visible[i].symbol==Symbol::Scatter);
      if(holdView&&!phoenixCell(visible[i])){drawHoldCell(holdBackdropCaptured?holdBase[i]:visible[i],RX+c*TW,RY+r*TH,now,RY+r*TH,RY+(r+1)*TH,false,!holdBackdropCaptured);continue;}
      // HzpqxpE-Ht8 at160s: both awarded Abaci keep moving while the
      // centre Queen line is announced. Their full cycle is independent of
      // line selection; a losing Abacus and held backdrops remain idle.
      // mQZbc-_wjO4 cut129.75..131.75s: the three paid Abaci continue
      // moving through the free-game retrigger counter; unawarded ones rest.
      const bool retriggerAbacus=freeRetriggerIntro()&&(phase==Phase::FreeIntro||phase==Phase::FreeCounter)&&visible[i].symbol==Symbol::Abacus&&visible[i].highlight;
      const bool animate=winner||retriggerAbacus||(lineDisplayPhase()&&(visible[i].symbol==Symbol::Abacus||visible[i].symbol==Symbol::Book||visible[i].symbol==Symbol::Bell)&&visible[i].highlight);
      if(!holdView&&lineDisplayPhase()&&visible[i].symbol==Symbol::Bell&&visible[i].highlight){image(pa::background,RX+c*TW,RY+r*TH,RX,RY,RX+RW,RY+RH);continue;}
      drawCell(visible[i],RX+c*TW,RY+r*TH,now,animate,RY,RY+RH,holdView&&!phoenixCell(visible[i]));
    }
  }
  // Adjacent Sage faces join into the recorded two/three-cell figure. This
  // affects art only: every joined square was already a Wild in the result.
  if(phase!=Phase::Spin&&!holdView){
    uint16_t awarded=0;for(unsigned i=0;i<15;++i)if(visible[i].highlight)awarded|=uint16_t(1u<<i);
    const uint16_t joins=phoenix::Engine::joinedSageMask(visible,awarded);
    for(int c=0;c<5;++c)for(int r=0;r<2;++r){const int i=c*3+r;
      if((joins&(3u<<i))!=(3u<<i))continue;
      const int rows=r==0&&(joins&(1u<<(i+2)))?3:2;
      const uint32_t sageClock=phase==Phase::Expand?now-phaseAt:lineClockValid?lineElapsed(now):667;
      image(pa::sage(rows,sageClock),RX+c*TW,RY+r*TH,RX,RY,RX+RW,RY+RH);break;
    }
  }
  frameBorder();
  // Retain the exact canonical inside rows. The separate outside-only
  // cap follows the same merge-head position/reveal into the grey header.
  if(phase!=Phase::Spin&&!holdView){
    uint16_t awarded=0;for(unsigned i=0;i<15;++i)if(visible[i].highlight)awarded|=uint16_t(1u<<i);
    const uint16_t joins=phoenix::Engine::joinedSageMask(visible,awarded);
    for(int c=0;c<5;++c){const int i=c*3;if(visible[i].symbol!=Symbol::Wild)continue;
      unsigned rows=1;bool winning=visible[i].highlight&&(phase!=Phase::Expand||now-phaseAt>=650);
      uint32_t clock=winning?((phase==Phase::FreeIntro||phase==Phase::FreeCounter||spinAwardPhase()||(phase==Phase::Ready&&spinAward.finished))?now-lineDisplayAt:activeLineIndex(now)>=0?lineElapsed(now)%kPaylinePeriodMs:now-readyAt):now;
      if(winning&&lineDisplayPhase())clock=lineElapsed(now);
      if((joins&(3u<<i))==(3u<<i)){rows=(joins&(1u<<(i+2)))?3:2;clock=phase==Phase::Expand?now-phaseAt:lineClockValid?lineElapsed(now):667;}
#ifdef UI_SAGE_REGISTRATION_TEST_HOOK
      if(!sageCrownTestEnabled)continue;
#endif
      image(pa::sageCrown(rows,clock,winning),RX+c*TW,RY-2,RX,RY,RX+RW,RY+3);
      drawTopSageOutside133(RX+c*TW,rows,clock);
    }
  }
  // One full passing cap also owns stopped earlier reels. Internal
  // settled/merging caps retain their existing registration.
  if(!holdView&&phase==Phase::Spin){
    // Read the SAME passing strip tile and scroll coordinate as drawCell.
    // This is physical artwork visibility, never the future target mask.
    for(int c=0;c<5;++c){const auto split122=reel_motion::split(reelOffset(c,e),TH);
      for(int r=0;r<4;++r){const int index122=split122.first+r,yy122=RY+r*TH-int(split122.remainder);if(index122< -1||index122>=int(kReelStripCells))continue;
        const auto& passing122=strips[c][index122<0?kReelStripCells-1:unsigned(index122)];if(passing122.symbol!=Symbol::Wild)continue;
        sage_crown122::draw(pa::sageCrown(1,0,false),RX+c*TW,yy122,[](int){return 255u;},paintSageCrown133);
      }
    }
  }else if(!holdView){
    uint16_t awarded122=0;for(unsigned i=0;i<15;++i)if(visible[i].highlight)awarded122|=uint16_t(1u<<i);
    const uint16_t joins122=phoenix::Engine::joinedSageMask(visible,awarded122);
    const auto paint122=[&](int x,int y,uint16_t c,unsigned a){if(x>=RX&&x<RX+RW&&y>=RY&&y<RY+RH)dot(x,y,c,a);};
    for(int c=0;c<5;++c)for(int r=0;r<3;++r){const int i=c*3+r;
      const Cell& cell122=visible[i];
      if(cell122.symbol!=Symbol::Wild)continue;
      if(r&&((joins122&(3u<<(i-1)))==(3u<<(i-1))))continue;
      const unsigned rows122=r<2&&((joins122&(3u<<i))==(3u<<i))?(r==0&&(joins122&(1u<<(i+2)))?3:2):1;
      uint32_t clock122=0;bool winning122=false;
      if(rows122>1)clock122=phase==Phase::Expand?now-phaseAt:lineClockValid?lineElapsed(now):667;
      else {winning122=cell122.highlight&&(phase!=Phase::Expand||now-phaseAt>=650);clock122=winning122?((phase==Phase::FreeIntro||phase==Phase::FreeCounter||spinAwardPhase()||(phase==Phase::Ready&&spinAward.finished))?now-lineDisplayAt:activeLineIndex(now)>=0?lineElapsed(now)%kPaylinePeriodMs:now-readyAt):now;if(winning122&&lineDisplayPhase())clock122=lineElapsed(now);}
      const int x122=RX+c*TW,y122=RY+r*TH;
      if(rows122==1||clock122>=667){if(r)sage_crown122::draw(pa::sageCrown(rows122,clock122,winning122),x122,y122,[](int){return 255u;},paint122);continue;}
      // Match the CURRENT reconstructed merge bank's sampled position/reveal.
      // This is not a new claim about vendor transition alpha/timers.
      constexpr unsigned times122[]={0,167,292,333,375,417,458,500,542,583,625,667};unsigned frame122=0;while(frame122+1<12&&clock122>=times122[frame122+1])++frame122;
      const unsigned sample122=times122[frame122],motion122=sample122>(rows122==2?42u:0u)?sample122-(rows122==2?42u:0u):0u;
      constexpr unsigned motionTimes122[]={292,333,375,417,458,500,542,583,625};constexpr float positions122[]={70,70,62,49,37,20,7,0,0};
      float position122=70;if(motion122>=625)position122=0;else for(unsigned k=0;k+1<9;++k)if(motion122>=motionTimes122[k]&&motion122<motionTimes122[k+1]){const float t=float(motion122-motionTimes122[k])/float(motionTimes122[k+1]-motionTimes122[k]);position122=positions122[k]+t*(positions122[k+1]-positions122[k]);break;}
      position122*=float(rows122-1)/2.f;
      const auto mix122=[&](int yy){if(motion122<=310)return 0u;if(motion122>=583)return 255u;const float v=(float(yy-y122)-position122+7.f)/14.f;return unsigned(std::lround(std::max(0.f,std::min(1.f,v))*255.f));};
      if(motion122<=310){for(unsigned old122=0;old122<rows122;++old122)if(r+int(old122)>0)sage_crown122::draw(pa::sageCrown(1,0,false),x122,y122+int(old122)*TH,[](int){return 255u;},paint122);}
      else {uint16_t oldRgb122[235];const auto& oldImage122=pa::sageCrown(1,0,false);std::memcpy(oldRgb122,oldImage122.rgb,sizeof(oldRgb122));const pa::Image oldCap122{47,5,oldRgb122,oldImage122.alpha,0};const auto& newCap122=position122<=0.f?pa::sageCrown(rows122,clock122,false):pa::sageCrown(1,0,true);
        sage_crown122::transition(oldCap122,newCap122,x122,y122,rows122,y122+int(std::lround(position122)),r?RY:RY+3,mix122,paint122);
      }
    }
  }
  // Measured winning Bell silhouettes cross both gold dividers. Submit the
  // complete alpha art after backdrops/lattice, retaining viewport clipping.
  if(!holdView&&lineDisplayPhase())for(int i=0;i<15;++i)if(visible[i].symbol==Symbol::Bell&&visible[i].highlight){
   const auto& bell=pa::symbol(unsigned(Symbol::Bell),lineElapsed(now),true);
   image(bell,RX+i/3*TW+(TW-int(bell.w))/2,RY+i%3*TH+(TH-int(bell.h))/2+symbolOffsetY(Symbol::Bell),0,RY,W,RY+RH);
  }
  // Actual landed flames overhang the dividers. Draw their alpha silhouettes
  // after the lattice, with the committed number remaining in its own cell.
  for(int i=0;i<15;++i){const Cell* cell=nullptr;if(phase!=Phase::Spin)cell=&visible[i];else {if(holdView&&(heldMask&(1u<<i)))cell=&previous[i];else if(e>=reelMs[i/3])cell=&target[i];}if(!cell||!phoenixCell(*cell))continue;const int x=RX+i/3*TW,y=RY+i%3*TH;const auto& flame=pa::symbol(unsigned(Symbol::Phoenix),phoenixRenderClock(unsigned(i),now),false);image(flame,x+(TW-int(flame.w))/2,y+(TH-int(flame.h))/2,RX-2,RY-2,RX+RW+2,RY+RH+1);prizeText(*cell,x,y);}
  // The source indicates paylines at the dividers and with numbered end tabs.
  // It does not draw an uninterrupted gold polyline across the original art.
  const auto& p=engine.presentation;
  if(lineMarkersVisible(now)){const auto& win=p.lineWins[activeLineIndex(now)];const auto& path=phoenix::local::linePath(win.line,engine.state.lineCount);const uint16_t color=lineMarkerColor(win.line,engine.state.lineCount);for(int c=1;c<5;++c){const int x=RX+c*TW,y=RY+TH/2+TH*(int(path[c-1])+path[c])/2;const int slope=int(path[c])-path[c-1];line(x-2,y-slope,x+2,y+slope,color);line(x-2,y-slope+1,x+2,y+slope+1,color);}const int leftY=RY+TH/2+TH*path[0],rightY=RY+TH/2+TH*path[4];const int badgeWidth=win.line>=99?10:6,badgeCenter=badgeWidth/2;fill(0,leftY-3,badgeWidth,7,color);fill(W-badgeWidth,rightY-3,badgeWidth,7,color);text(integer(win.line+1).c_str(),badgeCenter,leftY-3,6,WHITE,1);text(integer(win.line+1).c_str(),W-badgeCenter,rightY-3,6,WHITE,1);}
}
void shade(){fill(0,0,W,H,BLACK,170);}
void panel(const char* title){shade();fill(4,11,232,121,0x0923);rect(4,11,232,121,GOLD);rect(6,13,228,117,BRIGHT);text(title,120,20,12,BRIGHT,1);line(16,37,224,37,GOLD);}

// New CGI material moves continuously through the live framebuffer. Its
// geometry/envelope is reconstructed from independent footage; no photographed
// fire pixels or short camera-frame loop are used by the production wipe.
fire_field::Workspace fireWorkspace;
effect_frames::Bank preparedEffects;
bool effectsCacheReady=false,effectsUseCache=true;
uint32_t effectsCacheBuildUs=0;
constexpr auto fireX=[](){std::array<unsigned,W> a{};for(unsigned x=0;x<W;++x)a[x]=(x*(fire_field::width-1)+(W-1)/2)/(W-1);return a;}();
constexpr auto fireY=[](){std::array<unsigned,H> a{};for(unsigned y=0;y<H;++y)a[y]=(y*(fire_field::height-1)+(H-1)/2)/(H-1);return a;}();
constexpr auto fireHeight=[](){std::array<float,H> a{};for(int y=0;y<H;++y)a[y]=float(H-y)/H;return a;}();
constexpr auto fireRadial=[](){std::array<float,W> a{};for(int x=0;x<W;++x){const float dx=x-W/2.f;a[x]=(dx<0?-dx:dx)/W;}return a;}();
constexpr auto fireHeat=[](){std::array<float,64> a{};for(unsigned g=0;g<64;++g)a[g]=float(g)/63.f;return a;}();
void fireWipe(uint32_t elapsed,uint32_t duration=phoenix_fx::wipeDurationMs){
 if(!elapsed||!duration||elapsed>=duration)return;
 const bool cached=effectsCacheReady&&effectsUseCache;
 const uint16_t* source=cached?preparedEffects.fireAt(elapsed):nullptr;
 const float t=float(elapsed)/duration;
 const float reach=t/.42f*1.4f,depart=(t-.65f)/.35f;
 if(cached&&t>=.42f&&t<.65f){
  // Fully opaque fire hides the entire display, including both HUD bars.
  // A contiguous copy avoids evaluating an unnecessary blend per texel.
  if(dotClipTop==0&&dotClipBottom==H)memcpy(pixels,source,sizeof(pixels));
  else for(int y=std::max(0,dotClipTop);y<std::min(H,dotClipBottom);++y)memcpy(pixels+y*W,source+y*W,size_t(W)*2);
  return;
 }
 // Avoid rebuilding the expensive deformation grid on the cached path.
 alignas(fire_field::Flow) unsigned char fieldStorage[sizeof(fire_field::Flow)];
 const auto* field=cached?nullptr:new(fieldStorage)fire_field::Flow(effect_frames::sample(elapsed),&fireWorkspace);
 float fringe[W];for(int x=0;x<W;++x)fringe[x]=.038f*std::sin(x*.13f+elapsed*.011f)+.022f*std::sin(x*.31f-elapsed*.019f);
 for(int y=0;y<H;++y){for(int x=0;x<W;++x){
   // Independent footage shows fire passing over the header, footer and
   // outer reel edges. Only ordinary reel symbols retain viewport clipping.
   uint16_t flame=0;unsigned alpha=255;
   if(t<.42f){alpha=fire_alpha::rising(reach-(fireHeight[y]+fireRadial[x]*.38f+fringe[x]));if(!alpha)continue;}
   else {
    // Break up the departing edge with moving tongues and the fire's own
    // luminance, rather than clearing the screen with a horizontal crop bar.
    if(t>=.65f){flame=cached?source[y*W+x]:field->pixel(fireX[x],fireY[y]);
     alpha=fire_alpha::leaving(fireHeight[y]-depart+fringe[x]+(fireHeat[(flame>>5)&63]-.5f)*.05f);}
   }
   if(t<.65f)flame=cached?source[y*W+x]:field->pixel(fireX[x],fireY[y]);
   if(alpha)dot(x,y,flame,alpha);
 }}
 if(field)field->~Flow();
}
bool cashCollectCell(const Cell& cell){return phoenixCell(cell)&&cell.kind!=phoenix::PrizeKind::Major;}
unsigned collectCount(){unsigned n=0;for(const auto& cell:visible)n+=cashCollectCell(cell);return n;}
int64_t namedAward(){int64_t n=0;for(const auto& cell:visible)if(phoenixCell(cell)&&cell.kind==phoenix::PrizeKind::Major){int64_t minor=0;if(!phoenix::Engine::cellAmountMinor(cell,engine.state.coinValue,minor)||!phoenix::money::add(n,minor,n)){enterAmountFault190();return 0;}}return n;}
int64_t collectCashTotal(){int64_t n=0;for(const auto& cell:visible)if(cashCollectCell(cell)){int64_t minor=0;if(!phoenix::Engine::cellAmountMinor(cell,engine.state.coinValue,minor)||!phoenix::money::add(n,minor,n)){enterAmountFault190();return 0;}}return n;}
int64_t namedTitleTotal190(){
 int64_t grand=engine.presentation.grandAwarded?engine.presentation.grandWinMinor:0,ceiling=0,total=0;
 if(!phoenix::money::multiply(phoenix::local::maximumCreditPrize,uint64_t(engine.state.coinValue),ceiling)||
    grand<0||grand>ceiling||(engine.presentation.grandAwarded&&!grand)||
    !phoenix::money::add(namedAward(),grand,total)){enterAmountFault190();return 0;}
 return total;
}
bool collectionFountain(){
 if(collectionNamedDone)return false; // Major/Grand already had their award.
 // Yfbaa375..390 ends Free Games:25x new Hold,92.2x combined round.
 // Intermediate nested settlements remain unestablished;50x stays provisional.
 const auto& p=engine.presentation;
 const bool combined=p.holdSettled&&p.freeEnded&&p.roundCompleted;
 return feature_award::cashFountain(combined?engine.state.roundPayout:collectCashTotal(),engine.betMinor());
}
// A new lower cash-only VISUAL fountain must not promote an unqualified
// BigAward phrase, change the existing prelude or alter transfer/return clocks.
bool lowerCashCollectionFountain149(){
 const auto& p=engine.presentation;
 if(!p.valid||!p.holdSettled||p.grandAwarded||collectionNamedDone||collectionFountain())return false;
 bool cash=false;
 for(const auto& cell:visible)if(phoenixCell(cell)){
  if(cell.kind!=phoenix::PrizeKind::Credits)return false;
  cash=true;
 }
 return cash&&feature_award::lowerCashFountain149(collectCashTotal(),engine.betMinor());
}
bool largeCollectionPrelude(){
 // The25x Hold /92.2x final-Free source uses the ordinary first-pickup lead.
 // Its combined round still qualifies for the later fountain/award phrase.
 return !collectionNamedDone&&feature_award::cashFountain(collectCashTotal(),engine.betMinor());
}
// Diagnostic context only: capture the exact visible collection board and
// payout basis before later presentation phases can change the active view.
void captureAwardContext(bool collection){
 const auto& p=engine.presentation;
 lastAward.collection=collection;lastAward.autoPlayAtBegin=autoPlay;
 lastAward.holdSettled=p.holdSettled;lastAward.freeEnded=p.freeEnded;lastAward.roundCompleted=p.roundCompleted;
 lastAward.combinedRoundBasis=collection&&p.holdSettled&&p.freeEnded&&p.roundCompleted;
 lastAward.roundPayout=engine.state.roundPayout;
 lastAward.fountainBasis=lastAward.combinedRoundBasis?lastAward.roundPayout:lastAward.cash;
 for(const auto& cell:visible)if(phoenixCell(cell)){
  ++lastAward.phoenixCount;
  switch(cell.kind){
   case phoenix::PrizeKind::Credits:++lastAward.cashCellCount;break;
   case phoenix::PrizeKind::Mini:++lastAward.miniCount;break;
   case phoenix::PrizeKind::Minor:++lastAward.minorCount;break;
   case phoenix::PrizeKind::Major:++lastAward.majorCount;break;
  }
 }
}
void beginAwardTrace(bool test=false){
 lastAward=AwardTrace{};lastAward.valid=true;lastAward.test=test;lastAward.action=engine.state.actions;
 lastAward.cash=collectCashTotal();lastAward.bet=engine.betMinor();lastAward.named=namedAward();lastAward.grand=engine.presentation.grandAwarded;
 captureAwardContext(true);
 lastAward.fountainEligible=collectionFountain()||lowerCashCollectionFountain149()||lastAward.named>0||lastAward.grand;lastAward.beganAt=millis();
#ifndef UI_HOST_TEST
 lastAward.backendStartsBefore=audioMusicDiagnostics().awardStarts;
#endif
}
void traceAwardCue(FeatureMusicPart part,uint32_t elapsed,uint32_t now){
 if(!lastAward.valid||lastAward.cueRequested||(part!=FeatureMusicPart::MajorAward&&part!=FeatureMusicPart::BigAward))return;
 lastAward.cueRequested=true;lastAward.part=part;lastAward.cueFirstMs=elapsed;lastAward.cueFirstAt=now;
}
void traceAwardFountain(uint32_t elapsed,uint32_t now){
 if(!lastAward.valid||lastAward.fountainStarted)return;
 lastAward.fountainStarted=true;lastAward.fountainFirstMs=elapsed;lastAward.fountainFirstAt=now;
}
void startAwardFountain(uint32_t origin,uint32_t now,FeatureMusicPart part,bool beginAward=false){
 fountainContinuous=false;fountainStopMs=0;fountainAt=origin;
 if(part!=FeatureMusicPart::Off&&!beginAward)fountainAwardMusic=false;fountainActive=now-fountainAt<coin_motion::fountainDurationMs;
 if(fountainActive){
  traceAwardFountain(now-phaseAt,now);traceAwardCue(part,now-phaseAt,now);
  // The existing pre-roll still starts first. Reassert its SAME program at
  // the actual fountain event: the worker is idempotent, so this cannot
  // restart a sounding phrase or let a visible burst silently bypass it.
  // A silent new visual fountain must not interrupt an earlier approved
  // phrase. The independent music epoch below keeps its original expiry.
  if(part!=FeatureMusicPart::Off)requestFeatureMusic(part,beginAward);
 }
}


uint32_t collectionLead(){return largeCollectionPrelude()?feature_award::largeCollectLead:kCollectLead;}
uint32_t collectionTail(){return collectionNamedDone?feature_award::namedCashTailMs:kCollectTail;}
int collectIndex(uint32_t elapsed){const auto lead=collectionLead();return elapsed<lead?-1:collection_steps101::indexAt(visible,elapsed-lead,kCollectStep,kCollectRepeatStep);}
unsigned collectedAt(uint32_t elapsed){
 const auto lead=collectionLead();if(elapsed<lead)return 0;
 return collection_steps101::completedAt(visible,elapsed-lead,kCollectStep,kCollectRepeatStep,kCollectArrival);
}
// The oracle owns a large temporary Frame. Inlining it into the benchmark
// kept that space live during every nested renderer and overflowed loopTask.
__attribute__((noinline)) void burningGoldReference65(const pa::Image& im,int x,int y,unsigned opacity,int clipTop,int clipBottom,unsigned dissolve){
 const reference_gold65::Frame burn(im,dissolve,&fireWorkspace);if(!burn.valid())return;
 for(unsigned r=0;r<im.h;++r)for(unsigned c=0;c<im.w;++c){const int yy=y+int(r);if(yy<clipTop||yy>=clipBottom)continue;
  const unsigned at=r*im.w+c,a=(im.alpha?im.alpha[at]:255)*opacity/255*burn.remaining(c,r)/255;
  if(a)dot(x+int(c),yy,im.rgb[at],a);
 }
 for(int yy=std::max(-int(reference_gold65::margin),clipTop-y);yy<std::min(int(im.h+reference_gold65::margin),clipBottom-y);++yy)
  for(int xx=-int(reference_gold65::margin);xx<int(im.w+reference_gold65::margin);++xx){const auto p=burn.fire(xx,yy);if(p.alpha)dot(x+xx,y+yy,p.color,p.alpha*opacity/255);}
 for(unsigned i=0;i<reference_gold65::embers;++i){const auto p=burn.ember(i);if(!p.alpha)continue;
  const int ix=int(std::floor(p.x)),iy=int(std::floor(p.y));const float u=p.x-ix,v=p.y-iy;
  for(unsigned r=0;r<2;++r)for(unsigned c=0;c<2;++c){const int yy=y+iy+int(r);if(yy<clipTop||yy>=clipBottom)continue;
   const unsigned a=std::min(255u,unsigned(p.alpha*2.f*(c?u:1-u)*(r?v:1-v)))*opacity/255;if(a)dot(x+ix+int(c),yy,p.color,a);
  }
 }
}
void burningGold(const pa::Image& im,int x,int y,unsigned opacity,int clipTop,int clipBottom,unsigned dissolve){
 const gold_burn::Frame burn(im,dissolve,&fireWorkspace);if(!burn.valid())return;
 const int top=std::max(0,clipTop-y),bottom=std::min(int(im.h),clipBottom-y);
 for(unsigned c=0;c<im.w;++c){
  if(x+int(c)<0||x+int(c)>=W)continue;
  const auto span=burn.remainingSpan(c);
  const int full=std::min(bottom,int(span.fullAfter)),after=std::min(bottom,int(span.after));
  for(int r=top;r<full;++r){
   const unsigned at=unsigned(r)*im.w+c,sourceAlpha=im.alpha?im.alpha[at]:255;
   const unsigned a=opacity==255?sourceAlpha:sourceAlpha*opacity/255;
   if(a)dot(x+int(c),y+r,im.rgb[at],a);
  }
  for(int r=std::max(top,full);r<after;++r){
   const unsigned at=unsigned(r)*im.w+c,sourceAlpha=im.alpha?im.alpha[at]:255;if(!sourceAlpha)continue;
   const unsigned a=sourceAlpha*opacity/255*burn.remaining(c,unsigned(r))/255;
   if(a)dot(x+int(c),y+r,im.rgb[at],a);
  }
 }
 const int first=std::max(std::max(-int(gold_burn::margin),clipTop-y),burn.firstFireRow());
 const int after=std::min(std::min(int(im.h+gold_burn::margin),clipBottom-y),burn.afterFireRow());
 for(int yy=first;yy<after;++yy){const auto row=burn.row(yy);
  for(int xx=-int(gold_burn::margin);xx<int(im.w+gold_burn::margin);++xx){const auto p=row.fire(xx);if(p.alpha)dot(x+xx,y+yy,p.color,p.alpha*opacity/255);}
 }
 for(unsigned i=0;i<gold_burn::embers;++i){const auto p=burn.ember(i);if(!p.alpha)continue;
  const int ix=int(std::floor(p.x)),iy=int(std::floor(p.y));const float u=p.x-ix,v=p.y-iy;
  for(unsigned r=0;r<2;++r)for(unsigned c=0;c<2;++c){const int yy=y+iy+int(r);if(yy<clipTop||yy>=clipBottom)continue;
   const unsigned a=std::min(255u,unsigned(p.alpha*2.f*(c?u:1-u)*(r?v:1-v)))*opacity/255;if(a)dot(x+ix+int(c),yy,p.color,a);
  }
 }
}
void fadedSprite(const pa::Image& im,int x,int y,unsigned opacity,int clipTop=RY,int clipBottom=122,unsigned dissolve=0){
 if(!opacity)return;
 if(dissolve){burningGold(im,x,y,opacity,clipTop,clipBottom,dissolve);return;}
 for(unsigned r=0;r<im.h;++r)for(unsigned c=0;c<im.w;++c){
  const int yy=y+int(r);if(yy<clipTop||yy>=clipBottom)continue;
  const unsigned at=r*im.w+c;unsigned a=(im.alpha?im.alpha[at]:255)*opacity/255;uint16_t color=im.rgb[at];
  if(a)dot(x+int(c),yy,color,a);
 }
}
void entryBird(uint32_t elapsed);
void collectBird(uint32_t elapsed){
 // The rising firebird and the later gold collection figure are distinct
 // poses in the footage. Never make the gold figure perform the fiery rise.
 const unsigned backdrop=collection_motion::backdropOpacity(elapsed);if(backdrop)fill(RX,RY,RW,RH,BLACK,backdrop);
 entryBird(elapsed);
 if(elapsed>=collection_motion::fireDelay)fireWipe(elapsed-collection_motion::fireDelay,collection_motion::fireDuration);
 const unsigned opacity=collection_motion::goldOpacity(elapsed);
 if(opacity)fadedSprite(winged_phoenix::hero(),56,18,opacity,RY,122,collection_motion::goldDissolve(elapsed));
}
unsigned collectedPhoenixCount(){unsigned n=0;for(const auto& cell:visible)n+=phoenixCell(cell);return n;}
void collectEmblem(uint32_t elapsed){
 const auto& im=collect_art::emblem();if(!im.rgb||!im.alpha)return;
 const float progress=elapsed<kCollectedHold?0.f:std::min(1.f,float(elapsed-kCollectedHold)/kCollectedBurn);
 const unsigned tail=elapsed+100>kCollectedIntro?(elapsed>=kCollectedIntro?0:(kCollectedIntro-elapsed)*255/100):255;
 if(progress>0&&!tail)return;
 float windX[100],windY[100],radialX[100],burnX[100];
 for(unsigned r=0;r<100;++r)windX[r]=.65f*std::sin(r*.41f+elapsed*.0107f)+.35f*std::sin(r*.19f-elapsed*.0131f);
 for(unsigned c=0;c<100;++c){
  windY[c]=.75f*std::sin(c*.27f-elapsed*.0127f)+.40f*std::sin(c*.11f+elapsed*.017f);
  radialX[c]=(float(c)-50)/50;
  if(progress>0)burnX[c]=(float(c)-im.w*.5f)/(im.w*.65f);
 }
 const float radius=1.05f*(1-progress);
 for(unsigned y=1;y+1<im.h;++y){
  const float dy=(float(y)-51)/50;
  float burnY=0;if(progress>0)burnY=(float(y)-im.h*(.43f-.18f*progress))/(im.h*.70f);
  for(unsigned x=1;x+1<im.w;++x){
  const float dx=radialX[x];
  const float outer=std::max(0.f,std::min(1.f,(dx*dx+dy*dy-.28f)*2.6f));
  float burnEdge=0,burnAlpha=1;
  if(progress>0){
   const float nx=burnX[x],ny=burnY;
   burnEdge=radius*radius-nx*nx-ny*ny+windY[x]*.015f;
   burnAlpha=std::max(0.f,std::min(1.f,(burnEdge+.05f)*10));
   // The original final blend has zero alpha here for every source color.
   // Avoid sampling invisible pixels without changing any nonzero blend.
   if(burnAlpha==0.f)continue;
  }
  uint16_t color;unsigned opacity;
  if(outer==0.f){
   // The centre has exactly zero displacement. At an integer texel the
   // original premultiplied filter returns that same color and alpha.
   const unsigned at=y*im.w+x;color=im.rgb[at];opacity=im.alpha[at];
   if(!opacity)continue;
  }else{
   const float sx=x+windX[y]*outer,sy=y+windY[x]*outer;
   if(sx<0||sy<0||sx>=im.w-1||sy>=im.h-1)continue;
   const int ix=int(sx),iy=int(sy);
   const unsigned u=unsigned((sx-ix)*256),v=unsigned((sy-iy)*256);
   const unsigned at=unsigned(iy)*im.w+unsigned(ix),offsets[]={at,at+1,at+im.w,at+im.w+1};
   const unsigned alpha[]={im.alpha[offsets[0]],im.alpha[offsets[1]],im.alpha[offsets[2]],im.alpha[offsets[3]]};
   if(!(alpha[0]|alpha[1]|alpha[2]|alpha[3]))continue;
   const unsigned weights[]={(256-u)*(256-v),u*(256-v),(256-u)*v,u*v};
   if(alpha[0]==alpha[1]&&alpha[0]==alpha[2]&&alpha[0]==alpha[3]){
    // Equal nonzero alpha is an exact common factor, even for this
    // material's translucent252-alpha interior. Preserve that alpha.
    uint32_t r=32768,g=32768,b=32768;
    for(unsigned n=0;n<4;++n){const auto sample=im.rgb[offsets[n]];const auto w=weights[n];r+=(sample>>11)*w;g+=((sample>>5)&63)*w;b+=(sample&31)*w;}
    color=uint16_t((r>>16)<<11|(g>>16)<<5|(b>>16));opacity=alpha[0];
   }else{
    uint32_t a=0,r=0,g=0,b=0;
    for(unsigned n=0;n<4;++n){const uint32_t w=weights[n]*alpha[n];const auto sample=im.rgb[offsets[n]];a+=w;r+=(sample>>11)*w;g+=((sample>>5)&63)*w;b+=(sample&31)*w;}
    if(!a)continue;
    color=uint16_t(((r+a/2)/a)<<11|((g+a/2)/a)<<5|(b+a/2)/a);
    opacity=std::min(255u,(a+32768)>>16);
   }
  }
  if(progress>0){
   opacity=unsigned(opacity*burnAlpha)*tail/255;
   if(burnEdge>-.05f&&burnEdge<.1f)color=blend(0xffe5,color,180);
  }
  dot(70+int(x),18+int(y),color,opacity);
 }}
}
void collectAnnouncement(uint32_t elapsed){
 if(elapsed>=kCollectedIntro)return;
 const unsigned fade=elapsed<kCollectedHold?255:(kCollectedIntro-elapsed)*255/kCollectedBurn;
 fill(RX,RY,RW,RH,BLACK,190*fade/255);
 if(collect_art::ready()){
  collectEmblem(elapsed);
  const auto& number=collect_art::number(collectedPhoenixCount());
  fadedSprite(number,(W-int(number.w))/2,57,fade,RY,RY+RH);
  const auto& label=collect_art::collected();fadedSprite(label,(W-int(label.w))/2,84,fade,RY,RY+RH);
  const auto& condition=collect_art::grandCondition();fadedSprite(condition,(W-int(condition.w))/2,96,fade,RY,RY+RH);
 }else{
  text(integer(collectedPhoenixCount()).c_str(),120,56,12,BRIGHT,1);
  text("COLLECTED",120,76,10,BRIGHT,1);text("15 CAN WIN GRAND",120,93,8,BRIGHT,1);
 }
}
void meterEntryRibbon(uint32_t elapsed){
 if(elapsed>1120)return;
 // The head leaves the counter, hooks down the right-hand reels, then curls
 // left and exits below the middle. Draw a short tapered flame trail; an
 // unrelated rotating arc or a straight line would not follow the footage.
 constexpr unsigned segments=56;
 const float tail=430.f,head=float(elapsed);
 const unsigned fade=elapsed>960?(1120-elapsed)*255/160:255;
 feature_entry::Point prior{};bool have=false;
 for(unsigned n=0;n<=segments;++n){
  const float t=head-tail+tail*n/segments;
  if(t<0||t>934){have=false;continue;}
  auto p=feature_entry::ribbon(t);
  const float u=float(n)/segments;
  // Small advected tongues soften the stroke without reversing its motion.
  p.x+=std::sin(t*.063f)*.35f*u;p.y+=std::sin(t*.079f+.8f)*.40f*u;
  if(have){
   const unsigned a=unsigned((25+180*u*u)*fade/255);
   for(int dy=-3;dy<=3;++dy)for(int dx=-3;dx<=3;++dx){
    const int radius=dx*dx+dy*dy;if(radius>9)continue;
    const uint16_t color=radius<2?(u>.8f?0xffe7:0xff60):radius<5?0xfcc0:0xf980;
    const unsigned opacity=radius<2?a:radius<5?a*3/4:a/4;
    line(int(prior.x+.5f)+dx,int(prior.y+.5f)+dy,int(p.x+.5f)+dx,int(p.y+.5f)+dy,color,opacity);
   }
  }
  prior=p;have=true;
 }
 if(elapsed<=934){
  const auto p=feature_entry::ribbon(float(elapsed));
  const auto& bird=pa::symbol(unsigned(Symbol::Phoenix),elapsed,false);
  // A small head from the existing original digital publisher art, rather
  // than a flat dot at the front of the flame trail. Its source is value-free.
  for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x){
   const unsigned sx=17+x*19/8,sy=y*22/8,at=sy*bird.w+sx;
   const unsigned a=(bird.alpha?bird.alpha[at]:255)*fade/255;
   if(a)dot(int(p.x)-4+int(x),int(p.y)-4+int(y),bird.rgb[at],a);
  }
 }
}
void returnBird(uint32_t elapsed){
 if(elapsed>760)return;
 const float t=std::min(1.f,float(elapsed)/feature_entry::birdRiseMs),rise=t*t*(3-2*t);
 const int top=int(135-173*rise),width=230,height=173,left=(W-width)/2;
 const auto& im=winged_phoenix::hero();
 const unsigned opacity=elapsed>520?unsigned((760-elapsed)*255/240):255;
 // One uniform scale of the clean reconstruction. Restrict the overlay to
 // the viewport; the bank and feature counter remain legible throughout.
 for(int y=std::max(RY,top);y<std::min(122,top+height);++y){
  const unsigned sy=unsigned((y-top)*im.h/height);
  for(int x=left;x<left+width;++x){
   const unsigned sx=unsigned((x-left)*im.w/width),at=sy*im.w+sx;
   const unsigned a=(im.alpha?im.alpha[at]:255)*opacity/255;
   if(a)dot(x,y,im.rgb[at],a);
  }
 }
}
void entryBirdUncached(uint32_t elapsed,uint16_t* captureRgb=nullptr,uint8_t* captureAlpha=nullptr){
 if(elapsed>440)return;
 if(!entry_art::ready()){returnBird(elapsed);return;}
 const auto& im=entry_art::sprite();
 const float top=feature_entry::birdTop(elapsed);
 constexpr int left=3,width=234,height=176;
 if(top>=H||top+height<=0)return;
 // Retarget the generated crest/neck to the measured40px head-to-wing rise.
 // The collection bird has a different head/pose and never uses this mesh.
 constexpr float wingY=40.f;
 const float sourceWing=2.f+(im.h-4)*.31f;
 const float time=elapsed*.001f;
 struct Column {float dx,headZone,feather;};
 static constexpr auto columns=[](){
  std::array<Column,width> result{};
  for(int x=0;x<width;++x){const float dx=x-width*.5f,absolute=dx<0?-dx:dx;
   result[x]={dx,std::max(0.f,1.f-absolute/48.f),std::min(1.f,absolute/50.f)};}
  return result;
 }();
 float horizontal[width],flatX[width];
 for(int x=0;x<width;++x){horizontal[x]=std::sin(x*.085f-time*23.f);flatX[x]=(columns[x].dx+width*.5f)*im.w/width;}
 for(int y=std::max(0,int(std::floor(top)));y<std::min(H,int(std::ceil(top+height)));++y){
  const float ly=y-top;
  if(ly<0||ly>=height)continue;
  const float baseY=ly<wingY?ly*sourceWing/wingY:sourceWing+(ly-wingY)*(im.h-sourceWing)/(height-wingY);
  const float headWeight=std::max(0.f,1.f-ly/52.f);
  const float wave=std::sin(ly*.095f-time*27.f);
  const float trailing=std::max(0.f,std::min(1.f,(ly-wingY)/40.f));
  const float tailFeather=std::max(0.f,(ly-100.f)/76.f);
  for(int x=0;x<width;++x){
   const auto& column=columns[x];
   const float sx=headWeight>0&&column.headZone>0?(column.dx/(1.f-.35f*headWeight*column.headZone)+width*.5f)*im.w/width:flatX[x];
   const float feather=std::max(trailing*column.feather,tailFeather);
   const float sampleX=sx+wave*.65f*feather,sampleY=baseY+horizontal[x]*.70f*feather;
   if(sampleX<0||sampleY<0||sampleX>=im.w-1||sampleY>=im.h-1)continue;
   const int ix=int(sampleX),iy=int(sampleY);
   const unsigned u=unsigned((sampleX-ix)*256),v=unsigned((sampleY-iy)*256);
   const unsigned weights[]={(256-u)*(256-v),u*(256-v),(256-u)*v,u*v};
   const unsigned at=unsigned(iy)*im.w+unsigned(ix),offsets[]={at,at+1,at+im.w,at+im.w+1};
   uint32_t a=0,r=0,g=0,b=0;
   for(unsigned n=0;n<4;++n){
    const uint32_t w=weights[n]*im.alpha[offsets[n]];const uint16_t color=im.rgb[offsets[n]];
    a+=w;r+=(color>>11)*w;g+=((color>>5)&63)*w;b+=(color&31)*w;
   }
   if(!a)continue;
   // Premultiplied-alpha filtering avoids dark fringes at moving flame tips.
   const uint16_t color=uint16_t(((r+a/2)/a)<<11|((g+a/2)/a)<<5|(b+a/2)/a);
   const unsigned opacity=std::min(255u,(a+32768u)>>16);
   if(captureRgb&&captureAlpha){captureRgb[y*W+left+x]=color;captureAlpha[y*W+left+x]=uint8_t(opacity);}
   else dot(left+x,y,color,opacity);
  }
 }
}
void entryBird(uint32_t elapsed){
 if(elapsed>440)return;
 if(effectsCacheReady&&effectsUseCache){
  const auto* rgb=preparedEffects.birdAt(elapsed);const auto* alpha=preparedEffects.opacityAt(elapsed);const auto* spans=preparedEffects.spansAt(elapsed);
  for(int y=std::max(0,dotClipTop);y<std::min(H,dotClipBottom);++y){
   const unsigned offset=unsigned(y)*W;
   for(unsigned x=spans[y].left;x<spans[y].right;++x){const unsigned at=offset+x,a=alpha[at];if(a)pixels[at]=a==255?rgb[at]:blend(rgb[at],pixels[at],a);}
  }
 }
 else entryBirdUncached(effect_frames::sample(elapsed));
}
bool prepareEffectFrames(){
 if(effectsCacheReady)return true;
 if(!entry_art::ready())return false;
#ifndef UI_HOST_TEST
 auto* block=ps_malloc(effect_frames::bytes);
#else
 auto* block=std::malloc(effect_frames::bytes);
#endif
 if(!block)return false;
 const uint32_t began=micros();preparedEffects.bind(block);
 for(unsigned n=0;n<effect_frames::fireCount;++n){
  const fire_field::Flow field(effect_frames::time(n),&fireWorkspace);
  auto* rgb=preparedEffects.fire+size_t(n)*W*H;
  for(int y=0;y<H;++y){const auto row=field.row(fireY[y]);for(int x=0;x<W;++x)rgb[y*W+x]=row.pixel(fireX[x]);}
#ifndef UI_HOST_TEST
  delay(1);
#endif
 }
 std::memset(preparedEffects.bird,0,size_t(W)*H*effect_frames::birdCount*3);
 for(unsigned n=0;n<effect_frames::birdCount;++n){
  entryBirdUncached(effect_frames::time(n),preparedEffects.bird+size_t(n)*W*H,preparedEffects.opacity+size_t(n)*W*H);
  auto* spans=preparedEffects.spans+size_t(n)*H;const auto* alpha=preparedEffects.opacity+size_t(n)*W*H;
  for(unsigned y=0;y<H;++y){unsigned left=W,right=0;for(unsigned x=0;x<W;++x)if(alpha[y*W+x]){left=std::min(left,x);right=x+1;}spans[y]={uint8_t(left),uint8_t(right)};}
#ifndef UI_HOST_TEST
  delay(1);
#endif
 }
 effectsCacheBuildUs=micros()-began;effectsCacheReady=true;return true;
}
#include "reference_effect_v0238.h"
void holdEntryEffect(uint32_t elapsed){
 if(holdEntryMeter)meterEntryRibbon(elapsed);
 const uint32_t lead=feature_entry::lead(holdEntryMeter);
 if(elapsed<lead)return;
 const uint32_t e=elapsed-lead;entryBird(e);
 if(e>=feature_entry::fireDelayMs)fireWipe(e-feature_entry::fireDelayMs,feature_entry::fireMs);
}
using CollectPoint=collection_pickup::Point;
CollectPoint collectFlight(unsigned elapsed,float sx,float sy){return collection_pickup::at(elapsed,sx,sy);}
// Inverse-sampled premultiplied RGBA avoids holes, black corners and jagged
// stamps when a small bird banks or a coin rolls. Source storage is unchanged.
void turnedSprite(const pa::Image& im,float cx,float cy,float width,float height,float angle,unsigned opacity,float anchorX=-1,float anchorY=-1){
 if(!opacity||!im.rgb||!im.alpha||width<=0||height<=0)return;
 const float cs=std::cos(angle),sn=std::sin(angle);
 const bool anchored=anchorX>=0&&anchorY>=0;
 const float extentX=anchored?std::max(anchorX+1,im.w-anchorX)*width/im.w:width*.5f;
 const float extentY=anchored?std::max(anchorY+1,im.h-anchorY)*height/im.h:height*.5f;
 const int reach=anchored?int(std::ceil(std::sqrt(extentX*extentX+extentY*extentY)+1)):int(std::ceil(std::sqrt(width*width+height*height)*.5f+1));
 if(!anchored){anchorX=(im.w-1)*.5f;anchorY=(im.h-1)*.5f;}
 for(int y=std::max(dotClipTop,int(std::floor(cy))-reach);y<std::min(dotClipBottom,int(std::ceil(cy))+reach);++y)
  for(int x=std::max(0,int(std::floor(cx))-reach);x<std::min(W,int(std::ceil(cx))+reach);++x){
   const float dx=x-cx,dy=y-cy;
   const float u=(dx*cs+dy*sn)*im.w/width+anchorX;
   const float v=(-dx*sn+dy*cs)*im.h/height+anchorY;
   const int ix=int(std::floor(u)),iy=int(std::floor(v));
   if(ix<-1||iy<-1||ix>=int(im.w)||iy>=int(im.h))continue;
   const unsigned fx=unsigned((u-ix)*256),fy=unsigned((v-iy)*256);
   const unsigned weight[]={(256-fx)*(256-fy),fx*(256-fy),(256-fx)*fy,fx*fy};
   uint32_t alpha=0,r=0,g=0,b=0;
   for(unsigned j=0;j<4;++j){const int xx=ix+int(j&1),yy=iy+int(j>>1);if(xx<0||yy<0||xx>=int(im.w)||yy>=int(im.h))continue;
    const unsigned at=unsigned(yy)*im.w+unsigned(xx);const uint32_t a=weight[j]*im.alpha[at];const auto color=im.rgb[at];
    alpha+=a;r+=(color>>11)*a;g+=((color>>5)&63)*a;b+=(color&31)*a;
   }
   if(!alpha)continue;
   const uint16_t color=uint16_t(((r+alpha/2)/alpha)<<11|((g+alpha/2)/alpha)<<5|(b+alpha/2)/alpha);
   dot(x,y,color,((alpha+32768)>>16)*opacity/255);
  }
}
void drawCoinParticleReference62(const coin_motion::Particle& p){
 if(!p.active||!p.opacity)return;
 const auto& art=clean_coins::frame(p.frame);const pa::Image im={uint16_t(art.w),uint16_t(art.h),art.rgb,art.alpha,0};
 turnedSprite(im,p.x,p.y,p.size,p.size,p.roll*(6.2831853f/32.f),p.opacity);
}
bool coinMapsUse=true;uint32_t coinMapsBuildUs=0;
void drawCoinParticle(const coin_motion::Particle& p){
 if(!p.active||!p.opacity)return;
 const auto view=coinMapsUse?coin_sampler::view(p.size,p.roll,p.frame):coin_sampler::View{};
 if(!view.map){drawCoinParticleReference62(p);return;}
 const auto& im=clean_coins::frame(p.frame);const int reach=view.map->reach;
 for(int y=std::max({0,dotClipTop,p.y-reach});y<std::min({H,dotClipBottom,p.y+reach});++y){
  const auto& row=view.rows[y-p.y+reach];
  const int scaledY=(y-p.y)*32,denominator=int(p.size);int band=scaledY/denominator;if(scaledY<0&&scaledY%denominator)--band;
  int nativeLeft=127,nativeRight=-127;
  for(int offset=-1;offset<=1;++offset){const int at=band+offset-coin_sampler::canonicalTop;
   if(at<0||at>=coin_sampler::canonicalRows)continue;nativeLeft=std::min(nativeLeft,int(view.phase[at].left));nativeRight=std::max(nativeRight,int(view.phase[at].right));
  }
  if(nativeRight<=nativeLeft)continue;
  const int lo=nativeLeft*denominator,hi=nativeRight*denominator;
  const int phaseLeft=lo>=0?lo/32:-((-lo+31)/32),phaseRight=hi>=0?(hi+31)/32:-((-hi)/32);
  const int left=std::max({0,p.x+row.left,p.x+phaseLeft-1}),right=std::min({W,p.x+row.right,p.x+phaseRight+1});
  for(int x=left;x<right;++x){const uint32_t packed=view.texels[row.offset+unsigned(x-p.x-row.left)];
   if(!(packed&coin_sampler::validBit))continue;
   const int ix=int(packed&63)-1,iy=int((packed>>6)&63)-1;
   const unsigned fx=(packed>>12)&255,fy=(packed>>20)&255;
   const unsigned weight[]={(256-fx)*(256-fy),fx*(256-fy),(256-fx)*fy,fx*fy};
   if(ix>=0&&iy>=0&&ix+1<int(im.w)&&iy+1<int(im.h)){
    const unsigned at=unsigned(iy)*im.w+unsigned(ix);
    if((im.alpha[at]&im.alpha[at+1]&im.alpha[at+im.w]&im.alpha[at+im.w+1])==255){
     const uint16_t c[]={im.rgb[at],im.rgb[at+1],im.rgb[at+im.w],im.rgb[at+im.w+1]};uint32_t r=32768,g=32768,b=32768;
     for(unsigned j=0;j<4;++j){r+=(c[j]>>11)*weight[j];g+=((c[j]>>5)&63)*weight[j];b+=(c[j]&31)*weight[j];}
     dot(x,y,uint16_t((r>>16)<<11|(g>>16)<<5|(b>>16)),p.opacity);continue;
    }
   }
   uint32_t alpha=0,r=0,g=0,b=0;
   for(unsigned j=0;j<4;++j){const int xx=ix+int(j&1),yy=iy+int(j>>1);if(xx<0||yy<0||xx>=int(im.w)||yy>=int(im.h))continue;
    const unsigned at=unsigned(yy)*im.w+unsigned(xx);const uint32_t a=weight[j]*im.alpha[at];if(!a)continue;const auto color=im.rgb[at];
    alpha+=a;r+=(color>>11)*a;g+=((color>>5)&63)*a;b+=(color&31)*a;
   }
   if(!alpha)continue;
   const uint16_t color=uint16_t(((r+alpha/2)/alpha)<<11|((g+alpha/2)/alpha)<<5|(b+alpha/2)/alpha);
   dot(x,y,color,((alpha+32768)>>16)*p.opacity/255);
  }
 }
}
void coinShower(uint32_t elapsed){
 // Final transfer fountain: launch below the reels, rise through the centre,
 // fan out, then fall. No top-edge particle spawning or game RNG is used.
 const int oldTop=dotClipTop,oldClip=dotClipBottom;dotClipTop=0;dotClipBottom=H;
 if(fountainContinuous){
  const unsigned first=coin_motion::firstLiveBirth(elapsed);
  const unsigned last=std::min(elapsed,fountainStopMs)/coin_motion::birthMs;
  if(first<=last)for(unsigned serial=first;serial<=last;++serial)drawCoinParticle(coin_motion::continuousAt(serial,elapsed));
 }else for(unsigned i=0;i<coin_motion::count;++i)drawCoinParticle(coin_motion::at(i,elapsed));
 dotClipTop=oldTop;dotClipBottom=oldClip;
}
CollectPoint pickupFlight(unsigned ms,float sx,float sy,float tx,float ty){
 const auto p=collectFlight(ms,sx,sy);const float t=std::min(1.f,float(ms)/kCollectArrival),ease=t*t*(3-2*t);
 float y=p.y+(ty-7)*ease;
 if(tx==120.f&&ty==83.f&&ms<kCollectArrival){
  const unsigned row=unsigned(std::max(0,std::min(2,int(std::lround((sy-33.5f)/35.f)))));
  if(row){
   // Preserve the source-derived curl. Move its upper-cabinet destination
   // into the reel area only over the last two100ms route legs; moving
   // the whole loop down early sends its body below the reel viewport.
   const float u=ms>500?float(ms-500)/200.f:0.f;
   y=p.y+(ty-7)*(u*u*(3-2*u));
  }
 }
 return {p.x+(tx-148)*ease,y};
}
CollectPoint pickupDirection(unsigned ms,float sx,float sy,float tx,float ty){
 // A finite-span heading banks the body through the spline/arrival join.
 // An instantaneous4ms derivative made a larger body visibly snap there.
 constexpr unsigned span=48;
 const auto before=pickupFlight(ms>span?ms-span:0,sx,sy,tx,ty);
 const auto after=pickupFlight(std::min(ms+span,collection_pickup::flightMs),sx,sy,tx,ty);
 return {after.x-before.x,after.y-before.y};
}
pickup_frames::Bank pickupSamples;bool pickupSamplesUse=true;
bool preparePickupSamples(){
 if(pickupSamples.ready())return true;
#ifdef UI_HOST_TEST
 void* memory=std::malloc(pickup_frames::bytes);
#else
 void* memory=ps_malloc(pickup_frames::bytes);
#endif
 if(!memory)return false;
 if(pickupSamples.bind(memory,pickup_frames::bytes))return true;
 std::free(memory);return false;
}
CollectPoint pickupPreparedPosition(unsigned ms,float sx,float sy,float tx,float ty,bool cached){
 if(!cached)return pickupFlight(ms,sx,sy,tx,ty);
 if(!pickupSamples.has(ms,pickup_frames::positionBit)){
  pickupSamples.at(ms).position=pickupFlight(ms,sx,sy,tx,ty);pickupSamples.mark(ms,pickup_frames::positionBit);
 }
 return pickupSamples.at(ms).position;
}
CollectPoint pickupPreparedDirection(unsigned ms,float sx,float sy,float tx,float ty,bool cached){
 constexpr unsigned span=48;
 const auto before=pickupPreparedPosition(ms>span?ms-span:0,sx,sy,tx,ty,cached);
 const auto after=pickupPreparedPosition(std::min(ms+span,collection_pickup::flightMs),sx,sy,tx,ty,cached);
 return {after.x-before.x,after.y-before.y};
}
void pickupPreparedEdges(unsigned ms,float sx,float sy,float tx,float ty,CollectPoint edges[2],bool cached){
 if(cached&&pickupSamples.has(ms,pickup_frames::edgesBit)){
  edges[0]=pickupSamples.at(ms).edges[0];edges[1]=pickupSamples.at(ms).edges[1];return;
 }
 const auto p=pickupPreparedPosition(ms,sx,sy,tx,ty,cached);
 const auto direction=pickupPreparedDirection(ms,sx,sy,tx,ty,cached);
 const float dx=direction.x,dy=direction.y;
 const float length=std::max(.01f,std::sqrt(dx*dx+dy*dy));
 const float tailX=(winged_phoenix::flightTailX-winged_phoenix::flightAnchorX)*winged_phoenix::flightSpriteSize/32;
 const float tailY=(winged_phoenix::flightTailY-winged_phoenix::flightAnchorY)*winged_phoenix::flightSpriteSize/32;
 const float forwardX=std::cos(winged_phoenix::flightForwardAngle),forwardY=std::sin(winged_phoenix::flightForwardAngle);
 const float along=tailX*forwardX+tailY*forwardY,across=-tailX*forwardY+tailY*forwardX;
 const CollectPoint tail={p.x+(dx*along-dy*across)/length,p.y+(dy*along+dx*across)/length};
 const float gap=.8f+.3f*std::sin(ms*.021f);
 for(unsigned lane=0;lane<2;++lane){const float side=lane?gap:-gap;edges[lane]={tail.x-dy/length*side,tail.y+dx/length*side};}
 if(cached){pickupSamples.at(ms).edges[0]=edges[0];pickupSamples.at(ms).edges[1]=edges[1];pickupSamples.mark(ms,pickup_frames::edgesBit);}
}
float pickupPreparedHeading(unsigned ms,float sx,float sy,float tx,float ty,bool cached){
 if(cached&&pickupSamples.has(ms,pickup_frames::headingBit))return pickupSamples.at(ms).heading;
 const auto direction=pickupPreparedDirection(ms,sx,sy,tx,ty,cached);
 const float angle=std::atan2(direction.y,direction.x)-winged_phoenix::flightForwardAngle;
 if(cached){pickupSamples.at(ms).heading=angle;pickupSamples.mark(ms,pickup_frames::headingBit);}
 return angle;
}
void drawPickupFlight(unsigned local,float sx,float sy,float tx=148,float ty=7){
 const bool cached=pickupSamplesUse&&pickupSamples.select({sx,sy,tx,ty});
 const int oldClip=dotClipBottom;dotClipBottom=122;
 // The source flares while its spirit flies out. A single translated frontal
 // stamp loses both this lingering flare and the changing flight direction.
 const unsigned sourceAlpha=collection_pickup::sourceOpacity(local);
 if(sourceAlpha)turnedSprite(winged_phoenix::pickup(),sx,sy,34.f+2.f*std::sin(local*.03f),24,0,sourceAlpha);
 CollectPoint prior[2]{};bool hasPrior=false;
 for(unsigned j=0;j<=64;++j){
  const int history=int(local)-int(j*collection_pickup::trailMs/64);
  if(history<0||history>int(collection_pickup::flightMs)){hasPrior=false;continue;}
  CollectPoint edges[2];pickupPreparedEdges(unsigned(history),sx,sy,tx,ty,edges,cached);
  const unsigned a=(230u*(64-j)*(64-j))/(64*64);
  for(unsigned lane=0;lane<2;++lane){
   const auto edge=edges[lane];
   if(hasPrior){
    for(int glow=-1;glow<=1;++glow)line(int(std::lround(prior[lane].x)),int(std::lround(prior[lane].y))+glow,int(std::lround(edge.x)),int(std::lround(edge.y))+glow,0xfba2,a/3);
    line(int(std::lround(prior[lane].x)),int(std::lround(prior[lane].y)),int(std::lround(edge.x)),int(std::lround(edge.y)),0xffe9,a);
   }
   prior[lane]=edge;
  }
  hasPrior=true;
 }
 const unsigned headAlpha=pickup_arrival::flightAlpha(local,collection_pickup::headOpacity(local));
 if(headAlpha){
  const auto p=pickupPreparedPosition(local,sx,sy,tx,ty,cached);
  // Keep the arrival-facing flight pose while it fades into the frontal
  // ember. Following the tiny post-impact ribbon orbit spins the body.
  const float angle=pickupPreparedHeading(pickup_arrival::headingMs(local),sx,sy,tx,ty,cached);
  // The observed route follows the bird's visible body mass. Eye/beak
  // landmarks register the keys, but are not the path's body pivot.
  turnedSprite(winged_phoenix::flight(local),p.x,p.y,winged_phoenix::flightSpriteSize,winged_phoenix::flightSpriteSize,angle,headAlpha,winged_phoenix::flightAnchorX,winged_phoenix::flightAnchorY);
 }
 const unsigned impact=pickup_arrival::flareAlpha(local);
 if(impact)turnedSprite(winged_phoenix::pickup(),tx,ty+7.f,30.f,22.f,0.f,impact);
 dotClipBottom=oldClip;
}
#include "reference_pickup_v0267.h"
bool holdCenterWinnerActive89(uint32_t now);
void collectRibbon(uint32_t elapsed){
 const int number=collectIndex(elapsed);const auto step=collection_steps101::at(visible,number);if(!step)return;
 const uint32_t age=elapsed-collectionLead();
 const bool centered=holdCenterWinnerActive89(uiGameMillis());
 auto flight=[&](int index){
  const auto item=collection_steps101::at(visible,index);if(!item)return;
  const auto start=collection_steps101::startAt(visible,index,kCollectStep,kCollectRepeatStep);
  if(age<start)return;const uint32_t local=age-start;
  if(local>=collection_pickup::flightMs+collection_pickup::trailMs)return;
  const float sx=RX+(item.cell/3+.5f)*TW,sy=RY+(item.cell%3+.5f)*TH;
  if(centered)drawPickupFlight(local,sx,sy,120,83);else drawPickupFlight(local,sx,sy);
 };
 // Only a same-cell repeat overlaps. Its old head/flare and sampled ribbon
 // tail finish naturally behind the new launch, instead of being reset.
 // Two such flights suffice:1160+900 <2*1067. Ordinary paths stay exact.
 if(step.repetition)flight(number-1);
 flight(number);
}
bool ordinaryCollectionTransfer(){return !collectionNamedDone&&!collectionFountain()&&!engine.presentation.grandAwarded&&!namedAward();}
uint32_t collectionTransferDuration(){return collectionFountain()?feature_award::bigHoldMs:ordinaryCollectionTransfer()?feature_award::normalWinTransferMs:feature_award::normalHoldMs;}
uint32_t collectionReturnLead(){return collectionNamedDone&&!collectionFountain()?1800:ordinaryCollectionTransfer()?feature_award::normalWinSettleMs:0;}
void fireBand(int x,int y,int width,int height,uint32_t elapsed,uint32_t duration){
 if(!duration||elapsed>=duration||width<=0||height<=0)return;
 const uint32_t sample=uint64_t(elapsed)*1400/duration;
 const bool cached=effectsCacheReady&&effectsUseCache;
 const uint16_t* material=cached?preparedEffects.fireAt(sample):nullptr;
 alignas(fire_field::Flow) unsigned char fieldStorage[sizeof(fire_field::Flow)];
 const auto* field=cached?nullptr:new(fieldStorage)fire_field::Flow(effect_frames::sample(sample),&fireWorkspace);
 const float t=float(elapsed)/duration;
 for(int xx=0;xx<width;++xx){
  const float nx=float(xx)/width;
  const float fringe=.038f*std::sin(xx*.13f+elapsed*.011f)+.022f*std::sin(xx*.31f-elapsed*.019f);
  const unsigned sourceX=unsigned(xx)*(W-1)/std::max(1,width-1);
  for(int yy=0;yy<height;++yy){
   const unsigned sourceY=unsigned(yy)*(H-1)/std::max(1,height-1);
   const uint16_t color=material?material[sourceY*W+sourceX]:field->pixel(fireX[sourceX],fireY[sourceY]);
   const float ny=float(yy)/height;
   float opacity=1;
   if(t<.42f)opacity=(t/.42f*1.4f-(1-ny+std::abs(nx-.5f)*.38f+fringe))/.09f;
   else if(t>=.65f){const float heat=float((color>>5)&63)/63.f;opacity=(1-ny-(t-.65f)/.35f+fringe+(heat-.5f)*.05f)/.10f;}
   const unsigned alpha=unsigned(std::max(0.f,std::min(1.f,opacity))*255);
   if(alpha)dot(x+xx,y+yy,color,alpha);
  }
 }
 if(field)field->~Flow();
}
void timedCollectedAnnouncement(uint32_t elapsed,bool named){
 const uint32_t hold=named?3500:3300,burn=named?1300:900;
 if(elapsed>=hold+burn)return;
 const uint32_t source=elapsed<hold?elapsed*kCollectedHold/hold:kCollectedHold+(elapsed-hold)*kCollectedBurn/burn;
 collectAnnouncement(source);
}
uint32_t postCountSweepMs(){return collectionNamedDone?feature_award::namedHeaderFireMs:1400;}
void fullCollectionSweep(uint32_t elapsed,uint32_t fireDuration){
 entryBird(elapsed);
 if(elapsed>=feature_entry::fireDelayMs)fireWipe(elapsed-feature_entry::fireDelayMs,fireDuration);
}
int wideCashWidth112(const char* value,unsigned size){int width=0;for(auto p=(const unsigned char*)value;*p;++p)width+=(pmoney::glyph(size,*p).advance*15+4)/8;return width;}
void wideCashText112(const char* value,int right,int y,unsigned size,uint16_t color){
 int x=right-wideCashWidth112(value,size);const auto* alpha=pmoney::pixels(size);
 for(auto p=(const unsigned char*)value;*p;++p){const auto& g=pmoney::glyph(size,*p);const unsigned width=(g.w*15+7)/8;
  for(unsigned row=0;row<g.h;++row)for(unsigned col=0;col<width;++col){
   const int q=int((col*2+1)*8*128/15)-128;int source=q/256;if(q<0&&q%256)--source;const unsigned f=unsigned(q-source*256);
   auto at=[&](int column){return column>=0&&column<int(g.w)?alpha[g.offset+row*g.w+unsigned(column)]:0;};
   const unsigned a=(unsigned(at(source))*(256-f)+unsigned(at(source+1))*f+128)>>8;if(a)dot(x+int(col),y+int(row),color,a);
  }
  x+=(g.advance*15+4)/8;
 }
}
// Grand stays on the continuous collection display; Major uses its title.
bool drawHoldCenterWinner89(uint32_t now);
void awardPanel(uint32_t elapsed){
 if(engine.presentation.grandAwarded){drawHoldCenterWinner89(phaseAt+elapsed);return;}
 auto paint=[](int x,int y,uint16_t color,unsigned alpha){dot(x,y,color,alpha);};
 authored_major112::vignette(paint);
 if(elapsed>=500){
  const bool filled=elapsed>=feature_award::revealBurnAt+700;
  authored_major112::title(elapsed,filled,paint);
 }
 authored_major112::amountFrame(elapsed,paint);
 if(elapsed>=feature_award::amountAt){
  const auto amount=money(namedAward());unsigned size=20;
  for(unsigned candidate:{20u,16u,10u,7u}){size=candidate;if(wideCashWidth112(amount.c_str(),size)<=184)break;}
  const int right=120+wideCashWidth112(amount.c_str(),size)/2,y=86-int(size)/2;
  wideCashText112(amount.c_str(),right+1,y+2,size,BLACK);
  for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)wideCashText112(amount.c_str(),right+dx,y+dy,size,0xcdca);
  wideCashText112(amount.c_str(),right,y,size,0x2548);
 }
 // Existing phase clocks and moving Phoenix paths remain untouched. The
 // amount is green when it first appears, independently of the title fill.
 if(elapsed>=feature_award::revealBurnAt&&elapsed<feature_award::revealEnd)fireBand(22,34,196,66,elapsed-feature_award::revealBurnAt,feature_award::revealEnd-feature_award::revealBurnAt);
 if(elapsed>=feature_award::exitBurnAt)fireBand(22,34,196,66,elapsed-feature_award::exitBurnAt,feature_award::durationMs-feature_award::exitBurnAt);
 if(elapsed>=feature_award::birdAt&&elapsed<feature_award::pickupAt)collectBird(elapsed-feature_award::birdAt);
 if(elapsed>=feature_award::pickupAt&&elapsed<feature_award::pickupAt+collection_pickup::flightMs){
  int cell=7;for(int i=0;i<15;++i)if(phoenixCell(visible[i])&&visible[i].kind==phoenix::PrizeKind::Major){cell=i;break;}
  drawPickupFlight(elapsed-feature_award::pickupAt,RX+(cell/3+.5f)*TW,RY+(cell%3+.5f)*TH,120,83);
 }
}

void drawHoldWinner(uint32_t now){
 const auto& im=pa::awardWinner();if(!im.rgb)return;
 // One existing collection epoch spans Collect, WinTransfer and ReturnWipe.
 // Consume the clean sprite directly: no second image or mutable frame cache.
 const uint32_t elapsed=now-lastAward.beganAt;
 for(unsigned y=0;y<std::min(14u,unsigned(im.h));++y)for(unsigned x=0;x<std::min(72u,unsigned(im.w));++x){
  const unsigned at=y*im.w+x;const auto color=im.rgb[at];
  if(im.key&&color==im.key)continue;
  const unsigned alpha=im.alpha?im.alpha[at]:255;
  // Keep the faint antialias/glow fringe fixed, avoiding silhouette breathing.
  if(alpha)dot(2+int(x),int(y),alpha>=32?hold_winner::shade(color,x,y,elapsed):color,alpha);
 }
}
#include "hold_center_winner89.inc"
void drawTransition(uint32_t now){const uint32_t e=now-phaseAt;switch(phase){
  case Phase::Revive:
   if(e<kCollectedIntro)collectAnnouncement(e);
   if(e>=revival_timing::birdAt)entryBird(e-revival_timing::birdAt);
   if(e>=revival_timing::fireAt)fireWipe(e-revival_timing::fireAt,feature_entry::fireMs);
   if(e>=revival_timing::promptAt){fill(77,0,49,14,native_title::background);headerCaption("PRESS START",0,6,BRIGHT);headerCaption("FEATURE",7,6,BRIGHT);}
   break;
  case Phase::HoldWipe:holdEntryEffect(e);break;
  case Phase::ReturnWipe:{const uint32_t lead=collectionReturnLead();fill(0,0,186,14,native_title::background);if(!drawHoldCenterWinner89(now)){drawHoldWinner(now);if(ordinaryCollectionTransfer()&&e<lead)cashText(money(collectCashTotal()).c_str(),181,2,10,BRIGHT);}if(e>=lead){entryBird(e-lead);if(e>=lead+feature_entry::fireDelayMs)fireWipe(e-lead-feature_entry::fireDelayMs);}break;}
  case Phase::FreeIntro:{if(freeRetriggerIntro()){std::string count=integer(pendingFreeCount)+" FREE GAMES WON";fill(0,0,126,14,native_title::background);text(count.c_str(),63,0,7,BRIGHT,1);text("FREE GAMES ADDED",63,8,6,WHITE,1);}else drawWisdomEntry29(e,pendingFreeCount);break;}
  case Phase::SpinAward:case Phase::SpinAwardSettle:drawSpinAwardHeader(now);break;
  case Phase::HoldIntro:fill(77,0,49,14,native_title::background);headerCaption("PRESS START",0,6,BRIGHT);headerCaption("FEATURE",7,6,BRIGHT);break;
  case Phase::AwardIntro:fill(0,0,186,14,native_title::background);if(engine.presentation.grandAwarded)drawHoldCenterWinner89(now);else timedCollectedAnnouncement(e,true);break;
  case Phase::Jackpot:fill(0,0,186,14,native_title::background);awardPanel(e);break;
  case Phase::Collect:{fill(0,0,186,14,native_title::background);if(!drawHoldCenterWinner89(now)){drawHoldWinner(now);if(displayWin>0)cashText(money(displayWin).c_str(),181,2,10,BRIGHT);}if(!collectionNamedDone){if(largeCollectionPrelude()){timedCollectedAnnouncement(e,true);if(e>=feature_award::largeBirdAt&&e<collectionLead())collectBird(e-feature_award::largeBirdAt);}else if(e<kCollectLead){timedCollectedAnnouncement(e,false);
   // This was squeezed into a14px header. Bring the Phoenix up from below
   // the LCD, then let its fire cross the entire viewport before pickups.
   if(e>=feature_award::ordinarySweepAt)fullCollectionSweep(e-feature_award::ordinarySweepAt,feature_entry::fireMs);
  }}collectRibbon(e);break;}
  case Phase::WinTransfer:fill(0,0,186,14,native_title::background);if(!drawHoldCenterWinner89(now)){drawHoldWinner(now);cashText(money(collectCashTotal()).c_str(),181,2,10,BRIGHT);}
   // Retarget the post-count upper-display fire across the full viewport as
   // well. Its old1200/1400ms end and later credit/fountain timing remain.
   fullCollectionSweep(e,postCountSweepMs()-feature_entry::fireDelayMs);break;
  case Phase::Outro:if(completedFreeBoard29())drawWisdomCompletion29();else{fill(0,0,126,14,native_title::background);rect(0,0,126,14,GOLD);text("FEATURE COMPLETED",63,3,8,BRIGHT,1);}break;
  default:break;}}
bool valuesAvailable201();
struct MenuEntry {int id;const char* label;};
constexpr MenuEntry menuEntries[]={ {0,"GAME RULES"},{9,"PAYLINES"},{3,"CREDITS"},{4,"COUNTER & JACKPOTS"},{10,"SETTINGS"},{7,"SIMULATOR RETURN"},{8,"RETURN TO GAME"} };
constexpr int kMenuItemCount=std::size(menuEntries);
// Retain logical return IDs used by the value editors and return selector.
void nextMenuItem(){for(unsigned row=0;row<kMenuItemCount;++row)if(menuEntries[row].id==menuItem){menuItem=menuEntries[(row+1)%kMenuItemCount].id;return;}menuItem=0;}
void drawMenu(){panel("GAME MENU");for(int row=0;row<kMenuItemCount;++row){const auto& item=menuEntries[row];const int y=41+row*9;const bool disabled=item.id==4&&!valuesAvailable201();if(item.id==menuItem)fill(35,y,170,9,0x2a86);text(item.label,42,y,9,disabled?0x7bef:item.id==menuItem?BRIGHT:WHITE);}text(*statusLine?statusLine:"GREY: NEXT   BLUE: SELECT",120,112,6,BRIGHT,1);text(build_version::menu,224,119,10,WHITE,2);}
void drawSettings(){panel("SETTINGS");const char* items[]={"SOUND","DISPLAY","BACK TO MENU"};for(int i=0;i<3;++i){const int y=50+i*18;if(settingsItem==i)fill(28,y-1,184,13,0x2a86);text(items[i],36,y,10,settingsItem==i?BRIGHT:WHITE);}text("GREY: NEXT   BLUE: SELECT",120,121,6,BRIGHT,1);}
unsigned paylinesLineCount(){
 if(!paylinesLayout)return engine.state.lineCount;
 unsigned position=1;for(auto count:phoenix::local::lineOptions)if(count!=engine.state.lineCount){if(position++==paylinesLayout)return count;}
 return engine.state.lineCount;
}
unsigned paylinesPageCount(){return (paylinesLineCount()+5)/6;}
void nextPaylinesLayout(){paylinesLayout=(paylinesLayout+1)%std::size(phoenix::local::lineOptions);paylinesPage=0;}
void drawPaylines(){
 panel("PAYLINES");const unsigned count=paylinesLineCount();char heading[48];snprintf(heading,sizeof(heading),"%u LINES%s  PAGE %u / %u",count,count==engine.state.lineCount?" CURRENT":"",paylinesPage+1,paylinesPageCount());text(heading,120,39,7,WHITE,1);
 for(unsigned slot=0;slot<6;++slot){const unsigned index=paylinesPage*6+slot;if(index>=count)break;
  const int x=12+int(slot%2)*110,y=50+int(slot/2)*22,gx=x+25;const auto& path=phoenix::local::linePath(index,count);const uint16_t color=lineMarkerColor(index,count);
  text(integer(index+1).c_str(),x+3,y+5,7,WHITE);rect(gx,y,75,18,0x4a69);for(int c=1;c<5;++c)line(gx+c*15,y,gx+c*15,y+17,0x2265);for(int row=1;row<3;++row)line(gx,y+row*6,gx+74,y+row*6,0x2265);
  for(int c=0;c<5;++c){const int px=gx+7+c*15,py=y+3+int(path[c])*6;if(c<4)line(px,py,px+15,y+3+int(path[c+1])*6,color);fill(px-1,py-1,3,3,color);}
 }
 text("GREY: PAGE  BLUE: LINES  HOLD: BACK",120,121,6,BRIGHT,1);
}
void drawMath(){
 using Page=return_selector86::Page;
 panel(returnSelector86.page==Page::Help?"ABOUT MODEL RETURN":"SIMULATOR RETURN");
 if(engine.hasCustomJackpots()){text("RETURN SETTING LOCKED",120,47,10,BRIGHT,1);text("RESTORE DEFAULT JACKPOTS",120,67,8,WHITE,1);text("COUNTER & JACKPOTS > RESTORE",120,86,7,WHITE,1);text("BLUE OR GREY: BACK",120,121,7,BRIGHT,1);return;}
 if(returnSelector86.page==Page::Locked){
  text("SETTING LOCKED",120,47,12,BRIGHT,1);
  text(!returnChoiceRecognized86()?"STORED SETTING NEEDS SERVICE":returnClosedBoundary86()?"CURRENT PROFILE UNAVAILABLE":"FINISH CURRENT PLAY FIRST",120,67,7,WHITE,1);
  text("GAME AND OWED PRIZES PRESERVED",120,83,7,WHITE,1);
  text("ORIGINAL CASINO RTP UNKNOWN",120,100,7,BRIGHT,1);
  text("BLUE OR GREY: BACK",120,121,7,WHITE,1);return;
 }
 if(returnSelector86.page==Page::Help){
  text("CHANGES FUTURE GRAND ONLY",120,44,7,BRIGHT,1);text("REELS / HITS / OTHER PAYS SAME",120,57,7,WHITE,1);
  text("KEEP BET AND OPTION FIXED",120,70,7,WHITE,1);text("FUNDED PLAY WITHOUT LUCKY",120,83,7,WHITE,1);
  text("AVERAGE OVER MANY ROUNDS",120,96,7,WHITE,1);text("ORIGINAL CASINO RTP UNKNOWN",120,109,7,BRIGHT,1);
  text("BLUE OR GREY: BACK",120,121,6,WHITE,1);return;
 }
 if(returnSelector86.page==Page::Detail){
  const auto&d=returnSelector86.detail;char heading[32],range[32],edge[32];
  snprintf(heading,sizeof(heading),"RETURN OPTION %u",unsigned(d.option)+1);panel(heading);
  return_selector86::range(d.low,d.high,range,sizeof(range));return_selector86::edge(d.low,d.high,edge,sizeof(edge));
  text("LONG-RUN MODEL RETURN",120,42,7,WHITE,1);text(range,120,52,12,BRIGHT,1);
  text("FIXED BET + OPTION / FUNDED / NO LUCKY",120,68,6,WHITE,1);
  text("MODEL HOUSE EDGE",120,81,7,WHITE,1);text(edge,120,90,10,BRIGHT,1);
  const auto grand=money(d.grand*d.profile.coinMinor);text(("FUTURE GRAND "+grand).c_str(),120,105,8,WHITE,1);
  text(returnSelector86.contextChanged?"VALUES CHANGED - REVIEW AGAIN":statusLine[0]?statusLine:"BLUE: APPLY   GREY: BACK",120,121,7,BRIGHT,1);return;
 }
 char profile[64];snprintf(profile,sizeof(profile),"%s / %u LINES / M%u / %s",money(engine.state.coinValue).c_str(),engine.state.lineCount,engine.state.betMultiplier,engine.state.prizeCapProfile==phoenix::PrizeCapProfile::HighCashOnly?"CASH CAP":"NAMED CAP");
 text(profile,120,39,6,WHITE,1);
 const auto selected=effectiveMathChoice();const int saved=selected.family!=static_grand::Fingerprint{}?return_selector86::optionFor(selected.slot):-1;
 for(unsigned i=0;i<8;++i){const int y=47+int(i)*8;const bool focused=returnSelector86.cursor==i;
  if(focused)fill(16,y-1,208,9,0x2a86);
  if(i<6){char label[24],range[32];snprintf(label,sizeof(label),"%u%s",i+1,i==return_selector86::defaultOption?" DEFAULT":"");
   if(int(i)==saved)text("*",18,y,8,BRIGHT);text(label,24,y,8,focused?BRIGHT:WHITE);
   return_selector86::Detail d;if(returnDetail86(i,d))return_selector86::range(d.low,d.high,range,sizeof(range));else snprintf(range,sizeof(range),"UNAVAILABLE");
   text(range,214,y,8,focused?BRIGHT:WHITE,2);
  }else text(i==6?"ABOUT MODEL RETURN":"BACK",24,y,8,focused?BRIGHT:WHITE);
 }
 text("* SESSION; LOCAL MODEL; CASINO RTP UNKNOWN",120,113,6,BRIGHT,1);text("GREY: NEXT  BLUE: SELECT  HOLD GREY: BACK",120,121,6,WHITE,1);
}
void drawService(){
 if(amountFailureShown190){fill(8,30,224,72,0x0000);text("AMOUNT DATA INVALID",120,48,10,WHITE,1);text("GAME STOPPED",120,70,8,WHITE,1);return;}
 if(!engine.randomHealthy()){panel("RANDOM SOURCE ERROR");text("PLAY STOPPED",120,49,10,BRIGHT,1);text("NO OUTCOME WAS COMMITTED",120,69,7,WHITE,1);text("RESTART FOR A NEW SESSION",120,93,7,WHITE,1);return;}
 panel(checkpointBlocked||storagePause!=UiCheckpointPause::None?"SESSION SETUP ERROR":"GAME CAPACITY LIMIT");
 text("PLAY STOPPED",120,49,10,BRIGHT,1);text("THIS SESSION IS IN MEMORY ONLY",120,70,7,WHITE,1);text("RESTART FOR A NEW SESSION",120,94,7,WHITE,1);
}

#include "cabinet_wager.inc"
#include "jackpot_board198.inc"
void drawBet(){
 const auto& coin=cabinetCoin();const auto& row=cabinetRow();
 cabinetAvailable=phase==Phase::Ready&&engine.canChangeBet()&&row.count;
 drawCabinetWager({cabinet_profile::denominations,cabinet_profile::denominationCount,coin.lines,coin.count,row.multipliers,row.count});
 if(!row.count||statusLine[0]){fill(5,117,173,16,BLACK);text(row.count?statusLine:"SELECT PLAY 5",8,121,8,BRIGHT,0,8,177);}
}

// Award units follow the current engine; no probability or price changes.
constexpr int kRulesPageCount=6;
void drawRules(){
 const char* titles[]={"HOW TO PLAY","FEATURES","BASE SYMBOL PAYS","FREE SYMBOL PAYS","WIN UNITS","PHOENIX AWARDS"};
 panel(titles[rulesPage]);
 if(rulesPage==0){
  const char* rows[]={"BLUE: SPIN / SELECT", "HOLD BLUE: AUTOPLAY ON / OFF", "DOUBLE GREY: JACKPOTS", "HOLD GREY: GAME MENU", "LINE WINS PAY FROM THE LEFT", "GREY: SELECT BET"};
  for(int i=0;i<6;++i)text(rows[i],40,48+i*9,8,WHITE);
 }else if(rulesPage==1){
  const char* rows[]={"BASE 3+ WISDOMS: 6 FREE GAMES", "TWIN REELS 1 + 2, AND 4 + 5", "6 PHOENIXES: HOLD & SPIN", "NEW PHOENIXES RESET 3 SPINS", "COLLECT PHOENIXES FOR FREE FEATURE", "PHOENIX CASH IS NOT A LINE PAY"};
  for(int i=0;i<6;++i)text(rows[i],39,48+i*9,7,WHITE);
 }else if(rulesPage==2||rulesPage==3){
  text("SYMBOL",40,45,7,BRIGHT);text("2",128,45,7,BRIGHT,2);text("3",153,45,7,BRIGHT,2);text("4",178,45,7,BRIGHT,2);text("5",203,45,7,BRIGHT,2);
  const char* names[]={"9","10","JACK","QUEEN","KING","BELL","ABACUS","BOOK","CANDLE","WILD"};
  const Symbol ids[]={Symbol::Nine,Symbol::Ten,Symbol::Jack,Symbol::Queen,Symbol::King,Symbol::Bell,Symbol::Abacus,Symbol::Book,Symbol::Candle,Symbol::Wild};
  for(int row=0;row<10;++row){int y=54+row*6;text(names[row],40,y,6,WHITE);for(int n=2;n<=5;++n)text(integer(phoenix::local::linePay(unsigned(ids[row]),n,engine.state.betMultiplier,rulesPage==3,engine.state.lineCount,engine.state.coinValue)).c_str(),128+(n-2)*25,y,6,WHITE,2);}
  text(("UNIT CREDITS X BET MULTIPLIER "+integer(engine.state.betMultiplier)).c_str(),120,115,6,BRIGHT,1);
 }else if(rulesPage==4){
  const char* rows[]={"LINE: UNIT CREDITS X BET MULTIPLIER", "WISDOM SCATTER: FACTOR X TOTAL BET", "SCATTERS PAY IN ANY POSITION", "TOTAL BET CREDITS = LINES X MULT", "MULT = SELECTED BET MULTIPLIER", "CASH = CREDITS X DENOMINATION"};
  for(int i=0;i<6;++i)text(rows[i],22,46+i*11,7,WHITE);
 }else{
  const char* rows[]={"PHOENIX: COLLECT SHOWN VALUES", "SHOWN CASH / CREDITS ARE THE PRIZE", "DO NOT MULTIPLY BY TOTAL BET", "NAMED 2X / 3X: MINI, MINOR, MAJOR", "15 PHOENIXES: GRAND PLUS PRIZES", "HOLD & SPIN HAS NO LINE PAYS"};
  for(int i=0;i<6;++i)text(rows[i],22,46+i*11,7,WHITE);
 }
 text("GREY: PAGE  BLUE/HOLD GREY: MENU",120,rulesPage>=2?122:112,7,BRIGHT,1);
}
bool clearBalanceAvailable(){
 const auto& s=engine.state;
 return initialized&&!runtimeFinancialBlocked()&&!uiReturnSelectionInputFenced()&&
  phase==Phase::Ready&&engine.canChangeBet()&&!s.freeSession&&!s.freeLeft&&!s.respinsLeft&&
  !s.pendingMeterHolds&&!pendingFree&&!pendingHold&&!pendingOutro&&!pendingChained&&
  !spinAward.pending&&!returnSelector86.pending&&!returnSelector86.recoveryFence;
}
#include "game_values_editor201.inc"
void drawReset(){
 if(creditsPanel201!=CreditsPanel201::Credits){drawValues201();return;}
 panel("CREDITS");
 const char* labels[]={"ADD $1,000","ADD $10,000","ADD $100,000","CLEAR BALANCE","AUTO REFILL","BACK TO MENU"};
 const bool available=phase==Phase::Ready&&engine.canChangeBet();
 for(int i=0;i<6;++i){const int y=43+i*10;if(i==creditItem)fill(16,y-1,208,9,0x2a86);const bool disabled=i<3?!available:i==3?!clearBalanceAvailable():false;text(labels[i],24,y,8,disabled?0x7bef:i==creditItem?BRIGHT:WHITE);if(i==4)text(autoRefill?"ON":"OFF",213,y,8,autoRefill?0x07e5:WHITE,2);}
 text(("BALANCE "+money(engine.state.balance)).c_str(),120,106,7,WHITE,1);
 text(available?("REFILL TO "+money(lastAddedMinor,true)).c_str():"FUNDING AVAILABLE AFTER FEATURE",120,115,6,BRIGHT,1);
 text("GREY: NEXT  BLUE: SELECT  HOLD GREY: BACK",120,122,6,WHITE,1);
}
bool soundMasterMuted(){
#ifndef UI_HOST_TEST
 return audioMuted();
#else
 return hostMuted;
#endif
}
void drawOptions(){
 panel("DISPLAY");
 const char* labels[]={"BRIGHTNESS","AMOUNT UNITS","FLIP SCREEN"};
 const std::string values[]={integer(M5.Display.getBrightness()),creditsDisplay?"CREDITS":"CASH",screenFlipped?"ON":"OFF"};
 for(int i=0;i<3;++i){const int y=48+i*18;if(i==optionItem)fill(17,y-2,206,15,0x2a86);text(labels[i],27,y,10,i==optionItem?BRIGHT:WHITE);text(values[i].c_str(),213,y,10,WHITE,2);}
 const char* hints[]={"BLUE: CHANGE BRIGHTNESS","BLUE: CASH / CREDITS","BLUE: FLIP 180 DEGREES"};
 text(hints[optionItem],120,108,8,BRIGHT,1);text("GREY: NEXT   HOLD GREY: BACK",120,121,7,WHITE,1);
}
void drawSound(){panel("SOUND");const char* labels[]={"MASTER SOUND","STOPS / LANDINGS","VOLUME","TEST FREE-GAMES INTRO","TEST HOLD MUSIC","TEST AWARD JINGLE"};for(int i=0;i<6;++i){int y=43+i*12;if(i==soundItem)fill(17,y-1,206,11,0x2a86);text(labels[i],25,y,10,i==soundItem?BRIGHT:WHITE);if(i<3){std::string value=i==0?(soundMasterMuted()?"OFF":"ON"):i==1?(landingSounds?"ON":"OFF"):integer((unsigned(soundVolume)*100+127)/255)+"%";text(value.c_str(),215,y,10,WHITE,2);}}
const char* footer="GREY: NEXT  BLUE: CHANGE  HOLD GREY: BACK";if(testJingleAt&&millis()-testJingleAt<4000)footer=testJingleQueued?(testAwardJingle?"AWARD JINGLE":"FREE-GAMES INTRO"):soundMasterMuted()?"TURN MASTER SOUND ON":"JINGLE PLAYBACK UNAVAILABLE";text(footer,120,121,7,BRIGHT,1);}
unsigned volumePercent(){return (unsigned(soundVolume)*100+127)/255;}
void adjustVolume(bool louder){
 const unsigned current=volumePercent();
 // Fine control in the quiet range, then five-percent steps. Both ends clamp
 // rather than wrapping a quiet setting back to full volume.
 const unsigned next=louder?std::min(100u,current<10?current+1:(current/5+1)*5)
                            :current==0?0:current<=10?current-1:((current-1)/5)*5;
 uiSetSoundPreferences(spinSounds,landingSounds,uint8_t((next*255+50)/100));
}
void auditionSound(bool music){
 testAwardJingle=false;testJingleAt=millis();
#ifndef UI_HOST_TEST
 testJingleQueued=music?audioTestFeatureMusic():audioTestJingle();
#else
 (void)music;testJingleQueued=!hostMuted;
#endif
 dirty=true;
}
void auditionAwardSound(){
 testAwardJingle=true;testJingleAt=millis();
#ifndef UI_HOST_TEST
 testJingleQueued=audioTestAwardMusic();
#else
 testJingleQueued=!hostMuted;
#endif
 dirty=true;
}
void drawVolume(){
 panel("VOLUME");const unsigned percent=volumePercent();
 text((integer(percent)+"%").c_str(),120,46,16,WHITE,1);
 rect(29,70,182,10,GOLD);fill(31,72,int(percent*178/100),6,BRIGHT);
 const char* status=soundMasterMuted()?"MASTER SOUND IS OFF":percent==0?"SILENT":"FINE CONTROL AT 0 - 10%";
 if(testJingleAt&&millis()-testJingleAt<4000)status=testJingleQueued?"TEST JINGLE":soundMasterMuted()?"MASTER SOUND IS OFF":"JINGLE UNAVAILABLE";
 text(status,120,87,7,BRIGHT,1);
 text("GREY: -   BLUE: +",120,102,8,WHITE,1);
 text("HOLD BLUE: TEST   HOLD GREY: BACK",120,120,7,BRIGHT,1);
}

void drawLucky(){panel("LUCKY CHANCE");text(("REMAINING  "+money(engine.state.balance)).c_str(),120,46,10,WHITE,1);text(("CURRENT BET  "+money(engine.betMinor())).c_str(),120,61,10,WHITE,1);overlay(pdfa::button(pdfa::Button::PressPhoenix),37,77);overlay(pdfa::button(pdfa::Button::ReturnToGame),128,77);text("BLUE: PLAY",74,114,8,BRIGHT,1);text("GREY: CANCEL",165,114,8,BRIGHT,1);}

bool opaqueSceneFire(uint32_t now){
 if(!effectsCacheReady||!effectsUseCache||dotClipTop!=0||dotClipBottom!=H)return false;
 const uint32_t elapsed=now-phaseAt;uint32_t age=0,duration=phoenix_fx::wipeDurationMs;
 switch(phase){
  case Phase::HoldWipe:{const uint32_t start=feature_entry::lead(holdEntryMeter)+feature_entry::fireDelayMs;
   if(elapsed<start)return false;age=elapsed-start;duration=feature_entry::fireMs;break;}
  case Phase::ReturnWipe:{const uint32_t start=collectionReturnLead()+feature_entry::fireDelayMs;
   if(elapsed<start)return false;age=elapsed-start;break;}
  case Phase::Revive:
   if(elapsed<revival_timing::fireAt||elapsed>=revival_timing::promptAt)return false;
   age=elapsed-revival_timing::fireAt;duration=feature_entry::fireMs;break;
  case Phase::Collect:
   if(collectionNamedDone||elapsed>=collectionLead())return false;
   if(largeCollectionPrelude()){
    if(elapsed<feature_award::largeBirdAt+collection_motion::fireDelay||collection_motion::goldOpacity(elapsed-feature_award::largeBirdAt))return false;
    age=elapsed-feature_award::largeBirdAt-collection_motion::fireDelay;duration=collection_motion::fireDuration;
   }else{
    const auto start=feature_award::ordinarySweepAt+feature_entry::fireDelayMs;
    if(elapsed<start)return false;age=elapsed-start;duration=feature_entry::fireMs;
   }break;
  case Phase::Jackpot:
   if(elapsed<feature_award::birdAt+collection_motion::fireDelay||elapsed>=feature_award::pickupAt||collection_motion::goldOpacity(elapsed-feature_award::birdAt))return false;
   age=elapsed-feature_award::birdAt-collection_motion::fireDelay;duration=collection_motion::fireDuration;break;
  case Phase::WinTransfer:
   if(elapsed<feature_entry::fireDelayMs)return false;
   age=elapsed-feature_entry::fireDelayMs;duration=postCountSweepMs()-feature_entry::fireDelayMs;break;
  default:return false;
 }
 const float t=float(age)/duration;
 if(age>=duration||t<.42f||t>=.65f)return false;
 // Only this opaque interval covers every earlier layer. Overlays drawn
 // AFTER the transition (coins and menus) still compose in their old order.
 memcpy(pixels,preparedEffects.fireAt(age),sizeof(pixels));return true;
}
// Diagnostic-only alternate pipeline. Normal rasterScene below stays byte-exact.
void drawSceneBackdrop103(){
 const auto& bg=pa::backdrop;
 const bool opaque=[](const pa::Image& image){return image.rgb&&!image.alpha&&!image.key;}(bg);
 if(phase==Phase::Spin||dotClipTop!=0||dotClipBottom!=H||!opaque||bg.w!=W||bg.h!=H||
    RW!=5*TW||RH!=3*TH||RY<14||RY+RH>122||RX<0||RX+RW>W||
    !pa::background.rgb||pa::background.alpha||pa::background.key||pa::background.w!=TW||pa::background.h!=TH||
    !pa::holdBackground.rgb||pa::holdBackground.alpha||pa::holdBackground.key||pa::holdBackground.w!=TW||pa::holdBackground.h!=TH||
    !pa::footer.rgb||pa::footer.alpha||pa::footer.key||pa::footer.w!=W||pa::footer.h!=H-122){image(bg,0,0);return;}
 for(int y=14;y<RY;++y)memcpy(pixels+y*W,bg.rgb+y*W,W*sizeof(uint16_t));
 for(int y=RY;y<RY+RH;++y){
  memcpy(pixels+y*W,bg.rgb+y*W,size_t(RX)*sizeof(uint16_t));
  memcpy(pixels+y*W+RX+RW,bg.rgb+y*W+RX+RW,size_t(W-RX-RW)*sizeof(uint16_t));
 }
 for(int y=RY+RH;y<122;++y)memcpy(pixels+y*W,bg.rgb+y*W,W*sizeof(uint16_t));
}
#if defined(UI_HOST_TEST) && defined(UI_BACKDROP_AB_TEST_HOOK)
enum class BackdropABTestStage115:uint8_t {Before,After};
void (*backdropABTestObserver115)(unsigned,bool,uint32_t,BackdropABTestStage115)=nullptr;
void* (*backdropABTestAlloc115)(size_t)=nullptr;
#endif
#if defined(UI_HOST_TEST) && defined(UI_MAJOR_BENCH_TEST_HOOK)
enum class MajorBenchStage119:uint8_t {Before,After};
void (*majorBenchObserver119)(unsigned,uint32_t,uint32_t,MajorBenchStage119)=nullptr;
#endif
void rasterSceneBackend103(uint32_t now,bool fastBackdrop){if(storagePause!=UiCheckpointPause::None){fill(0,0,W,H,native_title::background);drawService();return;}if(view==View::Jackpots){drawJackpots198(now);return;}if(!opaqueSceneFire(now)){if(fastBackdrop)drawSceneBackdrop103();else image(pa::backdrop,0,0);drawHeader(now);drawReels(now);drawFooter();drawTransition(now);}drawNestedFreeProgress138();if(fountainActive&&view==View::Game)coinShower(now-fountainAt);if(view==View::Menu)drawMenu();else if(view==View::Bet)drawBet();else if(view==View::Rules)drawRules();else if(view==View::Paylines)drawPaylines();else if(view==View::Settings)drawSettings();else if(view==View::Options)drawOptions();else if(view==View::Reset)drawReset();else if(view==View::Lucky)drawLucky();else if(view==View::Sound)drawSound();else if(view==View::Volume)drawVolume();else if(view==View::Math)drawMath();else if(view==View::Service)drawService();if(storagePause!=UiCheckpointPause::None)drawService();if(*statusLine&&storagePause==UiCheckpointPause::None&&view==View::Game&&phase==Phase::Ready){fill(33,109,174,11,0x0000,225);text(statusLine,120,111,7,BRIGHT,1);}}
void rasterScene(uint32_t now){if(storagePause!=UiCheckpointPause::None){fill(0,0,W,H,native_title::background);drawService();return;}if(view==View::Jackpots){drawJackpots198(now);return;}if(!opaqueSceneFire(now)){image(pa::backdrop,0,0);drawHeader(now);drawReels(now);drawFooter();drawTransition(now);}drawNestedFreeProgress138();if(fountainActive&&view==View::Game)coinShower(now-fountainAt);if(view==View::Menu)drawMenu();else if(view==View::Bet)drawBet();else if(view==View::Rules)drawRules();else if(view==View::Paylines)drawPaylines();else if(view==View::Settings)drawSettings();else if(view==View::Options)drawOptions();else if(view==View::Reset)drawReset();else if(view==View::Lucky)drawLucky();else if(view==View::Sound)drawSound();else if(view==View::Volume)drawVolume();else if(view==View::Math)drawMath();else if(view==View::Service)drawService();if(storagePause!=UiCheckpointPause::None)drawService();if(*statusLine&&storagePause==UiCheckpointPause::None&&view==View::Game&&phase==Phase::Ready){fill(33,109,174,11,0x0000,225);text(statusLine,120,111,7,BRIGHT,1);}}
#include "reference_effect_v0258.h"
void render(uint32_t now){uint32_t begin=micros();rasterScene(now);M5.Display.pushImage(0,0,W,H,pixels);lastDraw=now;dirty=false;lastDrawUs=micros()-begin;maxDrawUs=std::max(maxDrawUs,lastDrawUs);++frameCount;}
void ready(){if(phase!=Phase::Land&&phase!=Phase::Expand&&phase!=Phase::FreeCounter&&phase!=Phase::SpinAwardSettle)lineClockValid=false;phase=Phase::Ready;readyAt=millis();displayBank=engine.state.roundOpen?engine.state.balance-engine.state.roundPayout:engine.state.balance;winFrom=displayWin;winTo=engine.state.roundPayout;winAt=millis();autoAt=millis()+(spinAward.finished?spin_award::readyDelayMs:engine.presentation.actionWinMinor?2600:650);dirty=true;}
void revealHold(){if(introChained)holdBackdropCaptured=false;holdView=true;copy(visible,introChained?engine.presentation.nextHeldResult:engine.presentation.heldResult);copy(target,visible);holdWipeRevealed=true;}
void enterHoldIntro(bool chained){displayQueuedHolds=engine.state.pendingMeterHolds;introChained=chained;holdWipeRevealed=false;holdEntryMeter=!chained&&engine.presentation.meterTriggered;cueFeature(false,chained||engine.presentation.holdFromMeter);startPhase(Phase::HoldWipe,feature_entry::duration(holdEntryMeter));}
#include "twin_free_scope89.inc"
void updateFeatureSounds(uint32_t now){
 if(amountFailureShown190){requestFeatureMusic(FeatureMusicPart::Off);return;}
 // Retire the lease once, so an idle device cannot revive an old phrase
 // when the32-bit millisecond counter completes another whole cycle.
 if(fountainAwardMusic&&now-fountainMusicAt>=spin_award::jingleMs)fountainAwardMusic=false;
 const uint32_t elapsed=now-phaseAt;
 constexpr uint32_t completionMs=uint32_t((uint64_t(feature_music::collectedEndFrame-feature_music::collectedBeginFrame)*1000+44099)/44100);
 const bool revived=phase==Phase::Revive,collected=phase==Phase::Collect||phase==Phase::AwardIntro||phase==Phase::WinTransfer||phase==Phase::ReturnWipe;
 const bool cache=completionReady();
 const bool terminalHoldBed=holdView&&engine.presentation.holdSettled&&!engine.presentation.grandAwarded;
 requestCompletionCue(cache&&!awaitingBonusResume&&((phase==Phase::Collect&&!collectionNamedDone)||phase==Phase::AwardIntro||revived)&&elapsed<completionMs,elapsed,terminalHoldBed);
 const bool endingLand=phase==Phase::Land&&(engine.presentation.revived||engine.presentation.holdSettled||engine.presentation.grandAwarded);
 const bool hold=phase==Phase::HoldWipe||phase==Phase::HoldIntro||(holdView&&(phase==Phase::Spin||phase==Phase::Land||(phase==Phase::Ready&&engine.state.mode==Mode::Hold)));
 FeatureMusicPart part=FeatureMusicPart::Off;bool continuousAward=false;
 // Source66: terminal Hold music continues to the completion handoff.
 // Revival and direct Grand retain their existing routing; no phase changes.
 if(!awaitingBonusResume&&(!endingLand||terminalHoldBed)){
  if(phase==Phase::Jackpot&&elapsed>=feature_award::musicAt)part=FeatureMusicPart::MajorAward;
  else if(phase==Phase::WinTransfer&&collectionFountain()&&elapsed>=feature_award::bigMusicAt)part=FeatureMusicPart::BigAward;
  else if(fountainContinuous&&fountainAwardMusic&&now-fountainMusicAt<spin_award::jingleMs){part=FeatureMusicPart::BigAward;continuousAward=true;}
  else if(revived)part=elapsed>=revival_timing::musicAt?FeatureMusicPart::Hold:cache?FeatureMusicPart::Off:FeatureMusicPart::Collected;
  else if(twinFreeReturnMusic89(now))part=FeatureMusicPart::TwinFree;
  else if(collected)part=cache?FeatureMusicPart::Off:FeatureMusicPart::Collected;
  else if(hold)part=FeatureMusicPart::Hold;
  else if(twinFreeMusicWanted89(now))part=FeatureMusicPart::TwinFree;
 }
 // An inherited phrase is not a new award event for the current trace.
 if(!continuousAward||int32_t(fountainMusicAt-lastAward.beganAt)>=0)traceAwardCue(part,elapsed,now);
 requestFeatureMusic(part);
}
void enterCollect(bool afterAward=false){lastCollectSound=-1;winFrom=winTo=displayWin=0;collectionNamedDone=afterAward;
 startPhase(Phase::Collect,collectionLead()+collection_steps101::duration(visible,kCollectStep,kCollectRepeatStep)+collectionTail());
 if(afterAward)phaseAt-=kCollectLead-feature_award::cashLeadAfterAward;
 updateFeatureSounds(millis());}
void beginCollect(){collectionNamedDone=false;beginAwardTrace();
 if(engine.presentation.grandAwarded||namedAward())startPhase(Phase::AwardIntro,feature_award::introMs);else enterCollect();
 updateFeatureSounds(millis());
}
void afterCollection(){if(pendingChained){pendingChained=false;introChained=true;displayQueuedHolds=engine.state.pendingMeterHolds;if(!holdWipeRevealed)revealHold();cueFeature(false,true);startPhase(Phase::HoldIntro,1000);}else if(pendingFree){pendingFree=false;cueFeature(true);startPhase(Phase::FreeIntro,featureIntroDuration(true));}else if(pendingOutro){pendingOutro=false;startPhase(Phase::Outro,feature_entry::freeCompletionMs);}else {
 const bool resumeFree=engine.state.mode==Mode::Free&&engine.state.freeSession&&
                       engine.presentation.holdSettled&&!pendingHold;
 ready();
 // The observed nested Hold returns directly into the next Free reel motion.
 // Keep the source-fitted return-fire lead, without the old generic win pause.
 if(resumeFree)autoAt=readyAt+60;
}}
void beginSpinAward(){
 spinAward.pending=false;spinAward.shown=0;winFrom=winTo=displayWin=spinAward.before;
 lastAward=AwardTrace{};lastAward.valid=true;lastAward.action=spinAward.action;
 lastAward.cash=spinAward.amount;lastAward.bet=spinAward.bet;lastAward.fountainEligible=spinAward.withCoins;lastAward.beganAt=millis();
 captureAwardContext(false);
#ifndef UI_HOST_TEST
 lastAward.backendStartsBefore=audioMusicDiagnostics().awardStarts;
#endif
 startPhase(Phase::SpinAward,UINT32_MAX);const uint32_t now=millis();spinAward.visualAt=phaseAt;
 if(spinAward.withCoins){
  // Coins and the approved87 phrase have separate evidence. Preserve the
  // existing50x music gate; lower newly observed visual fountains do not
  // silently inherit an unverified song. Existing ordinary win audio remains.
  const bool phrase=feature_award::cashFountain(spinAward.amount,spinAward.bet);
  if(phrase){fountainAwardMusic=true;fountainMusicAt=phaseAt;}
  startAwardFountain(phaseAt,now,phrase?FeatureMusicPart::BigAward:FeatureMusicPart::Off,phrase);
  fountainContinuous=true;fountainStopMs=UINT32_MAX;
 }
 updateFeatureSounds(now);
}
void finishSpinAward(){
 spinAward.shown=spinAward.amount;spinAward.finished=true;
 displayWin=winFrom=winTo=spinAward.before+spinAward.amount;
 if(spinAward.withCoins)fountainStopMs=uint32_t(millis()-fountainAt)+spin_award::emissionAfterCountMs;
 startPhase(Phase::SpinAwardSettle,spin_award::settleMs);
}
const wisdom_win88::Profile* wisdomWinProfile88(){
 const auto& p=engine.presentation;
 if(creditsDisplay||phase!=Phase::FreeIntro||wisdomEntry30.stage!=wisdom_ctrl30::Stage::Announcement||
    !p.valid||!p.hasSpinResult||!p.paidSpin||p.freeSpin||!p.freeAwarded||
    p.freeAwardedCount!=6||p.scatterCount!=3||p.phoenixCount||p.expandedWild||
    p.holdEntered||p.holdSettled||p.grandAwarded||p.revived||p.meterTriggered||
    p.pendingHoldsBefore||p.pendingHoldsAfter||p.chainedHoldEntered||
    pendingHold||spinAward.pending||engine.state.mode==Mode::Hold||
    p.lineWinMinor||p.phoenixWinMinor||winFrom!=0)return nullptr;
 const auto* profile=wisdom_win88::sourceProfile(engine.state.coinValue,engine.state.lineCount,engine.state.betMultiplier);
 return profile&&p.scatterWinMinor==profile->awardMinor&&
        p.actionWinMinor==profile->awardMinor&&engine.state.roundPayout==profile->awardMinor&&
        winTo==profile->awardMinor?profile:nullptr;
}
void beginLandCount(){
 winFrom=displayWin;
 winTo=(engine.presentation.holdSettled||engine.presentation.grandAwarded)?engine.state.roundPayout-engine.presentation.phoenixWinMinor:engine.state.roundPayout;
 if(spinAward.pending)winFrom=winTo=displayWin=spinAward.before;
 winAt=millis();
}
bool initialWisdomAward30(){
 const auto& p=engine.presentation;
 return p.valid&&p.hasSpinResult&&p.paidSpin&&!p.freeSpin&&p.freeAwarded&&p.freeAwardedCount==6&&p.scatterCount==3&&
        pendingFree&&!pendingHold&&!p.holdEntered&&!p.holdSettled&&!p.grandAwarded&&!p.revived&&engine.state.mode!=Mode::Hold&&!spinAward.pending;
}
bool playWisdomStart87(uint32_t now){
 if(wisdomEntry30.cueStarted)return true;
#if !defined(UI_HOST_TEST) || defined(UI_WISDOM_BACKEND_TEST)
 if(!audioPlay(AudioCue::WisdomThreeStart))return false;
#endif
 wisdomEntry30.cueStarted=true;wisdomEntry30.cueAt=now;return true;
}
void updateWisdomStart87(uint32_t now){
 if(phase!=Phase::Spin||!initialWisdomAward30()||wisdomEntry30.cueStarted)return;
 // This chosen reference cut maps to~130ms after the final R5 impact.
 // A slow frame or deliberate quick-stop starts the finite phrase once now.
 const uint32_t cut=phaseAt+reel_motion::impactMs(reelMs[4])+130;
 if(int32_t(now-cut)>=0)playWisdomStart87(now);
}
uint32_t remainingWisdomStart87(uint32_t now){
 uint32_t duration=7120;
#if !defined(UI_HOST_TEST) || defined(UI_WISDOM_BACKEND_TEST)
 duration=audioCueDurationMs(AudioCue::WisdomThreeStart);
#endif
 const uint32_t elapsed=now-wisdomEntry30.cueAt;
 return elapsed<duration?duration-elapsed:1;
}
bool beginInitialWisdom30(){
 if(!initialWisdomAward30())return false;
 wisdomEntry30.stage=wisdom_ctrl30::Stage::Announcement;
 wisdomEntry30.awarded=engine.presentation.freeAwardedCount;wisdomEntry30.triggerAction=engine.presentation.actionNumber;
 pendingFree=false;
 const auto now=uint32_t(millis());
 // Preserve a phrase already launched in final recoil; never stack a second
 // legacy FreeStart over it. Missing/muted output still has a bounded intro.
 const bool began=playWisdomStart87(now);
 startPhase(Phase::FreeIntro,began?remainingWisdomStart87(now):featureIntroDuration(true));return true;
}
void acceptInitialWisdom30(){
 const uint32_t now=millis();
 // This commits only the already-earned DISPLAY count; the Engine bank,
 // award, freeLeft, meter and RNG were committed by the original paid spin.
 displayWin=winFrom=winTo=engine.state.roundPayout;winAt=now;
 wisdomEntry30.stage=wisdom_ctrl30::Stage::Accepted;wisdomEntry30.acceptedAt=now;
 awaitingBonusResume=false; // START explicitly resumes an owed feature after a checkpoint pause.
 requestFeatureMusic(FeatureMusicPart::TwinFree); // accepted START precedes the500ms first-strip preparation.
 ready();autoAt=now+wisdom_ctrl30::preparationMs;dirty=true;
}
void afterLand(){
  const auto& p=engine.presentation;
  if(p.revived){startPhase(Phase::Revive,revival_timing::duration);updateFeatureSounds(millis());return;}
  if(pendingHold){
    if(spinAward.pending){beginSpinAward();return;}
    // The source WIN has finished its line count before the counter ribbon
    // starts. Let the current count finish; never snap it or run it under the
    // large transition. This changes presentation time only.
    const uint32_t counted=millis()-winAt;
    if(winTo!=winFrom&&counted<1600){startPhase(Phase::Land,1600-counted);return;}
    displayWin=winFrom=winTo;pendingHold=false;enterHoldIntro(false);return;
  }
  if(p.holdSettled||p.grandAwarded){beginCollect();return;}
  if(pendingFree&&engine.state.mode!=Mode::Hold){if(beginInitialWisdom30())return;pendingFree=false;cueFeature(true);startPhase(Phase::FreeIntro,featureIntroDuration(true));return;}
  if(spinAward.pending){beginSpinAward();return;}
  if(pendingOutro){
    const uint32_t counted=millis()-winAt;
    if(winTo!=winFrom&&counted<1600){startPhase(Phase::Land,1600-counted);return;}
    displayWin=winFrom=winTo;pendingOutro=false;startPhase(Phase::Outro,feature_entry::freeCompletionMs);return;
  }
  ready();
}
void finishPhase(){switch(phase){
  case Phase::Spin:lineDisplayAt=millis();lineClockValid=true;copy(visible,target);displayMeter=engine.presentation.meterAfter;displayQueuedHolds=engine.presentation.pendingHoldsAfter+(engine.presentation.chainedHoldEntered?1:0);stopReelSound();winCuePending=ordinaryWinAudio();if(engine.presentation.expandedWild){startPhase(Phase::Expand,2000);break;}displayBank=engine.state.roundOpen?engine.state.balance-engine.state.roundPayout:displayBank;beginLandCount();if(beginInitialWisdom30())break;startPhase(Phase::Land,engine.presentation.actionWinMinor?1000:engine.presentation.freeEnded?feature_entry::freeLastMissLandMs:400);break;
  case Phase::Land:afterLand();break;
  case Phase::Revive:startPhase(Phase::HoldIntro,1);break;
  case Phase::Expand:copy(target,engine.presentation.spinResult);copy(visible,target);displayBank=engine.state.roundOpen?engine.state.balance-engine.state.roundPayout:displayBank;beginLandCount();if(beginInitialWisdom30())break;startPhase(Phase::Land,700);break;
  case Phase::HoldWipe:if(!holdWipeRevealed)revealHold();startPhase(Phase::HoldIntro,std::max(1u,featureIntroDuration(false)-std::min(featureIntroDuration(false)-1,feature_entry::duration(holdEntryMeter))));break;
  case Phase::HoldIntro:holdView=true;heldMask=phoenix::Engine::phoenixMask(visible);if(engine.presentation.holdSettled&&!introChained)beginCollect();else ready();break;
  case Phase::AwardIntro:startPhase(Phase::Jackpot,feature_award::durationMs);break;
  case Phase::Jackpot:enterCollect(true);break;
  case Phase::FreeIntro:
   if(wisdomEntry30.stage==wisdom_ctrl30::Stage::Announcement){acceptInitialWisdom30();break;}
   if(freeRetriggerIntro()&&featureAudioBusy())break;
   if(freeRetriggerIntro()&&pendingFreeCount)startPhase(Phase::FreeCounter,pendingFreeCount*spin_award::counterTickMs);
   else if(spinAward.pending)beginSpinAward();else ready();break;
  case Phase::FreeCounter:freeIntroIsRetrigger=false;if(spinAward.pending)beginSpinAward();else ready();break;
  case Phase::SpinAward:finishSpinAward();break;
  case Phase::SpinAwardSettle:afterLand();break;
  case Phase::Collect:displayWin=engine.state.roundPayout-engine.presentation.phoenixWinMinor;winFrom=winTo=displayWin;
   startPhase(Phase::WinTransfer,collectionTransferDuration());break;
  case Phase::WinTransfer:if(ordinaryCollectionTransfer())displayWin=engine.state.roundPayout;winFrom=winTo=displayWin;
   if(pendingChained){introChained=true;holdWipeRevealed=false;}
   startPhase(Phase::ReturnWipe,collectionReturnLead()+feature_entry::returnDurationMs);break;
  case Phase::ReturnWipe:if(lowerCashCollectionFountain149())fountainActive=false;displayWin=winFrom=winTo=engine.state.roundPayout;afterCollection();break;
  case Phase::Outro:
// The old2.4s procedural FreeEnd score has no qualified match across the
// independently checked plain completions. Keep its bank/ID for historical
// compatibility, without automatically appending an invented ending jingle.
ready();break;
  default:ready();break;}}
void prepareStrips(){
 // Public deterministic cosmetic salt only. Financial draws never use this stream.
 uint32_t seed=uint32_t(engine.state.actions*0x9e3779b9u)^0x12345678;
 const bool twin=engine.presentation.twinSpin;
 for(int c=0;c<5;++c){
  // Native Twin stops are left pair, centre, right pair,900ms apart.
  // Use the same450ms underlying column spacing for ordinary five reels.
  const int group=twin?(c==1?0:c==3?4:c):c;
  reelMs[c]=uint32_t((twin?3100:1100)+group*450);
  reelSteps[c]=reel_motion::steps(reelMs[c],TH);
  for(unsigned j=0;j<kReelStripCells;++j){
   Cell cell;cell.symbol=Symbol(cosmetic(seed)%unsigned(Symbol::Count));
   if(cell.symbol==Symbol::Wild&&(c==0||(twin&&c==1)))cell.symbol=Symbol::Book;
   if(phoenixCell(cell))cell.prize=engine.state.betCoins*phoenix::local::prizeValues[cosmetic(seed)%(sizeof(phoenix::local::prizeValues)/sizeof(phoenix::local::prizeValues[0]))];
   strips[c][j]=cell;
  }
  for(unsigned j=4;j+2<kReelStripCells;++j)if(strips[c][j].symbol==Symbol::Wild){for(unsigned k=1;k<3;++k){strips[c][j+k]=Cell{};strips[c][j+k].symbol=Symbol::Wild;}j+=2;}
  if(twin&&(c==1||c==4)){
   memcpy(strips[c],strips[c-1],sizeof(strips[c]));
   // Twin reels link symbol types; the original passing Phoenix values can
   // differ too. Keep this local cosmetic RNG separate from financial RNG.
   for(auto& cell:strips[c])if(phoenixCell(cell))cell.prize=engine.state.betCoins*phoenix::local::prizeValues[cosmetic(seed)%(sizeof(phoenix::local::prizeValues)/sizeof(phoenix::local::prizeValues[0]))];
  }
  for(int r=0;r<3;++r){strips[c][r]=target[c*3+r];strips[c][reelSteps[c]+r]=previous[c*3+r];}
 }
}
void presentAction(){
  const auto& entryResult30=engine.presentation;
  const bool firstFree30=wisdomEntry30.stage==wisdom_ctrl30::Stage::Accepted&&entryResult30.valid&&entryResult30.freeSpin&&entryResult30.actionNumber==wisdomEntry30.triggerAction+1;
  if(!firstFree30)wisdomEntry30={};
  spinLandingCoalesced=false;
  lineClockValid=false;
  awaitingBonusResume=false;
  winCuePending=false;
  const auto& result=engine.presentation;
  spinAward=SpinAward{};
  const auto awardStyle=spin_award::style(result.actionWinMinor,engine.betMinor(),result.freeSpin);
  if(result.valid&&result.hasSpinResult&&!result.holdSettled&&!result.grandAwarded&&
     awardStyle!=spin_award::Style::None){
   spinAward.pending=true;spinAward.withCoins=awardStyle==spin_award::Style::Fountain;
   spinAward.action=result.actionNumber;spinAward.amount=result.actionWinMinor;
   spinAward.before=engine.state.roundPayout-result.actionWinMinor;spinAward.bet=engine.betMinor();
  }
  if(result.luckyAttempt&&!result.luckyWon){displayBank=engine.state.balance;displayWin=winFrom=winTo=0;phase=Phase::Ready;view=View::Game;autoPlay=false;snprintf(statusLine,sizeof(statusLine),"LUCKY CHANCE: NO WIN");dirty=true;return;}
const auto& p=engine.presentation;displayMeter=p.meterBefore;displayQueuedHolds=p.pendingHoldsBefore;displayRespins=p.holdRespin?std::max(0,int(p.respinsBefore)-1):int(p.respinsAfter);pendingFree=pendingFree||p.freeAwarded;if(p.freeAwarded){pendingFreeCount=p.freeAwardedCount;freeIntroIsRetrigger=p.freeSpin;freeIntroOldTotal=p.freeSpin?uint16_t(engine.state.freePlayed+p.freeBefore-1):0;}pendingHold=p.holdEntered;pendingOutro=p.freeEnded;pendingChained=p.chainedHoldEntered;heldMask=p.holdRespin?p.heldMask:0;landedMask=p.landedMask;displayBank=p.paidSpin?p.balanceBefore-p.stakeMinor:p.balanceBefore-(engine.state.roundPayout-p.actionWinMinor);if(p.paidSpin)displayWin=0;winFrom=winTo=displayWin;
  if(p.hasSpinResult)copy(target,p.expandedWild?p.beforeExpansion:p.spinResult);else if(p.hasHeldResult)copy(target,p.heldResult);else copy(target,engine.state.cells);
  // beforeExpansion was captured before awarded-cell flags were computed.
  // Preserve its symbols, but carry the committed win flags so actual paid
  // Sage joins animate instead of remaining blue until the phase ends.
  if(p.hasSpinResult&&p.expandedWild)for(unsigned i=0;i<15;++i)target[i].highlight=p.spinResult[i].highlight;
  if(p.hasSpinResult)holdBackdropCaptured=false;
  if(p.holdRespin)copy(previous,p.heldBefore);if(p.holdEntered&&p.hasSpinResult){copy(holdBase,p.spinResult);holdBackdropCaptured=true;}
  prepareStrips();
  if(holdView){
    holdStripStartsBlank=!holdBackdropCaptured;
    // Two independent native-rate Hold sequences show a much wider stop
    // stagger than the base game's170ms. This500ms camera-fitted cadence is
    // presentation only, not a recovered vendor timer or weighted reel.
    for(int c=0;c<5;++c)reelMs[c]=holdReelDuration(c);
    uint32_t seed=uint32_t(engine.state.actions*0x85ebca6bu)^0x484f4c44u; // Cosmetic passing symbols only.
    for(int i=0;i<15;++i){
      const Cell initial=phoenixCell(previous[i])||!holdBackdropCaptured?previous[i]:holdBase[i];
      for(auto& cell:holdStrips[i]){
        cell=Cell{};cell.symbol=cosmetic(seed)%4==0?Symbol::Phoenix:Symbol(cosmetic(seed)%unsigned(Symbol::Phoenix));
        if(phoenixCell(cell))cell.prize=engine.state.betCoins*phoenix::local::prizeValues[cosmetic(seed)%(sizeof(phoenix::local::prizeValues)/sizeof(phoenix::local::prizeValues[0]))];
      }
      Cell landed=target[i];
      if(!phoenixCell(landed)){landed=Cell{};landed.symbol=Symbol(cosmetic(seed)%unsigned(Symbol::Phoenix));}
      holdStrips[i][0]=landed;holdStrips[i][holdReelSteps(i/3)]=initial;
      // The existing200-byte, game-bound scene sidecar keeps these cosmetic
      // ordinary results across reboot without changing any engine bytes.
      holdBase[i]=landed;
    }
    holdBackdropCaptured=true;
  }
  spinQuickStopped=false;lastArrived=-1;startReelSound();startPhase(Phase::Spin,reelMs[4]);
  if(firstFree30){wisdomEntry30.stage=wisdom_ctrl30::Stage::FirstSpin;wisdomEntry30.motionAt=phaseAt;}
}
bool refillIfNeeded(){
  if(runtimeFinancialBlocked()||!autoRefill||phase!=Phase::Ready||view!=View::Game||!engine.canChangeBet()||engine.state.balance>=engine.betMinor()||!customWagerUsable())return false;
  const int64_t target=std::max<int64_t>(lastAddedMinor,engine.betMinor());
  if(!engine.addCredits(target-engine.state.balance))return false;
  ++creditRevision;displayBank=engine.state.balance;dirty=true;cueCredit();return true;
}
void offerLucky(){if(runtimeFinancialBlocked()||!referenceLocalMath())return;autoPlay=false;view=View::Lucky;luckyOfferAction=engine.state.actions;dirty=true;}
bool outcomeCheckpointOwned59(){
 if(outcomeCheckpointHandler)return true;
 uiBlockCheckpoint();return false; // wiring fault: service, not a retry claim
}
bool checkpointPresentedOutcome59(){
 // No frame is rendered here. The RAM session owner acknowledges the already
 // committed Engine result; no durable-storage claim is made.
 const uint32_t wallAt=uiWallMillis(),gameAt=wallAt-uiClockOffset;
 const bool durable=outcomeCheckpointHandler&&outcomeCheckpointHandler();
 if(storagePause==UiCheckpointPause::None)uiClockOffset+=uint32_t(uiWallMillis()-wallAt);
 else{
  // Existing storage pause owns time after its start. Freeze only the prior
  // synchronous write interval here, without double-counting retry time.
  uiClockOffset+=uint32_t(uiPauseWallAt-wallAt);uiPauseGameAt=gameAt;
 }
 if(!durable&&storagePause==UiCheckpointPause::None)uiSetCheckpointPause(UiCheckpointPause::UndurableRecord);
 return durable;
}
void applyReturnSelection86(){
 if(returnSelector86.page!=return_selector86::Page::Detail||!returnClosedBoundary86()){returnSelector86.page=return_selector86::Page::Locked;dirty=true;return;}
 return_selector86::Detail current;
 if(!returnDetail86(returnSelector86.detail.option,current)){returnSelector86.page=return_selector86::Page::Locked;dirty=true;return;}
 if(!return_selector86::same(current,returnSelector86.detail)){
  returnSelector86.detail=current;returnSelector86.contextChanged=true;dirty=true;return;
 }
 // Missing ownership never changes financial bytes or a retained raw intent.
 if(!outcomeCheckpointHandler){snprintf(statusLine,sizeof(statusLine),"STORAGE OWNER REQUIRED");dirty=true;return;}
 const auto requested=static_grand::retained(*mathFamily,return_selector86::optionToSlot[current.option]);
 uint8_t normalized[static_grand::selectionBytes];
 if(static_grand::encode(requested,normalized,sizeof(normalized))!=sizeof(normalized))return;
 const bool changed=engine.state.jackpotUnits[3]!=phoenix::money::Unit::Credits||engine.state.jackpotCoins[3]!=current.grand||std::memcmp(normalized,mathPreference,sizeof(mathPreference));
 if(changed){
  auto active=effectiveMathChoice();
  const auto result=static_grand::selectFuture(engine,active,requested,*mathFamily,returnClosedBoundary86());
  if(result!=static_grand::Result::Applied&&result!=static_grand::Result::NoChange)return;
  mathSelection=static_grand::current(active);mathStoredModel=phoenix::local::modelId;
  std::memcpy(mathPreference,normalized,sizeof(mathPreference));++runtimeRevision;
 }
 returnSelector86.pending=returnSelector86.recoveryFence=true;autoPlay=false;dirty=true;
 // Main owns the exact game/scene/raw-preference/service tuple and immutable
 // failure retry. No optimistic exit or rollback over an ambiguous commit.
 const bool durable=checkpointPresentedOutcome59();
 if(durable&&!runtimeFinancialBlocked()&&returnSelector86.pending)finishReturnSelection86();
}

void enterAmountFault190(){
 if(!amountFailureShown190){autoPlay=false;winCuePending=false;stopReelSound();requestFeatureMusic(FeatureMusicPart::Off);audioStop();amountFailureShown190=true;}
 view=View::Service;statusLine[0]=0;dirty=true;
}
void enterRandomFault125(){
 if(!randomFailureShown125){autoPlay=false;winCuePending=false;stopReelSound();requestFeatureMusic(FeatureMusicPart::Off);audioStop();randomFailureShown125=true;}
 view=View::Service;statusLine[0]=0;dirty=true;
}
void action(){if(!initialized||runtimeFinancialBlocked()||phase!=Phase::Ready||view!=View::Game)return;
  if(!outcomeCheckpointOwned59())return;
  if(wisdomEntry30.stage==wisdom_ctrl30::Stage::Accepted)awaitingBonusResume=false;
  if(wisdom_ctrl30::preparing(wisdomEntry30,millis())){autoAt=wisdomEntry30.acceptedAt+wisdom_ctrl30::preparationMs;return;}
  if(engine.state.mode==Mode::Hold){
   if(holdOpening31.stage==hold_opening31::Stage::Resume){
    // Restored Hold stays silent until an explicit play/resume request.
    awaitingBonusResume=false;armHoldOpening();requestFeatureMusic(FeatureMusicPart::Hold);dirty=true;
   }
   if(holdOpening31.blocked(millis(),holdOpeningAudio()))return;
  }
  statusLine[0]=0;
  if(!prepareCustomRound())return;
  refillIfNeeded();
  if(engine.canLuckyChance()){if(!referenceLocalMath()){autoPlay=false;snprintf(statusLine,sizeof(statusLine),"ADD CREDITS FOR CUSTOM PLAY");dirty=true;}else offerLucky();return;}
  if(engine.state.mode==Mode::Free)requestFeatureMusic(FeatureMusicPart::TwinFree);
  copy(previous,visible);const bool wasHold=engine.state.mode==Mode::Hold;
  if(!engine.action()){if(!engine.randomHealthy()){enterRandomFault125();return;}noteCapacityFailure(phoenix_capacity::Attempt::Normal);if(engine.capacityFaultMask()&phoenix::CapacityAmount){enterAmountFault190();return;}autoPlay=false;snprintf(statusLine,sizeof(statusLine),"NOT ENOUGH CREDITS - HOLD GREY FOR MENU");dirty=true;return;}
  if(wasHold)holdOpening31.clear();
  holdView=wasHold;
  presentAction();
  checkpointPresentedOutcome59();
}
void playLucky(){if(runtimeFinancialBlocked()||!referenceLocalMath()||!outcomeCheckpointOwned59())return;copy(previous,visible);view=View::Game;statusLine[0]=0;if(!engine.tryLuckyChance()){if(!engine.randomHealthy()){enterRandomFault125();return;}noteCapacityFailure(phoenix_capacity::Attempt::Lucky);if(engine.capacityFaultMask()&phoenix::CapacityAmount)enterAmountFault190();dirty=true;return;}if(engine.presentation.luckyWon)holdView=false;presentAction();checkpointPresentedOutcome59();}

void openJackpotBoard198(){
 jackpotBoardRequested198=false;cabinetMenuRequested=false;autoPlay=false;
 if(engine.state.mode!=Mode::Base)awaitingBonusResume=true;
 statusLine[0]=0;view=View::Jackpots;dirty=true;
}
void closeJackpotBoard198(){
 jackpotBoardRequested198=false;cabinetMenuRequested=false;autoPlay=false;
 if(engine.state.mode!=Mode::Base)awaitingBonusResume=true;
 view=View::Game;dirty=true;
}
void openCabinetWager(){
 jackpotBoardRequested198=false;cabinetMenuRequested=false;cabinetDenom=engine.state.coinValue;cabinetLines=engine.state.lineCount;cabinetFocus=0;
 if(!cabinet_profile::find(cabinetDenom))cabinetDenom=cabinet_profile::denominations[0];
 const auto& coin=cabinetCoin();if(cabinet_profile::lineIndex(coin,cabinetLines)==coin.count)cabinetLines=coin.lines[0];
 for(unsigned i=0;i<cabinet_profile::denominationCount;++i)if(cabinet_profile::coins[i].minor==cabinetDenom)cabinetFocus=i;cabinetAvailable=phase==Phase::Ready&&engine.canChangeBet()&&cabinetRow().count;
 if(engine.state.mode!=Mode::Base)awaitingBonusResume=true;
 statusLine[0]=0;view=View::Bet;dirty=true;
}
void chooseMenu(){cueButton();switch(menuItem){
 case 0:rulesPage=0;view=View::Rules;break;
 
 
 case 3:creditItem=0;creditsPanel201=CreditsPanel201::Credits;statusLine[0]=0;view=View::Reset;break;
 case 4:if(valuesAvailable201()){creditsPanel201=CreditsPanel201::GameValues;valuesItem201=0;statusLine[0]=0;view=View::Reset;}else valuesNotice201("FINISH CURRENT PLAY FIRST");break;
 
 
 case 7:openReturnSelector86();break;
 case 8:view=View::Game;break;
 case 9:paylinesPage=paylinesLayout=0;statusLine[0]=0;view=View::Paylines;break;
 case 10:settingsItem=0;view=View::Settings;break;
 }dirty=true;}

}

void uiSetOutcomeCheckpointHandler(UiOutcomeCheckpointHandler handler){outcomeCheckpointHandler=handler;}
bool uiInitFresh(phoenix::random::Service& source){amountFailureShown190=false;randomFailureShown125=false;returnSelector86={};outcomeCheckpointHandler=nullptr;wisdomEntry30={};holdOpening31.clear();cabinetMenuRequested=jackpotBoardRequested198=false;storagePause=UiCheckpointPause::None;uiClockOffset=uiPauseWallAt=uiPauseGameAt=0;initializeMathPreference();runtimeRevision=0;capacityService.clear();checkpointBlocked=false;lastAward=AwardTrace{};collectionNamedDone=fountainActive=false;fountainAt=0;clearSpinAward();phoenixClockAt=0;memset(phoenixPhases,0,sizeof(phoenixPhases));phoenixPhasesRestored=false;spinQuickStopped=spinLandingCoalesced=false;lineClockValid=false;paylinesPage=paylinesLayout=0;settingsItem=0;uiSetScreenFlipped(false);entry_art::init();collect_art::init();phoenix_art::init();awaitingBonusResume=false;winCuePending=false;autoRefill=false;lastAddedMinor=1000000;creditRevision=0;creditItem=0;creditsPanel201=CreditsPanel201::Credits;valuesItem201=jackpotItem201=editFocus201=0;editedValue201=0;holdBackdropCaptured=false;holdStripStartsBlank=false;landingSounds=true;soundVolume=192;hostMuted=false;testJingleAt=0;testAwardJingle=false;creditsDisplay=false;luckyOfferAction=UINT64_MAX;const bool randomReady125=engine.initializeFresh(source,1000);copy(visible,engine.state.cells);copy(target,visible);copy(previous,visible);copy(holdBase,visible);displayBank=engine.state.balance;displayMeter=engine.state.meter;displayQueuedHolds=engine.state.pendingMeterHolds;displayWin=0;phase=Phase::Ready;view=View::Game;autoPlay=false;holdView=false;pendingFree=pendingHold=pendingOutro=pendingChained=introChained=false;pendingFreeCount=0;freeIntroIsRetrigger=false;freeIntroOldTotal=0;initialized=true;dirty=true;readyAt=millis();M5.Display.setSwapBytes(true);
 fire_alpha::init();
 initHoldMaterial();
#ifndef UI_HOST_TEST
  audioRegisterReferenceCues();
#endif
 if(randomReady125)prepareCustomRound(); // Own the default future-Grand tuple at the fresh closed boundary.
 if(!randomReady125){autoPlay=false;view=View::Service;dirty=true;}
 return randomReady125;
}
bool uiRandomHealthy(){return initialized&&engine.randomHealthy();}
uint8_t uiRandomFault(){return uint8_t(engine.randomFault());}
bool uiPrepareEffects(){preparePickupSamples();const bool ready=prepareEffectFrames();if(!coin_sampler::info().ready){const uint32_t begin=micros();coin_sampler::prepare();coinMapsBuildUs=micros()-begin;}return ready;}
UiEffectCache uiEffectCache(){return {effectsCacheReady,effectsCacheReady?uint32_t(effect_frames::bytes):0,effect_frames::fireCount,effect_frames::birdCount,effect_frames::rate,effectsCacheBuildUs,fire_alpha::verified,fire_alpha::verificationChecks,fire_alpha::data::boundaryFNV,pickupSamples.ready(),pickupSamples.ready()?uint32_t(pickup_frames::bytes):0,pickupSamples.hitCount(),pickupSamples.missCount()};}
UiEntryBench uiBenchmarkEntry(){
 UiEntryBench result;
 if(runtimeFinancialBlocked())return result;
 if(!initialized||view!=View::Game||phase!=Phase::Ready||autoPlay||fountainActive||engine.state.mode!=Mode::Base||engine.state.freeSession||engine.state.roundOpen||featureAudioBusy()||!entry_art::ready()||!collect_art::ready())return result;
#ifdef UI_HOST_TEST
 auto* shown=static_cast<uint16_t*>(std::malloc(sizeof(pixels)));
#else
 auto* shown=static_cast<uint16_t*>(ps_malloc(sizeof(pixels)));
#endif
 if(!shown)return result;
#ifdef UI_HOST_TEST
 auto* reference=static_cast<uint16_t*>(std::malloc(sizeof(pixels)));
#else
 auto* reference=static_cast<uint16_t*>(ps_malloc(sizeof(pixels)));
#endif
 if(!reference){std::free(shown);return result;}
 struct HoldRenderBackup {
  Cell target[15],previous[15],backdrop[15],strips[15][kHoldStripCells];
  uint32_t reelMs[5];uint16_t heldMask;int stopped;bool holdView,captured,startsBlank,quickStopped,landingCoalesced;
 };
#ifdef UI_HOST_TEST
 void* savedMemory=std::malloc(sizeof(HoldRenderBackup));
#else
 void* savedMemory=ps_malloc(sizeof(HoldRenderBackup));
#endif
 if(!savedMemory){std::free(reference);std::free(shown);return result;}
 auto* held=new(savedMemory)HoldRenderBackup;
 memcpy(held->target,target,sizeof(target));memcpy(held->previous,previous,sizeof(previous));
 memcpy(held->backdrop,holdBase,sizeof(holdBase));memcpy(held->strips,holdStrips,sizeof(holdStrips));
 memcpy(held->reelMs,reelMs,sizeof(reelMs));held->heldMask=heldMask;held->holdView=holdView;
 held->captured=holdBackdropCaptured;held->startsBlank=holdStripStartsBlank;held->stopped=lastArrived;held->quickStopped=spinQuickStopped;held->landingCoalesced=spinLandingCoalesced;
 memcpy(shown,pixels,sizeof(pixels));Cell kept[15];memcpy(kept,visible,sizeof(visible));
 const auto priorPhase=phase;const uint32_t priorAt=phaseAt;const bool priorMeter=holdEntryMeter;
 const int priorClipTop=dotClipTop,priorClipBottom=dotClipBottom;
 const bool priorGrandAwarded119=engine.presentation.grandAwarded;
 const bool priorNamed=collectionNamedDone,priorFountain=fountainActive,priorContinuous=fountainContinuous,priorFountainMusic=fountainAwardMusic,priorRetrigger=freeIntroIsRetrigger;const uint32_t priorFountainAt=fountainAt,priorStop=fountainStopMs,priorMusicAt=fountainMusicAt;const uint16_t priorFreeTotal=freeIntroOldTotal;const auto priorSpinAward=spinAward;const auto priorHoldOpening=holdOpening31;const auto priorWisdom=wisdomEntry30;const auto priorReturnSelector=returnSelector86;
 auto engineHash=[](){const auto* b=reinterpret_cast<const uint8_t*>(&engine);uint32_t h=2166136261u;for(size_t i=0;i<sizeof(engine);++i)h=(h^b[i])*16777619u;return h;};
 auto rendererHash=[](){uint32_t h=2166136261u;auto add=[&](const void* p,size_t n){const auto* b=static_cast<const uint8_t*>(p);while(n--)h=(h^*b++)*16777619u;};
  add(visible,sizeof(visible));add(target,sizeof(target));add(previous,sizeof(previous));add(holdBase,sizeof(holdBase));
  add(holdStrips,sizeof(holdStrips));add(reelMs,sizeof(reelMs));add(&heldMask,sizeof(heldMask));add(&holdView,sizeof(holdView));
  add(&holdBackdropCaptured,sizeof(holdBackdropCaptured));add(&holdStripStartsBlank,sizeof(holdStripStartsBlank));
  add(phoenixPhases,sizeof(phoenixPhases));add(&phoenixClockAt,sizeof(phoenixClockAt));add(&lastArrived,sizeof(lastArrived));add(&spinQuickStopped,sizeof(spinQuickStopped));add(&spinLandingCoalesced,sizeof(spinLandingCoalesced));
  add(&phase,sizeof(phase));add(&phaseAt,sizeof(phaseAt));add(&holdEntryMeter,sizeof(holdEntryMeter));
  add(&collectionNamedDone,sizeof(collectionNamedDone));add(&fountainActive,sizeof(fountainActive));add(&fountainAt,sizeof(fountainAt));add(&fountainContinuous,sizeof(fountainContinuous));add(&fountainAwardMusic,sizeof(fountainAwardMusic));add(&fountainMusicAt,sizeof(fountainMusicAt));add(&fountainStopMs,sizeof(fountainStopMs));add(&spinAward,sizeof(spinAward));add(&freeIntroIsRetrigger,sizeof(freeIntroIsRetrigger));add(&freeIntroOldTotal,sizeof(freeIntroOldTotal));add(&holdOpening31,sizeof(holdOpening31));add(&wisdomEntry30,sizeof(wisdomEntry30));
  const uint32_t coinHash=coin_sampler::verifyHash();add(&coinHash,sizeof(coinHash));add(&coinMapsUse,sizeof(coinMapsUse));
  if(holdMaterialPixels)add(holdMaterialPixels,kHoldMaterialPixels*2);return h;};
 result.beforeHash=engineHash();
 result.rendererBeforeHash=rendererHash();
 collectionNamedDone=false;fountainActive=false;fountainAt=0;clearSpinAward();freeIntroIsRetrigger=false;freeIntroOldTotal=0;
 auto compare=[&](unsigned effect,unsigned elapsed){
  memcpy(pixels,shown,sizeof(pixels));uint32_t start=micros();
  if(effect)referenceCollectEmblem38(elapsed);else referenceEntryBird38(effect_frames::sample(elapsed));
  result.parityLegacyUs+=micros()-start;memcpy(reference,pixels,sizeof(pixels));
  memcpy(pixels,shown,sizeof(pixels));start=micros();
  if(effect)collectEmblem(elapsed);else entryBird(elapsed);
  result.parityOptimizedUs+=micros()-start;
  // The legacy oracle intentionally lacks full-screen coverage. Compare
  // its original reel interior at the explicit30Hz pose clock; verify all
  // new cache pixels against the uncached full-screen renderer below.
  for(unsigned pixel=0;pixel<W*H;++pixel)if((effect||(pixel/W>=RY&&pixel/W<RY+RH))&&pixels[pixel]!=reference[pixel]){
   ++result.parityMismatchPixels;
   if(result.mismatchEffect<0){result.mismatchEffect=int(effect);result.mismatchMs=int(elapsed);result.mismatchAt=int(pixel);}
  }
  ++result.parityFrames;
  // Keep the actual display unchanged; allow the same panel transfer/yield
  // used by the timing fixtures. No artificial game state reaches the LCD.
  M5.Display.pushImage(0,0,W,H,shown);
 };
 for(unsigned elapsed:{0u,20u,37u,80u,120u,159u,160u,200u,240u,280u,320u,360u,400u,440u,441u})compare(0,elapsed);
 for(unsigned elapsed=0;elapsed<=2300;elapsed+=100)compare(1,elapsed);
 for(unsigned elapsed:{1401u,1599u,2299u})compare(1,elapsed);
 if(effectsCacheReady){
  const bool previousCacheUse=effectsUseCache;
  auto cacheCompare=[&](bool fire,unsigned elapsed,unsigned duration){
   memcpy(pixels,shown,sizeof(pixels));effectsUseCache=false;uint32_t begin=micros();
   if(fire)referenceFireWipe58(elapsed,duration);else entryBird(elapsed);
   result.cacheReferenceUs+=micros()-begin;memcpy(reference,pixels,sizeof(pixels));
   memcpy(pixels,shown,sizeof(pixels));effectsUseCache=true;begin=micros();
   if(fire)fireWipe(elapsed,duration);else entryBird(elapsed);
   result.cachePreparedUs+=micros()-begin;
   for(unsigned i=0;i<W*H;++i)result.cacheParityMismatchPixels+=pixels[i]!=reference[i];
   ++result.cacheParityFrames;M5.Display.pushImage(0,0,W,H,shown);
  };
  for(unsigned elapsed:{0u,37u,80u,120u,159u,240u,360u,440u})cacheCompare(false,elapsed,0);
  for(unsigned duration:{780u,1260u,1400u})for(unsigned elapsed:{1u,117u,321u,660u,779u})cacheCompare(true,elapsed,duration);
  effectsUseCache=previousCacheUse;
 }
 // Native65gold-burn oracle versus the bounded scanline renderer. The real
 // LCD remains unchanged; fixture pixels never reach the panel or game.
 for(unsigned progress:{1u,17u,33u,49u,65u,81u,97u,113u,129u,145u,161u,177u,193u,209u,241u,255u})for(unsigned placement=0;placement<2;++placement){
  const int x=placement?-33:56,y=placement?-25:18,top=placement?0:16,bottom=placement?135:122;
  const unsigned opacity=placement?128:255;
  auto pattern=[](){for(unsigned i=0;i<W*H;++i)pixels[i]=uint16_t((i*701u)^(i/uint32_t(W)*9871u));};
  pattern();uint32_t begin=micros();burningGoldReference65(winged_phoenix::hero(),x,y,opacity,top,bottom,progress);result.goldReferenceUs+=micros()-begin;memcpy(reference,pixels,sizeof(pixels));
  pattern();begin=micros();burningGold(winged_phoenix::hero(),x,y,opacity,top,bottom,progress);result.goldPreparedUs+=micros()-begin;
  for(unsigned i=0;i<W*H;++i)result.goldParityMismatchPixels+=pixels[i]!=reference[i];
  ++result.goldParityFrames;if(!(result.goldParityFrames%4))M5.Display.pushImage(0,0,W,H,shown);
 }
 // All60 checked atlas poses: registry rows versus identical copied images,
 // which intentionally use the original full-alpha scan. Include viewport
 // clipping and existing background pixels; only the saved LCD is sent.
 if(phoenix_art::alphaReady())for(unsigned frame=0;frame<60;++frame){
  const auto& art=phoenix_art::frameIndex(frame);const pa::Image fullScan=art;
  const int x=frame%3==0?-11:frame%3==1?198:92,y=frame%2?-7:103;
  auto pattern=[](){for(unsigned i=0;i<W*H;++i)pixels[i]=uint16_t((i*701u)^(i/uint32_t(W)*9871u));};
  pattern();uint32_t begin=micros();image(fullScan,x,y,0,0,W,H);result.alphaReferenceUs+=micros()-begin;memcpy(reference,pixels,sizeof(pixels));
  pattern();begin=micros();image(art,x,y,0,0,W,H);result.alphaPreparedUs+=micros()-begin;
  for(unsigned i=0;i<W*H;++i)result.alphaParityMismatchPixels+=pixels[i]!=reference[i];
  ++result.alphaParityFrames;if(!(result.alphaParityFrames%8))M5.Display.pushImage(0,0,W,H,shown);
 }
 // Compare the current arrival pose with and without exact-ms memoization.
 // Frozen67 incoming-flight identity is checked separately on the host;
 // post-impact orientation deliberately changed to the recorded flare role.
 if(pickupSamples.ready())for(unsigned placement=0;placement<3;++placement){
  const float sx=kReelLeft+(placement*2+.5f)*TW,sy=RY+(placement+.5f)*TH;
  for(unsigned elapsed:{0u,40u,100u,240u,480u,699u,700u,740u,900u,1080u,1159u,1160u}){
   auto pattern=[](){for(unsigned i=0;i<W*H;++i)pixels[i]=uint16_t((i*701u)^(i/uint32_t(W)*9871u));};
   const bool priorPickup=pickupSamplesUse;pickupSamplesUse=false;
   pattern();uint32_t begin=micros();drawPickupFlight(elapsed,sx,sy,148,7);result.pickupReferenceUs+=micros()-begin;memcpy(reference,pixels,sizeof(pixels));
   pickupSamplesUse=true;pattern();begin=micros();drawPickupFlight(elapsed,sx,sy,148,7);result.pickupPreparedUs+=micros()-begin;pickupSamplesUse=priorPickup;
   for(unsigned i=0;i<W*H;++i)result.pickupParityMismatchPixels+=pixels[i]!=reference[i];
   ++result.pickupParityFrames;if(!(result.pickupParityFrames%4))M5.Display.pushImage(0,0,W,H,shown);
  }
 }
 // Compare the old sampler with all16coin art poses, six sizes and mixed
 //rolls/edge clips on this target's floating-point implementation. No test
 //coin pixels reach the LCD; only the unchanged original frame is resent.
 if(coin_sampler::info().ready){
  const unsigned sizes[]={16,17,21,23,29,30},opacity[]={255,128,1,254,73,240};
  const int xs[]={120,0,239,120,120,0},ys[]={67,64,64,0,134,134};
  auto pattern=[](){for(unsigned i=0;i<W*H;++i)pixels[i]=uint16_t((i*701u)^(i/uint32_t(W)*9871u));};
  for(unsigned frame=0;frame<16;++frame)for(unsigned sample=0;sample<6;++sample){
   const coin_motion::Particle p={true,xs[sample],ys[sample],frame,sizes[sample],opacity[sample],(frame*11+sample*7)%32};
   pattern();uint32_t begin=micros();drawCoinParticleReference62(p);result.coinReferenceUs+=micros()-begin;memcpy(reference,pixels,sizeof(pixels));
   pattern();begin=micros();drawCoinParticle(p);result.coinPreparedUs+=micros()-begin;
   for(unsigned i=0;i<W*H;++i)result.coinParityMismatchPixels+=pixels[i]!=reference[i];
   ++result.coinParityFrames;if(!(result.coinParityFrames%8))M5.Display.pushImage(0,0,W,H,shown);
  }
 }
 // Check the optimization on the target too, including two partially
 // visible tiles. No fixture pixels reach the physical LCD.
 phase=Phase::Spin;holdView=true;
 for(unsigned id=0;id<kHoldMaterialCount;++id)for(unsigned placement=0;placement<4;++placement){
  Cell cell;cell.symbol=Symbol(id);const int x=kReelLeft+int(placement)*TW;
  const int y=RY+TH+(placement==1?-17:placement==2?17:0),top=std::max(RY+TH,y),bottom=std::min(RY+2*TH,y+TH);
  memcpy(pixels,shown,sizeof(pixels));uint32_t begin=micros();drawHoldOrdinaryReference(cell,x,y,top,bottom);
  result.holdParityLegacyUs+=micros()-begin;memcpy(reference,pixels,sizeof(pixels));
  memcpy(pixels,shown,sizeof(pixels));begin=micros();drawHoldCell(cell,x,y,0,RY+TH,RY+2*TH);
  result.holdParityOptimizedUs+=micros()-begin;
  for(unsigned pixel=0;pixel<W*H;++pixel)if(pixels[pixel]!=reference[pixel])++result.holdParityMismatchPixels;
  ++result.holdParityFrames;M5.Display.pushImage(0,0,W,H,shown);
 }
 phase=priorPhase;holdView=held->holdView;
 if(effectsCacheReady){
  for(unsigned effect=0;effect<3;++effect)for(unsigned elapsed:{600u,649u,650u,700u,900u,938u}){
   phase=effect==0?Phase::HoldWipe:effect==1?Phase::ReturnWipe:Phase::Revive;
   holdEntryMeter=false;const uint32_t now=millis();phaseAt=now-elapsed-(effect==2?revival_timing::fireAt:0);
   uint32_t begin=micros();referenceRasterScene58(now);result.sceneReferenceUs+=micros()-begin;memcpy(reference,pixels,sizeof(pixels));
   begin=micros();rasterScene(now);result.sceneOptimizedUs+=micros()-begin;
   for(unsigned i=0;i<W*H;++i)result.sceneParityMismatchPixels+=pixels[i]!=reference[i];
   ++result.sceneParityFrames;M5.Display.pushImage(0,0,W,H,shown);
  }
 }
 std::free(reference);holdEntryMeter=false;phase=Phase::HoldWipe;
 for(unsigned fixture=0;fixture<4;++fixture){
  phase=fixture>=2?Phase::Collect:Phase::HoldWipe;
  if(fixture)for(unsigned i=0;i<15;++i){visible[i]=Cell{};visible[i].symbol=Symbol::Phoenix;visible[i].prize=engine.state.betCoins*(i+1);}
  for(unsigned elapsed=0;elapsed<feature_entry::duration(false);elapsed+=40){
   const uint32_t now=millis();phaseAt=now-elapsed-(fixture==3?kCollectedIntro+900:0);const uint32_t begin=micros();rasterScene(now);
   // Include the real panel transfer cost without displaying fabricated
   // results or feature imagery: resend precisely the original LCD pixels.
   M5.Display.pushImage(0,0,W,H,shown);const uint32_t used=micros()-begin;
   ++result.frames;result.totalUs+=used;result.maxUs=std::max(result.maxUs,used);
   auto& timing=result.fixtures[fixture];++timing.frames;timing.totalUs+=used;if(used>timing.maxUs){timing.maxUs=used;timing.maxElapsedMs=elapsed+(fixture==3?kCollectedIntro+900:0);}
  }
 }
 // Exercise Hold mini reels on the actual target CPU. The saved LCD pixels are the ONLY pixels transferred
 // to the panel. No action, audio or financial fixture is ever submitted.
 auto holdSpinFixture=[&](unsigned fixture,uint16_t mask){
  holdView=true;holdBackdropCaptured=true;holdStripStartsBlank=false;phase=Phase::Spin;
  lastArrived=-1;spinQuickStopped=spinLandingCoalesced=false;fountainActive=false;heldMask=mask;
  for(unsigned c=0;c<5;++c)reelMs[c]=holdReelDuration(c);
  for(unsigned i=0;i<15;++i){
   for(unsigned j=0;j<kHoldStripCells;++j){Cell cell;cell.symbol=Symbol((i+j)%unsigned(Symbol::Count));if(phoenixCell(cell))cell.prize=engine.state.betCoins*(j+1);holdStrips[i][j]=cell;}
   target[i]=holdStrips[i][0];holdBase[i]=target[i];previous[i]=holdStrips[i][holdReelSteps(i/3)];visible[i]=target[i];
   if(heldMask&(1u<<i)){previous[i]=Cell{};previous[i].symbol=Symbol::Phoenix;previous[i].prize=engine.state.betCoins*(i+1);}
  }
  for(unsigned sample=0;sample<51;++sample){
   const uint32_t now=millis();phaseAt=now-sample*66;const uint32_t begin=micros();rasterScene(now);
   M5.Display.pushImage(0,0,W,H,shown);const uint32_t used=micros()-begin;
   ++result.frames;result.totalUs+=used;result.maxUs=std::max(result.maxUs,used);
   auto& timing=result.fixtures[fixture];++timing.frames;timing.totalUs+=used;if(used>timing.maxUs){timing.maxUs=used;timing.maxElapsedMs=sample*66;}
  }
 };
 // Preserve the original0/6-held fixture positions and scene inputs.
 holdSpinFixture(4,0);holdSpinFixture(5,0x3fu);
 // Exercise the new revival renderer on the target without requesting a
 // jingle, advancing a spin or sending any synthetic pixels to the LCD.
 phase=Phase::Revive;
 for(unsigned elapsed:{0u,1000u,2300u,3949u,3950u,4671u,6580u,6700u,7100u,7700u,7959u,7960u}){
  const uint32_t now=millis();phaseAt=now-elapsed;const uint32_t begin=micros();rasterScene(now);
  M5.Display.pushImage(0,0,W,H,shown);const uint32_t used=micros()-begin;
  ++result.frames;result.totalUs+=used;result.maxUs=std::max(result.maxUs,used);
  auto& timing=result.fixtures[6];++timing.frames;timing.totalUs+=used;if(used>timing.maxUs){timing.maxUs=used;timing.maxElapsedMs=elapsed;}
 }
 // The old fourth fixture now takes the small-win director. Keep it intact
 // for comparison, and explicitly cover the distinct large-win material.
 for(unsigned i=0;i<15;++i){visible[i]=Cell{};visible[i].symbol=Symbol::Phoenix;visible[i].prize=engine.state.betCoins*500;}
 phase=Phase::Collect;holdView=true;
 for(unsigned sample=0;sample<35;++sample){
  const unsigned elapsed=feature_award::largeBirdAt+900+sample*40;const uint32_t now=millis();phaseAt=now-elapsed;
  const uint32_t begin=micros();rasterScene(now);M5.Display.pushImage(0,0,W,H,shown);const uint32_t used=micros()-begin;
  ++result.frames;result.totalUs+=used;result.maxUs=std::max(result.maxUs,used);
  auto& timing=result.fixtures[7];++timing.frames;timing.totalUs+=used;if(used>timing.maxUs){timing.maxUs=used;timing.maxElapsedMs=elapsed;}
 }
 fountainActive=true;
 for(unsigned sample=0;sample<200;++sample){
  const unsigned age=sample*40,elapsed=feature_award::bigFountainAt+age;const uint32_t now=millis();
  if(elapsed<feature_award::bigHoldMs){phase=Phase::WinTransfer;phaseAt=now-elapsed;}
  else if(elapsed<feature_award::bigHoldMs+feature_entry::returnDurationMs){phase=Phase::ReturnWipe;phaseAt=now-(elapsed-feature_award::bigHoldMs);}
  else {phase=Phase::Ready;phaseAt=now;holdView=false;}
  fountainAt=now-age;const uint32_t begin=micros();rasterScene(now);M5.Display.pushImage(0,0,W,H,shown);const uint32_t used=micros()-begin;
  ++result.frames;result.totalUs+=used;result.maxUs=std::max(result.maxUs,used);
  auto& timing=result.fixtures[8];++timing.frames;timing.totalUs+=used;if(used>timing.maxUs){timing.maxUs=used;timing.maxElapsedMs=age;}
 }
 fountainActive=false;phase=Phase::Collect;holdView=true;
 for(unsigned sample=0;sample<35;++sample){
  const unsigned elapsed=collectionLead()+sample*40;const uint32_t now=millis();phaseAt=now-elapsed;
  const uint32_t begin=micros();rasterScene(now);M5.Display.pushImage(0,0,W,H,shown);const uint32_t used=micros()-begin;
  ++result.frames;result.totalUs+=used;result.maxUs=std::max(result.maxUs,used);
  auto& timing=result.fixtures[9];++timing.frames;timing.totalUs+=used;if(used>timing.maxUs){timing.maxUs=used;timing.maxElapsedMs=sample*40;}
 }
 // Append nearly-full Hold coverage after all ten original fixtures. The
 // original sequence/timers remain intact. Vacancies are renderer-only:
 // index1/7/13 span left/centre/right; index7 is the sole centre vacancy.
 constexpr uint16_t twelveHeld=uint16_t(0x7fffu^(1u<<1)^(1u<<7)^(1u<<13));
 constexpr uint16_t fourteenHeld=uint16_t(0x7fffu^(1u<<7));
 holdSpinFixture(10,twelveHeld);holdSpinFixture(11,fourteenHeld);
 // Decode every Book78 logical record on the target. Retain the original
 // twelve fixtures, then measure one/three/fifteen animated Book placements.
 // Only the saved real LCD is transferred; no game, RNG or audio action runs.
 phase=Phase::Ready;holdView=false;fountainActive=false;
 for(auto& cell:visible){cell=Cell{};cell.symbol=Symbol::Book;}
 for(unsigned fixture=12;fixture<15;++fixture){
  const unsigned placements=fixture==12?1:fixture==13?3:15;
  for(unsigned record=0;record<103;++record){
   const unsigned slot=record?record-1:0;
   const unsigned clock=slot<30?(slot*1000+23)/24:3000+((slot-30)*3000+71)/72;
   const uint32_t now=millis(),begin=micros();rasterScene(now);
   const auto& art=pa::symbol(unsigned(Symbol::Book),clock,record!=0);
   if(art.w!=47||art.h!=35||!art.rgb||!art.alpha)++result.bookInvalidRecords;
   else {
    if(fixture==12){
     ++result.bookRecords;
     for(unsigned pixel=0;pixel<1645;++pixel){
      result.bookFNV=(result.bookFNV^uint8_t(art.rgb[pixel]))*16777619u;
      result.bookFNV=(result.bookFNV^uint8_t(art.rgb[pixel]>>8))*16777619u;
      result.bookFNV=(result.bookFNV^art.alpha[pixel])*16777619u;
      ++result.bookPixelPairs;
     }
    }
    for(unsigned i=0;i<placements;++i){const int x=kReelLeft+int(i/3)*TW,y=RY+int(i%3)*TH;
     image(pa::background,x,y,kReelLeft,RY,kReelLeft+RW,RY+RH);
     image(art,x+(TW-int(art.w))/2,y+(TH-int(art.h))/2,kReelLeft,RY,kReelLeft+RW,RY+RH);
    }
   }
   M5.Display.pushImage(0,0,W,H,shown);const uint32_t used=micros()-begin;
   ++result.frames;result.totalUs+=used;result.maxUs=std::max(result.maxUs,used);
   auto& timing=result.fixtures[fixture];++timing.frames;timing.totalUs+=used;
   if(used>timing.maxUs){timing.maxUs=used;timing.maxElapsedMs=clock;}
  }
 }
 // Append same-cell Triple Mini pickup overlap after the original935
 // frames. Only rasterize and resend the saved real LCD; no action, sound,
 // payout, checkpoint or synthetic result is submitted to the game owner.
 fountainActive=false;collectionNamedDone=false;phase=Phase::Collect;holdView=true;
 for(unsigned i=0;i<15;++i){visible[i]=Cell{};visible[i].symbol=Symbol::Phoenix;visible[i].prize=engine.state.betCoins*(i+1);}
 visible[0].kind=phoenix::PrizeKind::Mini;visible[0].prize=1000;visible[0].multiplier=3;
 for(unsigned sample=0;sample<80;++sample){
  const unsigned elapsed=collectionLead()+sample*40;const uint32_t now=millis();phaseAt=now-elapsed;
  const uint32_t begin=micros();rasterScene(now);M5.Display.pushImage(0,0,W,H,shown);const uint32_t used=micros()-begin;
  ++result.frames;result.totalUs+=used;result.maxUs=std::max(result.maxUs,used);
  auto& timing=result.fixtures[15];++timing.frames;timing.totalUs+=used;
  if(used>timing.maxUs){timing.maxUs=used;timing.maxElapsedMs=sample*40;}
 }
 // Append80paired measurements after the unchanged1015fixtures and their
 //1171saved-panel transfers. Only these diagnostic calls can select fastBackdrop.
 // Each pair freezes one scene/now and restarts both paths from the saved RAM.
 uint16_t* backdropReference=nullptr;
#if defined(UI_HOST_TEST) && defined(UI_BACKDROP_AB_TEST_HOOK)
 if(backdropABTestAlloc115)backdropReference=static_cast<uint16_t*>(backdropABTestAlloc115(sizeof(pixels)));
 else backdropReference=static_cast<uint16_t*>(std::malloc(sizeof(pixels)));
#elif defined(UI_HOST_TEST)
 backdropReference=static_cast<uint16_t*>(std::malloc(sizeof(pixels)));
#else
 backdropReference=static_cast<uint16_t*>(ps_malloc(sizeof(pixels)));
#endif
 if(!backdropReference)result.backdropAB.oom=true;
 else {
  auto& ab=result.backdropAB;
  for(unsigned sample=0;sample<80;++sample){
   const uint32_t now=millis();const unsigned elapsed=collectionLead()+sample*40;phaseAt=now-elapsed;
   auto pass=[&](bool fast){
    memcpy(pixels,shown,sizeof(pixels));dotClipTop=0;dotClipBottom=H;
#if defined(UI_HOST_TEST) && defined(UI_BACKDROP_AB_TEST_HOOK)
    if(backdropABTestObserver115)backdropABTestObserver115(sample,fast,now,BackdropABTestStage115::Before);
#endif
    const uint32_t begin=micros();rasterSceneBackend103(now,fast);const uint32_t used=micros()-begin;
    if(fast){ab.fastTotalUs+=used;ab.fastMaxUs=std::max(ab.fastMaxUs,used);}
    else {ab.fullTotalUs+=used;ab.fullMaxUs=std::max(ab.fullMaxUs,used);}
#if defined(UI_HOST_TEST) && defined(UI_BACKDROP_AB_TEST_HOOK)
    if(backdropABTestObserver115)backdropABTestObserver115(sample,fast,now,BackdropABTestStage115::After);
#endif
   };
   const bool fastFirst=bool(sample&1u);
   if(fastFirst)++ab.fastFirstPairs;else ++ab.fullFirstPairs;
   pass(fastFirst);memcpy(backdropReference,pixels,sizeof(pixels));pass(!fastFirst);
   bool mismatch=false;
   for(unsigned i=0;i<W*H;++i)if(pixels[i]!=backdropReference[i]){
    ++ab.mismatchPixels;mismatch=true;
    if(ab.firstMismatchSample<0){ab.firstMismatchSample=int(sample);ab.firstMismatchAt=int(i);}
   }
   ab.comparedPixels+=W*H;ab.mismatchPairs+=mismatch;++ab.pairs;
   const uint32_t panelBegin=micros();M5.Display.pushImage(0,0,W,H,shown);const uint32_t panelUsed=micros()-panelBegin;
   ab.panelTotalUs+=panelUsed;ab.panelMaxUs=std::max(ab.panelMaxUs,panelUsed);
  }
  ab.ran=true;std::free(backdropReference);
 }
 // Measure the new Major panel separately after the original fixture and AB
 // prefixes. Only presentation cells and the Grand-routing bit are temporary;
 // no action, wager, financial award, audio request or save is submitted.
 constexpr unsigned majorPoses119[]={0,499,500,1000,5000,5100,6500,7899,7900,8500,8600,9500,10000,10699,10700,11400,12699,12700,13000,15000,16899,16900,18000,18399};
 constexpr unsigned cash119[]={50,50,50,25,125,0,25,25,25,50,0,50,0,0,25};
 for(unsigned i=0;i<15;++i){visible[i]=Cell{};visible[i].symbol=cash119[i]?Symbol::Phoenix:Symbol::Nine;if(cash119[i])visible[i].prize=int64_t(cash119[i])*5;}
 visible[10].symbol=Symbol::Phoenix;visible[10].kind=phoenix::PrizeKind::Major;visible[10].prize=57204;
 engine.presentation.grandAwarded=false;phase=Phase::Jackpot;holdView=true;fountainContinuous=false;
 for(unsigned sample=0;sample<sizeof(majorPoses119)/sizeof(*majorPoses119);++sample){
  const uint32_t elapsed=majorPoses119[sample],now=millis();phaseAt=now-elapsed;
  fountainActive=elapsed>=feature_award::fountainAt;fountainAt=phaseAt+feature_award::fountainAt;
  memcpy(pixels,shown,sizeof(pixels));dotClipTop=0;dotClipBottom=H;
#if defined(UI_HOST_TEST) && defined(UI_MAJOR_BENCH_TEST_HOOK)
  if(majorBenchObserver119)majorBenchObserver119(sample,elapsed,now,MajorBenchStage119::Before);
#endif
  const uint32_t begin=micros();rasterScene(now);const uint32_t used=micros()-begin;
  auto& timing=result.majorTiming;++timing.frames;timing.rendererTotalUs+=used;timing.rendererMaxUs=std::max(timing.rendererMaxUs,used);
#if defined(UI_HOST_TEST) && defined(UI_MAJOR_BENCH_TEST_HOOK)
  if(majorBenchObserver119)majorBenchObserver119(sample,elapsed,now,MajorBenchStage119::After);
#endif
  const uint32_t panelBegin=micros();M5.Display.pushImage(0,0,W,H,shown);const uint32_t panelUsed=micros()-panelBegin;
  timing.panelTotalUs+=panelUsed;timing.panelMaxUs=std::max(timing.panelMaxUs,panelUsed);
 }
 result.majorTiming.ran=true;engine.presentation.grandAwarded=priorGrandAwarded119;
 memcpy(target,held->target,sizeof(target));memcpy(previous,held->previous,sizeof(previous));
 memcpy(holdBase,held->backdrop,sizeof(holdBase));memcpy(holdStrips,held->strips,sizeof(holdStrips));
 memcpy(reelMs,held->reelMs,sizeof(reelMs));heldMask=held->heldMask;holdView=held->holdView;
 lastArrived=held->stopped;spinQuickStopped=held->quickStopped;spinLandingCoalesced=held->landingCoalesced;
 holdBackdropCaptured=held->captured;holdStripStartsBlank=held->startsBlank;held->~HoldRenderBackup();std::free(held);
 memcpy(visible,kept,sizeof(visible));phase=priorPhase;phaseAt=priorAt;holdEntryMeter=priorMeter;
 memcpy(pixels,shown,sizeof(pixels));std::free(shown);dirty=true;dotClipTop=priorClipTop;dotClipBottom=priorClipBottom;
 result.afterHash=engineHash();result.unchanged=result.beforeHash==result.afterHash;
 holdOpening31=priorHoldOpening;wisdomEntry30=priorWisdom;returnSelector86=priorReturnSelector;collectionNamedDone=priorNamed;fountainActive=priorFountain;fountainAt=priorFountainAt;fountainContinuous=priorContinuous;fountainAwardMusic=priorFountainMusic;fountainMusicAt=priorMusicAt;fountainStopMs=priorStop;spinAward=priorSpinAward;freeIntroIsRetrigger=priorRetrigger;freeIntroOldTotal=priorFreeTotal;
 result.rendererAfterHash=rendererHash();result.rendererUnchanged=result.rendererBeforeHash==result.rendererAfterHash;
 result.ran=true;return result;
}
void uiFrame(){if(initialized&&amountFailureShown190){enterAmountFault190();if(dirty)render(millis());return;}if(initialized&&!engine.randomHealthy()){enterRandomFault125();if(dirty)render(millis());return;}uint32_t now=millis();
 if(storagePause==UiCheckpointPause::None&&!returnSelector86.pending&&returnSelector86.recoveryFence&&!M5.BtnA.isPressed()&&!M5.BtnB.isPressed())returnSelector86.recoveryFence=false;
  if(storagePause!=UiCheckpointPause::None){if(dirty)render(now);return;}
  updateFeatureSounds(now);
  holdOpening31.blocked(now,holdOpeningAudio());
 if(phase==Phase::Spin){
  for(int first=lastArrived+1;first<5;){
   if(now-phaseAt<reel_motion::impactMs(reelMs[first]))break;
   int last=first;
   // All arrivals first shown in this frame share one audible impact. In
   // particular, quick-stop must not replay five obsolete stops for560ms
   // after all five reels have already landed. Timely Twin groups still
   // share their original stop; ordinary separate frames keep five cues.
   while(last+1<5&&now-phaseAt>=reel_motion::impactMs(reelMs[last+1]))++last;
   for(int c=first;c<=last;++c)for(int r=0;r<3;++r){
    const int i=c*3+r;
    if(phoenixCell(target[i])&&!(heldMask&(1u<<i))){
     rebasePhoenixClocks(now);
     phoenixPhases[i]=uint16_t(spinQuickStopped?0:(now-phaseAt-reel_motion::impactMs(reelMs[c]))%pa::phoenixPeriodMs());
     if(engine.presentation.meterArrivalMask&(1u<<i)){
      displayMeter=engine.presentation.meterAfterCell[i];
      if(engine.presentation.holdRespin)displayQueuedHolds=engine.presentation.queuedAfterCell[i];
     }else displayMeter=engine.presentation.meterAfter;
     if(engine.presentation.holdRespin)displayRespins=3;
    }
   }
   if(last>first)spinLandingCoalesced=true;
   const auto tone=landingForGroup(first,last);
   const uint32_t impact=spinQuickStopped?now:phaseAt+reel_motion::impactMs(reelMs[last]);
   if(tone==landing_cues::Tone::Initial)cuePhoenix(false,impact);
   else if(tone==landing_cues::Tone::Follow)cuePhoenix(true,impact);
   else if(tone==landing_cues::Tone::Third)cuePhoenix(tone,impact);
   else if(tone==landing_cues::Tone::Stop)cueStop(impact);
   dirty=true; // present the corresponding stop in this same frame
   lastArrived=last;first=last+1;
  }
 }
  updateWisdomStart87(now);
  if(winCuePending&&!landingAudioBusy()){winCuePending=false;cueWin();}
  if(phase!=Phase::Ready&&phase!=Phase::SpinAward&&now-phaseAt>=phaseMs&&
     (phase!=Phase::Land||!landingAudioBusy())&&
     ((phase!=Phase::FreeIntro&&phase!=Phase::HoldIntro)||((autoPlay||(phase==Phase::FreeIntro&&freeRetriggerIntro()))&&!featureAudioBusy()))){
    finishPhase();
    // startPhase()/ready() sample the live clock. A tick during that call
    // can put their origins after this frame's old `now`; unsigned elapsed
    // would then look 49 days old, flashing the final count/animation pose.
    now=millis();
  }
  if(phase==Phase::SpinAward){
   const uint32_t elapsed=now-phaseAt;
   spinAward.shown=spin_award::countedUpper(spinAward.amount,engine.state.coinValue,elapsed);
   displayWin=winFrom=winTo=spinAward.before+spinAward.shown;
   // Our custom autoplay collects after one full clean phrase plus1.5s.
   // Source jumps at6.68/10.33s do not establish a vendor auto-cap/skip.
   if(spinAward.shown>=spinAward.amount||(autoPlay&&elapsed>=spin_award::localAutoCollectMs)){
    finishSpinAward();now=millis();
   }
  }
  if(winTo!=winFrom){
   if(const auto* profile=wisdomWinProfile88())displayWin=wisdom_win88::counted(*profile,uint32_t(now-phaseAt));
   else{unsigned e=std::min(uint32_t(now-winAt),uint32_t(1600));displayWin=money_math::lerp(winFrom,winTo,e,1600);}
  }
  if(phase==Phase::HoldWipe&&!holdWipeRevealed&&now-phaseAt>=feature_entry::reveal(holdEntryMeter))revealHold();
  if(phase==Phase::ReturnWipe&&pendingChained&&!holdWipeRevealed&&now-phaseAt>=collectionReturnLead()+feature_entry::returnRevealMs){displayQueuedHolds=engine.state.pendingMeterHolds;revealHold();}
  if(phase==Phase::Collect){const unsigned completed=collectedAt(now-phaseAt);if(!collection_steps101::minorAfter(visible,completed,engine.state.coinValue,displayWin))enterAmountFault190();
   // The impact belongs to the amount arriving,700ms into the flight.
   // A slow frame coalesces missed pickups, without queuing historical tones.
   const auto currentPickup=collection_steps101::at(visible,int(completed)-1);
   const auto priorPickup=collection_steps101::at(visible,lastCollectSound-1);
   // Retain one existing impact request per physical cell. Named subawards
   // have no qualified match for repeating the ordinary impact sound.
   if(completed>0&&(!priorPickup||currentPickup.cell!=priorPickup.cell)){lastCollectSound=int(completed);requestCollectionCue();}
  }
  if(fountainActive){
   const uint32_t elapsed=now-fountainAt;
   if(fountainContinuous){if(fountainStopMs!=UINT32_MAX&&elapsed>fountainStopMs&&elapsed-fountainStopMs>coin_motion::lifeMs)fountainActive=false;}
   else if(elapsed>=coin_motion::fountainDurationMs)fountainActive=false;
  }
  if(!fountainActive&&phase==Phase::Jackpot&&now-phaseAt>=feature_award::fountainAt)startAwardFountain(phaseAt+feature_award::fountainAt,now,FeatureMusicPart::MajorAward);
  if(phase==Phase::WinTransfer){
   const int64_t before=engine.state.roundPayout-engine.presentation.phoenixWinMinor;const uint32_t e=now-phaseAt;displayWin=before;
   if(collectionFountain()){
    if(!fountainActive&&e>=feature_award::bigFountainAt)startAwardFountain(phaseAt+feature_award::bigFountainAt,now,FeatureMusicPart::BigAward);
    const uint32_t counted=e>feature_award::bigFountainAt?std::min(e-feature_award::bigFountainAt,coin_motion::countDurationMs):0;
    displayWin=money_math::lerp(before,engine.state.roundPayout,counted,coin_motion::countDurationMs);
   }else if(ordinaryCollectionTransfer())displayWin=before+feature_award::normalWinCount(engine.presentation.phoenixWinMinor,e);
   if(lowerCashCollectionFountain149()){
    constexpr uint32_t burstAt=feature_award::normalWinTransferMs+feature_award::normalWinSettleMs-feature_award::lowerCashFountainLeadMs149;
    if(!fountainActive&&e>=burstAt)startAwardFountain(phaseAt+burstAt,now,FeatureMusicPart::Off);
   }
  }
  if(fountainActive&&(phase==Phase::WinTransfer||phase==Phase::Jackpot))traceAwardFountain(now-phaseAt,now);
  if(phase==Phase::ReturnWipe){
   const uint32_t duration=collectionReturnLead()?collectionReturnLead():200;
   const uint32_t e=std::min(now-phaseAt,duration);const int64_t before=engine.state.roundPayout-engine.presentation.phoenixWinMinor;
   // The qualified short lower-cash burst is finished at the returning
   // crest tail. Do not carry its particles into a queued Hold/Free/Base view.
   if(lowerCashCollectionFountain149()&&now-phaseAt>=collectionReturnLead()+feature_award::lowerCashFountainTailMs149)fountainActive=false;
   displayWin=collectionFountain()||ordinaryCollectionTransfer()?engine.state.roundPayout:money_math::lerp(before,engine.state.roundPayout,e,duration);
  }
  if(jackpotBoardRequested198&&phase==Phase::Ready)openJackpotBoard198();
  else if(cabinetMenuRequested&&phase==Phase::Ready)openCabinetWager();
  if(autoPlay)refillIfNeeded();
  if(!runtimeFinancialBlocked()&&referenceLocalMath()&&phase==Phase::Ready&&view==View::Game&&engine.state.mode==Mode::Base&&engine.canLuckyChance()&&luckyOfferAction!=engine.state.actions)offerLucky();
  if(!runtimeFinancialBlocked()&&phase==Phase::Ready&&view==View::Game&&!awaitingBonusResume&&int32_t(now-autoAt)>=0&&(autoPlay||engine.state.mode!=Mode::Base)&&(!featureAudioBusy()||wisdomEntry30.stage==wisdom_ctrl30::Stage::Accepted)){
    action();now=millis(); // draw the new strip with its own current clock
  }
  const bool fireMotion=phase==Phase::HoldWipe||phase==Phase::ReturnWipe||phase==Phase::Revive||
                        (phase==Phase::Collect&&largeCollectionPrelude()&&now-phaseAt>=feature_award::largeBirdAt&&now-phaseAt<feature_award::largeBirdAt+900);
  if(dirty||now-lastDraw>=(phase==Phase::Spin?30u:fireMotion?33u:40u))render(now);
}
String uiSageDiagnostic(){
 const auto& p=engine.presentation;uint16_t awarded=0;
 for(unsigned i=0;i<15;++i)if(visible[i].highlight)awarded|=uint16_t(1u<<i);
 const uint16_t geometry=view==View::Game&&phase!=Phase::Spin&&!holdView?phoenix::Engine::joinedSageMask(visible,awarded):0;
 // Separate bounded diagnostic: the normal status is already close to its
 //4096-byte USB queue limit. Geometry is not a framebuffer measurement.
 char value[320];
 snprintf(value,sizeof(value),"{\"valid\":%s,\"action\":%llu,\"phase\":\"%s\",\"view\":\"%s\",\"auto\":%s,\"paid\":%s,\"free\":%s,\"hold\":%s,\"hasSpin\":%s,\"coinMinor\":%lld,\"lines\":%u,\"multiplier\":%u,\"winMask\":%u,\"joinMask\":%u,\"gameJoinGeometryMask\":%u,\"rawStackMask\":%u}",p.valid?"true":"false",(unsigned long long)engine.state.actions,phaseName(),viewName(),autoPlay?"true":"false",p.paidSpin?"true":"false",p.freeSpin?"true":"false",p.holdRespin?"true":"false",p.hasSpinResult?"true":"false",(long long)engine.state.coinValue,engine.state.lineCount,engine.state.betMultiplier,p.winMask,p.expandedMask,geometry,phoenix::Engine::joinedSageMask(engine.state.cells,0x7fff));
 return value;
}
String uiAwardDiagnostic(){
 String value=String("{\"valid\":")+(lastAward.valid?"true":"false")+",\"test\":"+(lastAward.test?"true":"false");
 value+=String(",\"action\":")+integer(lastAward.action).c_str()+",\"cashMinor\":"+integer(lastAward.cash).c_str()+",\"betMinor\":"+integer(lastAward.bet).c_str()+",\"namedMinor\":"+integer(lastAward.named).c_str();
 value+=String(",\"collection\":")+(lastAward.collection?"true":"false")+",\"autoPlayAtBegin\":"+(lastAward.autoPlayAtBegin?"true":"false");
 value+=String(",\"phoenixCount\":")+lastAward.phoenixCount+",\"cashCellCount\":"+lastAward.cashCellCount+",\"miniCount\":"+lastAward.miniCount+",\"minorCount\":"+lastAward.minorCount+",\"majorCount\":"+lastAward.majorCount;
 value+=String(",\"holdSettled\":")+(lastAward.holdSettled?"true":"false")+",\"freeEnded\":"+(lastAward.freeEnded?"true":"false")+",\"roundCompleted\":"+(lastAward.roundCompleted?"true":"false")+",\"combinedRoundBasis\":"+(lastAward.combinedRoundBasis?"true":"false");
 value+=String(",\"fountainBasisMinor\":")+integer(lastAward.fountainBasis).c_str()+",\"roundPayoutMinor\":"+integer(lastAward.roundPayout).c_str();
 value+=String(",\"grand\":")+(lastAward.grand?"true":"false")+",\"fountainEligible\":"+(lastAward.fountainEligible?"true":"false")+",\"cueRequested\":"+(lastAward.cueRequested?"true":"false")+",\"cuePart\":"+unsigned(lastAward.part);
 value+=String(",\"cueFirstPhaseMs\":")+lastAward.cueFirstMs+",\"cueFirstUptimeMs\":"+lastAward.cueFirstAt+",\"fountainStarted\":"+(lastAward.fountainStarted?"true":"false")+",\"fountainFirstPhaseMs\":"+lastAward.fountainFirstMs+",\"fountainFirstUptimeMs\":"+lastAward.fountainFirstAt+",\"backendStartsBefore\":"+lastAward.backendStartsBefore+"}";return value;
}
UiAwardSceneTest uiTestAwardScene(bool spinOnly,bool automatic){
 UiAwardSceneTest result;
 if(runtimeFinancialBlocked())return result;
 if(!initialized||view!=View::Game||phase!=Phase::Ready||(autoPlay&&!automatic)||engine.state.mode!=Mode::Base||engine.state.roundOpen||featureAudioBusy()||winCuePending)return result;
#ifndef UI_HOST_TEST
 const auto music=audioMusicDiagnostics();if(!audioReady()||audioMuted()||!audioVolume()||!music.awardCacheReady||music.wanted||music.playing)return result;
#endif
 struct Backup {
  explicit Backup(const phoenix::Engine& source):game(source){}
  return_selector86::State returnSelector;phoenix::Engine game;Cell visible[15],target[15],previous[15],backdrop[15];AwardTrace trace;SpinAward spinAward;hold_opening31::State holdOpening;wisdom_ctrl30::State wisdomEntry;bool continuous,awardMusic,retrigger;uint16_t freeTotal;uint32_t stop,musicAt;
  Phase phase;uint32_t at,ms,fountainAt,winAt,lineAt,readyAt,autoAt;int64_t bank,win,winFrom,winTo;
  uint16_t freeCount,phoenixPhases[15];uint32_t phoenixClock;bool phasesRestored,landingCoalesced;
  bool hold,backdropCaptured,named,fountain,pendingFree,pendingHold,pendingOutro,pendingChained,introChained,awaiting,winCue,lineValid,autoPlaying;
 };
#ifdef UI_HOST_TEST
 void* memory=std::malloc(sizeof(Backup));
#else
 void* memory=ps_malloc(sizeof(Backup));
#endif
 if(!memory)return result;static_assert(std::is_trivially_copyable<phoenix::Engine>::value,"Exact temporary engine backup");auto* kept=new(memory)Backup(engine);std::memcpy(&kept->game,&engine,sizeof(engine));
 auto hash=[](){const auto* bytes=reinterpret_cast<const uint8_t*>(&engine);uint32_t h=2166136261u;for(size_t i=0;i<sizeof(engine);++i)h=(h^bytes[i])*16777619u;return h;};
 result.beforeHash=hash();
 memcpy(kept->visible,visible,sizeof(visible));memcpy(kept->target,target,sizeof(target));memcpy(kept->previous,previous,sizeof(previous));memcpy(kept->backdrop,holdBase,sizeof(holdBase));
 kept->returnSelector=returnSelector86;kept->holdOpening=holdOpening31;kept->wisdomEntry=wisdomEntry30;kept->spinAward=spinAward;kept->continuous=fountainContinuous;kept->awardMusic=fountainAwardMusic;kept->musicAt=fountainMusicAt;kept->stop=fountainStopMs;kept->retrigger=freeIntroIsRetrigger;kept->freeTotal=freeIntroOldTotal;kept->trace=lastAward;kept->phase=phase;kept->at=phaseAt;kept->ms=phaseMs;kept->fountainAt=fountainAt;kept->winAt=winAt;kept->lineAt=lineDisplayAt;
 kept->bank=displayBank;kept->win=displayWin;kept->winFrom=winFrom;kept->winTo=winTo;
 kept->readyAt=readyAt;kept->autoAt=autoAt;kept->autoPlaying=autoPlay;kept->freeCount=pendingFreeCount;memcpy(kept->phoenixPhases,phoenixPhases,sizeof(phoenixPhases));kept->phoenixClock=phoenixClockAt;kept->phasesRestored=phoenixPhasesRestored;kept->landingCoalesced=spinLandingCoalesced;
 kept->hold=holdView;kept->backdropCaptured=holdBackdropCaptured;kept->named=collectionNamedDone;kept->fountain=fountainActive;
 kept->pendingFree=pendingFree;kept->pendingHold=pendingHold;kept->pendingOutro=pendingOutro;kept->pendingChained=pendingChained;kept->introChained=introChained;kept->awaiting=awaitingBonusResume;kept->winCue=winCuePending;kept->lineValid=lineClockValid;
 // This is a temporary presentation fixture. No Engine::action(), pay(),
 // save, wager change or persistent preference write occurs in this function.
 const auto cash=engine.betMinor()*60;engine.presentation=phoenix::Presentation{};engine.presentation.phoenixWinMinor=cash;engine.presentation.holdSettled=true;
 engine.state.roundPayout=cash;engine.state.freeSession=false;engine.state.freeLeft=0;
 for(unsigned i=0;i<15;++i){visible[i]=Cell{};if(i<6){visible[i].symbol=Symbol::Phoenix;visible[i].prize=engine.state.betCoins*10;}}
 copy(target,visible);copy(previous,visible);copy(holdBase,visible);
 holdOpening31.clear();wisdomEntry30={};holdView=holdBackdropCaptured=true;collectionNamedDone=fountainActive=false;fountainAt=0;clearSpinAward();
 pendingFree=pendingHold=pendingOutro=pendingChained=introChained=awaitingBonusResume=winCuePending=false;freeIntroIsRetrigger=false;freeIntroOldTotal=0;
 autoPlay=automatic;autoAt=millis()+60000;
 displayWin=winFrom=winTo=0;beginAwardTrace(true);
 if(spinOnly){
  const int64_t award=engine.betMinor()*50,prior=engine.betMinor()*15;
  engine.presentation=phoenix::Presentation{};auto& p=engine.presentation;
  p.valid=p.hasSpinResult=p.freeSpin=p.freeAwarded=true;p.actionNumber=engine.state.actions;
  p.actionWinMinor=award;p.freeBefore=2;p.freeAwardedCount=3;
  engine.state.mode=Mode::Free;engine.state.freeSession=engine.state.roundOpen=true;
  engine.state.freePlayed=8;engine.state.freeLeft=4;engine.state.roundPayout=prior+award;
  for(unsigned i=0;i<15;++i){visible[i]=Cell{};visible[i].symbol=i<5?Symbol::Scatter:Symbol::Book;visible[i].highlight=i<5;}
  copy(target,visible);copy(previous,visible);holdView=false;
  spinAward.pending=true;spinAward.action=engine.state.actions;spinAward.amount=award;spinAward.before=prior;spinAward.bet=engine.betMinor();
  displayWin=winFrom=winTo=prior;lineDisplayAt=millis();lineClockValid=true;
  pendingFreeCount=3;freeIntroIsRetrigger=true;freeIntroOldTotal=9;
  cueFeature(true);startPhase(Phase::FreeIntro,featureIntroDuration(true));
 }else startPhase(Phase::WinTransfer,feature_award::bigHoldMs);
 const uint32_t began=millis(),framesBefore=frameCount;
 while(uint32_t(millis()-began)<(spinOnly?18000u:feature_award::bigHoldMs+250)){
#ifndef UI_HOST_TEST
  audioUpdate(millis());
#endif
  uiFrame();
  if(spinOnly){
   if(phase==Phase::FreeCounter){const unsigned tick=unsigned(shownFreeTotal(millis())-freeIntroOldTotal);if(tick>=1&&tick<=3)result.counterMask|=1u<<(tick-1);}
   if(phase==Phase::SpinAward&&millis()-phaseAt>=spin_award::localAutoCollectMs)finishSpinAward();
   if(phase==Phase::Ready){awaitingBonusResume=true;result.countedMinor=displayWin;if(!fountainActive)break;}
  }
#ifndef UI_HOST_TEST
  result.maxMusicLevel=std::max(result.maxMusicLevel,uint32_t(audioMusicOutputLevel()));
#endif
  delay(1);
 }
 result.frames=frameCount-framesBefore;result.cueRequested=lastAward.cueRequested;result.cueFirstMs=lastAward.cueFirstMs;result.fountainTriggered=lastAward.fountainStarted;result.fountainFirstMs=lastAward.fountainFirstMs;
 std::memcpy(&engine,&kept->game,sizeof(engine));memcpy(visible,kept->visible,sizeof(visible));memcpy(target,kept->target,sizeof(target));memcpy(previous,kept->previous,sizeof(previous));memcpy(holdBase,kept->backdrop,sizeof(holdBase));
 returnSelector86=kept->returnSelector;holdOpening31=kept->holdOpening;wisdomEntry30=kept->wisdomEntry;spinAward=kept->spinAward;fountainContinuous=kept->continuous;fountainAwardMusic=kept->awardMusic;fountainMusicAt=kept->musicAt;fountainStopMs=kept->stop;freeIntroIsRetrigger=kept->retrigger;freeIntroOldTotal=kept->freeTotal;lastAward=kept->trace;phase=kept->phase;phaseAt=kept->at;phaseMs=kept->ms;fountainAt=kept->fountainAt;winAt=kept->winAt;lineDisplayAt=kept->lineAt;
 displayBank=kept->bank;displayWin=kept->win;winFrom=kept->winFrom;winTo=kept->winTo;
 readyAt=kept->readyAt;autoAt=kept->autoAt;autoPlay=kept->autoPlaying;pendingFreeCount=kept->freeCount;memcpy(phoenixPhases,kept->phoenixPhases,sizeof(phoenixPhases));phoenixClockAt=kept->phoenixClock;phoenixPhasesRestored=kept->phasesRestored;spinLandingCoalesced=kept->landingCoalesced;
 holdView=kept->hold;holdBackdropCaptured=kept->backdropCaptured;collectionNamedDone=kept->named;fountainActive=kept->fountain;
 pendingFree=kept->pendingFree;pendingHold=kept->pendingHold;pendingOutro=kept->pendingOutro;pendingChained=kept->pendingChained;introChained=kept->introChained;awaitingBonusResume=kept->awaiting;winCuePending=kept->winCue;lineClockValid=kept->lineValid;
 kept->~Backup();std::free(memory);requestFeatureMusic(FeatureMusicPart::Off);result.afterHash=hash();result.unchanged=result.beforeHash==result.afterHash;result.ran=true;dirty=true;render(millis());return result;
}
#include "arrival_scene_qa.h"
#include "profile_scene_qa.h"
void uiRedraw(){dirty=true;}
void uiStatus(const char* message){snprintf(statusLine,sizeof(statusLine),"%s",message?message:"");dirty=true;}
bool uiReady(){return initialized&&!runtimeFinancialBlocked()&&phase==Phase::Ready&&view==View::Game;}
bool uiOverlayVisible(){return storagePause!=UiCheckpointPause::None||view!=View::Game||phase==Phase::FreeIntro||phase==Phase::FreeCounter||spinAwardPhase()||phase==Phase::HoldIntro||phase==Phase::Collect||phase==Phase::Outro;}
uint64_t uiActionCount(){return engine.state.actions;}
const uint16_t* uiFramePixels(){return pixels;}
void uiAction(){if(uiReturnSelectionInputFenced()||runtimeFinancialBlocked())return;if(autoPlay){autoPlay=false;dirty=true;return;}if(phase==Phase::AwardIntro||phase==Phase::Collect||phase==Phase::WinTransfer||phase==Phase::ReturnWipe||phase==Phase::Jackpot||phase==Phase::Outro||phase==Phase::FreeIntro||phase==Phase::FreeCounter||spinAwardPhase()||phase==Phase::HoldIntro){finishPhase();return;}if(phase==Phase::Spin){spinQuickStopped=true;phaseAt=millis()-phaseMs;dirty=true;return;}if(phase==Phase::Ready)action();}
void uiNextBet(){if(!uiReturnSelectionInputFenced()&&!runtimeFinancialBlocked()&&uiReady()&&engine.canChangeBet()){const size_t i=engine.state.wagerIndex<engine.wagerCount()?(engine.state.wagerIndex+1)%engine.wagerCount():0;const auto&w=phoenix::local::wagers[i];applyRuntimeWager(w.denomination,w.lines,w.multiplier);}}
void uiNewBank(){
 if(uiReturnSelectionInputFenced()||runtimeFinancialBlocked()||phase!=Phase::Ready||!engine.setCredits(phoenix::local::newBankMinor))return;
 ++creditRevision;displayBank=engine.state.balance;view=View::Game;autoPlay=false;statusLine[0]=0;dirty=true;cueCredit();
}

void uiButtonA(){if(uiReturnSelectionInputFenced())return;if(storagePause!=UiCheckpointPause::None)return;if(view==View::Game){uiAction();return;}if(view==View::Jackpots){cueButton();closeJackpotBoard198();return;}cueButton();if(view==View::Menu)chooseMenu();else if(view==View::Bet){
 if(cabinetFocus==cabinetReturnFocus())view=View::Game;
 else if(phase==Phase::Ready&&engine.canChangeBet()){
  if(cabinetFocus<cabinet_profile::denominationCount){statusLine[0]=0;cabinetDraftDenom(cabinetFocus);}
  else if(cabinetFocus<cabinetBetBase()){statusLine[0]=0;cabinetLines=cabinetCoin().lines[cabinetFocus-cabinet_profile::denominationCount];cabinetFocusBet();}
  else if(cabinetFocus<cabinetReturnFocus()){
   const auto multiplier=cabinetRow().multipliers[cabinetFocus-cabinetBetBase()];
   const int64_t stake=cabinetDenom*int64_t(cabinetLines)*multiplier;
   // Browsing is read-only. The observed red BET control commits and spins;
   // an unfunded press cannot create a meter bucket or consume a game draw.
   if(engine.state.balance<stake&&!autoRefill){snprintf(statusLine,sizeof(statusLine),"NOT ENOUGH CREDITS");dirty=true;}
   else if(applyRuntimeWager(cabinetDenom,cabinetLines,multiplier)){view=View::Game;action();}
  }
 }
}else if(view==View::Settings){if(settingsItem==0){soundItem=0;view=View::Sound;}else if(settingsItem==1){optionItem=0;view=View::Options;}else{menuItem=10;view=View::Menu;}}else if(view==View::Paylines){nextPaylinesLayout();}else if(view==View::Rules)view=View::Menu;else if(view==View::Reset){if(creditsPanel201!=CreditsPanel201::Credits){valuesSelect201();}else if(creditItem<3){if(uiAddCredits(creditAmounts[creditItem])){view=View::Game;autoPlay=false;statusLine[0]=0;}}else if(creditItem==3){if(clearBalanceAvailable()&&engine.setCredits(0)){++creditRevision;displayBank=0;view=View::Game;autoPlay=false;statusLine[0]=0;}}else if(creditItem==4){autoRefill=!autoRefill;++creditRevision;cueCredit();}else if(creditItem==5){menuItem=3;view=View::Menu;statusLine[0]=0;}}else if(view==View::Lucky)playLucky();else if(view==View::Options){if(optionItem==0){auto b=M5.Display.getBrightness();M5.Display.setBrightness(b>=200?40:b<80?100:b<150?160:220);}else if(optionItem==1)creditsDisplay=!creditsDisplay;else uiFlipScreen();}
else if(view==View::Sound){if(soundItem==0){
#ifndef UI_HOST_TEST
 audioSetMuted(!audioMuted());
#else
 hostMuted=!hostMuted;
#endif
}else if(soundItem==1)uiSetSoundPreferences(false,!landingSounds,soundVolume);else if(soundItem==2){testJingleAt=0;view=View::Volume;}else if(soundItem==5)auditionAwardSound();else auditionSound(soundItem==4);}
else if(view==View::Volume)adjustVolume(true);
else if(view==View::Math){selectReturnOption86();}

dirty=true;}
void uiButtonB(){if(uiReturnSelectionInputFenced())return;if(storagePause!=UiCheckpointPause::None)return;if(view==View::Game)return;cueButton();if(view==View::Jackpots)closeJackpotBoard198();else if(view==View::Menu)nextMenuItem();else if(view==View::Settings)settingsItem=(settingsItem+1)%3;else if(view==View::Paylines)paylinesPage=(paylinesPage+1)%paylinesPageCount();else if(view==View::Bet)cabinetFocus=(cabinetFocus+1)%(cabinetReturnFocus()+1);else if(view==View::Rules)rulesPage=(rulesPage+1)%kRulesPageCount;else if(view==View::Options)optionItem=(optionItem+1)%3;else if(view==View::Sound)soundItem=(soundItem+1)%6;else if(view==View::Volume)adjustVolume(false);else if(view==View::Reset){if(creditsPanel201==CreditsPanel201::Credits)creditItem=(creditItem+1)%6;else valuesNext201();}else if(view==View::Lucky)view=View::Game;else if(view==View::Math)nextReturnOption86();dirty=true;}
void uiButtonBHold(){if(uiReturnSelectionInputFenced())return;if(view==View::Reset&&creditsPanel201!=CreditsPanel201::Credits){valuesBack201();return;}if(view==View::Jackpots)closeJackpotBoard198();else if(view==View::Bet)uiCabinetMenu();else uiMenu();}
void uiMenu(){
 if(uiReturnSelectionInputFenced())return;
 if(storagePause!=UiCheckpointPause::None){dirty=true;return;}
 if(runtimeFinancialBlocked()){enterService();return;}
 autoPlay=false;jackpotBoardRequested198=false;cueButton();
 if(view==View::Reset){
  if(creditsPanel201!=CreditsPanel201::Credits)valuesBack201();
  else{menuItem=3;view=View::Menu;statusLine[0]=0;}
 }else if(view==View::Settings){menuItem=10;view=View::Menu;statusLine[0]=0;}else if(view==View::Paylines){menuItem=9;view=View::Menu;statusLine[0]=0;}else if(view==View::Rules){menuItem=0;view=View::Menu;statusLine[0]=0;}
 else if(view==View::Sound){settingsItem=0;view=View::Settings;statusLine[0]=0;}
 else if(view==View::Options){settingsItem=1;view=View::Settings;statusLine[0]=0;}
 else if(view==View::Volume)view=View::Sound;
 else if(view==View::Math){
  using Page=return_selector86::Page;
  if(returnSelector86.page==Page::Detail||returnSelector86.page==Page::Help)nextReturnOption86();
  else{menuItem=7;view=View::Menu;statusLine[0]=0;}
 }else if(view==View::Game||view==View::Bet){cabinetMenuRequested=false;menuItem=0;view=View::Menu;statusLine[0]=0;}
 else view=View::Game;
 dirty=true;
}

void uiPreparePanelInput(){
 if(!initialized||uiReturnSelectionInputFenced())return;
 autoPlay=false;if(engine.state.mode!=Mode::Base)awaitingBonusResume=true;
}
void uiJackpots(){
 if(uiReturnSelectionInputFenced()||!initialized||storagePause!=UiCheckpointPause::None)return;
 if(runtimeFinancialBlocked()){enterService();return;}
 uiPreparePanelInput();cabinetMenuRequested=false;cueButton();
 if(phase!=Phase::Ready){jackpotBoardRequested198=true;dirty=true;return;}
 openJackpotBoard198();
}
void uiCabinetMenu(){
 if(uiReturnSelectionInputFenced())return;
 jackpotBoardRequested198=false;
 if(!initialized||storagePause!=UiCheckpointPause::None)return;
 if(runtimeFinancialBlocked()){enterService();return;}
 autoPlay=false;cueButton();
 if(view==View::Bet){cabinetMenuRequested=false;view=View::Game;dirty=true;return;}
 if(cabinetMenuRequested){cabinetMenuRequested=false;dirty=true;return;}
 if(phase!=Phase::Ready){cabinetMenuRequested=true;dirty=true;return;}
 openCabinetWager();
}
bool uiIsGameView(){return storagePause==UiCheckpointPause::None&&view==View::Game;}
void uiSetScreenFlipped(bool flipped){screenFlipped=flipped;M5.Display.setRotation(flipped?3:1);dirty=true;}
void uiFlipScreen(){if(uiReturnSelectionInputFenced())return;uiSetScreenFlipped(!screenFlipped);}
bool uiScreenFlipped(){return screenFlipped;}
void uiToggleAuto(){if(uiReturnSelectionInputFenced())return;if(!initialized||runtimeFinancialBlocked())return;if(view==View::Volume){auditionSound(false);return;}if(view!=View::Game)return;autoPlay=!autoPlay;if(autoPlay){awaitingBonusResume=false;view=View::Game;autoAt=millis()+250;}cueButton();dirty=true;}
void uiSetSoundPreferences(bool spin,bool landings,uint8_t volume){(void)spin;landingSounds=landings;soundVolume=volume;
#ifndef UI_HOST_TEST
 audioSetSpinEnabled(false);audioSetLandingsEnabled(landings);audioSetVolume(volume);
#endif
 dirty=true;
}
bool uiAddCredits(uint32_t amountMinor){
 if(!amountMinor||uiReturnSelectionInputFenced()||runtimeFinancialBlocked()||phase!=Phase::Ready||!engine.canChangeBet()||!engine.addCredits(amountMinor))return false;
 for(auto amount:creditAmounts)if(amountMinor==amount)lastAddedMinor=amount;
 ++creditRevision;displayBank=engine.state.balance;dirty=true;cueCredit();return true;
}
void uiSetCreditPreferences(bool enabled,uint32_t amount){
 // An old $1m refill selection becomes the largest remaining menu amount.
 // Only the refill preference changes; the current bank/meter/RNG do not.
 autoRefill=enabled;lastAddedMinor=amount==100000000?10000000:1000000;
 for(auto valid:creditAmounts)if(amount==valid)lastAddedMinor=amount;
 dirty=true;
}
bool uiAutoRefillEnabled(){return autoRefill;}
uint32_t uiLastAddedMinor(){return lastAddedMinor;}
uint32_t uiCreditRevision(){return creditRevision;}
bool uiSpinSoundEnabled(){return spinSounds;}
bool uiLandingSoundEnabled(){return landingSounds;}
uint8_t uiSoundVolume(){return soundVolume;}
void uiSetCreditsDisplay(bool value){creditsDisplay=value;dirty=true;}
bool uiCreditsDisplay(){return creditsDisplay;}
String uiSnapshot(){char b[1500];snprintf(b,sizeof(b),"{\"theme\":\"Confucius Say\",\"model\":\"prototype-unfitted-math-0x%08x\",\"mode\":\"%s\",\"animation\":\"%s\",\"ready\":%s,\"autoPlay\":%s,\"balanceMinor\":%lld,\"winMinor\":%lld,\"featureWinMinor\":%lld,\"coinValue\":%lld,\"betCoins\":%lld,\"lines\":%u,\"betMultiplier\":%u,\"freeLeft\":%u,\"respinsLeft\":%u,\"meter\":%u,\"actions\":%llu,\"rounds\":%llu,\"renderFrames\":%u,\"lastDrawUs\":%u,\"maxDrawUs\":%u,\"grid\":[",unsigned(phoenix::local::modelId),modeName(engine.state.mode),phaseName(),uiReady()?"true":"false",autoPlay?"true":"false",(long long)engine.state.balance,(long long)engine.state.win,(long long)engine.state.featureWin,(long long)engine.state.coinValue,(long long)engine.state.betCoins,engine.state.lineCount,engine.state.betMultiplier,engine.state.freeLeft,engine.state.respinsLeft,engine.state.meter,(unsigned long long)engine.state.actions,(unsigned long long)engine.state.rounds,frameCount,lastDrawUs,maxDrawUs);String result(b);for(int i=0;i<15;++i){if(i)result+=",";result+=int(engine.state.cells[i].symbol);}result+="],\"view\":\"";result+=viewName();result+="\",\"creditsDisplay\":";result+=creditsDisplay?"true":"false";result+=",\"spinSounds\":";result+=spinSounds?"true":"false";result+=",\"landingSounds\":";result+=landingSounds?"true":"false";result+=",\"screenFlipped\":";result+=screenFlipped?"true":"false";result+=",\"awaitingBonusResume\":";result+=awaitingBonusResume?"true":"false";result+=",\"autoRefill\":";result+=autoRefill?"true":"false";result+=",\"lastAddedMinor\":";result+=String(lastAddedMinor);result+=",\"phoenixArtReady\":";result+=phoenix_art::ready()?"true":"false";result+=",\"phoenixFramesVerified\":";result+=String(phoenix_art::verifiedFrames());result+=",\"phoenixArtDecodedFNV\":";result+=String(phoenix_art::decodedHash());result+=",\"phoenixPeriodMs\":";result+=String(phoenix_art::periodMs());result+=",\"holdMaterialReady\":";result+=holdMaterialPixels?"true":"false";result+=",\"holdMaterialFNV\":";result+=String(holdMaterialHash);result+=",\"holdMaterialBytes\":";result+=String(holdMaterialPixels?(kHoldMaterialPixels+TW*TH)*2:0);result+=",\"phoenixPhasesRestored\":";result+=phoenixPhasesRestored?"true":"false";result+=",\"phoenixAssetReadBytes\":";result+=String(phoenix_art::assetReadBytes());result+=",\"phoenixAssetReadFNV\":";result+=String(phoenix_art::assetReadHash());result+=",\"phoenixAlphaReady\":";result+=phoenix_art::alphaReady()?"true":"false";result+=",\"phoenixAlphaMetadataBytes\":";result+=String(phoenix_art::alphaMetadataBytes());result+=",\"phoenixImageWidth\":";result+=String(phoenix_art::frameIndex(0).w);result+=",\"coinSamplerReady\":";result+=coin_sampler::info().ready?"true":"false";result+=",\"coinSamplerBytes\":";result+=String(coin_sampler::info().bytes);result+=",\"coinSamplerEntries\":";result+=String(coin_sampler::info().entries);result+=",\"coinSamplerBuildUs\":";result+=String(coinMapsBuildUs);result+=",\"coinSamplerFNV\":";result+=String(coin_sampler::decodedHash());result+=",\"coinArtFNV\":";result+=String(clean_coins::decodedHash());result+=",\"coinNativeSize\":";result+=String(clean_coins::nativeSize);result+=",\"collectionBirdFNV\":";result+=String(winged_phoenix::decodedHash());result+=",\"collectionFlightKeys\":";result+=String(winged_phoenix::flightKeyFrames);result+=",\"collectionFlightPeriodMs\":";result+=String(winged_phoenix::flightPeriodMs);result+=",\"phoenixPhases\":[";for(unsigned i=0;i<15;++i){if(i)result+=",";result+=String(phoenixRenderClock(i,millis()));}result+="]}";return result;}

uint32_t uiRuntimeRevision(){return runtimeRevision;}
bool uiCheckpointBlocked(){return checkpointBlocked;}
void uiBlockCheckpoint(){checkpointBlocked=true;enterService();}
String uiMathDiagnostic(){
 const auto selected=effectiveMathChoice();const static_grand::Entry*row=nullptr;
 const auto status=engine.hasCustomJackpots()?static_grand::Result::UnqualifiedEntry:static_grand::resolve(*mathFamily,selected,static_grand::profile(engine),row);
 String out=String("{\"selectionKind\":\"retainedSchedule\",\"defaultSlot\":4,\"rtpChoicesEnabled\":false");
 out+=String(",\"customJackpotMask\":")+String(unsigned(engine.customJackpotMask()))+",\"editedCounter\":"+String(unsigned(engine.state.meter));
 out+=String(",\"preferenceFormat\":\"")+(mathSelection.wire==static_grand::Wire::Legacy1?"SGR1":"SGR2")+"\"";
 out+=String(",\"legacyPreferencePending\":")+(legacyMathPending()?"true":"false");
 out+=String(",\"priorNeutralPreferencePending\":")+(priorNeutralMathPending()?"true":"false");
 out+=String(",\"legacyUntunedDefaultPending\":")+(defaultMathPending()?"true":"false");
 out+=String(",\"preferenceRecognized\":")+(selected.family!=static_grand::Fingerprint{}?"true":"false");
 out+=String(",\"neutralScheduleSlot\":")+(selected.family!=static_grand::Fingerprint{}?String(selected.slot):String("null"));
 out+=String(",\"currentCachedGrandCredits\":")+(engine.state.jackpotUnits[3]==phoenix::money::Unit::Credits?integer(engine.state.jackpotCoins[3]).c_str():"null");
 int64_t grandMinor190=0;out+=String(",\"currentCachedGrandMinor\":")+(engine.jackpotAmountMinor(3,grandMinor190)?integer(grandMinor190).c_str():"null");
 out+=",\"proofKind\":\"localAnalyticEnclosure\",\"proofScope\":\"fixedProfileFundedNoLuckyIdealDraws\"";
 out+=String(",\"futureEnclosureQualified\":")+(status==static_grand::Result::Applied?"true":"false");
 if(row){out+=String(",\"futureChosenGrandCredits\":")+integer(row->grandCredits).c_str();out+=String(",\"futureLocalLowerPpm\":")+row->lowReturnPpm+",\"futureLocalNumericalPointPpm\":"+row->estimateReturnPpm+",\"futureLocalUpperPpm\":"+row->highReturnPpm;}
 if(mathSelection.wire==static_grand::Wire::Legacy1){out+=String(",\"legacySelectorProvenanceOnly\":true,\"legacySelectorBps\":")+mathSelection.legacy.requestedBps+",\"legacySelectorWasCeiling\":"+(mathSelection.legacy.setting==legacy_static_grand::Setting::UpperCeiling?"true":"false");}
 out+=String(",\"capacityService\":")+(capacityService.active()?"true":"false")+",\"capacityMask\":"+capacityService.reasonMask()+",\"checkpointBlocked\":"+(checkpointBlocked?"true":"false");
 out+=",\"mathFamily\":\"";out+=math_runtime_identity::mathematicalFamily;out+="\",\"runtimeFingerprint\":\"";out+=math_runtime_identity::fingerprint;out+="\",\"runtimeAccepted\":";out+=math_runtime_identity::accepted?"true":"false";out+=",\"returnEvidence\":\"ideal-uniform-local-model-only\",\"actualCSPRNGInfiniteHorizonReturnQualified\":false,\"originalCasinoCalibration\":false}";return out;
}

bool uiReturnSelectionInputFenced(){return returnSelector86.pending||returnSelector86.recoveryFence;}
bool uiReturnSelectionCheckpointPending(){return returnSelector86.pending;}
bool uiCheckpointPaused(){return storagePause!=UiCheckpointPause::None;}
void uiSetCheckpointPause(UiCheckpointPause reason){
 const uint32_t wall=uiWallMillis();
 if(reason!=UiCheckpointPause::None){
  if(storagePause==UiCheckpointPause::None){uiPauseWallAt=wall;uiPauseGameAt=wall-uiClockOffset;
#ifndef UI_HOST_TEST
   if(audioReady())audioStop();
#endif
  }
  if(holdOpening31.pending())holdOpening31.resume();
  storagePause=reason;autoPlay=false;awaitingBonusResume=true;
 }else if(storagePause!=UiCheckpointPause::None){uiClockOffset+=uint32_t(wall-uiPauseWallAt);storagePause=reason;autoPlay=false;awaitingBonusResume=true;}
 if(reason==UiCheckpointPause::None&&returnSelector86.pending&&!runtimeFinancialBlocked())finishReturnSelection86();
 dirty=true;
}
#pragma pop_macro("millis")

#include "sage_scene_qa.h"
