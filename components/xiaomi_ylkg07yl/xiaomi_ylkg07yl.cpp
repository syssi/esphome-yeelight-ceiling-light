#include "xiaomi_ylkg07yl.h"
#include "esphome/core/log.h"

namespace esphome::xiaomi_ylkg07yl {

ESPHOME_LOG_TAG(TAG, "xiaomi_ylkg07yl");

void XiaomiYLKG07YL::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi YLKG07YL");
  this->dump_bindkey_(TAG);
  LOG_SENSOR("  ", "Keycode", this->keycode_sensor_);
  LOG_SENSOR("  ", "Encoder value", this->encoder_value_sensor_);
  LOG_SENSOR("  ", "Action type", this->action_type_sensor_);
}

bool XiaomiYLKG07YL::decrypt_payload(uint8_t *raw, size_t size, const uint8_t *bindkey) {
  // Older firmware sends MiBeacon V2/V3 frames encrypted with the 12 byte bindkey,
  // newer firmware MiBeacon V4/V5 frames encrypted with the padded 16 byte bindkey
  if (xiaomi_ble::decrypt_xiaomi_legacy_payload(raw, size, bindkey, this->address_))
    return true;
  return xiaomi_ble::decrypt_xiaomi_payload(raw, size, bindkey, this->address_);
}

void XiaomiYLKG07YL::publish_result(const xiaomi_ble::XiaomiParseResult &res) {
  if (!res.keycode.has_value())
    return;

  // The encoder value is a signed rotation step count
  optional<int> encoder_value;
  if (res.encoder_value.has_value())
    encoder_value = static_cast<int8_t>(*res.encoder_value);

  if (this->keycode_sensor_ != nullptr)
    this->keycode_sensor_->publish_state(*res.keycode);

  if (this->encoder_value_sensor_ != nullptr && encoder_value.has_value())
    this->encoder_value_sensor_->publish_state(*encoder_value);

  if (this->action_type_sensor_ != nullptr && res.action_type.has_value())
    this->action_type_sensor_->publish_state(*res.action_type);

  if (encoder_value.has_value() && res.action_type.has_value()) {
    if (*res.action_type == ACTION_TYPE_PRESS && *res.keycode == KEYCODE_BUTTON)
      this->press_callback_.call(*encoder_value);
    if (*res.action_type == ACTION_TYPE_ROTATE && *res.keycode == KEYCODE_BUTTON)
      this->rotate_callback_.call(*encoder_value);
    if (*res.action_type == ACTION_TYPE_ROTATE && *res.keycode != KEYCODE_BUTTON)
      this->press_and_rotate_callback_.call(*res.keycode);
  }
}

}  // namespace esphome::xiaomi_ylkg07yl
