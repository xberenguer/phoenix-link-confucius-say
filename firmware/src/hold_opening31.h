#pragma once
#include <cstdint>
#include <type_traits>

namespace hold_opening31 {
constexpr uint32_t leadMs=1000,startTimeoutMs=3000;
enum class Stage:uint8_t {Idle,Resume,Waiting,Lead,Ready};
struct Observation {bool audible=false,backendReady=false,holdPlaying=false;uint32_t queuedFrames=0;};
// Presentation-only. No wall clock, game state, engine or persistent storage.
struct State {
 uint32_t armedAt=0,startedAt=0,framesAtArm=0;
 Stage stage=Stage::Idle;
 bool pending()const{return stage!=Stage::Idle;}
 void clear(){*this=State{};}
 void resume(){clear();stage=Stage::Resume;}
 void arm(uint32_t now,uint32_t frames){armedAt=now;startedAt=0;framesAtArm=frames;stage=Stage::Waiting;}
 bool blocked(uint32_t now,const Observation& audio){
  if(stage==Stage::Idle||stage==Stage::Ready)return false;
  if(stage==Stage::Resume)return true;
  if(!audio.audible||!audio.backendReady){stage=Stage::Ready;return false;}
  if(stage==Stage::Waiting){
   // A request or a stale playing flag cannot start the audible lead timer.
   // queuedFrames is cumulative; inequality also handles its uint32 wrap.
   if(audio.holdPlaying&&audio.queuedFrames!=framesAtArm){startedAt=now;stage=Stage::Lead;}
   else if(uint32_t(now-armedAt)>=startTimeoutMs){stage=Stage::Ready;return false;}
  }
  if(stage==Stage::Lead&&uint32_t(now-startedAt)>=leadMs){stage=Stage::Ready;return false;}
  return true;
 }
};
static_assert(std::is_trivially_copyable<State>::value,"Hold opening state must preserve exactly in UI fixtures");
}
