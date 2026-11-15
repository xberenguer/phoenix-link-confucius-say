#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
// Asset-independent adapter for a qualified, immutable flash PCM program.
// This header contains no music, melody or source loop-boundary assumption.
// Caller stops the mixer before replacing a source. Queued samples are never
// copied or overwritten, including at the source's declared loop boundary.
class ImmutablePcmLoop89 {
public:
 static constexpr uint32_t chunkFrames=4608;
 bool bind(const int16_t* data,uint32_t frames,uint32_t rate,uint32_t begin,uint32_t end){
  if(!data||!frames||rate<8000||rate>48000||begin>=end||end>frames)return false;
  data_=data;frames_=frames;rate_=rate;begin_=begin;end_=end;restart();return true;
 }
 bool restart(){if(!data_)return false;position_=0;queued_=0;loops_=retries_=0;return true;}
 template<class Sink> bool pump(Sink& sink){
  if(!data_||sink.queued()>=2)return false;
  const uint32_t count=std::min(chunkFrames,end_-position_);
  if(!sink.play(data_+position_,count,rate_)){++retries_;return false;}
  position_+=count;queued_+=count;
  if(position_==end_){position_=begin_;++loops_;}
  return true;
 }
 uint64_t framesQueued()const{return queued_;}
 uint32_t loops()const{return loops_;}
 uint32_t retries()const{return retries_;}
 uint32_t position()const{return position_;}
 uint32_t sampleRate()const{return rate_;}
 uint32_t sourceFrames()const{return frames_;}
private:
 const int16_t* data_=nullptr;
 uint32_t frames_=0,rate_=0,begin_=0,end_=0,position_=0,loops_=0,retries_=0;
 uint64_t queued_=0;
};
