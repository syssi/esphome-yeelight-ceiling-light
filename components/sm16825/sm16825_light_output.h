#pragma once

#ifdef USE_ESP32

#include <atomic>

#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/channel_colors.h"
#include "esphome/core/component.h"

#include <driver/rmt_tx.h>

namespace esphome::sm16825 {

// Five outputs (OUTR, OUTG, OUTB, OUTW, OUTY) with 16 bit grayscale each
static const uint8_t OUTPUTS_PER_CHIP = 5;
static const uint8_t BYTES_PER_CHIP = OUTPUTS_PER_CHIP * 2;

// Current gain, standby and reserved bits at the end of the frame
static const uint8_t TRAILER_BYTES = 4;

struct RmtSymbols {
  rmt_symbol_word_t bit0;
  rmt_symbol_word_t bit1;
  rmt_symbol_word_t reset;
};

class SM16825LightOutput final : public light::AddressableLight {
 public:
  void setup() override;
  void write_state(light::LightState *state) override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  int32_t size() const override { return this->num_leds_; }
  light::LightTraits get_traits() override;
  void clear_effect_data() override;

  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_inverted(bool inverted) { this->invert_out_ = inverted; }
  void set_num_leds(uint16_t num_leds) { this->num_leds_ = num_leds; }
  void set_channel_colors(light::ChannelColors channel_colors) { this->channel_colors_ = channel_colors; }
  void set_rmt_symbols(uint32_t rmt_symbols) { this->rmt_symbols_ = rmt_symbols; }
  void set_current_gain(uint8_t outr, uint8_t outg, uint8_t outb, uint8_t outw, uint8_t outy);

 protected:
  light::ESPColorView get_view_internal(int32_t index) const override;

  size_t get_frame_size_() const { return this->num_leds_ * BYTES_PER_CHIP + TRAILER_BYTES; }
  static bool on_transmit_done(rmt_channel_handle_t channel, const rmt_tx_done_event_data_t *event, void *context);

  uint8_t *colors_{nullptr};
  uint8_t *effect_data_{nullptr};
  uint8_t *frame_{nullptr};

  RmtSymbols symbols_;
  rmt_channel_handle_t channel_{nullptr};
  rmt_encoder_handle_t encoder_{nullptr};
  std::atomic<bool> transmitting_{false};

  uint32_t rmt_symbols_{192};
  uint16_t num_leds_;
  uint8_t pin_;
  bool invert_out_{false};
  uint8_t trailer_[TRAILER_BYTES]{};

  light::ChannelColors channel_colors_{0, 1, 2, light::ChannelColors::NO_WHITE};
};

}  // namespace esphome::sm16825

#endif  // USE_ESP32
