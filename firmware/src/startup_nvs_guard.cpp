#include "startup_nvs_guard.h"
#include <esp_partition.h>

namespace {
// Used before setup(), including by initArduino(). Constant initialization only.
unsigned char blockedFullErase = 0;
}

extern "C" esp_err_t __real_esp_partition_erase_range(
    const esp_partition_t* partition, size_t offset, size_t size);

extern "C" esp_err_t __wrap_esp_partition_erase_range(
    const esp_partition_t* partition, size_t offset, size_t size) {
  // Arduino 2.0.17 formats the complete default NVS partition after certain
  // initialization errors. Preserve that unread bank for read-only recovery.
  // Normal NVS GC erases one 4096-byte page in our 20480-byte partition;
  // such calls and all non-NVS partitions retain the original implementation.
  if (partition && partition->type == ESP_PARTITION_TYPE_DATA &&
      partition->subtype == ESP_PARTITION_SUBTYPE_DATA_NVS && size &&
      offset == 0 && size == partition->size) {
    __atomic_store_n(&blockedFullErase, 1, __ATOMIC_RELAXED);
    return ESP_ERR_INVALID_STATE;
  }
  return __real_esp_partition_erase_range(partition, offset, size);
}

bool startupNvsFullEraseBlocked() {
  return __atomic_load_n(&blockedFullErase, __ATOMIC_RELAXED) != 0;
}
