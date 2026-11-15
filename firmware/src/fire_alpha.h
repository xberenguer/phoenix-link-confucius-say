#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include "fire_alpha_data.h"

namespace fire_alpha {
inline bool verified=false;
inline unsigned verificationChecks=0;
inline unsigned reference(float n,float denominator){
 const float enter=n/denominator;
 return unsigned(std::max(0.f,std::min(1.f,enter))*255);
}
inline unsigned lookup(float n,float denominator,float factor,const float* boundaries){
 // Full-clear/full-cover pixels dominate the field. No division, unsigned
 // conversion or threshold lookup is needed for either range.
 if(!(n>0))return n<=0?0:255; // Also preserves the old clamp's NaN result.
 if(n>=denominator)return 255;
 unsigned estimate=std::min(254u,unsigned(n*factor));
 // A multiply supplies an estimate only. IEEE boundaries then restore the
 // original divide/multiply/truncate result, including all one-ULP edges.
 while(estimate&&n<boundaries[estimate])--estimate;
 while(estimate<255&&n>=boundaries[estimate+1])++estimate;
 return estimate;
}
inline unsigned rising(float n){return verified?lookup(n,.09f,255.f/.09f,data::rising):reference(n,.09f);}
inline unsigned leaving(float n){return verified?lookup(n,.10f,255.f/.10f,data::leaving):reference(n,.10f);}
inline bool init(){
 if(verified)return true;
 verificationChecks=0;
 for(unsigned kind=0;kind<2;++kind){const float denominator=kind?.10f:.09f;const float* table=kind?data::leaving:data::rising;
  for(unsigned k=1;k<=255;++k){uint32_t bits;std::memcpy(&bits,table+k,4);
   for(int delta=-2;delta<=2;++delta){const uint32_t sample=uint32_t(int64_t(bits)+delta);float n;std::memcpy(&n,&sample,4);
    ++verificationChecks;
    if(reference(n,denominator)!=lookup(n,denominator,255.f/denominator,table))return false;
   }
   if(reference(table[k],denominator)!=k)return false;
  }
 }
 verified=true;return true;
}
}
