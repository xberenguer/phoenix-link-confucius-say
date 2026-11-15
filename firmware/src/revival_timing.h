#pragma once
#include <cstdint>
// Relative to the clean completion cue in independent l874jFsUyV8 footage:
// cue182.371s, returning music186.325s, bird~188.95s, fire~189.07s,
// restart prompt~190.10s and clear board~190.33s. Camera-derived retarget;
// not a recovered manufacturer timer or proof of hidden revival limits.
namespace revival_timing {
constexpr uint32_t musicAt=3950,birdAt=6580,fireAt=6700,promptAt=7700,duration=7960;
static_assert(musicAt<birdAt&&birdAt<fireAt&&fireAt<promptAt&&promptAt<duration,"Ordered revival cues");
}
