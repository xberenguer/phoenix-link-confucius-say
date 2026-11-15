#pragma once
#include <cstddef>
#include <cstdint>
namespace platform_entropy125 {
struct Hooks {bool(*acquire)(void*);bool(*fill)(void*,uint8_t*,size_t);void(*release)(void*);void*context;};
enum class Fault:uint8_t {None,UnsupportedProfile,InvalidRequest,Reentrant,Acquire,Fill,MissingHooks,FlatOutput};
// Single game-thread owner; exactly one bootstrap capture, then explicit
// runtime qualification. No entropy bytes or DRBG state live in this object.
class Source final {
 public:
  explicit Source(Hooks hooks,bool bootstrapQualified):hooks_(hooks),fault_(bootstrapQualified?Fault::None:Fault::UnsupportedProfile){}
  Source(const Source&)=delete;Source&operator=(const Source&)=delete;
  bool qualifyRuntime(bool qualified){if(active_||runtimeQualified_||!qualified){latch(Fault::UnsupportedProfile);return false;}runtimeQualified_=true;return healthy();}
  bool healthy()const{return fault_==Fault::None;}
  Fault fault()const{return fault_;}
  uint64_t captures()const{return captures_;}
  static bool callback(void*opaque,uint8_t*out,size_t size){if(!opaque){wipeBounded(out,size);return false;}return static_cast<Source*>(opaque)->capture(out,size);}
  bool capture(uint8_t*out,size_t size){
   if(!out||size!=48){latch(Fault::InvalidRequest);wipeBounded(out,size);return false;}
   if(active_){latch(Fault::Reentrant);wipeBounded(out,size);return false;}
   if(captures_&&!runtimeQualified_){latch(Fault::UnsupportedProfile);wipeBounded(out,size);return false;}
   if(!healthy()){wipeBounded(out,size);return false;}
   if(!hooks_.acquire||!hooks_.fill||!hooks_.release){latch(Fault::MissingHooks);wipeBounded(out,size);return false;}
   active_=true;
   if(!hooks_.acquire(hooks_.context)){active_=false;latch(Fault::Acquire);wipeBounded(out,size);return false;}
   const bool filled=hooks_.fill(hooks_.context,out,size);
   hooks_.release(hooks_.context);active_=false;
   if(!filled||!healthy()){if(!filled)latch(Fault::Fill);wipeBounded(out,size);return false;}
   // Obvious stuck-output fuse only; not a min-entropy/90B qualification.
   bool flat=true;for(size_t i=4;i<size;++i)if(out[i]!=out[i%4]){flat=false;break;}
   if(flat){latch(Fault::FlatOutput);wipeBounded(out,size);return false;}
   ++captures_;return true;
  }
 private:
  void latch(Fault fault){if(fault_==Fault::None)fault_=fault;}
  // Invalid oversized/null requests are rejected without touching unknown
  // storage. Every failed valid48-byte request is fully wiped.
  static void wipeBounded(uint8_t*out,size_t size){if(!out||size>48)return;volatile uint8_t*p=out;for(size_t i=0;i<size;++i)p[i]=0;}
  Hooks hooks_;Fault fault_;uint64_t captures_=0;bool active_=false,runtimeQualified_=false;
};
}
