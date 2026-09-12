#include "front_panel_binary_sensor.h"

#include "esphome/core/log.h"

namespace esphome {
namespace yeelight_front_panel {

static const char *const TAG = "yeelight_front_panel.binary_sensor";

void FrontPanelBinarySensor::dump_config() { LOG_BINARY_SENSOR("", "Yeelight front panel", this); }

}  // namespace yeelight_front_panel
}  // namespace esphome
