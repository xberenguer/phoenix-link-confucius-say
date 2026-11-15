#pragma once
#include <cstdint>
#ifndef PHOENIX_TWIN_MUSIC_PCM_HEADER
#define PHOENIX_TWIN_MUSIC_PCM_HEADER "twin_music42.h"
#endif
#include PHOENIX_TWIN_MUSIC_PCM_HEADER
#ifndef PHOENIX_TWIN_MUSIC_PCM_NAMESPACE
#define PHOENIX_TWIN_MUSIC_PCM_NAMESPACE twin_music42
#endif
// Root may accept the sealed reconstruction FOR DEVICE AUDITION only.
// This flag never means a recovered vendor bank or perceptual1:1 audio.
namespace twin_music_bank39 {
namespace bank=PHOENIX_TWIN_MUSIC_PCM_NAMESPACE;
inline constexpr bool rootAcceptedForDevice=true;
#ifdef PHOENIX_TWIN_MUSIC_HOST_REVIEW
inline constexpr bool hostReviewEnabled=true;
#else
inline constexpr bool hostReviewEnabled=false;
#endif
inline constexpr bool enabled=rootAcceptedForDevice||hostReviewEnabled;
inline constexpr uint32_t sampleRate=bank::sampleRate;
inline constexpr uint32_t frames=bank::frames;
inline constexpr uint32_t loopBegin=bank::loopBegin;
inline constexpr uint32_t loopEnd=bank::loopEnd;
inline constexpr uint32_t pcmFNV1a=bank::pcmFNV1a;
inline constexpr uint32_t pcmBytes=frames*sizeof(int16_t);
inline constexpr bool metadataValid=sampleRate==22050&&frames&&loopBegin<loopEnd&&loopEnd<=frames;
inline const int16_t* data(){return bank::samples;}
}
