#pragma once
#include <stddef.h>

// Registers clean procedural and repeat-derived spectral reconstructions,
// including the selected Phoenix root, measured follow-up and Third family.
// Call during uiInit before audioInit. Returns registered cue count.
size_t audioRegisterReferenceCues();
