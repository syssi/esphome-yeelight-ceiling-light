#include "sm16825_light_output.h"

#ifdef USE_ESP32

#include <cinttypes>
#include <cstring>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <esp_attr.h>
#include <esp_clk_tree.h>

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::sm16825 {

ESPHOME_LOG_TAG(TAG, "sm16825");

static const size_t RMT_SYMBOLS_PER_BYTE = 8;

// Typical timings of the SM16825E datasheet
static const uint32_t BIT0_HIGH_NS = 300;
static const uint32_t BIT0_LOW_NS = 900;
static const uint32_t BIT1_HIGH_NS = 900;
static const uint32_t BIT1_LOW_NS = 300;
// The chips latch the frame after at least 200 µs low
static const uint32_t RESET_LOW_NS = 300000;

static uint32_t rmt_resolution_hz() {
  uint32_t frequency;
  esp_clk_tree_src_get_freq_hz((soc_module_clk_t) RMT_CLK_SRC_DEFAULT, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
                               &frequency);
  return frequency;
}

static size_t IRAM_ATTR HOT encoder_callback(const void *data, size_t size, size_t symbols_written, size_t symbols_free,
                                             rmt_symbol_word_t *symbols, bool *done, void *arg) {
  auto *rmt_symbols = static_cast<RmtSymbols *>(arg);
  const auto *bytes = static_cast<const uint8_t *>(data);
  size_t index = symbols_written / RMT_SYMBOLS_PER_BYTE;

  // Convert byte to symbols, MSB first
  if (index < size) {
    if (symbols_free < RMT_SYMBOLS_PER_BYTE) {
      return 0;
    }
    for (size_t bit = 0; bit < RMT_SYMBOLS_PER_BYTE; bit++) {
      symbols[bit] = (bytes[index] & (1 << (7 - bit))) ? rmt_symbols->bit1 : rmt_symbols->bit0;
    }
    return RMT_SYMBOLS_PER_BYTE;
  }

  // Send reset
  if (symbols_free < 1) {
    return 0;
  }
  symbols[0] = rmt_symbols->reset;
  *done = true;
  return 1;
}

bool IRAM_ATTR SM16825LightOutput::on_transmit_done_(rmt_channel_handle_t channel,
                                                     const rmt_tx_done_event_data_t *event, void *context) {
  static_cast<SM16825LightOutput *>(context)->transmitting_.store(false, std::memory_order_release);
  return false;
}

void SM16825LightOutput::set_current_gain(uint8_t outr, uint8_t outg, uint8_t outb, uint8_t outw, uint8_t outy) {
  // 5 bit gain per output, standby bits 00 (active), reserved bits 11111
  uint32_t word = (uint32_t) (outr & 0x1F) << 27 | (uint32_t) (outg & 0x1F) << 22 | (uint32_t) (outb & 0x1F) << 17 |
                  (uint32_t) (outw & 0x1F) << 12 | (uint32_t) (outy & 0x1F) << 7 | 0x1F;
  this->trailer_[0] = word >> 24;
  this->trailer_[1] = word >> 16;
  this->trailer_[2] = word >> 8;
  this->trailer_[3] = word;
}

void SM16825LightOutput::setup() {
  RAMAllocator<uint8_t> allocator(RAMAllocator<uint8_t>::ALLOC_INTERNAL);
  const size_t colors_size = this->num_leds_ * this->channel_colors_.bytes_per_led();
  this->colors_ = allocator.allocate(colors_size);
  this->effect_data_ = allocator.allocate(this->num_leds_);
  this->frame_ = allocator.allocate(this->get_frame_size_());
  if (this->colors_ == nullptr || this->effect_data_ == nullptr || this->frame_ == nullptr) {
    ESP_LOGE(TAG, "Cannot allocate LED buffers!");
    this->mark_failed();
    return;
  }
  memset(this->colors_, 0, colors_size);
  memset(this->effect_data_, 0, this->num_leds_);
  memset(this->frame_, 0, this->get_frame_size_());
  memcpy(this->frame_ + this->num_leds_ * BYTES_PER_CHIP, this->trailer_, TRAILER_BYTES);

  const float ticks_per_ns = (float) rmt_resolution_hz() / 1e09f;
  this->symbols_.bit0.duration0 = (uint32_t) (ticks_per_ns * BIT0_HIGH_NS);
  this->symbols_.bit0.level0 = 1;
  this->symbols_.bit0.duration1 = (uint32_t) (ticks_per_ns * BIT0_LOW_NS);
  this->symbols_.bit0.level1 = 0;
  this->symbols_.bit1.duration0 = (uint32_t) (ticks_per_ns * BIT1_HIGH_NS);
  this->symbols_.bit1.level0 = 1;
  this->symbols_.bit1.duration1 = (uint32_t) (ticks_per_ns * BIT1_LOW_NS);
  this->symbols_.bit1.level1 = 0;
  // A zero duration would end the transmission, so both halves of the reset symbol are low
  this->symbols_.reset.duration0 = (uint32_t) (ticks_per_ns * RESET_LOW_NS / 2);
  this->symbols_.reset.level0 = 0;
  this->symbols_.reset.duration1 = (uint32_t) (ticks_per_ns * RESET_LOW_NS / 2);
  this->symbols_.reset.level1 = 0;

  rmt_tx_channel_config_t channel;
  memset(&channel, 0, sizeof(channel));
  channel.clk_src = RMT_CLK_SRC_DEFAULT;
  channel.resolution_hz = rmt_resolution_hz();
  channel.gpio_num = gpio_num_t(this->pin_);
  channel.mem_block_symbols = this->rmt_symbols_;
  channel.trans_queue_depth = 1;
  channel.flags.invert_out = this->invert_out_;
  if (rmt_new_tx_channel(&channel, &this->channel_) != ESP_OK) {
    ESP_LOGE(TAG, "Channel creation failed");
    this->mark_failed();
    return;
  }

  rmt_simple_encoder_config_t encoder;
  memset(&encoder, 0, sizeof(encoder));
  encoder.callback = encoder_callback;
  encoder.arg = &this->symbols_;
  encoder.min_chunk_size = RMT_SYMBOLS_PER_BYTE;
  if (rmt_new_simple_encoder(&encoder, &this->encoder_) != ESP_OK) {
    ESP_LOGE(TAG, "Encoder creation failed");
    this->mark_failed();
    return;
  }

  rmt_tx_event_callbacks_t callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.on_trans_done = on_transmit_done_;
  if (rmt_tx_register_event_callbacks(this->channel_, &callbacks, this) != ESP_OK ||
      rmt_enable(this->channel_) != ESP_OK) {
    ESP_LOGE(TAG, "Enabling channel failed");
    this->mark_failed();
    return;
  }
}

void SM16825LightOutput::write_state(light::LightState *state) {
  if (this->channel_ == nullptr) {
    return;
  }
  // Don't block while the previous frame is still on the wire
  if (this->transmitting_.load(std::memory_order_acquire)) {
    this->schedule_show();
    return;
  }
  this->mark_shown_();

  // Expand each 8 bit color to the 16 bit grayscale of its output (0xFF -> 0xFFFF)
  const uint8_t bytes_per_led = this->channel_colors_.bytes_per_led();
  const uint8_t *color = this->colors_;
  uint8_t *chip = this->frame_;
  for (uint16_t led = 0; led < this->num_leds_; led++) {
    for (uint8_t output = 0; output < bytes_per_led; output++) {
      chip[output * 2] = color[output];
      chip[output * 2 + 1] = color[output];
    }
    color += bytes_per_led;
    chip += BYTES_PER_CHIP;
  }

  rmt_transmit_config_t config;
  memset(&config, 0, sizeof(config));
  this->transmitting_.store(true, std::memory_order_release);
  if (rmt_transmit(this->channel_, this->encoder_, this->frame_, this->get_frame_size_(), &config) != ESP_OK) {
    this->transmitting_.store(false, std::memory_order_release);
    ESP_LOGE(TAG, "RMT TX error");
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();
}

light::LightTraits SM16825LightOutput::get_traits() {
  auto traits = light::LightTraits();
  if (this->channel_colors_.has_white()) {
    traits.set_supported_color_modes({light::ColorMode::RGB_WHITE, light::ColorMode::WHITE});
  } else {
    traits.set_supported_color_modes({light::ColorMode::RGB});
  }
  return traits;
}

void SM16825LightOutput::clear_effect_data() { memset(this->effect_data_, 0, this->num_leds_); }

light::ESPColorView SM16825LightOutput::get_view_internal(int32_t index) const {
  const light::ChannelColors &colors = this->channel_colors_;
  uint8_t *led = this->colors_ + (index * colors.bytes_per_led());
  return {led + colors.r,
          led + colors.g,
          led + colors.b,
          colors.has_white() ? led + colors.w : nullptr,
          &this->effect_data_[index],
          &this->correction_};
}

void SM16825LightOutput::dump_config() {
  char channel_colors[5];
  ESP_LOGCONFIG(TAG,
                "SM16825:\n"
                "  Pin: %u\n"
                "  Number of LEDs: %u\n"
                "  Channel colors: %s\n"
                "  Current gain word: 0x%02X%02X%02X%02X\n"
                "  RMT Symbols: %" PRIu32,
                this->pin_, this->num_leds_, this->channel_colors_.to_string(channel_colors), this->trailer_[0],
                this->trailer_[1], this->trailer_[2], this->trailer_[3], this->rmt_symbols_);
}

}  // namespace esphome::sm16825

#endif  // USE_ESP32
