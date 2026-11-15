#pragma once
// Keep the scalar MP3 decoder's S16 cache reproducible across Clang/ARM and
// GCC/Xtensa. Fused multiply-add contraction changes rounding and checksums.
// Included only by decoder implementation units, never by the UI renderer.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize ("fp-contract=off")
#endif
