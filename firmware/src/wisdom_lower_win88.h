#pragma once
#include <cstddef>
#include <cstdint>
// Display-only reconstruction of three filmed, scatter-only configurations
// from two recordings. Repeated events are not independent casino samples.
// These dollar anchors are observations, not a vendor algorithm or RTP rule.
namespace wisdom_win88 {
struct Sample {uint32_t ms; int64_t minor;};
struct Profile {
 int64_t coinMinor;uint16_t lines;uint8_t multiplier;int64_t awardMinor;
 const Sample* samples;size_t sampleCount;uint32_t continuationMinorPerSecond;
};
// 5BUJOWxIBN4: origin is first "6 FREE GAMES WON" at610.650043.
// Hold onset bounded at7.033..7.050s. Later anchors precede manual START.
constexpr Sample fiveDollar[]={
 {7040,0},{8850,5158},{9350,6535},{9850,8013},{10350,9313},
 {10850,10741},{11350,12182},{11850,13610},{12350,15025},
 {12850,16339},{13350,17767},{13850,19195},{14350,20623},
 {14850,22051},{15350,23479},{15850,24907},{16350,26335},
 {16850,27763},{17350,29064},{17850,30492},{18350,31920},
 {18850,33351},{19350,34779},{19850,36210},{20350,37638},
 {20850,38922}
};
// 3pfZnZUWGhI: origin223.756867. First nonzero frame230.797233.
// Milliseconds are rounded from decoded native PTS, not inferred from payout.
constexpr Sample oneCent[]={
 {7040,0},{7074,1},{7107,2},{7274,4},{7474,8},{7674,12},
 {7774,14},{8275,25}
};
// Independent 3pf repeat: 25c/25/x1, first six-free message1512.978133.
// Full $12.50 is observed while START still waits, at1524.189333.
constexpr Sample quarter[]={
 {7040,0},{7074,16},{7141,31},{7207,46},{7274,61},{7340,76},
 {7374,91},{7541,131},{8041,287},{8542,440},{9042,582},
 {9543,742},{10043,902},{10544,1061},{11044,1223},
 {11111,1240},{11211,1250}
};
constexpr Profile profiles[]={
 {500,50,3,150000,fiveDollar,sizeof(fiveDollar)/sizeof(*fiveDollar),2856},
 {1,50,5,500,oneCent,sizeof(oneCent)/sizeof(*oneCent),20},
 {25,25,1,1250,quarter,sizeof(quarter)/sizeof(*quarter),0}
};
inline const Profile* sourceProfile(int64_t coinMinor,unsigned lines,unsigned multiplier){
 for(const auto& p:profiles)if(p.coinMinor==coinMinor&&p.lines==lines&&p.multiplier==multiplier)return &p;
 return nullptr;
}
inline int64_t counted(const Profile& p,uint32_t elapsed){
 if(elapsed<=p.samples[0].ms)return 0;
 for(size_t i=1;i<p.sampleCount;++i){
  const auto& b=p.samples[i];
  if(elapsed<=b.ms){const auto& a=p.samples[i-1];
   return a.minor+int64_t(elapsed-a.ms)*(b.minor-a.minor)/(b.ms-a.ms);
  }
 }
 // The quarter profile ends at an observed complete award. For the other
 // profiles, an unobserved tail continues their measured median prefix slope.
 // Never normalize across wagers or claim those two natural end times.
 const auto& last=p.samples[p.sampleCount-1];
 const int64_t estimate=last.minor+int64_t(elapsed-last.ms)*p.continuationMinorPerSecond/1000;
 return estimate<p.awardMinor?estimate:p.awardMinor;
}
} // namespace wisdom_win88
