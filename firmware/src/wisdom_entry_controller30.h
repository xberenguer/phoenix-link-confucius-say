#pragma once
#include <stdint.h>
#include <type_traits>
// Presentation-only normal three-Wisdom entry. No Engine, RNG or persisted
// financial field is stored here. All epochs are the existing UI game clock.
namespace wisdom_ctrl30 {
enum class Stage : uint8_t {Idle,Announcement,Accepted,FirstSpin};
struct State {
 Stage stage=Stage::Idle;
 uint16_t awarded=0;
 uint64_t triggerAction=0;
 uint32_t acceptedAt=0,motionAt=0,cueAt=0,gongAt=0;
 bool cueStarted=false,gongPending=false;
};
static_assert(std::is_trivially_copyable<State>::value,"QA can copy/restore the transient as one value");
// Source START visual boundary631.700 -> first motion632.183, a~483ms
// observed interval. Rounded500ms is an explicit presentation fit, not a
// recovered vendor constant. Manual waiting time is excluded.
constexpr uint32_t preparationMs=500;
inline bool preparing(const State& state,uint32_t now){
 return state.stage==Stage::Accepted&&uint32_t(now-state.acceptedAt)<preparationMs;
}
}
