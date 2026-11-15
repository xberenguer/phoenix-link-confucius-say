#pragma once
#include "completion_cache.h"
#include "award_music_seek.h"
// The clean87.074..92.321s phrase is already in the verified offline MP3.
// Decode it once at boot, without resampling, processing or a second asset.
namespace award_cache {
constexpr size_t frames=feature_music::bigAwardEndFrame-feature_music::awardBeginFrame;
constexpr size_t storageBytes=((frames+1)&~size_t(1))*sizeof(int16_t);
static_assert(frames==award_music_seek::frames,"Award cache/source range mismatch");
inline bool decode(MusicStream& stream,const void* mp3,size_t bytes,int16_t* out,uint32_t& actualFNV){
 actualFNV=0;
 if(!out||!stream.init(mp3,bytes)||!stream.setOneShotRegion(feature_music::awardBeginFrame,feature_music::bigAwardEndFrame)||!stream.setLoopSeekPoint(award_music_seek::point))return false;
 completion_cache::CopySink sink(out,frames);
 while(!stream.finished())if(!stream.pump(sink))return false;
 if(sink.written()!=frames)return false;
 actualFNV=completion_cache::checksum(out,frames);
 return actualFNV==award_music_seek::decodedFNV;
}
}
