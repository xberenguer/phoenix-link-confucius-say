#pragma once
#include <stdint.h>

// Presentation-only Third-family selection. No wager/RNG/persistent state.
// Preserve IDs0..3; Third is an appended family, not a new rising scale.
namespace landing_cues {
enum class Tone : uint8_t { None, Stop, Initial, Follow, Third };

inline Tone legacy(const uint8_t counts[5], unsigned first, unsigned last,
                   bool hold, bool anyVacant = true) {
  if (first > last || last >= 5) return Tone::None;
  unsigned before = 0, arrived = 0;
  for (unsigned c = 0; c < first; ++c) before += counts[c];
  for (unsigned c = first; c <= last; ++c) arrived += counts[c];
  if (hold) return !anyVacant ? Tone::None : arrived ? Tone::Initial : Tone::Stop;
  if (!arrived) return Tone::Stop;
  const unsigned seen = before + arrived;
  if (seen >= 6 || seen + 3 * (4 - last) < 6) return Tone::Stop;
  return before ? Tone::Follow : Tone::Initial;
}

inline Tone choose(const uint8_t counts[5], unsigned first, unsigned last,
                   bool hold, bool anyVacant = true,
                   bool regularSingleSpin = false) {
  const Tone old = legacy(counts, first, last, hold, anyVacant);
  // Caller enables only a non-Twin, non-Hold, non-quick-stop spin, with
  // separately presented single-column arrivals. Coalesced groups retain
  // legacy behavior. Existing held Phoenixes must be excluded from counts.
  if (!regularSingleSpin || hold || first != last || last >= 5) return old;
  if (!counts[first]) return old;
  unsigned before = 0, containingGroups = 0;
  for (unsigned c = 0; c < first; ++c) {
    before += counts[c];
    containingGroups += counts[c] != 0;
  }
  const unsigned seen = before + counts[first];
  if (seen + 3 * (4 - last) < 6) return old;
  // Source A07/A09/A11: third containing group, including crossings to7.
  // This path must precede legacy's seen>=6 suppression. Already6BEFORE
  // this arrival remains unobserved and conservatively keeps legacy Stop.
  if (containingGroups == 2 && before < 6) return Tone::Third;
  // No fourth family is invented; unreviewed ordinals retain legacy output.
  return old;
}
}
