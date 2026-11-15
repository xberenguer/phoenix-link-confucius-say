#pragma once
#include <cstdint>
// A fresh seed comes from browser crypto; restored PCG state comes from C++.
uint32_t esp_random();
