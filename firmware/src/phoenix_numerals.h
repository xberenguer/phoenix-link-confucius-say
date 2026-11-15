#pragma once
#include <cstdint>
namespace pnum {
struct Glyph { uint8_t w,h,advance; const uint16_t* rgb; const uint8_t* alpha; };
// Digits use clean vector reconstructions, not recovered vendor artwork.
// Digits8/9 are provisional related-shape completions; original vendor glyphs remain unrecovered.
// Native heights 8 and 10; any requested height <=8 selects8, otherwise10.
const Glyph* glyph(char character,unsigned height=10);
bool supports(const char* text);
// Returns0 if any glyph is unavailable. Spacing is between glyphs only.
unsigned width(const char* text,unsigned height=10,unsigned spacing=0);
}
