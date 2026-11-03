#pragma once

// Compatibility layer for ESPHome < 2026.11.0
//
// ESPHome 2026.11.0 replaced AddressableLight::get_view_internal() by the color accessors
// (light::CorrectedBufferColorAccessor, AddressableLight::accessor_). Older releases get a
// backport of the part of CorrectedBufferColorAccessor this driver uses, which hands out the
// ESPColorView the old API expects.

#include "esphome/core/version.h"

#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/channel_colors.h"

#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 11, 0)

#define SM16825_USE_COLOR_ACCESSOR

namespace esphome::sm16825 {
using ColorBufferAccessor = light::CorrectedBufferColorAccessor;
}  // namespace esphome::sm16825

#else

#include <cstring>

#include "esphome/core/helpers.h"

namespace esphome::sm16825 {

class ColorBufferAccessor {
 public:
  ColorBufferAccessor(uint8_t *color_data, uint8_t *effect_data, light::ChannelColors channel_colors,
                      const light::ESPColorCorrection *color_correction)
      : color_data(color_data),
        effect_data(effect_data),
        channel_colors(channel_colors),
        color_correction(color_correction) {}

  /// Allocate the color and effect data for `num_leds` LEDs as one zeroed block. Returns false if it failed.
  bool allocate(RAMAllocator<uint8_t> &allocator, size_t num_leds) {
    const size_t color_bytes = num_leds * this->channel_colors.bytes_per_led();
    const size_t total_bytes = color_bytes + num_leds;
    uint8_t *buffer = allocator.allocate(total_bytes);
    if (buffer == nullptr)
      return false;
    memset(buffer, 0, total_bytes);
    this->color_data = buffer;
    this->effect_data = buffer + color_bytes;
    return true;
  }

  /// Zero the effect data of the first `num_leds` LEDs; does nothing before the effect data is allocated.
  void clear_effect_data(size_t num_leds) {
    if (this->effect_data != nullptr)
      memset(this->effect_data, 0, num_leds);
  }

  light::ESPColorView get_view(int32_t index) const {
    const light::ChannelColors &colors = this->channel_colors;
    uint8_t *led = this->color_data + index * colors.bytes_per_led();
    return {led + colors.r,
            led + colors.g,
            led + colors.b,
            colors.has_white() ? led + colors.w : nullptr,
            this->effect_data + index,
            this->color_correction};
  }

  uint8_t *color_data{nullptr};
  uint8_t *effect_data{nullptr};
  light::ChannelColors channel_colors{};
  const light::ESPColorCorrection *color_correction{nullptr};
};

}  // namespace esphome::sm16825

#endif
