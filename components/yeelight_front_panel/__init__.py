from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_DEBUG, CONF_ID, CONF_TRIGGER_PIN

CODEOWNERS = ["@syssi"]
DOMAIN = "yeelight_front_panel"

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

CONF_YEELIGHT_FRONT_PANEL_ID = "yeelight_front_panel_id"

yeelight_front_panel_ns = cg.esphome_ns.namespace("yeelight_front_panel")
YeelightFrontPanel = yeelight_front_panel_ns.class_(
    "YeelightFrontPanel", cg.Component, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(YeelightFrontPanel),
            cv.Required(CONF_TRIGGER_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_DEBUG, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x50))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_debug(config[CONF_DEBUG]))

    trigger_pin = await cg.gpio_pin_expression(config[CONF_TRIGGER_PIN])
    cg.add(var.set_trigger_pin(trigger_pin))
