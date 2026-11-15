#pragma once
#include <cstddef>
#include <cstdint>
#include "third_party/dr_mp3.h"
// Decoder/sink separation lets the actual sequential PCM and three-buffer
// lifetime policy run under host sanitizers without ESP32/M5 mocks.
class MusicSink {
 public: virtual ~MusicSink()=default;
 virtual unsigned queued()const=0;
 virtual bool play(const int16_t* pcm,size_t count,uint32_t rate)=0;
};
class MusicStream {
 public:
 static constexpr size_t kFrames=4608;
 MusicStream()=default;
 MusicStream(const MusicStream&)=delete;
 MusicStream& operator=(const MusicStream&)=delete;
 bool init(const void* data,size_t bytes);
 void close();
 bool restart();
 // Configure before the first queued sample. The opening plays from zero
 // once; later wraps repeat only[begin,end). Invalid regions are rejected.
 bool setLoopRegion(uint64_t begin,uint64_t end);
 // Play exactly[begin,end) once, then drain the final queued buffer.
 bool setOneShotRegion(uint64_t begin,uint64_t end);
 bool finished()const{return ready_&&oneShot_&&started_&&position_>=loopEnd_&&!pending_;}
 // Copy a precomputed point into stream-owned storage. Bind only before
 // playback, after the caller verifies the compressed source identity.
 bool setLoopSeekPoint(const drmp3_seek_point& point);
 bool loopSeekReady()const{return loopSeekReady_;}
 uint64_t loopBegin()const{return loopBegin_;}
 uint64_t loopEnd()const{return loopEnd_;}
 bool pump(MusicSink& sink);
 uint32_t sampleRate()const{return decoder_.sampleRate;}
 uint64_t framesQueued()const{return framesQueued_;}
 uint32_t loops()const{return loops_;}
 uint32_t retries()const{return retries_;}
 private:
 drmp3 decoder_{};bool ready_=false;unsigned next_=0;size_t pending_=0;
 uint64_t framesQueued_=0;uint32_t loops_=0,retries_=0;
 uint64_t position_=0,loopBegin_=0,loopEnd_=0;
 uint64_t startFrame_=0;bool oneShot_=false,started_=false;
 size_t sourceBytes_=0;drmp3_seek_point loopSeekPoint_{};bool loopSeekReady_=false;
 alignas(4) int16_t buffers_[3][kFrames]{};
};
