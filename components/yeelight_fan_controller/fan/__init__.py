import esphome.codegen as cg
from esphome.components import fan
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import (
    CONF_YEELIGHT_FAN_CONTROLLER_ID,
    YeelightFanController,
    yeelight_fan_controller_ns,
)

AUTO_LOAD = ["yeelight_fan_controller"]

YeelightFan = yeelight_fan_controller_ns.class_("YeelightFan", cg.Component, fan.Fan)

CONFIG_SCHEMA = fan.FAN_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(YeelightFan),
        cv.GenerateID(CONF_YEELIGHT_FAN_CONTROLLER_ID): cv.use_id(
            YeelightFanController
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_YEELIGHT_FAN_CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_ID], hub)
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
