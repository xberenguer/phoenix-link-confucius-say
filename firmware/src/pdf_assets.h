#pragma once
#include <cstdint>
#include "generated_assets.h"
// Isolated next-candidate derivatives; never linked into baseline by this script.
namespace pdfa {
enum class Prize:uint8_t { Mini=1,Minor=2,Major=3 };
enum class Button:uint8_t { CashOut,PressPhoenix,ReturnToGame };
struct Overlay { pa::Image image; const uint8_t* alpha; };
//47x35 canvas; RGB is unpremultiplied, alpha0..255 must be blended.
const Overlay& prize(unsigned kind,unsigned multiplier=1);
const Overlay& prize(Prize kind,unsigned multiplier=1);
//width47/75=fitted; width0=the unchanged native button rectangle.
const Overlay& button(Button which,unsigned width=75);
}
