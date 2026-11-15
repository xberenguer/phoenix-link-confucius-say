#include "mp3_fp_policy.h"
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#define DR_MP3_NO_SIMD
#include "music_stream.h"
#include <algorithm>
#include <cstring>
bool MusicStream::init(const void* data,size_t bytes){close();if(!data||!bytes)return false;ready_=drmp3_init_memory(&decoder_,data,bytes,nullptr);if(!ready_)return false;if(decoder_.channels!=1||decoder_.sampleRate!=44100){close();return false;}sourceBytes_=bytes;return true;}
void MusicStream::close(){if(ready_)drmp3_uninit(&decoder_);ready_=false;pending_=next_=0;framesQueued_=loops_=retries_=0;position_=loopBegin_=loopEnd_=startFrame_=0;oneShot_=started_=false;sourceBytes_=0;loopSeekReady_=false;loopSeekPoint_={};}
bool MusicStream::restart(){if(!ready_||!drmp3_seek_to_pcm_frame(&decoder_,startFrame_))return false;pending_=next_=0;position_=startFrame_;started_=true;return true;}
bool MusicStream::setLoopRegion(uint64_t begin,uint64_t end){
 if(!ready_||framesQueued_||pending_||position_||begin>=end)return false;
 const uint64_t total=drmp3_get_pcm_frame_count(&decoder_);
 if(!drmp3_seek_to_pcm_frame(&decoder_,0)||end>total)return false;
 if(begin!=loopBegin_){drmp3_bind_seek_table(&decoder_,0,nullptr);loopSeekReady_=false;loopSeekPoint_={};}
 loopBegin_=begin;loopEnd_=end;startFrame_=0;oneShot_=started_=false;return true;
}
bool MusicStream::setOneShotRegion(uint64_t begin,uint64_t end){if(!setLoopRegion(begin,end))return false;startFrame_=begin;oneShot_=true;return true;}
bool MusicStream::setLoopSeekPoint(const drmp3_seek_point& point){
 if(!ready_||!loopEnd_||framesQueued_||pending_||position_||
    point.seekPosInBytes<decoder_.streamStartOffset||point.seekPosInBytes>=sourceBytes_||
    point.pcmFrameIndex<point.pcmFramesToDiscard||
    point.pcmFrameIndex>loopBegin_+decoder_.delayInPCMFrames)return false;
 loopSeekPoint_=point;
 loopSeekReady_=drmp3_bind_seek_table(&decoder_,1,&loopSeekPoint_);
 return loopSeekReady_;
}
bool MusicStream::pump(MusicSink& sink){
 if(!ready_||sink.queued()>=2)return false;
 if(!started_){if(!drmp3_seek_to_pcm_frame(&decoder_,startFrame_))return false;position_=startFrame_;started_=true;}
 if(!pending_){
  // A loop boundary can leave only38samples (<1ms). Queueing that as a
  // separate buffer lets a2ms worker/DMA tick exhaust both hardware slots.
  // Fill across the exact boundary instead; only finite cues end short.
  while(pending_<kFrames){
   if(loopEnd_&&position_>=loopEnd_){if(oneShot_)break;if(!drmp3_seek_to_pcm_frame(&decoder_,loopBegin_))return false;position_=loopBegin_;++loops_;}
   const size_t remaining=kFrames-pending_;
   const size_t requested=loopEnd_?size_t(std::min<uint64_t>(remaining,loopEnd_-position_)):remaining;
   const size_t n=size_t(drmp3_read_pcm_frames_s16(&decoder_,requested,buffers_[next_]+pending_));
   if(!n){
    if(loopEnd_)break;
    if(!position_||!drmp3_seek_to_pcm_frame(&decoder_,0))return false;
    position_=0;++loops_;continue;
   }
   pending_+=n;position_+=n;
  }
  if(!pending_)return false;
 }
 // A transient queue failure retries the SAME decoded buffer; no skipped
 // samples, reused active memory, repeated intro or arbitrary chopped phrase.
 if(!sink.play(buffers_[next_],pending_,decoder_.sampleRate)){++retries_;return false;}
 framesQueued_+=pending_;pending_=0;next_=(next_+1)%3;return true;
}
