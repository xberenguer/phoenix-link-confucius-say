#pragma once
#include <stdint.h>

// Presentation-only fountain. Consecutive frames of si4wPYu_5co, source
//1520.25..1523.75, show a lower-centre launch, rise and outward falling arc.
// The fitted trajectories are a reconstruction, not the original particle bank.
namespace coin_motion {
constexpr unsigned count=64;
constexpr uint32_t birthMs=72,fountainDurationMs=6600,lifeMs=2000;
constexpr unsigned minSize=16,maxSize=30;
constexpr int launchY=150;
constexpr uint32_t transferDurationMs=8000;
constexpr uint32_t countDurationMs=5700;
struct Particle { bool active; int x,y; unsigned frame,size,opacity; unsigned roll=0; };
inline Particle at(unsigned index,uint32_t elapsed){
  if(index>=count)return {false,0,0,0,0,0};
  const uint32_t birth=index*birthMs;
  if(elapsed<birth)return {false,0,0,0,0,0};
  const uint32_t age=elapsed-birth;
  if(age>2000)return {false,0,0,0,0,0};
  // Both independent filmed fountains begin as a narrow central jet, with
  // the wider spray arriving later. Do not fan the very first coin left.
  const int spread=index<16?24:75;
  const int vx=int((index*47+23)%unsigned(2*spread+1))-spread;
  // Launch wholly below the135px LCD, rather than appear at the reel edge.
  // The34px/s increase retains the previously fitted upper arc after moving
  // the origin27px down. The added return path now crosses the credit bar.
  const int vy=344+int((index*29+7)%51);
  const int x=120+int((index*11)%13)-6+vx*int(age)/1000;
  // y increases down the LCD. Negative initial velocity, then gravity.
  const int64_t t=age;
  const int y=launchY-int(vy*t/1000)+int(210*t*t/1000000);
  const unsigned opacity=age>1750?(2000-age)*255/250:255;
  // Source full faces span roughly.3..55of a reel width after perspective
  // scaling, substantially larger than the old8..14px stamps. Keep the
  // existing deterministic cosmetic spread, count and emission timing;
  // original individual particle sizes/depth/RNG remain unrecovered.
  return {true,x,y,(age/(32+index%5*4)+index*3)%16,minSize+(index*2+10)%(maxSize-minSize+1),opacity,(age/48+index*11)%32};
}
// Reuse the existing deterministic64-launch programme with overlapping tails.
// Only each particle's age is local; the fountain clock itself never wraps.
inline unsigned firstLiveBirth(uint32_t elapsed){return elapsed>lifeMs?(elapsed-lifeMs+birthMs-1)/birthMs:0;}
inline Particle continuousAt(unsigned serial,uint32_t elapsed){
 const uint32_t birth=serial*birthMs;if(elapsed<birth||elapsed-birth>lifeMs)return {false,0,0,0,0,0};
 const unsigned pose=serial%count;return at(pose,pose*birthMs+elapsed-birth);
}
}
