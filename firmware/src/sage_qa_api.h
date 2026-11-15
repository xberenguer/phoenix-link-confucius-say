#pragma once
#include <cstdint>
struct UiSageSceneTest{
 bool ran=false,unchanged=false,joined=false,pixelWitness=false;
 uint32_t beforeHash=0,afterHash=0,frames=0,expandFrames=0,frameFNV=2166136261u,backupBytes=0;
 uint16_t winMask=0,expandedMask=0;uint8_t expandedReels=0,rows=0;
 uint32_t bodyFrames=0,bodyChecked=0,bodyMatched=0,bodyMismatched=0,bodyInvalidFrames=0,bodyDifferentBlue=0,bodyFirstClock=0;
};
// Explicit controlled visual QA. The source-like board uses public pure line
// evaluation, never a paid action, RNG draw, test-only engine hook or save.
UiSageSceneTest uiTestSageScene(unsigned rows=3);
