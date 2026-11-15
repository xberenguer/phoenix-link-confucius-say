#ifndef PHOENIX_RNG_TESTING
#define PHOENIX_RNG_TESTING
#endif
#define PHOENIX_ENGINE_TESTING
#include "phoenix_engine.h"
#include "phoenix_crypto124/prefix.h"
#include "phoenix_crypto124/mbedtls/aes.h"
#include "phoenix_crypto124/mbedtls/ctr_drbg.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
using namespace phoenix;
static unsigned checks=0;
static void verify(bool ok) { ++checks; if(!ok){std::fprintf(stderr,"check%u failed\n",checks);std::abort();} }
// Test-only deterministic entropy, never part of the production source tree.
struct Tape { size_t calls=0, fail=0, bytes=0; unsigned counter=1;
  static bool get(void* c,uint8_t* out,size_t n) {
    auto& t=*static_cast<Tape*>(c);++t.calls;
    if(t.fail==t.calls)return false;
    for(size_t i=0;i<n;++i)out[i]=uint8_t(t.counter++);
    t.bytes+=n;return true;
  }
};
struct Reentry {
  Tape tape;
  random::Service* source;
  unsigned when, method;
  bool nested = true;
  static bool get(void* c,uint8_t* out,size_t n) {
    auto& r=*static_cast<Reentry*>(c);
    if(r.tape.calls+1==r.when) {
      if(r.method==0)r.nested=r.source->beginAction();
      if(r.method==1)r.nested=r.source->initialize(Tape::get,&r.tape);
      if(r.method==2){r.source->endAction();r.nested=false;}
      if(r.method==3){uint32_t value=0;r.nested=r.source->nextWord(value);}
    }
    return Tape::get(&r.tape,out,n);
  }
};
static bool same(const Engine& a,const Engine& b) {
  return !std::memcmp(&a.state,&b.state,sizeof(State)) &&
         !std::memcmp(&a.presentation,&b.presentation,sizeof(Presentation));
}
static void init(Engine& e,random::Service& s,Tape& t) {
  verify(s.initialize(Tape::get,&t));verify(t.calls==1&&t.bytes==48);
  verify(e.initializeFresh(s,1000));verify(t.calls==2&&t.bytes==96);
  verify(e.randomHealthy()&&e.state.balance==100000&&e.state.coinValue==100);
}
int main() {
  static_assert(!std::is_copy_constructible<random::Service>::value,"opaque service not copyable");
  static_assert(!std::is_move_constructible<random::Service>::value,"opaque service not movable");
  verify(mbedtls_aes_self_test(0)==0);verify(mbedtls_ctr_drbg_self_test(0)==0);
  for(unsigned when: {1u,2u})for(unsigned method=0;method<4;++method) {
    random::Service s;Reentry r{{},&s,when,method,true};
    const bool seeded=s.initialize(Reentry::get,&r);
    if(when==1)verify(!seeded);else {verify(seeded);verify(!s.beginAction());}
    verify(!r.nested);verify(!s.healthy());verify(s.fault()==random::Fault::Reentrant);
    s.endAction();verify(!s.beginAction());
  }
  {
    Engine e;Engine before=e;verify(!e.action());verify(same(e,before));verify(!e.randomHealthy());
    random::Service s;Tape t;t.fail=1;verify(!s.initialize(Tape::get,&t));verify(s.fault()==random::Fault::Entropy);
    verify(!e.initializeFresh(s));verify(same(e,before));verify(!e.randomHealthy());
  }
  {
    Engine e;random::Service s;Tape t;init(e,s,t);verify(!e.restore(e.snapshot()));
    Engine before=e;t.fail=t.calls+1;verify(!e.action());verify(same(e,before));
    verify(e.randomFault()==random::Fault::Entropy);verify(!e.addCredits(100));verify(!e.setWager(10,25,1));
    verify(!e.action());verify(same(e,before));
  }
  for(unsigned mode=0;mode<4;++mode)for(unsigned limit: {0u,1u,2u,4u}) {
    Engine e;random::Service s;Tape t;init(e,s,t);
    if(mode==1){e.state.mode=Mode::Free;e.state.freeLeft=3;e.state.freeSession=e.state.roundOpen=true;}
    if(mode==2){e.state.mode=Mode::Hold;e.state.respinsLeft=3;e.state.roundOpen=true;e.state.holdStartCount=6;
      for(unsigned i=0;i<6;++i){e.state.cells[i].symbol=e.state.holdCells[i].symbol=Symbol::Phoenix;
        e.state.cells[i].prize=e.state.holdCells[i].prize=10;}}
    if(mode==3)e.state.balance=e.betMinor()-1;
    Engine before=e;s.testLimitWords(limit);
    verify(!(mode==3?e.tryLuckyChance():e.action()));verify(same(e,before));
    verify(e.randomFault()==random::Fault::Exhausted);verify(!e.randomHealthy());
  }
  {
    Engine e;random::Service s;Tape t;init(e,s,t);Engine before=e;s.testLimitWords(0);
    verify(!e.setWager(10,25,1));verify(same(e,before));verify(!e.randomHealthy());
  }
  {
    Engine e;random::Service old;Tape t;init(e,old,t);Engine before=e;
    random::Service replacement;Tape other;verify(replacement.initialize(Tape::get,&other));other.fail=2;
    verify(!e.initializeFresh(replacement));verify(same(e,before));verify(!e.randomHealthy());
    verify(e.randomFault()==random::Fault::Entropy);verify(!e.action());
  }
  {
    random::Service s;Tape t;verify(s.initialize(Tape::get,&t));verify(s.beginAction());
    uint32_t zeros[64]={};verify(s.testReplaceActiveWords(zeros,64));random::Draws d(&s);
    verify(d.bounded(0x80000001u)==0);verify(s.fault()==random::Fault::RejectionLimit);s.endAction();
  }
  for(uint32_t bound: {1u,2u,3u,5u,1401u,3630u,9986u,10000u,1000000u,0x80000001u,0xffffffffu}) {
    random::Service s;Tape t;verify(s.initialize(Tape::get,&t));verify(s.beginAction());
    const uint32_t threshold=uint32_t(-bound)%bound;
    const uint32_t words[4]={threshold?threshold-1:0,threshold,0xffffffffu,0x80000000u};
    verify(s.testReplaceActiveWords(words,4));random::Draws d(&s);
    verify(d.bounded(bound)==threshold%bound);verify(s.healthy());
    if(threshold)verify(s.testWordsUsed()==2);else verify(s.testWordsUsed()==1);
    s.endAction();verify(t.calls==2&&t.bytes==96);
  }
  // Exhaustive small-width independent oracle: equal accepted residue counts.
  for(unsigned width=2;width<=12;++width)for(unsigned b=1;b<(1u<<width);++b){
    const unsigned range=1u<<width, threshold=range%b;unsigned lo=range/b;
    for(unsigned residue=0;residue<b;++residue){unsigned n=0;for(unsigned v=threshold;v<range;++v)n+=v%b==residue;verify(n==lo);}
    if(width>8&&b>40)break;
  }
  // Actual shared service, two copyable Engine handles: they do not clone a pool.
  {
    Engine e;random::Service s;Tape t;init(e,s,t);Engine copy=e;
    verify(e.action());auto first=e.snapshot();verify(t.calls==3);verify(copy.action());verify(t.calls==4);
    auto second=copy.snapshot();verify(first.length==second.length);verify(std::memcmp(first.data,second.data,first.length)!=0);
  }
  // Real random game transitions, every offered profile, explicit no-fault smoke.
  {
    Engine e;random::Service s;Tape t;init(e,s,t);
    for(size_t p=0;p<Engine::wagerCount();++p){
      while(e.state.mode!=Mode::Base||e.state.roundOpen)verify(e.action());
      const auto&w=local::wagers[p];verify(e.setWager(w.denomination,w.lines,w.multiplier));
      verify(e.addCredits(100000000));
      for(unsigned spin=0;spin<10;++spin){verify(e.action());unsigned feature=0;
        while(e.state.mode!=Mode::Base||e.state.roundOpen){verify(e.action());verify(++feature<20000);}}
    }
    verify(e.randomHealthy());
  }
  std::printf("{\"status\":\"PASS\",\"checks\":%u,\"engineBytes\":%zu,\"stateBytes\":%zu,\"presentationBytes\":%zu,\"serviceBytes\":%zu}\n",checks,sizeof(Engine),sizeof(State),sizeof(Presentation),sizeof(random::Service));
}
