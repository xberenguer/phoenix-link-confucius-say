#pragma once
// Include phoenix_engine.h before this additive UI/checkpoint service helper.
// The record is a sticky corruption-checked liability marker, not a math setting.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace phoenix_capacity {
enum class Attempt:uint8_t { Normal=0, Lucky=1 };
constexpr size_t recordBytes=96;
inline uint32_t fnv32(const uint8_t*p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i){h^=p[i];h*=16777619u;}return h;}
inline void put32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=uint8_t(v>>(8*i));}
inline void put64(uint8_t*p,uint64_t v){for(unsigned i=0;i<8;++i)p[i]=uint8_t(v>>(8*i));}
inline uint32_t get32(const uint8_t*p){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(p[i])<<(8*i);return v;}
inline uint64_t get64(const uint8_t*p){uint64_t v=0;for(unsigned i=0;i<8;++i)v|=uint64_t(p[i])<<(8*i);return v;}

class Latch {
 public:
  bool active()const{return reasons_!=0;}
  uint32_t reasonMask()const{return reasons_;}
  Attempt attemptKind()const{return attempt_;}
  uint32_t gameHash()const{return gameHash_;}
  uint32_t gameLength()const{return gameLength_;}
  uint64_t actionNumber()const{return actions_;}
  int64_t reserveMinor()const{return reserve_;}
  // Runtime initialization only. Never bind this to a UI clear/reset button.
  void clear(){*this=Latch{};}
  bool capture(const phoenix::Engine&e,Attempt attempt){
    if(!e.capacityBlocked()||uint8_t(attempt)>1||e.capacityFaultMask()!=e.capacityPreflightMask(attempt==Attempt::Lucky))return false;
    return capturePreflight(e,attempt);
  }
  // Administrative caller guard: a wager change cannot wrap the action
  // counter even though it does not itself execute a spin or debit a stake.
  bool capturePreflight(const phoenix::Engine&e,Attempt attempt){
    if(active()||uint8_t(attempt)>1||!e.capacityPreflightMask(attempt==Attempt::Lucky))return false;
    const auto game=e.snapshot();if(!game.length)return false;
    Latch next;next.reasons_=e.capacityPreflightMask(attempt==Attempt::Lucky);next.attempt_=attempt;
    next.gameLength_=uint32_t(game.length);next.gameHash_=fnv32(game.data,game.length);
    next.mode_=e.state.mode;next.actions_=e.state.actions;next.rounds_=e.state.rounds;
    next.balance_=e.state.balance;next.roundPayout_=e.state.roundPayout;next.featureWin_=e.state.featureWin;
    next.reserve_=e.capacityReserveMinor();next.stake_=attempt==Attempt::Lucky?e.state.balance:e.betMinor();
    next.paid_=e.state.stats.paidMinor;
    if(next.reasons_!=e.capacityPreflightMask(attempt==Attempt::Lucky))return false;
    *this=next;return true;
  }
  // Caller treats active()+encode()==0 as a checkpoint failure, never omission.
  size_t encode(const void*game,size_t gameSize,void*out,size_t capacity)const{
    if(!active())return 0;
    if(!game||gameSize!=gameLength_||fnv32(static_cast<const uint8_t*>(game),gameSize)!=gameHash_)return 0;
    if(!out||capacity<recordBytes)return recordBytes;
    auto*p=static_cast<uint8_t*>(out);std::memset(p,0,recordBytes);
    put32(p,0x31465043u);p[4]=1;p[6]=uint8_t(recordBytes); // CPF1, v1, LE96
    put32(p+8,gameLength_);put32(p+12,gameHash_);put32(p+16,reasons_);
    p[20]=uint8_t(mode_);p[21]=uint8_t(attempt_);
    put64(p+24,actions_);put64(p+32,rounds_);put64(p+40,uint64_t(balance_));
    put64(p+48,uint64_t(roundPayout_));put64(p+56,uint64_t(featureWin_));
    put64(p+64,uint64_t(reserve_));put64(p+72,uint64_t(stake_));put64(p+80,paid_);
    put32(p+88,phoenix::local::modelId);put32(p+92,fnv32(p,92));return recordBytes;
  }
  static bool decode(const void*source,size_t length,const void*game,size_t gameSize,const phoenix::Engine&e,Latch&destination){
    if(length==0){destination.clear();return true;}
    if(!source||length!=recordBytes||!game||gameSize<20)return false;
    const auto*p=static_cast<const uint8_t*>(source);
    const auto*raw=static_cast<const uint8_t*>(game);
    const uint32_t storedModel=get32(p+88);
    const bool knownModel=storedModel==phoenix::local::modelId||storedModel==phoenix::local::previousGPriceModelId||storedModel==phoenix::local::previousFPriceModelId||storedModel==phoenix::local::previousEPriceModelId||storedModel==phoenix::local::previousPlazaPriceModelId||storedModel==phoenix::local::previousBellPriceModelId||storedModel==phoenix::local::previousVegasPriceModelId;
    if(get32(p)!=0x31465043u||p[4]!=1||p[5]||p[6]!=recordBytes||p[7]||p[22]||p[23]||
       get32(p+92)!=fnv32(p,92)||!knownModel||
       get32(raw)!=0x46434c50u||get32(raw+4)!=0x00100004u||get32(raw+8)!=gameSize||
       get32(raw+12)!=storedModel||get32(raw+gameSize-4)!=fnv32(raw,gameSize-4)||
       get32(p+8)!=gameSize||get32(p+12)!=fnv32(static_cast<const uint8_t*>(game),gameSize)||
       !get32(p+16)||(get32(p+16)&~phoenix::kCapacityKnownFaults)||p[20]>uint8_t(phoenix::Mode::Hold)||p[21]>1)return false;
    // Engine restore can clear its transient reason bits, so check the pure
    // preflight against its saved financial state rather than that transient.
    const auto actual=e.snapshot();
    // H recognizes the same B/C/D/E/F/G wire body. Validate the original header/CRC
    // above and every saved body/RNG byte here, without repricing a liability.
    if(actual.length!=gameSize||std::memcmp(actual.data+16,raw+16,gameSize-20))return false;
    Latch next;next.gameLength_=get32(p+8);next.gameHash_=get32(p+12);next.reasons_=get32(p+16);
    next.mode_=phoenix::Mode(p[20]);next.attempt_=Attempt(p[21]);
    next.actions_=get64(p+24);next.rounds_=get64(p+32);next.balance_=int64_t(get64(p+40));
    next.roundPayout_=int64_t(get64(p+48));next.featureWin_=int64_t(get64(p+56));
    next.reserve_=int64_t(get64(p+64));next.stake_=int64_t(get64(p+72));next.paid_=get64(p+80);
    if(next.mode_!=e.state.mode||next.actions_!=e.state.actions||next.rounds_!=e.state.rounds||
       next.balance_!=e.state.balance||next.roundPayout_!=e.state.roundPayout||next.featureWin_!=e.state.featureWin||
       next.reserve_!=e.capacityReserveMinor()||next.stake_!=(next.attempt_==Attempt::Lucky?e.state.balance:e.betMinor())||
       next.paid_!=e.state.stats.paidMinor||next.reasons_!=e.capacityPreflightMask(next.attempt_==Attempt::Lucky))return false;
    // Only binding metadata moves to the current normalized model. A later
    // canonical save can commit the same liability alongside the H header.
    next.gameLength_=uint32_t(actual.length);next.gameHash_=fnv32(actual.data,actual.length);
    destination=next;return true;
  }
 private:
  uint32_t reasons_=0,gameLength_=0,gameHash_=0;
  phoenix::Mode mode_=phoenix::Mode::Base;Attempt attempt_=Attempt::Normal;
  uint64_t actions_=0,rounds_=0,paid_=0;
  int64_t balance_=0,roundPayout_=0,featureWin_=0,reserve_=0,stake_=0;
};
} // namespace phoenix_capacity
