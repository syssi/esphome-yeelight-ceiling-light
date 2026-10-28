#include "front_panel_binary_sensor.h"

#include "esphome/core/log.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::yeelight_front_panel {

ESPHOME_LOG_TAG(TAG, "yeelight_front_panel.binary_sensor");

void FrontPanelBinarySensor::dump_config() { LOG_BINARY_SENSOR("", "Yeelight front panel", this); }

}  // namespace esphome::yeelight_front_panel
