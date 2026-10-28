#include "xiaomi_ylyk01yl.h"
#include "esphome/core/log.h"

namespace esphome::xiaomi_ylyk01yl {

ESPHOME_LOG_TAG(TAG, "xiaomi_ylyk01yl");

void XiaomiYLYK01YL::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi YLYK01YL");
  LOG_SENSOR("  ", "Keycode", this->keycode_);
}

void XiaomiYLYK01YL::publish_result(const xiaomi_ble::XiaomiParseResult &res) {
  if (!res.keycode.has_value())
    return;

  if (this->keycode_ != nullptr)
    this->keycode_->publish_state(*res.keycode);

  if (res.action_type.has_value())
    this->receive_callback_.call(*res.keycode, *res.action_type == ACTION_TYPE_LONG_PRESS);
}

void XiaomiYLYK01YL::add_on_receive_callback(std::function<void(uint8_t, bool)> &&callback) {
  this->receive_callback_.add(std::move(callback));
}

}  // namespace esphome::xiaomi_ylyk01yl
