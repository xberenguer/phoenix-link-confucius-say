#pragma once
#include "sage_qa_api.h"
namespace {
// Compare the actual completed framebuffer with the settled fused asset.
// The lower-body interior excludes frame rails, crown and divider markers.
// It must also differ from repeating the ordinary blue single-face texture.
void observeSageBody(UiSageSceneTest& result,unsigned rows,uint32_t clock){
 const auto& fused=pa::sage(rows,clock);const auto& blue=pa::sage(1,0);
 if(!fused.rgb||fused.w!=TW||fused.h!=rows*TH||!blue.rgb||blue.w!=TW||blue.h!=TH){++result.bodyInvalidFrames;return;}
 if(!result.bodyFrames)result.bodyFirstClock=clock;++result.bodyFrames;
 for(unsigned y=40;y<rows*TH-4;++y)for(unsigned x=5;x<unsigned(TW)-5;++x){
  const auto at=y*fused.w+x;const auto expected=fused.rgb[at];
  if((fused.alpha&&fused.alpha[at]!=255)||(fused.key&&expected==fused.key))continue;
  const auto actual=pixels[(RY+y)*W+RX+TW+x];++result.bodyChecked;
  if(actual==expected)++result.bodyMatched;else ++result.bodyMismatched;
  if(actual!=blue.rgb[(y%TH)*blue.w+x])++result.bodyDifferentBlue;
 }
}
}
UiSageSceneTest uiTestSageScene(unsigned rows){
 UiSageSceneTest result;result.rows=uint8_t(rows);
 if(rows<2||rows>3||runtimeFinancialBlocked()||storagePause!=UiCheckpointPause::None||!initialized||view!=View::Game||
    phase!=Phase::Ready||autoPlay||engine.state.mode!=Mode::Base||!engine.canChangeBet()||featureAudioBusy()||winCuePending)return result;
#ifndef UI_HOST_TEST
 if(M5.BtnA.isPressed()||M5.BtnB.isPressed()||M5.Speaker.isPlaying())return result;
 const auto music=audioMusicDiagnostics();if(music.wanted||music.playing||audioLandingsBusy()||audioDiagnostics().pending)return result;
#endif
 // The production Spin path changes ordinary strips and arrival clocks too.
 // Keep this table static: the native loop has a small measured stack margin.
 struct Region{void* pointer;size_t bytes;};
 static_assert(std::is_trivially_copyable<phoenix::Engine>::value,"Raw Engine QA restoration requires a trivial value object");
 static_assert(std::is_trivially_copyable<Cell>::value,"Raw cell QA restoration requires trivial cells");
 static_assert(std::is_trivially_copyable<AwardTrace>::value,"Raw award QA restoration requires a trivial trace");
 static_assert(std::is_trivially_copyable<SpinAward>::value,"Raw spin QA restoration requires a trivial value");
#define SAGE_QA_REGION(value) {&(value),sizeof(value)}
 static const Region regions[]={
  SAGE_QA_REGION(returnSelector86),SAGE_QA_REGION(engine),SAGE_QA_REGION(pixels),SAGE_QA_REGION(visible),SAGE_QA_REGION(target),SAGE_QA_REGION(previous),
  SAGE_QA_REGION(holdBase),SAGE_QA_REGION(holdBackdropCaptured),SAGE_QA_REGION(dotClipTop),SAGE_QA_REGION(dotClipBottom),SAGE_QA_REGION(strips),SAGE_QA_REGION(holdStrips),SAGE_QA_REGION(holdStripStartsBlank),
  SAGE_QA_REGION(phase),SAGE_QA_REGION(view),SAGE_QA_REGION(phaseAt),SAGE_QA_REGION(phaseMs),SAGE_QA_REGION(lastDraw),
  SAGE_QA_REGION(readyAt),SAGE_QA_REGION(autoAt),SAGE_QA_REGION(frameCount),SAGE_QA_REGION(lastDrawUs),SAGE_QA_REGION(maxDrawUs),
  SAGE_QA_REGION(lineDisplayAt),SAGE_QA_REGION(lineClockValid),SAGE_QA_REGION(reelMs),SAGE_QA_REGION(phoenixClockAt),
  SAGE_QA_REGION(phoenixPhases),SAGE_QA_REGION(phoenixPhasesRestored),SAGE_QA_REGION(spinQuickStopped),SAGE_QA_REGION(spinLandingCoalesced),
  SAGE_QA_REGION(displayRespins),SAGE_QA_REGION(lastCollectSound),SAGE_QA_REGION(collectionNamedDone),SAGE_QA_REGION(fountainActive),
  SAGE_QA_REGION(fountainAt),SAGE_QA_REGION(fountainContinuous),SAGE_QA_REGION(fountainAwardMusic),SAGE_QA_REGION(fountainStopMs),
  SAGE_QA_REGION(fountainMusicAt),SAGE_QA_REGION(spinAward),SAGE_QA_REGION(holdOpening31),SAGE_QA_REGION(wisdomEntry30),SAGE_QA_REGION(freeIntroIsRetrigger),SAGE_QA_REGION(freeIntroOldTotal),
  SAGE_QA_REGION(holdWipeRevealed),SAGE_QA_REGION(holdEntryMeter),SAGE_QA_REGION(reelSteps),SAGE_QA_REGION(lastArrived),
  SAGE_QA_REGION(autoPlay),SAGE_QA_REGION(holdView),SAGE_QA_REGION(pendingFree),SAGE_QA_REGION(pendingHold),SAGE_QA_REGION(pendingOutro),
  SAGE_QA_REGION(pendingChained),SAGE_QA_REGION(introChained),SAGE_QA_REGION(heldMask),SAGE_QA_REGION(landedMask),
  SAGE_QA_REGION(pendingFreeCount),SAGE_QA_REGION(displayMeter),SAGE_QA_REGION(displayQueuedHolds),SAGE_QA_REGION(displayBank),
  SAGE_QA_REGION(displayWin),SAGE_QA_REGION(winFrom),SAGE_QA_REGION(winTo),SAGE_QA_REGION(winAt),SAGE_QA_REGION(statusLine),
  SAGE_QA_REGION(luckyOfferAction),SAGE_QA_REGION(winCuePending),SAGE_QA_REGION(awaitingBonusResume),SAGE_QA_REGION(lastAward),
  SAGE_QA_REGION(dirty),SAGE_QA_REGION(autoRefill),SAGE_QA_REGION(runtimeRevision),SAGE_QA_REGION(creditRevision),
  SAGE_QA_REGION(lastAddedMinor),SAGE_QA_REGION(cabinetMenuRequested)
 };
#undef SAGE_QA_REGION
 size_t total=0;for(const auto& r:regions)total+=r.bytes;result.backupBytes=uint32_t(total);
#ifdef UI_HOST_TEST
 auto* saved=static_cast<uint8_t*>(std::malloc(total));
#else
 auto* saved=static_cast<uint8_t*>(ps_malloc(total));
#endif
 if(!saved)return result;
 auto hash=[](){uint32_t h=2166136261u;for(const auto& r:regions){const auto* b=static_cast<const uint8_t*>(r.pointer);for(size_t n=0;n<r.bytes;++n)h=(h^b[n])*16777619u;}return h;};
 result.beforeHash=hash();size_t offset=0;for(const auto& r:regions){memcpy(saved+offset,r.pointer,r.bytes);offset+=r.bytes;}
 // This board pays Book/Wild/Book. A bottom non-Wild prevents an accidental
 // three-high join in the two-face fixture. The line evaluator owns eligibility.
 // Construct in place: assigning a value-initialized Presentation can put a
 // multi-kilobyte temporary on the small native loop stack.
 new(&engine.presentation)phoenix::Presentation();auto& p=engine.presentation;
 p.valid=p.hasSpinResult=p.paidSpin=p.roundCompleted=true;p.actionNumber=engine.state.actions;
 p.meterBefore=p.meterAfter=engine.state.meter;p.balanceBefore=100000000;p.stakeMinor=engine.betMinor();
 for(unsigned i=0;i<15;++i){auto& cell=engine.state.cells[i];cell=Cell{};const unsigned c=i/3,r=i%3;
  cell.symbol=c==0||c==2?Symbol::Book:c==1?(r<rows?Symbol::Wild:Symbol::Bell):c==3?Symbol::Jack:Symbol::Queen;
 }
 copy(p.beforeExpansion,engine.state.cells);
 for(uint8_t line=0;line<engine.state.lineCount;++line){const auto win=phoenix::Engine::evaluateLine(engine.state.cells,line,engine.state.betMultiplier,false,engine.state.lineCount,engine.state.coinValue);
  if(win.credits){p.lineWins[p.lineWinCount++]=win;p.winMask|=win.mask;p.lineWinMinor+=win.credits*engine.state.coinValue;}
 }
 p.expandedMask=phoenix::Engine::joinedSageMask(engine.state.cells,p.winMask);p.expandedWild=p.expandedMask!=0;
 for(unsigned c=0;c<5;++c)if(p.expandedMask&(7u<<(3*c)))p.expandedReelsMask|=uint8_t(1u<<c);
 for(unsigned i=0;i<15;++i)engine.state.cells[i].highlight=p.winMask&(1u<<i);
 copy(p.spinResult,engine.state.cells);p.actionWinMinor=p.lineWinMinor;p.balanceAfter=p.balanceBefore-p.stakeMinor+p.lineWinMinor;
 engine.state.balance=p.balanceAfter;engine.state.roundPayout=p.lineWinMinor;engine.state.mode=Mode::Base;
 engine.state.roundOpen=engine.state.freeSession=false;engine.state.freeLeft=0;
 result.winMask=p.winMask;result.expandedMask=p.expandedMask;result.expandedReels=p.expandedReelsMask;
 copy(previous,visible);holdView=holdBackdropCaptured=false;autoPlay=autoRefill=false;
 pendingFree=pendingHold=pendingOutro=pendingChained=introChained=awaitingBonusResume=winCuePending=false;
 holdOpening31.clear();wisdomEntry30={};fountainActive=false;clearSpinAward();cabinetMenuRequested=false;statusLine[0]=0;
 presentAction();
 // Start just before the last reel stops, then traverse the actual Spin ->
 // Expand -> Land branches. No Ready phase or Engine::action() is reached.
 phaseAt=uiGameMillis()-phaseMs+40;
 for(unsigned frame=0;frame<57;++frame){
#ifndef UI_HOST_TEST
  audioUpdate(uiGameMillis());
#endif
  uiFrame();++result.frames;
  if(phase==Phase::Expand){++result.expandFrames;uint16_t awarded=0;for(unsigned i=0;i<15;++i)if(visible[i].highlight)awarded|=uint16_t(1u<<i);
   result.joined|=phoenix::Engine::joinedSageMask(visible,awarded)==result.expandedMask&&result.expandedMask!=0;
   // lastDraw is the exact clock used for rasterScene, before the LCD
   // transfer's wall-time cost; sampling the clock again can select new art.
   const uint32_t bodyClock=lastDraw-phaseAt;if(bodyClock>=700)observeSageBody(result,rows,bodyClock);
  }
  for(uint16_t pixel:pixels){result.frameFNV=(result.frameFNV^uint8_t(pixel))*16777619u;result.frameFNV=(result.frameFNV^uint8_t(pixel>>8))*16777619u;}
  // Native rendering adds wall time. End on first Land before an unrelated
  // win presentation or a Ready branch can become part of this visual QA.
  if(phase==Phase::Land)break;
  delay(40);
 }
 result.pixelWitness=result.bodyFrames&&result.bodyChecked&&result.bodyMatched==result.bodyChecked&&!result.bodyMismatched&&!result.bodyInvalidFrames&&result.bodyDifferentBlue>500;
 stopReelSound();
#ifndef UI_HOST_TEST
 audioStop();
#endif
 offset=0;for(const auto& r:regions){memcpy(r.pointer,saved+offset,r.bytes);offset+=r.bytes;}
 result.afterHash=hash();result.unchanged=result.beforeHash==result.afterHash;std::free(saved);
 M5.Display.pushImage(0,0,W,H,pixels);result.ran=true;return result;
}
