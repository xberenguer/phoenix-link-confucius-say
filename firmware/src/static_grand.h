#pragma once
// Current-session local Grand schedule. Numeric enclosures describe only the
// ideal uniform-draw local model under its stated fixed-profile assumptions.
// They do not qualify physical entropy, actual implementation aborts, casino
// odds, arbitrary wager switching, or an infinite-horizon CSPRNG return.
#include "static_grand_legacy.h"
namespace static_grand {
using Fingerprint=legacy_static_grand::Fingerprint;
using Profile=legacy_static_grand::Profile;
using Result=legacy_static_grand::Result;
using legacy_static_grand::profile;
using legacy_static_grand::checksum;
using legacy_static_grand::u32;
constexpr size_t selectionBytes=48;
constexpr uint8_t analyticalBound=1,independentValidation=2;
// Both evidence bits concern the ideal reference model, separately from source
// and CSPRNG engineering acceptance. A failed action commits no replacement.
constexpr bool idealReferenceOnly=true;
enum class Mode:uint8_t { RetainedStaticGrand=1 };
enum class Wire:uint8_t { Legacy1=1,Neutral2=2 };
enum class Proof:uint8_t { LocalAnalyticEnclosure=1 };
struct Entry {
 Profile profile;uint8_t slot=4;int64_t grandCredits=0;
 uint32_t lowReturnPpm=0,estimateReturnPpm=0,highReturnPpm=1000000;
 Proof proof=Proof::LocalAnalyticEnclosure;uint8_t evidence=0;
};
struct Family {uint32_t engineModel=0;Fingerprint fingerprint{};const Entry*entries=nullptr;size_t count=0;};
struct Selection {Mode mode=Mode::RetainedStaticGrand;uint8_t slot=4;Fingerprint family{};};
struct Preference {Wire wire=Wire::Neutral2;Selection neutral{};legacy_static_grand::Selection legacy{};};
inline bool same(const Selection&a,const Selection&b){return a.mode==b.mode&&a.slot==b.slot&&a.family==b.family;}
inline Selection retained(const Family&f,uint8_t slot){return {Mode::RetainedStaticGrand,slot,f.fingerprint};}
inline Preference current(const Selection&s){Preference p;p.neutral=s;return p;}
inline bool qualified(const Entry&e){
 return e.slot<6&&e.proof==Proof::LocalAnalyticEnclosure&&
  (e.evidence&(analyticalBound|independentValidation))==(analyticalBound|independentValidation)&&
  e.grandCredits>0&&e.grandCredits<=phoenix::local::maximumCreditPrize&&
  e.lowReturnPpm<=e.estimateReturnPpm&&e.estimateReturnPpm<=e.highReturnPpm&&e.highReturnPpm<1000000;
}
inline Result resolve(const Family&f,const Selection&s,const Profile&p,const Entry*&out){
 out=nullptr;
 if(s.mode!=Mode::RetainedStaticGrand||s.slot>=6||f.engineModel!=phoenix::local::modelId||f.fingerprint==Fingerprint{}||s.family!=f.fingerprint)return Result::UnknownFamily;
 if(f.entries)for(size_t i=0;i<f.count;++i)if(f.entries[i].slot==s.slot&&legacy_static_grand::same(f.entries[i].profile,p)){out=&f.entries[i];break;}
 if(!out)return Result::UnsupportedProfile;
 return qualified(*out)?Result::Applied:Result::UnqualifiedEntry;
}
inline Result selectFuture(phoenix::Engine&e,Selection&active,const Selection&requested,const Family&f,bool uiReady){
 if(!uiReady)return Result::UiBusy;
 if(!e.canChangeBet())return Result::ActiveRound;
 const Entry*row=nullptr;const auto result=resolve(f,requested,profile(e),row);if(result!=Result::Applied)return result;
 const bool changed=(!e.isCustomJackpot(3)&&(e.state.jackpotUnits[3]!=phoenix::money::Unit::Credits||e.state.jackpotCoins[3]!=row->grandCredits))||!same(active,requested);
 if(!e.isCustomJackpot(3)){e.state.jackpotCoins[3]=row->grandCredits;e.state.jackpotUnits[3]=phoenix::money::Unit::Credits;}active=requested;
 return changed?Result::Applied:Result::NoChange;
}
inline Result setWager(phoenix::Engine&e,const Selection&s,const Family&f,int64_t coin,uint8_t lines,uint8_t multiplier,bool uiReady){
 if(!uiReady)return Result::UiBusy;
 if(!e.canChangeBet())return Result::ActiveRound;
 const Entry*row=nullptr;const auto result=resolve(f,s,{coin,lines,multiplier,e.state.prizeCapProfile},row);if(result!=Result::Applied)return result;
 if(!e.setWager(coin,lines,multiplier))return Result::InvalidWager;
 if(!e.isCustomJackpot(3)){e.state.jackpotCoins[3]=row->grandCredits;e.state.jackpotUnits[3]=phoenix::money::Unit::Credits;}return Result::Applied;
}
inline size_t encode(const Selection&s,uint8_t*out,size_t capacity){
 if(s.mode!=Mode::RetainedStaticGrand||s.slot>=6||s.family==Fingerprint{})return 0;
 if(!out||capacity<selectionBytes)return selectionBytes;
 std::memset(out,0,selectionBytes);u32(out,0x32524753u); // SGR2
 out[4]=uint8_t(s.mode);out[5]=s.slot;std::memcpy(out+8,s.family.data(),32);
 u32(out+40,phoenix::local::modelId);u32(out+44,checksum(out,44));return selectionBytes;
}
// Structural import parsing is deliberately distinct from current-family
// eligibility. Unknown valid preferences stay exportable while paid Base blocks.
inline bool parsePreference(const uint8_t*in,size_t size,Preference&destination,uint32_t&storedModel){
 if(!in||size!=selectionBytes)return false;
 Preference p;uint32_t model=0;
 if(u32(in)==0x31524753u){
  if(!legacy_static_grand::parsePreference(in,size,p.legacy,model))return false;p.wire=Wire::Legacy1;
 }else{
  if(u32(in)!=0x32524753u||u32(in+44)!=checksum(in,44)||in[4]!=1||in[5]>=6||in[6]||in[7])return false;
  p.neutral.mode=Mode(in[4]);p.neutral.slot=in[5];std::memcpy(p.neutral.family.data(),in+8,32);
  if(p.neutral.family==Fingerprint{})return false;model=u32(in+40);
 }
 destination=p;storedModel=model;return true;
}
inline bool decode(const uint8_t*in,size_t size,const Family&f,Selection&destination){
 Preference p;uint32_t model=0;
 if(!parsePreference(in,size,p,model)||p.wire!=Wire::Neutral2||model!=phoenix::local::modelId||f.engineModel!=model||p.neutral.family!=f.fingerprint||f.fingerprint==Fingerprint{})return false;
 destination=p.neutral;return true;
}
inline bool knownUntunedModel(uint32_t model){return model==0x2000bu||model==0x2000cu||model==0x2000du||model==0x2000eu;}
inline bool untunedLegacy(const Preference&p,uint32_t model){return p.wire==Wire::Legacy1&&p.legacy.mode==legacy_static_grand::Mode::UntunedLocal&&p.legacy.setting==legacy_static_grand::Setting::NominalTarget&&!p.legacy.requestedBps&&p.legacy.family==Fingerprint{}&&knownUntunedModel(model);}
inline bool legacySlot(const Preference&p,uint32_t model,uint8_t&slot){
 if(untunedLegacy(p,model)){slot=4;return true;}
 if(p.wire!=Wire::Legacy1||p.legacy.mode!=legacy_static_grand::Mode::CustomStaticGrand)return false;
 constexpr Fingerprint d={0x8b,0xb6,0xa7,0x7b,0x3c,0xa0,0xc1,0x9a,0xef,0x5f,0xb4,0x3b,0xbb,0xd6,0x13,0xbf,0x6d,0xfd,0xf5,0x01,0x81,0xc1,0xac,0x0c,0x3b,0x0b,0x66,0x00,0xb6,0xd1,0xb3,0xa8};
 constexpr Fingerprint e={0x46,0x50,0x03,0x51,0x01,0x11,0xf8,0xd7,0xc5,0x3c,0x89,0x4b,0x23,0x49,0x97,0x59,0x16,0xdf,0x04,0x82,0x4e,0x36,0x99,0xd4,0x27,0x8d,0x79,0xb5,0x87,0x0b,0x96,0xdd};
 if(!((model==0x2000du&&p.legacy.family==d)||(model==0x2000eu&&p.legacy.family==e)))return false;
 if(p.legacy.setting!=legacy_static_grand::Setting::NominalTarget&&p.legacy.setting!=legacy_static_grand::Setting::UpperCeiling)return false;
 const unsigned request=p.legacy.requestedBps;const unsigned part=request==9000?0:request==9500?1:request==9800?2:6;
 if(part>=3)return false;slot=uint8_t(part+(p.legacy.setting==legacy_static_grand::Setting::UpperCeiling?3:0));return true;
}
// Only these exact known SGR2/F and SGR2/G pairs are prior neutral intents.
// Unknown valid wires remain exportable but never become eligible by parsing.
// The original48bytes remain pending until qualified closedReadyBase adoption.
inline bool priorNeutralSlot(const Preference&p,uint32_t model,uint8_t&slot){
 if(p.wire!=Wire::Neutral2||p.neutral.mode!=Mode::RetainedStaticGrand||p.neutral.slot>=6)return false;
 constexpr Fingerprint f={0x83,0xc5,0xdd,0xab,0xe1,0x59,0x11,0x9b,0x8c,0xbe,0x51,0x47,0xa2,0x14,0x1c,0xd3,0x52,0x22,0x5a,0xeb,0xf7,0x35,0xdc,0x86,0xf1,0x2f,0x64,0x91,0x81,0xa9,0xfe,0x1d};
 constexpr Fingerprint g={0x5a,0xd6,0x11,0xfb,0x0d,0xfc,0x17,0x76,0x23,0x0d,0x24,0x8b,0xb2,0x30,0xa2,0x1c,0x20,0x38,0x25,0xc8,0xd0,0xb1,0xdc,0xda,0xd0,0x5c,0x5d,0x43,0x36,0x68,0xa5,0x69};
 if(!((model==phoenix::local::previousFPriceModelId&&p.neutral.family==f)||
      (model==phoenix::local::previousGPriceModelId&&p.neutral.family==g)))return false;
 slot=p.neutral.slot;return true;
}
inline bool effective(const Preference&p,uint32_t model,const Family&f,Selection&out){
 if(f.engineModel!=phoenix::local::modelId||f.fingerprint==Fingerprint{})return false;
 if(p.wire==Wire::Neutral2){
  if(model==phoenix::local::modelId&&p.neutral.mode==Mode::RetainedStaticGrand&&p.neutral.slot<6&&p.neutral.family==f.fingerprint){out=p.neutral;return true;}
  uint8_t slot=0;if(!priorNeutralSlot(p,model,slot))return false;
  out=retained(f,slot);return true;
 }
 uint8_t slot=0;if(!legacySlot(p,model,slot))return false;out=retained(f,slot);return true;
}
} // namespace static_grand
