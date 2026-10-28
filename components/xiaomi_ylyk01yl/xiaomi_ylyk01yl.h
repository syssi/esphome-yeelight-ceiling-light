#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/xiaomi_ble/xiaomi_ble.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome::xiaomi_ylyk01yl {

// static const uint8_t BUTTON_ON = 0;
// static const uint8_t BUTTON_OFF = 1;
// static const uint8_t BUTTON_SUN = 2;
// static const uint8_t BUTTON_M = 4;
// static const uint8_t BUTTON_PLUS = 3;
// static const uint8_t BUTTON_MINUS = 5;

static const uint8_t ACTION_TYPE_LONG_PRESS = 2;

class XiaomiYLYK01YL final : public Component, public xiaomi_ble::XiaomiDevice {
 public:
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_keycode(sensor::Sensor *keycode) { keycode_ = keycode; }
  void add_on_receive_callback(std::function<void(uint8_t, bool)> &&callback);

 protected:
  void publish_result(const xiaomi_ble::XiaomiParseResult &res) override;

  sensor::Sensor *keycode_{nullptr};
  CallbackManager<void(uint8_t, bool)> receive_callback_{};
};

class OnPressTrigger : public Trigger<uint8_t> {
 public:
  OnPressTrigger(XiaomiYLYK01YL *parent) {
    parent->add_on_receive_callback([this](uint8_t keycode, bool is_long_press) {
      if (!is_long_press) {
        if (this->keycode_ != 255) {
          if (this->keycode_ == keycode) {
            this->trigger(keycode);
          }
        } else {
          this->trigger(keycode);
        }
      }
    });
  }

  void set_keycode(uint8_t keycode) { this->keycode_ = keycode; }

 protected:
  uint8_t keycode_ = 255;
};

class OnLongPressTrigger : public Trigger<uint8_t> {
 public:
  OnLongPressTrigger(XiaomiYLYK01YL *parent) {
    parent->add_on_receive_callback([this](uint8_t keycode, bool is_long_press) {
      if (is_long_press) {
        if (this->keycode_ != 255) {
          if (this->keycode_ == keycode) {
            this->trigger(keycode);
          }
        } else {
          this->trigger(keycode);
        }
      }
    });
  }

  void set_keycode(uint8_t keycode) { this->keycode_ = keycode; }

 protected:
  uint8_t keycode_ = 255;
};

}  // namespace esphome::xiaomi_ylyk01yl
