#pragma once
#include "generated_assets.h"
namespace winged_phoenix {
constexpr unsigned flightKeyFrames=3,flightPeriodMs=240;
constexpr float flightEyeX=21,flightEyeY=10,flightTailX=10,flightTailY=26;
constexpr float flightAnchorX=17,flightAnchorY=18;
constexpr float flightSpriteSize=36,flightForwardAngle=-0.96850898f;
const pa::Image& hero();const pa::Image& pickup();
const pa::Image& flight(uint32_t elapsed);uint32_t decodedHash();
}
