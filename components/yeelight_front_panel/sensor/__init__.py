import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import STATE_CLASS_MEASUREMENT

from .. import CONF_YEELIGHT_FRONT_PANEL_ID, YeelightFrontPanel, yeelight_front_panel_ns

DEPENDENCIES = ["yeelight_front_panel"]

FrontPanelSliderSensor = yeelight_front_panel_ns.class_(
    "FrontPanelSliderSensor", sensor.Sensor, cg.Component
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        FrontPanelSliderSensor,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(CONF_YEELIGHT_FRONT_PANEL_ID): cv.use_id(YeelightFrontPanel),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    hub = await cg.get_variable(config[CONF_YEELIGHT_FRONT_PANEL_ID])
    cg.add(var.set_parent(hub))
