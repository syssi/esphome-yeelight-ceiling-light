from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_ID, CONF_MODEL

CODEOWNERS = ["@syssi"]

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

CONF_YEELIGHT_FRONT_PANEL_ID = "yeelight_front_panel_id"
CONF_TRIGGER_PIN = "trigger_pin"
CONF_DEBUG = "debug"

yeelight_front_panel_ns = cg.esphome_ns.namespace("yeelight_front_panel")
YeelightFrontPanel = yeelight_front_panel_ns.class_(
    "YeelightFrontPanel", cg.Component, i2c.I2CDevice
)
Model = yeelight_front_panel_ns.enum("Model", is_class=True)

# Default I2C address per model, mirroring the constants in models.h.
MODELS = {
    "bslamp2": (Model.BSLAMP2, 0x2C),
    "lamp10": (Model.LAMP10, 0x50),
}

MODEL_OPTIONS = {name: value for name, (value, _) in MODELS.items()}


def _default_address(config):
    if CONF_ADDRESS not in config:
        _, address = MODELS[config[CONF_MODEL]]
        config[CONF_ADDRESS] = address
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(YeelightFrontPanel),
            cv.GenerateID(i2c.CONF_I2C_ID): cv.use_id(i2c.I2CBus),
            cv.Required(CONF_MODEL): cv.one_of(*MODELS, lower=True),
            cv.Optional(CONF_ADDRESS): cv.i2c_address,
            cv.Required(CONF_TRIGGER_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_DEBUG, default=False): cv.boolean,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _default_address,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_model(MODEL_OPTIONS[config[CONF_MODEL]]))
    cg.add(var.set_debug(config[CONF_DEBUG]))

    trigger_pin = await cg.gpio_pin_expression(config[CONF_TRIGGER_PIN])
    cg.add(var.set_trigger_pin(trigger_pin))
