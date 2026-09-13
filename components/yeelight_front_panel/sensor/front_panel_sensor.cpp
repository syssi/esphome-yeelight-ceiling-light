#include "front_panel_sensor.h"

#include "esphome/core/log.h"

namespace esphome::yeelight_front_panel {

static const char *const TAG = "yeelight_front_panel.sensor";

void FrontPanelSliderSensor::dump_config() { LOG_SENSOR("", "Yeelight front panel slider", this); }

}  // namespace esphome::yeelight_front_panel
