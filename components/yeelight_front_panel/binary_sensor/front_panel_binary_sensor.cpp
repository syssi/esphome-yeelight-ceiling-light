#include "front_panel_binary_sensor.h"

#include "esphome/core/log.h"

namespace esphome::yeelight_front_panel {

static const char *const TAG = "yeelight_front_panel.binary_sensor";

void FrontPanelBinarySensor::dump_config() { LOG_BINARY_SENSOR("", "Yeelight front panel", this); }

}  // namespace esphome::yeelight_front_panel
