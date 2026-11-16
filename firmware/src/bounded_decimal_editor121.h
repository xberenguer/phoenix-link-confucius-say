#pragma once
#include <cstdint>

namespace bounded_decimal_editor {
// Advance one digit within the prefix's legal range. If a larger prefix
// makes the old suffix impossible, keep as much as fits below the ceiling.
inline uint64_t next(uint64_t value, unsigned focus, unsigned digits,
                     uint64_t maximum) {
  if (!digits || digits > 18 || focus >= digits || value > maximum) return value;
  uint64_t place = 1;
  for (unsigned i = focus + 1; i < digits; ++i) place *= 10;
  const uint64_t prefix = value / (place * 10) * (place * 10);
  const unsigned limit = unsigned((maximum - prefix) / place < 9
      ? (maximum - prefix) / place : 9);
  const unsigned digit = unsigned(value / place % 10);
  const uint64_t candidate = prefix + uint64_t((digit + 1) % (limit + 1)) * place
      + value % place;
  return candidate > maximum ? maximum : candidate;
}
} // namespace bounded_decimal_editor
