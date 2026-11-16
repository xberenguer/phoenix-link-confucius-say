#ifndef PHOENIX_RNG_TESTING
#error This executable requires the test-only build; never link it into firmware or WASM.
#endif
#include "phoenix_engine.h"
#include "cabinet_profile.h"
#include "static_grand_table.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

using namespace phoenix;
static uint64_t checks=0;
static const char* group="setup";
static unsigned context[8]={}; // offer,slot,cap,mode,history,tape,step,detail
static void check(bool ok,const char* expression,int line){
 ++checks;if(ok)return;
 std::fprintf(stderr,"FAIL %s line%d: %s [",group,line,expression);
 for(unsigned i:context)std::fprintf(stderr,"%u,",i);
 std::fprintf(stderr,"]\n");std::abort();
}
#define CHECK(value) check(bool(value),#value,__LINE__)

// Intentionally deterministic public TEST INPUT, not an entropy source and not
// a PRNG used by the game. Both independent real MbedTLS Services see identical
// callback bytes. This file is compiled only under PHOENIX_RNG_TESTING.
struct TestTape {
 uint64_t counter=1,calls=0,bytes=0;
 unsigned tag=1;
 static bool get(void* context,uint8_t* out,size_t count){
  auto&t=*static_cast<TestTape*>(context);++t.calls;
  if(count!=48)return false;
  for(size_t i=0;i<count;++i){const uint64_t k=t.counter++;out[i]=uint8_t(k*37+t.tag+(k>>8)*11);}
  t.bytes+=count;return true;
 }
};
struct Offer {int64_t coin;uint8_t lines,mult;};
struct Offsets {
 int64_t bank=0;uint64_t actions=0,rounds=0;
 Statistics stats{};
};
struct Coverage {
 uint64_t traces=0,pairedActions=0,modes[3]={},nonzeroAwards=0;
 uint64_t holdEntered=0,holdSettled=0,freeAwarded=0,freeEnded=0;
 uint64_t meterTriggered=0,grandAwarded=0,chained=0,revived=0;
 uint64_t naturalRounds=0,owedContinuations=0,capacityRefusals=0,capacityBoundaries=0;
 uint64_t meterControls=0,meterTransitionDifferences=0,freeControls=0,freeBoardDifferences=0;
} counts;

#define EACH_STAT(X) X(paidRounds) X(paidHits) X(freeAwards) X(freeSpins) \
 X(holdAwards) X(meterAwards) X(grandAwards) X(stakedMinor) X(paidMinor) X(luckyAttempts) X(luckyWins)

