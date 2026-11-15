#pragma once
#include "music_stream.h"
#include "feature_music_timeline.h"
#include "collected_music_seek.h"
#include <cstring>
namespace completion_cache {
constexpr size_t frames=feature_music::collectedEndFrame-feature_music::collectedBeginFrame;
constexpr size_t alignedFrames=(frames+1)&~size_t(1),storageBytes=alignedFrames*sizeof(int16_t)*2;
static_assert(frames==collected_music_seek::frames,"Completion cache/source range mismatch");
inline uint32_t checksum(const int16_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i){const uint16_t v=uint16_t(p[i]);h=(h^uint8_t(v))*16777619u;h=(h^uint8_t(v>>8))*16777619u;}return h;}
class CopySink final:public MusicSink {
 public:
 CopySink(int16_t* out,size_t size):out_(out),size_(size){}
 unsigned queued()const override{return 0;}
 bool play(const int16_t* p,size_t n,uint32_t rate)override{if(rate!=44100||n>size_-at_)return false;std::memcpy(out_+at_,p,n*sizeof(int16_t));at_+=n;return true;}
 size_t written()const{return at_;}
 private:int16_t* out_;size_t size_,at_=0;
};
// Boot-only. Caller verifies compressed identity and starts no speaker task
// on this stream until it has reconfigured the ordinary Hold music.
inline bool decode(MusicStream& stream,const void* mp3,size_t bytes,int16_t* out,uint32_t& actualFNV){
 actualFNV=0;
 if(!out||!stream.init(mp3,bytes)||!stream.setOneShotRegion(feature_music::collectedBeginFrame,feature_music::collectedEndFrame)||!stream.setLoopSeekPoint(collected_music_seek::point))return false;
 CopySink sink(out,frames);
 while(!stream.finished())if(!stream.pump(sink))return false;
 if(sink.written()!=frames)return false;
 actualFNV=checksum(out,frames);return actualFNV==collected_music_seek::decodedFNV;
}
}
