#include "yeelight_front_panel.h"
#include "models.h"

#include "esphome/core/log.h"

#include <algorithm>
#include <cmath>

namespace esphome::yeelight_front_panel {

static const char *const TAG = "yeelight_front_panel";

void IRAM_ATTR HOT FrontPanelTriggerStore::gpio_intr(FrontPanelTriggerStore *store) { store->event_count++; }

void YeelightFrontPanel::setup() {
  this->model_ = get_model(this->model_id_);

  this->trigger_pin_->setup();
  this->trigger_pin_->attach_interrupt(FrontPanelTriggerStore::gpio_intr, &this->store_, gpio::INTERRUPT_FALLING_EDGE);
  ESP_LOGD(TAG, "Trigger pin level after setup: %d", this->trigger_pin_->digital_read());

  if (this->debug_) {
    // A trigger line stuck low, or a wrong pin that never moves, shows up here
    // even when no interrupt ever fires.
    this->set_interval("health", 10000, [this]() {
      ESP_LOGD(TAG, "Health: trigger level=%d, interrupts=%u, handled=%u, leds=0x%04X%s",
               this->trigger_pin_->digital_read(), this->store_.event_count, this->last_event_count_, this->led_state_,
               this->leds_dirty_ ? " (pending)" : "");
    });
  }
}

void YeelightFrontPanel::dump_config() {
  ESP_LOGCONFIG(TAG, "Yeelight front panel:");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->model_->name());
  ESP_LOGCONFIG(TAG, "  Message length: %u bytes", this->model_->message_length());
  ESP_LOGCONFIG(TAG, "  Slider: %u LEDs, %u levels", this->model_->slider_led_count(),
                this->model_->slider_level_count());
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Trigger pin: ", this->trigger_pin_);
}

void YeelightFrontPanel::loop() {
  const uint32_t event_count = this->store_.event_count;
  if (event_count != this->last_event_count_) {
    const uint32_t missed = event_count - this->last_event_count_ - 1;
    if (missed > 0) {
      ESP_LOGW(TAG, "Missed %u front panel event(s)", missed);
    }
    ESP_LOGD(TAG, "Trigger interrupt #%u, line now %d", event_count, this->trigger_pin_->digital_read());
    this->last_event_count_ = event_count;
    this->read_event_();
    this->last_read_ms_ = millis();
    this->last_trigger_level_ = false;
  } else {
    const bool level = this->trigger_pin_->digital_read();
    if (!level && millis() - this->last_read_ms_ >= 50) {
      // The falling edge alone loses events: the lamp10 panel was seen holding the
      // line low indefinitely after a burst of touches. Keep reading until released.
      ESP_LOGV(TAG, "Trigger line still low, polling");
      this->read_event_();
      this->last_read_ms_ = millis();
    } else if (level && !this->last_trigger_level_) {
      // The panel can release the line before its final state has been read -
      // a button release went missing that way. One more read picks it up.
      ESP_LOGD(TAG, "Trigger line released, reading final state");
      this->read_event_();
      this->last_read_ms_ = millis();
    }
    this->last_trigger_level_ = level;
  }

  if (this->leds_dirty_) {
    this->flush_leds_();
  }
}

void YeelightFrontPanel::read_event_() {
  const uint8_t length = this->model_->event_length();
  char hex[format_hex_pretty_size(MAX_MESSAGE_LENGTH)];

  const uint8_t *request = this->model_->event_request();
  if (request != nullptr) {
    ESP_LOGV(TAG, "Event request: %s", format_hex_pretty_to(hex, request, length, '.'));
    const i2c::ErrorCode err = this->write(request, length);
    if (err != i2c::ERROR_OK) {
      ESP_LOGW(TAG, "Requesting the pending event failed (i2c error %d)", err);
      return;
    }
  }

  uint8_t message[MAX_MESSAGE_LENGTH];
  const i2c::ErrorCode err = this->read(message, length);
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Reading the pending event failed (i2c error %d)", err);
    return;
  }

  FrontPanelEvent event;
  if (!this->model_->parse_event(message, &event)) {
    ESP_LOGW(TAG, "Unrecognised message: %s", format_hex_pretty_to(hex, message, length, '.'));
    return;
  }

  // Panels that hold the trigger line low are polled, and answer with an idle
  // message once nothing is pending. Those would drown everything else in the log.
  if (event.part == FrontPanelPart::UNKNOWN) {
    ESP_LOGV(TAG, "Idle message: %s", format_hex_pretty_to(hex, message, length, '.'));
  } else {
    if (this->debug_) {
      ESP_LOGI(TAG, "Message: %s", format_hex_pretty_to(hex, message, length, '.'));
    }
    ESP_LOGD(TAG, "Event: part=%u action=%u level=%u", static_cast<uint8_t>(event.part),
             static_cast<uint8_t>(event.action), event.slider_level);
  }

  this->event_callback_.call(event);
}

void YeelightFrontPanel::flush_leds_() {
  uint8_t message[MAX_MESSAGE_LENGTH];
  this->model_->encode_leds(this->led_state_, message);
  char hex[format_hex_pretty_size(MAX_MESSAGE_LENGTH)];
  ESP_LOGD(TAG, "LED update 0x%04X: %s", this->led_state_,
           format_hex_pretty_to(hex, message, this->model_->message_length(), '.'));
  const i2c::ErrorCode err = this->write(message, this->model_->message_length());
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Updating the front panel LEDs failed (i2c error %d)", err);
    return;
  }
  this->leds_dirty_ = false;
}

void YeelightFrontPanel::set_slider_level(float level) {
  if (!(level > 0.0f)) {
    this->turn_off_leds(LED_ALL_SLIDER);
    return;
  }

  const uint8_t count = this->model_->slider_led_count();
  uint8_t lit = static_cast<uint8_t>(std::ceil(std::min(level, 1.0f) * count));
  if (lit < 1)
    lit = 1;

  uint16_t leds = 0;
  for (uint8_t i = 0; i < lit; i++)
    leds |= 1 << i;

  this->turn_off_leds(LED_ALL_SLIDER);
  this->turn_on_leds(leds);
}

uint8_t YeelightFrontPanel::slider_level_count() const {
  return this->model_ == nullptr ? 0 : this->model_->slider_level_count();
}

}  // namespace esphome::yeelight_front_panel
