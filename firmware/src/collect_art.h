#pragma once
#include "generated_assets.h"
#include <cstdint>
namespace collect_art {
bool init();bool ready();unsigned storageBytes();
const pa::Image& emblem();const pa::Image& collected();const pa::Image& grandCondition();
const pa::Image& number(unsigned n);
}
