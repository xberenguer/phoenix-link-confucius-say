#pragma once
#include <cstddef>
#include <cstdint>

namespace phoenix { namespace random {
// Must supply fresh strong platform entropy for EVERY callback, including
// reseeds. No timestamp, game seed, static seed, noise-off hardware PRNG or
// weak fallback. The platform owner proves/adopts the source prerequisites.
using EntropyFn = bool (*)(void*, uint8_t*, size_t);
enum class Fault : uint8_t {
  None, NotInitialized, Entropy, Drbg, Reentrant, Exhausted,
  RejectionLimit, Allocation, InvalidBound
};

// Sole owner of all secret DRBG state and the current action reservoir.
// Never put this object into an Engine, checkpoint, USB response or JS export.
// Single game-thread use only. Copy/move are forbidden, including AES contexts
// with implementation-specific internal pointers/locks.
class Service final {
 public:
  static constexpr size_t kActionBytes = 1024;
  static constexpr size_t kActionWords = kActionBytes / 4;
  static constexpr unsigned kMaximumRejections = 64;
  Service() noexcept;
  ~Service();
  Service(const Service&) = delete;
  Service& operator=(const Service&) = delete;
  Service(Service&&) = delete;
  Service& operator=(Service&&) = delete;
  bool initialize(EntropyFn entropy, void* context) noexcept;
  bool beginAction() noexcept;
  bool nextWord(uint32_t& output) noexcept;
  void endAction() noexcept;
  bool healthy() const noexcept { return initialized_ && fault_ == Fault::None; }
  Fault fault() const noexcept { return fault_; }
  void latchFault(Fault fault) noexcept;
#ifdef PHOENIX_RNG_TESTING
  size_t testWordsUsed() const noexcept { return cursor_ / 4; }
  void testLimitWords(size_t words) noexcept { limit_ = words <= kActionWords ? words * 4 : kActionBytes; }
  bool testReplaceActiveWords(const uint32_t* words, size_t count) noexcept {
    if (!active_ || !healthy() || !words || count > kActionWords) return false;
    for (size_t i=0; i<count; ++i) for (unsigned b=0; b<4; ++b)
      reservoir_[4*i+b] = uint8_t(words[i] >> (8*b));
    cursor_=0; limit_=count*4; return true;
  }
#endif
 private:
  static int entropyBridge(void*, unsigned char*, size_t);
  struct Impl;
  Impl* implementation_ = nullptr;
  EntropyFn entropy_ = nullptr;
  void* entropyContext_ = nullptr;
  uint8_t reservoir_[kActionBytes] = {};
  size_t cursor_ = 0, limit_ = kActionBytes;
  Fault fault_ = Fault::NotInitialized;
  bool initialized_ = false, active_ = false, generating_ = false, entropyFailed_ = false;
};

// A copyable handle contains only a pointer to the external service. Engine
// copies never clone outputs, key material, counters or an unread reservoir.
class Draws {
 public:
  explicit Draws(Service* source = nullptr) : source_(source) {}
  void bind(Service* source) { source_ = source; }
  uint32_t bounded(uint32_t bound) {
    if (!source_) return 0; // transaction admission rejects the missing source
    if (!bound) { source_->latchFault(Fault::InvalidBound); return 0; }
    const uint32_t threshold = uint32_t(-bound) % bound;
    for (unsigned attempt = 0; attempt < Service::kMaximumRejections; ++attempt) {
      uint32_t value = 0;
      if (!source_->nextWord(value)) return 0;
      if (value >= threshold) return value % bound;
    }
    source_->latchFault(Fault::RejectionLimit);
    return 0; // discarded scratch only; never commit a fallback outcome
  }
 private:
  Service* source_;
};
}} // namespace phoenix::random
