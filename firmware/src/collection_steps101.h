#pragma once
#include "phoenix_engine.h"
#include <cstdint>

// Presentation plan only. A Mini/Minor multiplier contributes its single
// prize in successive visible increments, without paying or editing it here.
// Study104 pins source ordering/timing separately; this helper owns no clock and computes caller-supplied relative timing.
namespace collection_steps101 {
constexpr unsigned cellCount=15;
constexpr unsigned maximumSteps=cellCount*3;
struct Step {
  int cell=-1;
  unsigned repetition=0;
  unsigned repetitions=0;
  int64_t credits=0;
  explicit operator bool()const{return cell>=0;}
};
inline unsigned repetitions(const phoenix::Cell& cell){
  if(cell.symbol!=phoenix::Symbol::Phoenix||cell.kind==phoenix::PrizeKind::Major)return 0;
  if(cell.kind==phoenix::PrizeKind::Mini||cell.kind==phoenix::PrizeKind::Minor)return cell.multiplier;
  return 1;
}
inline unsigned count(const phoenix::Cell* cells){
  unsigned n=0;for(unsigned i=0;i<cellCount;++i)n+=repetitions(cells[i]);return n;
}
inline Step at(const phoenix::Cell* cells,int number){
  if(number<0)return {};
  for(unsigned i=0;i<cellCount;++i){
    const auto repeats=repetitions(cells[i]);
    if(unsigned(number)<repeats){
      const auto& cell=cells[i];
      const auto unit=cell.kind==phoenix::PrizeKind::Mini||cell.kind==phoenix::PrizeKind::Minor?
                      cell.prize:phoenix::Engine::cellCredits(cell);
      return {int(i),unsigned(number),repeats,unit};
    }
    number-=int(repeats);
  }
  return {};
}
inline int64_t creditsAfter(const phoenix::Cell* cells,unsigned completed){
  int64_t amount=0;
  for(unsigned i=0;i<cellCount&&completed;++i){
    const auto repeats=repetitions(cells[i]);
    const auto take=completed<repeats?completed:repeats;
    if(take){
      const auto& cell=cells[i];
      amount+=(cell.kind==phoenix::PrizeKind::Mini||cell.kind==phoenix::PrizeKind::Minor)?
               cell.prize*take:phoenix::Engine::cellCredits(cell);
      completed-=take;
    }
  }
  return amount;
}
// The duration after an intermediate subaward is the shorter repeat interval;
// the last subaward retains the normal pause before the next physical cell.
inline uint32_t interval(const Step& step,uint32_t normalMs,uint32_t repeatMs){
  return step.repetition+1<step.repetitions?repeatMs:normalMs;
}
inline uint32_t startAt(const phoenix::Cell* cells,int number,uint32_t normalMs,uint32_t repeatMs){
  if(number<0)return UINT32_MAX;
  uint32_t elapsed=0;
  for(int i=0;;++i){const auto step=at(cells,i);if(!step)return UINT32_MAX;if(i==number)return elapsed;elapsed+=interval(step,normalMs,repeatMs);}
}
inline uint32_t duration(const phoenix::Cell* cells,uint32_t normalMs,uint32_t repeatMs){
  uint32_t elapsed=0;for(int i=0;;++i){const auto step=at(cells,i);if(!step)return elapsed;elapsed+=interval(step,normalMs,repeatMs);}
}
inline int indexAt(const phoenix::Cell* cells,uint32_t elapsed,uint32_t normalMs,uint32_t repeatMs){
  uint32_t start=0;for(int i=0;;++i){const auto step=at(cells,i);if(!step)return -1;const auto end=start+interval(step,normalMs,repeatMs);if(elapsed<end)return i;start=end;}
}
inline unsigned completedAt(const phoenix::Cell* cells,uint32_t elapsed,uint32_t normalMs,uint32_t repeatMs,uint32_t arrivalMs){
  uint32_t start=0;unsigned done=0;
  for(int i=0;;++i){const auto step=at(cells,i);if(!step||elapsed<start+arrivalMs)return done;++done;start+=interval(step,normalMs,repeatMs);}
}
}
