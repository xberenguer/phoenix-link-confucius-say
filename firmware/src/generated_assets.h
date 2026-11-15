#pragma once
#include <cstdint>
namespace pa {
struct Image { uint16_t w, h; const uint16_t* rgb; const uint8_t* alpha=nullptr; uint16_t key=0; };
extern const Image background, header, footer, backdrop;
extern const Image creditLabel, betLabel, winLabel, frameVertical, frameHorizontal;
extern const Image sageTwoRows, sageThreeRows, holdBackground;
extern const Image holdHeader;
// id follows phoenix::Symbol. ms selects captured flame/candle animation frames.
const Image& symbol(unsigned id, uint32_t ms=0, bool winning=false);
// Named native accessor keeps generic inspection banks outside the live UI closure.
const Image& awardWinner();
const Image& asset(const char* name);
unsigned phoenixFrameCount();
uint32_t phoenixPeriodMs();
unsigned candleFrameCount();
const Image& sage(unsigned rows, uint32_t elapsedMs=0);
// Compact top-rail crown; internal images already own their complete head.
const Image& sageCrown(unsigned rows, uint32_t elapsedMs=0, bool winning=false);
unsigned sageFrameCount();
unsigned sageThreeFrameCount();
unsigned abacusFrameCount();
unsigned scatterFrameCount();
unsigned winningFrameCount(unsigned id);
}
