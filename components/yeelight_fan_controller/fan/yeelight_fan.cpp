#include "yeelight_fan.h"
#include "esphome/core/log.h"

namespace esphome {
namespace yeelight_fan_controller {

static const char *const TAG = "yeelight_fan_controller.fan";

static const uint8_t FUNCTION_TURN_OFF = 0x01;
static const uint8_t FUNCTION_TURN_ON = 0x04;
static const uint8_t FUNCTION_SET_SPEED = 0x03;

void YeelightFan::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(*this);
    this->write_state_();
  }
}

void YeelightFan::dump_config() { LOG_FAN("", "YeelightFanController Fan", this); }

fan::FanTraits YeelightFan::get_traits() { return fan::FanTraits(false, true, true, 100); }

void YeelightFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value()) {
    this->last_state_ = this->state;
    this->state = *call.get_state();
  }
  if (call.get_speed().has_value())
    this->speed = *call.get_speed();
  if (call.get_direction().has_value())
    this->direction = *call.get_direction();

  this->write_state_();
  this->publish_state();
}

void YeelightFan::write_state_() {
  if (this->state != this->last_state_) {
    if (this->state) {
      this->parent_->send_command(FUNCTION_TURN_ON, 0x13);
    } else {
      this->parent_->send_command(FUNCTION_TURN_OFF, 0x11);
    }
  }

  if (this->state) {
    // if (this->direction == fan::FanDirection::REVERSE) {
    this->parent_->send_command(FUNCTION_SET_SPEED, this->speed);
    // } else {
    //  this->parent_->send_command(FUNCTION_SET_SPEED, this->speed);
    // }
  }
}

}  // namespace yeelight_fan_controller
}  // namespace esphome
