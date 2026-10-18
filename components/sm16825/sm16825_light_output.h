#pragma once

#ifdef USE_ESP32

#include <atomic>

#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/channel_colors.h"
#include "esphome/core/component.h"

#include <driver/rmt_tx.h>

namespace esphome::sm16825 {

/// Constant current outputs per chip: OUTR, OUTG, OUTB, OUTW, OUTY.
static constexpr uint8_t OUTPUTS_PER_CHIP = 5;
/// 16 bit grayscale per output.
static constexpr uint8_t BYTES_PER_CHIP = OUTPUTS_PER_CHIP * 2;
/// The frame ends with one 32 bit word: 5 x 5 bit current gain, 2 bit standby, 5 reserved bits.
static constexpr uint8_t TRAILER_BYTES = 4;

struct RmtSymbols {
  rmt_symbol_word_t bit0;
  rmt_symbol_word_t bit1;
  rmt_symbol_word_t reset;
};

/// Chain of SM16825E 5-channel RGBWY drivers (16 bit, 800 kbit/s RZ code), sent with the RMT peripheral.
///
/// write_state() never waits: while the previous frame is still on the wire it only schedules another show.
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
  /// Raw 5 bit gains (0 = 10.2 mA ... 31 = 310 mA) of the outputs OUTR, OUTG, OUTB, OUTW and OUTY.
  void set_current_gain(uint8_t outr, uint8_t outg, uint8_t outb, uint8_t outw, uint8_t outy);

 protected:
  light::ESPColorView get_view_internal(int32_t index) const override;

  size_t get_frame_size_() const { return this->num_leds_ * BYTES_PER_CHIP + TRAILER_BYTES; }
  static bool on_transmit_done_(rmt_channel_handle_t channel, const rmt_tx_done_event_data_t *event, void *context);

  /// 8 bit colours, laid out like the outputs of a chip (see channel_colors_).
  uint8_t *colors_{nullptr};
  uint8_t *effect_data_{nullptr};
  /// Bytes on the wire. The RMT ISR reads it while the next colours are already being computed.
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
