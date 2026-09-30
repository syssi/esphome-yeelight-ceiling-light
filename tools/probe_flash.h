// Read-only flash instrumentation for a Yeelight being converted to ESPHome.
//
// Everything here reads; nothing writes. The target is the INACTIVE OTA slot -
// esp_ota_get_next_update_partition() returns the slot this build is NOT running
// from, which is where the vendor image still sits after a single flash. Reading
// it out over the API is what makes a stock image recoverable without UART, and
// therefore usable on a device still under warranty.
//
// Chunks are logged as base64 on one line, so the caller can reassemble them
// from an API log subscription. 2048 raw bytes expand to 2732 characters, which
// is why flash_probe.yaml raises tx_buffer_size to 4096.
//
// Pulled in by flash_probe.yaml; driven by tools/dump_stock.py.
#pragma once

#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "esphome/core/alloc_helpers.h"
#include "esphome/core/log.h"

namespace flash_probe {

static const char *const TAG = "probe";
static constexpr size_t MAX_CHUNK = 2048;

inline void log_part(const char *what, const esp_partition_t *p) {
  if (p == nullptr) {
    ESP_LOGI(TAG, "%s: none", what);
    return;
  }
  ESP_LOGI(TAG, "%s: label=%s type=0x%02x subtype=0x%02x addr=0x%06" PRIx32 " size=0x%06" PRIx32,
           what, p->label, p->type, p->subtype, p->address, p->size);
}

/// Which slot is running and which one the next OTA would overwrite. Confirms
/// the alternation first-hand rather than taking it from the documentation.
inline void log_slots() {
  log_part("running", esp_ota_get_running_partition());
  log_part("boot", esp_ota_get_boot_partition());
  log_part("next_update", esp_ota_get_next_update_partition(nullptr));
  log_part("otadata", esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                               ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr));
}

inline void dump_part(const esp_partition_t *part, const char *what, int offset, int length) {
  if (part == nullptr) {
    ESP_LOGW(TAG, "%s: partition not found", what);
    return;
  }
  if (offset < 0 || length <= 0) {
    ESP_LOGW(TAG, "%s: bad request offset=%d length=%d", what, offset, length);
    return;
  }
  if ((size_t) offset >= part->size) {
    ESP_LOGW(TAG, "%s: offset %d past end 0x%06" PRIx32, what, offset, part->size);
    return;
  }
  size_t len = (size_t) length > MAX_CHUNK ? MAX_CHUNK : (size_t) length;
  if ((size_t) offset + len > part->size)
    len = part->size - (size_t) offset;

  static uint8_t buf[MAX_CHUNK];
  const esp_err_t err = esp_partition_read(part, (size_t) offset, buf, len);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "%s: read at %d failed: %s", what, offset, esp_err_to_name(err));
    return;
  }
  // One line per chunk: tag, offset, length, base64. The offset is echoed so a
  // reassembling caller never has to trust the order lines arrive in.
  ESP_LOGI(TAG, "%s %d %u %s", what, offset, (unsigned) len,
           esphome::base64_encode(buf, len).c_str());
}

/// The inactive OTA slot - the vendor image, after exactly one flash.
inline void dump_flash(int offset, int length) {
  dump_part(esp_ota_get_next_update_partition(nullptr), "CHUNK", offset, length);
}

/// otadata, where the bootloader keeps the sequence numbers that decide which
/// slot boots. Separate partition, so it needs its own reader.
inline void dump_otadata(int offset, int length) {
  dump_part(esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                     ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr),
            "OTADATA", offset, length);
}

}  // namespace flash_probe
