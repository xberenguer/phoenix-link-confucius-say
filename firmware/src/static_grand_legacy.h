#pragma once
// Import/archive types, strict historical qualifier and SGR1 parser only.
// No legacy selection action or SGR1 writer exists in the F runtime.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace legacy_static_grand {
using Fingerprint = std::array<uint8_t,32>;
enum class Mode:uint8_t { UntunedLocal=0, CustomStaticGrand=1 };
enum class Setting:uint8_t { NominalTarget=0, UpperCeiling=1 };
enum class Result:uint8_t { NoChange, Applied, UiBusy, ActiveRound, UnknownFamily, UnsupportedProfile, UnqualifiedEntry, InvalidWager };
constexpr uint8_t analyticalBound=1, independentValidation=2;
constexpr size_t selectionBytes=48;

struct Profile {
  int64_t coinMinor=0;
  uint8_t lines=0, multiplier=0;
  phoenix::PrizeCapProfile cap=phoenix::PrizeCapProfile::HighCashOnly;
};
inline bool same(const Profile&a,const Profile&b){return a.coinMinor==b.coinMinor&&a.lines==b.lines&&a.multiplier==b.multiplier&&a.cap==b.cap;}
inline Profile profile(const phoenix::Engine&e){return {e.state.coinValue,e.state.lineCount,e.state.betMultiplier,e.state.prizeCapProfile};}
struct Entry {
  Profile profile;
  Setting setting=Setting::UpperCeiling;
  uint16_t requestedBps=0;
  int64_t grandCredits=0;
  uint32_t lowReturnPpm=0, estimateReturnPpm=0, highReturnPpm=1000000;
  uint32_t targetTolerancePpm=0;
  uint8_t evidence=0;
};
struct Family {
  uint32_t engineModel=0;
  Fingerprint fingerprint{};  // generated from immutable calibration/input manifest
  const Entry* entries=nullptr;
  size_t count=0;
};
struct Selection {
  Mode mode=Mode::UntunedLocal;
  Setting setting=Setting::NominalTarget;
  uint16_t requestedBps=0;
  Fingerprint family{};
};
inline bool same(const Selection&a,const Selection&b){return a.mode==b.mode&&a.setting==b.setting&&a.requestedBps==b.requestedBps&&a.family==b.family;}
inline Selection custom(const Family&f,Setting type,uint16_t bps){return {Mode::CustomStaticGrand,type,bps,f.fingerprint};}
inline bool qualified(const Entry&e){
  if((e.evidence&(analyticalBound|independentValidation))!=(analyticalBound|independentValidation) ||
     e.requestedBps==0||e.requestedBps>=10000||e.grandCredits<1||e.grandCredits>phoenix::local::maximumCreditPrize||
     e.lowReturnPpm>e.estimateReturnPpm||e.estimateReturnPpm>e.highReturnPpm||e.highReturnPpm>=1000000)return false;
  const uint32_t request=uint32_t(e.requestedBps)*100;
  if(e.setting==Setting::UpperCeiling)return e.highReturnPpm<=request;
  if(e.setting!=Setting::NominalTarget)return false;
  const uint32_t lower=request>e.targetTolerancePpm?request-e.targetTolerancePpm:0;
  const uint64_t upper=uint64_t(request)+e.targetTolerancePpm;
  return e.lowReturnPpm>=lower&&e.highReturnPpm<=upper;
}
inline uint32_t checksum(const uint8_t*b,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i){h^=b[i];h*=16777619u;}return h;}
inline void u32(uint8_t*b,uint32_t x){for(unsigned i=0;i<4;++i)b[i]=uint8_t(x>>(8*i));}
inline uint32_t u32(const uint8_t*b){uint32_t x=0;for(unsigned i=0;i<4;++i)x|=uint32_t(b[i])<<(8*i);return x;}
inline bool parsePreference(const uint8_t*in,size_t size,Selection&destination,uint32_t&storedModel){
  if(!in||size!=selectionBytes||u32(in)!=0x31524753u||u32(in+44)!=checksum(in,44))return false;
  Selection s;s.mode=Mode(in[4]);s.setting=Setting(in[5]);s.requestedBps=uint16_t(in[6])|(uint16_t(in[7])<<8);std::memcpy(s.family.data(),in+8,32);
  if(s.mode==Mode::UntunedLocal){if(s.requestedBps||s.family!=Fingerprint{}||s.setting!=Setting::NominalTarget)return false;}
  else if(s.mode==Mode::CustomStaticGrand){
    if(s.family==Fingerprint{}||s.requestedBps==0||s.requestedBps>=10000||uint8_t(s.setting)>1)return false;
  }else return false;
  destination=s;storedModel=u32(in+40);return true;
}
} // namespace legacy_static_grand
