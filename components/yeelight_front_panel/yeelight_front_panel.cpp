#include "yeelight_front_panel.h"

#include "esphome/core/log.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace esphome::yeelight_front_panel {

static const char *const TAG = "yeelight_front_panel";

void IRAM_ATTR HOT FrontPanelTriggerStore::gpio_intr(FrontPanelTriggerStore *store) {
  // Plain assignment: increment of a volatile object is deprecated in C++20. Only the ISR writes it.
  store->event_count = store->event_count + 1;
}

void YeelightFrontPanel::setup() {
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
  ESP_LOGCONFIG(TAG, "  Slider: %u LEDs, %u levels", SLIDER_LEDS, SLIDER_LEVELS);
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
      // The falling edge alone loses events: the panel was seen holding the
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
  char hex[format_hex_pretty_size(EVENT_LENGTH)];

  // No request message is needed; a plain read returns the panel state.
  uint8_t message[EVENT_LENGTH];
  const i2c::ErrorCode err = this->read(message, EVENT_LENGTH);
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Reading the pending event failed (i2c error %d)", err);
    return;
  }

  FrontPanelEvent event;
  if (!this->parse_event_(message, &event)) {
    ESP_LOGW(TAG, "Unrecognised message: %s", format_hex_pretty_to(hex, message, EVENT_LENGTH, '.'));
    return;
  }

  // While the trigger line is held low the panel is polled, and answers with an
  // idle message once nothing is pending. Those would drown everything else in the log.
  if (event.part == FrontPanelPart::UNKNOWN) {
    ESP_LOGV(TAG, "Idle message: %s", format_hex_pretty_to(hex, message, EVENT_LENGTH, '.'));
  } else {
    if (this->debug_) {
      ESP_LOGI(TAG, "Message: %s", format_hex_pretty_to(hex, message, EVENT_LENGTH, '.'));
    }
    ESP_LOGD(TAG, "Event: part=%u action=%u level=%u", static_cast<uint8_t>(event.part),
             static_cast<uint8_t>(event.action), event.slider_level);
  }

  this->event_callback_.call(event);
}

bool YeelightFrontPanel::parse_event_(const uint8_t *message, FrontPanelEvent *event) {
  if (message[0] != 0x0A || message[5] != 0x00 || message[4] >= SLIDER_LEVELS)
    return false;

  // The panel reports its whole state on every read and never clears stale
  // fields: after a button press the slider still shows its last state and
  // position. Only a field that changed since the previous message is an event.
  // Where several changed, the original touch_event_handler's precedence
  // applies: colour button over power button over slider.
  //
  // A button can also drop from touch or held straight to 00 without the 03
  // ever being read (seen when the panel released the trigger line early).
  // That drop counts as the release.
  const auto button_changed = [this, message](uint8_t i) {
    if (message[i] == this->last_message_[i])
      return false;
    return message[i] != 0x00 || this->last_message_[i] == 0x01 || this->last_message_[i] == 0x02;
  };
  const bool color = button_changed(2);
  const bool power = button_changed(1);
  const bool slider =
      (message[3] != this->last_message_[3] || message[4] != this->last_message_[4]) && message[3] != 0x00;
  std::memcpy(this->last_message_, message, EVENT_LENGTH);

  if (color || power) {
    event->part = color ? FrontPanelPart::COLOR_BUTTON : FrontPanelPart::POWER_BUTTON;
    switch (color ? message[2] : message[1]) {
      case 0x01:
        event->action = FrontPanelAction::TOUCH;
        return true;
      case 0x02:
        // Held. Sent once on the change, then repeated unchanged.
        event->action = FrontPanelAction::UNKNOWN;
        return true;
      case 0x00:
      case 0x03:
        event->action = FrontPanelAction::RELEASE;
        return true;
      default:
        return false;
    }
  }

  if (slider) {
    event->part = FrontPanelPart::SLIDER;
    // 01 on touch and while moving, 02 on release.
    switch (message[3]) {
      case 0x01:
        event->action = FrontPanelAction::TOUCH;
        break;
      case 0x02:
        event->action = FrontPanelAction::RELEASE;
        break;
      default:
        return false;
    }
    // Position 0 is the "-" end, next to the power button - already level 1.
    // The position sent with a release is not where the finger left: captures
    // show 06, 0B or 0C regardless. Only touches carry a level.
    if (event->action == FrontPanelAction::TOUCH)
      event->slider_level = message[4] + 1;
    return true;
  }

  // Nothing changed: a repeat, or the idle answer while the line is still low.
  return true;
}

void YeelightFrontPanel::flush_leds_() {
  const uint8_t message[LED_MESSAGE_LENGTH] = {LED_COMMAND, static_cast<uint8_t>(this->led_state_ >> 8),
                                               static_cast<uint8_t>(this->led_state_ & 0xFF)};
  char hex[format_hex_pretty_size(LED_MESSAGE_LENGTH)];
  ESP_LOGD(TAG, "LED update 0x%04X: %s", this->led_state_, format_hex_pretty_to(hex, message, LED_MESSAGE_LENGTH, '.'));
  const i2c::ErrorCode err = this->write(message, LED_MESSAGE_LENGTH);
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

  uint8_t lit = static_cast<uint8_t>(std::ceil(std::min(level, 1.0f) * SLIDER_LEDS));
  if (lit < 1)
    lit = 1;

  uint16_t leds = 0;
  for (uint8_t i = 0; i < lit; i++)
    leds |= 1 << i;

  this->turn_off_leds(LED_ALL_SLIDER);
  this->turn_on_leds(leds);
}

}  // namespace esphome::yeelight_front_panel
