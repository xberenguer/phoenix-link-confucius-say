#pragma once
#include <cstdint>
#include <limits>

// Exact nonnegative monetary arithmetic. Unit is explicit, never inferred.
namespace phoenix { namespace money {
enum class Unit:uint8_t { Credits=0, ExactMinor=1 };
struct Amount { int64_t value=0;Unit unit=Unit::Credits; };
inline bool known(Unit u){return u==Unit::Credits||u==Unit::ExactMinor;}
inline bool multiply(int64_t value,uint64_t factor,int64_t& out){
 if(value<0||(factor&&uint64_t(value)>uint64_t(std::numeric_limits<int64_t>::max())/factor))return false;
 out=int64_t(uint64_t(value)*factor);return true;
}
inline bool add(int64_t a,int64_t b,int64_t& out){
 if(a<0||b<0||a>std::numeric_limits<int64_t>::max()-b)return false;
 out=a+b;return true;
}
inline bool toMinor(Amount amount,int64_t denomination,unsigned multiplier,int64_t& out){
 if(!known(amount.unit)||amount.value<0||denomination<=0||multiplier<1||multiplier>3)return false;
 int64_t base=0;
 if(amount.unit==Unit::Credits){if(!multiply(amount.value,uint64_t(denomination),base))return false;}
 else base=amount.value;
 return multiply(base,multiplier,out);
}
}} // namespace phoenix::money