static bool sameCell(const Cell&a,const Cell&b){
 return a.symbol==b.symbol&&a.kind==b.kind&&a.multiplier==b.multiplier&&
        a.highlight==b.highlight&&a.prize==b.prize;
}
static bool sameCells(const Cell*a,const Cell*b){
 for(unsigned i=0;i<kCells;++i)if(!sameCell(a[i],b[i]))return false;
 return true;
}
static void sameSnapshot(const Engine&a,const Engine&b){
 const auto x=a.snapshot(),y=b.snapshot();
 CHECK(x.length>0&&x.length==y.length);
 CHECK(std::equal(x.data,x.data+x.length,y.data)); // canonical bytes, not struct padding
}
static void subtractOffsets(Engine&e,const Offsets&o){
 CHECK(e.state.balance>=o.bank&&e.state.actions>=o.actions&&e.state.rounds>=o.rounds);
 e.state.balance-=o.bank;e.state.actions-=o.actions;e.state.rounds-=o.rounds;
#define SUB(name) CHECK(e.state.stats.name>=o.stats.name);e.state.stats.name-=o.stats.name;
 EACH_STAT(SUB)
#undef SUB
}
static void samePresentation(const Presentation&a,const Presentation&b,const Offsets&o){
 for(unsigned i=0;i<kCells;++i){
  CHECK(sameCell(a.before[i],b.before[i]));CHECK(sameCell(a.beforeExpansion[i],b.beforeExpansion[i]));
  CHECK(sameCell(a.spinResult[i],b.spinResult[i]));CHECK(sameCell(a.heldBefore[i],b.heldBefore[i]));
  CHECK(sameCell(a.heldResult[i],b.heldResult[i]));CHECK(sameCell(a.nextHeldResult[i],b.nextHeldResult[i]));
  CHECK(a.meterAfterCell[i]==b.meterAfterCell[i]);CHECK(a.queuedAfterCell[i]==b.queuedAfterCell[i]);
 }
#define SAME(name) CHECK(a.name==b.name);
 SAME(beforeMode) SAME(afterMode) SAME(valid) SAME(paidSpin) SAME(freeSpin) SAME(holdRespin)
 SAME(freeAwarded) SAME(freeEnded) SAME(holdEntered) SAME(holdSettled) SAME(meterTriggered)
 SAME(grandAwarded) SAME(roundCompleted) SAME(chainedHoldEntered) SAME(holdFromMeter)
 SAME(holdRevived) SAME(revived) SAME(luckyAttempt) SAME(luckyWon) SAME(twinSpin)
 SAME(hasSpinResult) SAME(hasHeldResult) SAME(expandedWild) SAME(expandedReelsMask)
 SAME(freeAwardedCount) SAME(scatterCount) SAME(phoenixCount) SAME(respinsBefore)
 SAME(respinsAfter) SAME(lineWinCount) SAME(freeBefore) SAME(freeAfter) SAME(meterBefore)
 SAME(meterAfter) SAME(meterAwardsCount) SAME(meterArrivalMask) SAME(pendingHoldsBefore)
 SAME(pendingHoldsAfter) SAME(landedMask) SAME(heldMask) SAME(winMask) SAME(scatterMask)
 SAME(expandedMask) SAME(stakeMinor) SAME(lineWinMinor) SAME(scatterWinMinor)
 SAME(holdWinMinor) SAME(phoenixWinMinor) SAME(actionWinMinor)
#undef SAME
 CHECK(b.balanceBefore-o.bank==a.balanceBefore);
 CHECK(b.balanceAfter-o.bank==a.balanceAfter);
 CHECK(b.actionNumber-o.actions==a.actionNumber);
 CHECK(b.balanceAfter-b.balanceBefore==a.balanceAfter-a.balanceBefore);
 for(unsigned i=0;i<kMaxLines;++i){const auto&x=a.lineWins[i];const auto&y=b.lineWins[i];
  CHECK(x.line==y.line);CHECK(x.count==y.count);CHECK(x.symbol==y.symbol);
  CHECK(x.mask==y.mask);CHECK(x.credits==y.credits);
 }
}
static void equalAfter(const Engine&a,const Engine&b,const Offsets&o){
 Engine normalized=b;subtractOffsets(normalized,o);sameSnapshot(a,normalized);
 samePresentation(a.presentation,b.presentation,o);
 CHECK(a.randomHealthy()&&b.randomHealthy());
 CHECK(a.randomFault()==random::Fault::None&&b.randomFault()==random::Fault::None);
 CHECK(a.capacityFaultMask()==b.capacityFaultMask());
}
static std::vector<Offer> offers(){
 std::vector<Offer> result;
 for(const auto&coin:cabinet_profile::coins)for(unsigned line=0;line<coin.count;++line){
  const auto&row=coin.rows[line];CHECK(row.multipliers&&row.count==5);
  for(unsigned i=0;i<row.count;++i){
   Offer next{coin.minor,coin.lines[line],row.multipliers[i]};
   CHECK(cabinet_profile::offered(next.coin,next.lines,next.mult));result.push_back(next);
  }
 }
 CHECK(result.size()==55);CHECK(Engine::wagerCount()==50);return result;
}
static void initialize(Engine&e,random::Service&s,TestTape&t,const Offer&w,unsigned slot,unsigned cap,int64_t bank){
 CHECK(s.initialize(TestTape::get,&t));CHECK(t.calls==1);
 CHECK(e.initializeFresh(s,1000));CHECK(t.calls==2);
 e.state.prizeCapProfile=PrizeCapProfile(cap);
 const auto selection=static_grand::retained(static_grand_table::family,uint8_t(slot));
 CHECK(static_grand::setWager(e,selection,static_grand_table::family,w.coin,w.lines,w.mult,true)==static_grand::Result::Applied);
 const static_grand::Entry* row=nullptr;
 CHECK(static_grand::resolve(static_grand_table::family,selection,static_grand::profile(e),row)==static_grand::Result::Applied);
 CHECK(row&&e.state.jackpotCoins[3]==row->grandCredits);
 const auto before=t.calls;CHECK(e.setCredits(bank));CHECK(t.calls==before);
}
static void fixture(Engine&e,Mode mode,unsigned variant){
 auto&s=e.state;
 s.rounds=20;s.actions=41;
 s.stats={};s.stats.paidRounds=20;s.stats.paidHits=4;s.stats.freeAwards=2;
 s.stats.freeSpins=12;s.stats.holdAwards=3;s.stats.meterAwards=5;
 s.stats.stakedMinor=uint64_t(e.betMinor())*20;s.stats.paidMinor=uint64_t(e.betMinor())*7;
 s.stats.luckyAttempts=3;s.stats.luckyWins=1;
 s.mode=mode;s.freeLeft=s.freePlayed=0;s.respinsLeft=0;
 s.freeSession=s.roundOpen=false;s.win=s.featureWin=s.roundPayout=0;
 s.pendingMeterHolds=0;s.holdFromMeter=false;s.grandAwarded=false;
 if(mode==Mode::Base)return;
 s.roundOpen=true;
 s.roundPayout=e.betMinor()*5;s.featureWin=e.betMinor()*3;
 if(mode==Mode::Free){
  s.freeSession=true;s.freeLeft=variant==0?1:4;s.freePlayed=uint16_t(6-s.freeLeft);return;
 }
 // A new natural/meter Hold has not paid its own prizes yet. Only the nested
 // Free context below carries a preceding Free-feature subtotal.
 s.featureWin=0;
 const unsigned occupied=variant==1?0:variant==2?14:6;
 for(unsigned i=0;i<kCells;++i){s.cells[i]=s.holdCells[i]=Cell{};
  if(i<occupied){auto&c=s.holdCells[i];c.symbol=Symbol::Phoenix;
   c.prize=s.betCoins*int64_t(1+i%5);s.cells[i]=c;
  }
 }
 if(variant==2){s.holdCells[0].kind=PrizeKind::Mini;s.holdCells[0].prize=s.jackpotCoins[0];
  s.holdCells[0].multiplier=3;s.cells[0]=s.holdCells[0];}
 s.holdStartCount=uint8_t(occupied);s.holdNewCount=0;s.holdRevivals=0;
 s.respinsLeft=(variant==1||variant==2)?1:3;s.holdFromMeter=variant==1;
 if(variant==3){s.pendingMeterHolds=2;s.freeLeft=3;s.freePlayed=3;s.freeSession=true;s.featureWin=e.betMinor()*3;}
 CHECK(Engine::countMask(Engine::phoenixMask(s.holdCells))==occupied);
 CHECK(occupied>=local::phoenixHoldTrigger||s.holdFromMeter);
}
static Offsets history(Engine&b,unsigned which,int64_t bank){
 Offsets o;o.bank=bank;
 if(which==0)return o;
 o.actions=which==1?20000000:80000000;o.rounds=which==1?1000000:3000000;
 o.stats.paidRounds=o.rounds;o.stats.paidHits=which==1?30000:2000000;
 o.stats.freeAwards=1000;o.stats.freeSpins=6000;o.stats.holdAwards=2000;
 o.stats.meterAwards=900;o.stats.grandAwards=which==1?1:10;
 o.stats.stakedMinor=1000000000000ULL;o.stats.paidMinor=which==1?10000000000ULL:100000000000000ULL;
 o.stats.luckyAttempts=100;o.stats.luckyWins=20;
 b.state.actions+=o.actions;b.state.rounds+=o.rounds;
#define ADD(name) b.state.stats.name+=o.stats.name;
 EACH_STAT(ADD)
#undef ADD
 return o;
}
static void observe(const Engine&e){
 const auto&p=e.presentation;++counts.pairedActions;++counts.modes[unsigned(p.beforeMode)];
 CHECK(p.actionWinMinor==e.state.win&&p.balanceAfter==e.state.balance);
 // A queued next Hold deliberately changes current state; its prior result
 // remains in the separate committed presentation buffers.
 if(p.hasSpinResult&&!p.chainedHoldEntered)CHECK(sameCells(e.state.cells,p.spinResult));
 if(p.hasHeldResult&&!p.chainedHoldEntered)CHECK(sameCells(e.state.holdCells,p.heldResult));
 counts.nonzeroAwards+=p.actionWinMinor!=0;counts.holdEntered+=p.holdEntered;
 counts.holdSettled+=p.holdSettled;counts.freeAwarded+=p.freeAwarded;counts.freeEnded+=p.freeEnded;
 counts.meterTriggered+=p.meterTriggered;counts.grandAwarded+=p.grandAwarded;
 counts.chained+=p.chainedHoldEntered;counts.revived+=p.holdRevived;
}
static void stepPair(Engine&a,Engine&b,TestTape&ta,TestTape&tb,const Offsets&o){
 const auto ac=ta.calls,bc=tb.calls;
 CHECK(a.action());CHECK(b.action());CHECK(ta.calls==ac+1&&tb.calls==bc+1);
 CHECK(ta.calls==tb.calls&&ta.bytes==tb.bytes&&ta.counter==tb.counter);
 equalAfter(a,b,o);observe(a);
}
static void matrix(const std::vector<Offer>&all){
 group="matrix";const unsigned tags[]={1,17,103,251};
 for(unsigned wi=0;wi<all.size();++wi)for(unsigned slot=0;slot<6;++slot)
 for(unsigned cap=0;cap<2;++cap)for(unsigned mode=0;mode<3;++mode)
 for(unsigned hist=0;hist<3;++hist)for(unsigned ti=0;ti<4;++ti){
  context[0]=wi;context[1]=slot;context[2]=cap;context[3]=mode;context[4]=hist;context[5]=ti;
  random::Service sa,sb;TestTape ta,tb;ta.tag=tb.tag=tags[ti];Engine a,b;
  const int64_t bank=1000000000LL,offset=(hist+1)*111111111111LL;
  initialize(a,sa,ta,all[wi],slot,cap,bank);initialize(b,sb,tb,all[wi],slot,cap,bank+offset);
  fixture(a,Mode(mode),ti);fixture(b,Mode(mode),ti);const auto o=history(b,hist,offset);
  Engine normalized=b;subtractOffsets(normalized,o);sameSnapshot(a,normalized);
  ++counts.traces;
  for(unsigned step=0;step<4;++step){context[6]=step;stepPair(a,b,ta,tb,o);}
 }
}
static void setMeter(Engine&e,uint16_t counter,uint16_t threshold);
static void naturalRounds(const std::vector<Offer>&all){
 group="complete actual rounds from disclosed near-trigger meter";
 for(unsigned wi=0;wi<all.size();++wi){
  context[0]=wi;random::Service sa,sb;TestTape ta,tb;ta.tag=tb.tag=0x10000+wi;Engine a,b;
  initialize(a,sa,ta,all[wi],4,1,1000000000LL);initialize(b,sb,tb,all[wi],4,1,112111111111LL);
  setMeter(a,1886,1887);setMeter(b,1886,1887);
  const auto o=history(b,2,111111111111LL);
  for(unsigned round=0;round<32;++round){
   context[6]=round;CHECK(a.canChangeBet()&&b.canChangeBet());stepPair(a,b,ta,tb,o);
   unsigned feature=0;
   while(!a.canChangeBet()){CHECK(++feature<20000);stepPair(a,b,ta,tb,o);}
   ++counts.naturalRounds;
  }
 }
}
static void owedContinuations(const std::vector<Offer>&all){
 group="complete owed Hold-chain and Free continuation";
 for(unsigned wi=0;wi<all.size();++wi)for(unsigned cap=0;cap<2;++cap){
  context[0]=wi;context[2]=cap;random::Service sa,sb;TestTape ta,tb;
  ta.tag=tb.tag=0x20000+2*wi+cap;Engine a,b;
  initialize(a,sa,ta,all[wi],4,cap,1000000000LL);
  initialize(b,sb,tb,all[wi],4,cap,112111111111LL);
  fixture(a,Mode::Hold,3);fixture(b,Mode::Hold,3);const auto o=history(b,1,111111111111LL);
  unsigned steps=0;
  do{context[6]=steps;CHECK(++steps<2000);stepPair(a,b,ta,tb,o);}while(!a.canChangeBet());
  CHECK(!b.state.roundOpen&&b.state.mode==Mode::Base&&b.state.freeLeft==0);
  CHECK(a.state.pendingMeterHolds==0&&b.state.pendingMeterHolds==0);++counts.owedContinuations;
 }
}
static bool applies(unsigned bit,Mode mode){
 if(bit==1||bit==2)return mode!=Mode::Base;
 if(bit==9||bit==10||bit==11)return mode==Mode::Base;
 if(bit==12)return mode==Mode::Free;
 if(bit==13)return mode!=Mode::Hold;
 return bit<14;
}
static void exhaust(Engine&e,unsigned bit){
 const auto U=std::numeric_limits<uint64_t>::max();const auto I=local::maximumStoredMinor;
 switch(bit){
  case 0:e.state.balance=I;break;case 1:e.state.roundPayout=I;break;case 2:e.state.featureWin=I;break;
  case 3:e.state.stats.paidMinor=U;break;case 4:e.state.actions=U;break;
  case 5:e.state.stats.holdAwards=U;break;case 6:e.state.stats.grandAwards=U;break;
  case 7:e.state.stats.paidHits=U;break;case 8:e.state.stats.meterAwards=U;break;
  case 9:e.state.stats.stakedMinor=U;break;case 10:e.state.stats.paidRounds=U;break;
  case 11:e.state.rounds=U;break;case 12:e.state.stats.freeSpins=U;break;
  case 13:e.state.stats.freeAwards=U;break;default:CHECK(false);
 }
}
static void capacity(const std::vector<Offer>&all){
 group="capacity refusal";
 for(unsigned wi=0;wi<all.size();++wi)for(unsigned mode=0;mode<3;++mode)
 for(unsigned bit=0;bit<14;++bit)if(applies(bit,Mode(mode))){
  context[0]=wi;context[3]=mode;context[7]=bit;
  random::Service s;TestTape t;Engine e;initialize(e,s,t,all[wi],4,1,1000000000LL);
  fixture(e,Mode(mode),0);exhaust(e,bit);Engine before=e;
  const auto calls=t.calls,bytes=t.bytes;const uint32_t expected=1u<<bit;
  CHECK(e.capacityPreflightMask()==expected);CHECK(!e.action());CHECK(e.capacityFaultMask()==expected);
  sameSnapshot(e,before);samePresentation(e.presentation,before.presentation,{});
  CHECK(t.calls==calls&&t.bytes==bytes);CHECK(e.randomHealthy());++counts.capacityRefusals;
 }
 group="exact capacity boundary";
 for(unsigned wi=0;wi<all.size();++wi)for(unsigned mode=0;mode<3;++mode){
  context[0]=wi;context[3]=mode;random::Service sa,sb;TestTape ta,tb;Engine a,b;
  initialize(a,sa,ta,all[wi],4,1,1000000000LL);initialize(b,sb,tb,all[wi],4,1,1000000000LL);
  fixture(a,Mode(mode),0);fixture(b,Mode(mode),0);
  b.state.balance=local::maximumStoredMinor-b.capacityReserveMinor()+(mode==0?b.betMinor():0);
  Offsets o;o.bank=b.state.balance-a.state.balance;
  CHECK(a.capacityPreflightMask()==0&&b.capacityPreflightMask()==0);
  stepPair(a,b,ta,tb,o);++counts.capacityBoundaries;
 }
}
static void setMeter(Engine&e,uint16_t counter,uint16_t threshold){
 e.state.meter=counter;e.state.meterThreshold=threshold;
 size_t d=0,k=0;
 for(size_t i=0;i<local::denominationCount;++i)if(local::denominations[i]==e.state.coinValue)d=i;
 for(size_t i=0;i<local::meterBetCount;++i)if(local::meterBetCredits[i]==e.state.betCoins)k=i;
 const size_t index=local::meterSharedAcrossWagers?0:d*local::meterBetCount+k;
 e.state.meterBanks[index]={counter,threshold};
}
static void positiveControls(const std::vector<Offer>&all){
 group="legitimate state positive controls";
 const unsigned tags[]={1,17,103,251};const Offer&w=all[0];
 for(unsigned tag:tags){
  random::Service sa,sb;TestTape ta,tb;ta.tag=tb.tag=tag;Engine a,b;
  initialize(a,sa,ta,w,4,1,1000000000LL);initialize(b,sb,tb,w,4,1,1000000000LL);
  // Fixed equal-entropy action requests, each with the same fresh public
  // financial fixture. Repeated requests do not depend on finding a winner.
  const State clean=a.state;
  for(unsigned repeat=0;repeat<16;++repeat){
   a.state=b.state=clean;setMeter(a,0,1887);setMeter(b,1886,1887);
   CHECK(a.action()&&b.action());CHECK(ta.calls==tb.calls&&ta.bytes==tb.bytes);
   CHECK(sameCells(a.presentation.spinResult,b.presentation.spinResult));
   ++counts.meterControls;
   if(a.presentation.phoenixCount){
    CHECK(!a.presentation.meterTriggered&&b.presentation.meterTriggered);
    CHECK(a.presentation.meterAwardsCount==0&&b.presentation.meterAwardsCount>0);
    ++counts.meterTransitionDifferences;
   }
  }
 }
 CHECK(counts.meterTransitionDifferences>0);
 for(unsigned tag:tags){
  random::Service sa,sb;TestTape ta,tb;ta.tag=tb.tag=tag;Engine a,b;
  initialize(a,sa,ta,w,4,1,1000000000LL);initialize(b,sb,tb,w,4,1,1000000000LL);
  fixture(a,Mode::Base,0);fixture(b,Mode::Free,0);
  CHECK(a.action()&&b.action());CHECK(a.presentation.paidSpin&&!a.presentation.freeSpin);
  CHECK(b.presentation.freeSpin&&!b.presentation.paidSpin);CHECK(a.presentation.stakeMinor==a.betMinor());
  CHECK(b.presentation.stakeMinor==0&&b.presentation.freeBefore==1);
  if(!sameCells(a.presentation.spinResult,b.presentation.spinResult))++counts.freeBoardDifferences;
  ++counts.freeControls;
 }
 CHECK(counts.freeBoardDifferences>0);
}
int main(){
 static_assert(!std::is_copy_constructible<random::Service>::value,"Services must be independent owners");
 CHECK(static_grand_table::rootAccepted&&!static_grand_table::hostTrial);
 const auto all=offers();matrix(all);naturalRounds(all);owedContinuations(all);capacity(all);positiveControls(all);
 CHECK(counts.traces==23760&&counts.naturalRounds==1760);
 CHECK(counts.capacityRefusals==1705&&counts.capacityBoundaries==165);
 CHECK(counts.owedContinuations==110&&counts.holdEntered>0&&counts.holdSettled>0);
 CHECK(counts.meterTriggered>0&&counts.chained>0&&counts.freeAwarded>0&&counts.freeEnded>0);
 std::printf("{\"status\":\"PASS\",\"checks\":%llu,\"cabinetOffers\":%zu,\"legacyPresets\":%zu,"
  "\"grandSlots\":6,\"prizeCaps\":2,\"matrixTraces\":%llu,\"pairedActions\":%llu,"
  "\"actionsByMode\":[%llu,%llu,%llu],\"nonzeroAwards\":%llu,\"holdEntered\":%llu,"
  "\"holdSettled\":%llu,\"freeAwarded\":%llu,\"freeEnded\":%llu,\"meterTriggered\":%llu,"
  "\"grandAwarded\":%llu,\"chainedHolds\":%llu,\"revivals\":%llu,\"naturalRounds\":%llu,\"owedContinuations\":%llu,"
  "\"capacityRefusals\":%llu,\"capacityBoundaries\":%llu,\"meterControls\":%llu,"
  "\"meterTransitionDifferences\":%llu,\"freeControls\":%llu,\"freeBoardDifferences\":%llu}\n",
  (unsigned long long)checks,all.size(),Engine::wagerCount(),
  (unsigned long long)counts.traces,(unsigned long long)counts.pairedActions,
  (unsigned long long)counts.modes[0],(unsigned long long)counts.modes[1],(unsigned long long)counts.modes[2],
  (unsigned long long)counts.nonzeroAwards,(unsigned long long)counts.holdEntered,
  (unsigned long long)counts.holdSettled,(unsigned long long)counts.freeAwarded,
  (unsigned long long)counts.freeEnded,(unsigned long long)counts.meterTriggered,
  (unsigned long long)counts.grandAwarded,(unsigned long long)counts.chained,(unsigned long long)counts.revived,
  (unsigned long long)counts.naturalRounds,(unsigned long long)counts.owedContinuations,(unsigned long long)counts.capacityRefusals,
  (unsigned long long)counts.capacityBoundaries,(unsigned long long)counts.meterControls,
  (unsigned long long)counts.meterTransitionDifferences,(unsigned long long)counts.freeControls,
  (unsigned long long)counts.freeBoardDifferences);
}
