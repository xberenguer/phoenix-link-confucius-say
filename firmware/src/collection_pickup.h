#pragma once
#include <algorithm>
#include <cmath>
#include <stdint.h>

// Presentation reconstruction from the three consecutive row pickups in
// l874jFsUyV8,230.767..234.600s. See assets/collection-pickup-v0257/review.json.
// The upper cabinet is compressed into our HUD; these are adapted paths,
// not recovered manufacturer animation coordinates or randomness.
namespace collection_pickup {
constexpr uint32_t arrivalMs=700;
constexpr uint32_t flightMs=1160;
constexpr uint32_t trailMs=900;
struct Point {float x,y;};
struct Knot {float dx,y;};
constexpr Knot routes[3][8]={
 {{0,33.5f},{52,73},{103,88},{128,69},{122,36},{62,41},{33,45},{0,7}},
 {{0,68.5f},{11,45},{31,27},{47,47},{54,94},{87,109},{109,66},{0,7}},
 {{0,103.5f},{19,64},{34,43},{58,31},{84,63},{56,92},{32,66},{0,7}}
};
inline Point at(uint32_t ms,float sx,float sy){
  if(!ms)return {sx,sy};
  if(ms>=arrivalMs){
    // The bird reaches the amount first. Its tails continue curling above
    // the reels, as in231.50..231.90s; do not freeze or remove the whole
    // ribbon as soon as the amount is committed to the display.
    const float t=std::min(float(flightMs-arrivalMs),float(ms-arrivalMs));
    const float a=t*.012f;
    return {148.f+14.f*std::sin(a),7.f+4.f*std::sin(a*1.4f)};
  }
  const unsigned row=unsigned(std::max(0,std::min(2,int(std::lround((sy-33.5f)/35.f)))));
  const float direction=sx<148.f?1.f:-1.f;
  const float extent=row==0?128.f:row==1?109.f:84.f;
  const float room=direction>0?230.f-sx:sx-10.f;
  const float scale=std::min(1.f,std::max(.1f,room/extent));
  auto knot=[&](int n){
    n=std::max(0,std::min(7,n));
    if(n==0)return Point{sx,sy};
    if(n==7)return Point{148.f,7.f};
    return Point{sx+direction*routes[row][n].dx*scale,routes[row][n].y};
  };
  const unsigned segment=ms/100;
  const float t=(ms%100)*.01f,t2=t*t,t3=t2*t;
  const auto a=knot(int(segment)-1),b=knot(segment),c=knot(segment+1),d=knot(segment+2);
  auto curve=[&](float p,float q,float r,float s){
    return .5f*(2*q+(-p+r)*t+(2*p-5*q+4*r-s)*t2+(-p+3*q-3*r+s)*t3);
  };
  return {curve(a.x,b.x,c.x,d.x),curve(a.y,b.y,c.y,d.y)};
}
inline unsigned sourceOpacity(uint32_t ms){
  if(ms>=900)return 0;
  if(ms<90)return ms*255/90;
  return ms<430?255:(900-ms)*255/470;
}
inline unsigned headOpacity(uint32_t ms){
  if(ms>=flightMs)return 0;
  return ms<100?ms*255/100:ms>flightMs-120?(flightMs-ms)*255/120:255;
}
}
