#pragma once
#include "collection_pickup.h"

// The filmed flying body changes into a frontal ember after the amount hits.
// This is a fitted blend, not a recovered manufacturer animation timer.
namespace pickup_arrival {
constexpr unsigned blendMs=160;
inline unsigned flightAlpha(unsigned local,unsigned base){
  if(local<collection_pickup::arrivalMs)return base;
  const unsigned age=local-collection_pickup::arrivalMs;
  return age>=blendMs?0:base*(blendMs-age)/blendMs;
}
inline unsigned headingMs(unsigned local){return local<collection_pickup::arrivalMs?local:collection_pickup::arrivalMs;}
inline unsigned flareAlpha(unsigned local){
  if(local<collection_pickup::arrivalMs||local>=collection_pickup::flightMs)return 0;
  const unsigned age=local-collection_pickup::arrivalMs;
  const float t=age<blendMs?float(age)/blendMs:1.f;
  unsigned alpha=unsigned((t*t*(3-2*t))*255);
  if(local>collection_pickup::flightMs-120)alpha=alpha*(collection_pickup::flightMs-local)/120;
  return alpha;
}
}
