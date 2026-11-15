#include "phoenix_random.h"
#include "phoenix_crypto124/prefix.h"
#include "phoenix_crypto124/mbedtls/ctr_drbg.h"
#include "phoenix_crypto124/mbedtls/platform_util.h"
#include <new>

static_assert(MBEDTLS_CTR_DRBG_KEYSIZE == 32, "AES-256 CTR_DRBG required");
static_assert(MBEDTLS_CTR_DRBG_MAX_REQUEST >= phoenix::random::Service::kActionBytes,
              "action request exceeds vetted DRBG request maximum");

namespace phoenix { namespace random {
struct Service::Impl { mbedtls_ctr_drbg_context context; };
Service::Service() noexcept = default;
Service::~Service() {
  endAction();
  if (implementation_) {
    mbedtls_ctr_drbg_free(&implementation_->context);
    delete implementation_;
  }
}
int Service::entropyBridge(void* opaque, unsigned char* output, size_t length) {
  Service& self = *static_cast<Service*>(opaque);
  const bool supplied = self.entropy_ && self.entropy_(self.entropyContext_, output, length);
  if (!supplied || self.fault_ != Fault::None) {
    self.entropyFailed_ |= !supplied;
    mbedtls_platform_zeroize(output, length);
    return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
  }
  return 0;
}
void Service::latchFault(Fault fault) noexcept {
  if (fault_ == Fault::None) fault_ = fault;
  mbedtls_platform_zeroize(reservoir_, sizeof(reservoir_));
  cursor_ = 0; active_ = false;
  // A callback may reenter while the primitive still has live references.
  // Defer context clearing until that primitive returns to our outer frame.
  if (implementation_ && !generating_) {
    mbedtls_ctr_drbg_free(&implementation_->context);
    mbedtls_ctr_drbg_init(&implementation_->context);
  }
  initialized_ = false;
}
bool Service::initialize(EntropyFn entropy, void* opaque) noexcept {
  if (generating_ || active_) { latchFault(Fault::Reentrant); return false; }
  endAction();
  if (implementation_) {
    mbedtls_ctr_drbg_free(&implementation_->context);
    delete implementation_;
  }
  implementation_ = new (std::nothrow) Impl;
  initialized_ = false; entropyFailed_ = false;
  entropy_ = entropy; entropyContext_ = opaque; fault_ = Fault::None;
  if (!implementation_) { latchFault(Fault::Allocation); return false; }
  mbedtls_ctr_drbg_context& context = implementation_->context;
  mbedtls_ctr_drbg_init(&context);
  if (!entropy) { latchFault(Fault::Entropy); return false; }
  // 48 strong bytes provide the AES-256 seed/nonce entropy requirement in one
  // source call. Zero explicit nonce is valid with >=1.5*security-strength
  // entropy. Personalization is public domain separation, never a seed.
  mbedtls_ctr_drbg_set_entropy_len(&context, 48);
  if (mbedtls_ctr_drbg_set_nonce_len(&context, 0) != 0) {
    latchFault(Fault::Drbg); return false;
  }
  static const unsigned char personalization[] = "phoenix-simulator-csprng-v1";
  generating_ = true;
  const int result = mbedtls_ctr_drbg_seed(&context, entropyBridge, this,
                                        personalization, sizeof(personalization) - 1);
  generating_ = false;
  if (result != 0 || fault_ != Fault::None) {
    latchFault(entropyFailed_ ? Fault::Entropy : Fault::Drbg); return false;
  }
  mbedtls_ctr_drbg_set_prediction_resistance(&context, MBEDTLS_CTR_DRBG_PR_ON);
  initialized_ = true;
  return true;
}
bool Service::beginAction() noexcept {
  if (generating_ || active_) { latchFault(Fault::Reentrant); return false; }
  if (!healthy()) return false;
  endAction(); entropyFailed_ = false;
  // PR_ON obtains fresh entropy before this entire action request. Unused
  // bytes are discarded at endAction and never reused by another play.
  generating_ = true;
  const int result = mbedtls_ctr_drbg_random(&implementation_->context, reservoir_, sizeof(reservoir_));
  generating_ = false;
  if (result != 0 || fault_ != Fault::None) {
    latchFault(entropyFailed_ ? Fault::Entropy : Fault::Drbg); return false;
  }
  cursor_ = 0; active_ = true;
  return true;
}
bool Service::nextWord(uint32_t& output) noexcept {
  output = 0;
  if (generating_) { latchFault(Fault::Reentrant); return false; }
  if (!healthy()) return false;
  if (!active_) { latchFault(Fault::NotInitialized); return false; }
  if (cursor_ + 4 > limit_) { latchFault(Fault::Exhausted); return false; }
  output = uint32_t(reservoir_[cursor_]) | uint32_t(reservoir_[cursor_+1]) << 8 |
           uint32_t(reservoir_[cursor_+2]) << 16 | uint32_t(reservoir_[cursor_+3]) << 24;
  mbedtls_platform_zeroize(reservoir_ + cursor_, 4);
  cursor_ += 4;
  return true;
}
void Service::endAction() noexcept {
  if (generating_) { latchFault(Fault::Reentrant); return; }
  mbedtls_platform_zeroize(reservoir_, sizeof(reservoir_));
  cursor_ = 0; active_ = false;
}
}} // namespace phoenix::random
