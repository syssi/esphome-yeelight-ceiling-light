import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv

from .. import CONF_YEELIGHT_FRONT_PANEL_ID, YeelightFrontPanel, yeelight_front_panel_ns

DEPENDENCIES = ["yeelight_front_panel"]

CONF_PART = "part"

FrontPanelPart = yeelight_front_panel_ns.enum("FrontPanelPart", is_class=True)

PARTS = {
    "power_button": FrontPanelPart.POWER_BUTTON,
    "color_button": FrontPanelPart.COLOR_BUTTON,
    "slider": FrontPanelPart.SLIDER,
}

FrontPanelBinarySensor = yeelight_front_panel_ns.class_(
    "FrontPanelBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(FrontPanelBinarySensor)
    .extend(
        {
            cv.GenerateID(CONF_YEELIGHT_FRONT_PANEL_ID): cv.use_id(YeelightFrontPanel),
            cv.Required(CONF_PART): cv.one_of(*PARTS, lower=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    hub = await cg.get_variable(config[CONF_YEELIGHT_FRONT_PANEL_ID])
    cg.add(var.set_parent(hub))
    cg.add(var.set_part(PARTS[config[CONF_PART]]))
