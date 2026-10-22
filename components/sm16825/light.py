from esphome import pins
import esphome.codegen as cg
from esphome.components import esp32, esp32_rmt, light
from esphome.components.const import CONF_CHANNEL_COLORS
from esphome.components.esp32 import include_builtin_idf_component
import esphome.config_validation as cv
from esphome.const import (
    CONF_INVERTED,
    CONF_NUM_LEDS,
    CONF_NUMBER,
    CONF_OUTPUT_ID,
    CONF_PIN,
    CONF_RMT_SYMBOLS,
)
from esphome.types import ConfigType

from . import sm16825_ns

DEPENDENCIES = ["esp32"]

CONF_CURRENT_GAIN = "current_gain"
CHIP_OUTPUTS = ("outr", "outg", "outb", "outw", "outy")

SM16825LightOutput = sm16825_ns.class_("SM16825LightOutput", light.AddressableLight)

# 0 = 10.2 mA ... 31 = 310 mA
CURRENT_GAIN_SCHEMA = cv.Schema(
    {
        cv.Optional(output, default=0): cv.int_range(min=0, max=31)
        for output in CHIP_OUTPUTS
    }
)


def validate_idf_version(config: ConfigType) -> ConfigType:
    # rmt_new_simple_encoder requires ESP-IDF 5.3
    if esp32.idf_version() < cv.Version(5, 3, 0):
        raise cv.Invalid("The sm16825 light requires ESP-IDF 5.3 or newer")
    return config


CONFIG_SCHEMA = cv.All(
    esp32.only_on_variant(
        unsupported=list(esp32_rmt.VARIANTS_NO_RMT),
        msg_prefix="SM16825",
    ),
    light.ADDRESSABLE_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(SM16825LightOutput),
            cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_NUM_LEDS): cv.int_range(min=1, max=1024),
            cv.Optional(
                CONF_CHANNEL_COLORS, default="RGB"
            ): light.validate_channel_colors,
            cv.Optional(CONF_CURRENT_GAIN, default={}): CURRENT_GAIN_SCHEMA,
            cv.SplitDefault(
                CONF_RMT_SYMBOLS,
                esp32=192,
                esp32_c3=96,
                esp32_c5=96,
                esp32_c6=96,
                esp32_h2=96,
                esp32_p4=192,
                esp32_s2=192,
                esp32_s3=192,
            ): cv.int_range(min=2),
        }
    ).extend(cv.COMPONENT_SCHEMA),
)

FINAL_VALIDATE_SCHEMA = validate_idf_version


async def to_code(config: ConfigType) -> None:
    # Re-enable ESP-IDF's RMT driver (excluded by default to save compile time)
    include_builtin_idf_component("esp_driver_rmt")

    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    await cg.register_component(var, config)

    cg.add(var.set_num_leds(config[CONF_NUM_LEDS]))
    cg.add(var.set_pin(config[CONF_PIN][CONF_NUMBER]))
    if config[CONF_PIN][CONF_INVERTED]:
        cg.add(var.set_inverted(True))
    cg.add(
        var.set_channel_colors(light.channel_colors_struct(config[CONF_CHANNEL_COLORS]))
    )
    gains = config[CONF_CURRENT_GAIN]
    cg.add(var.set_current_gain(*(gains[output] for output in CHIP_OUTPUTS)))
    cg.add(var.set_rmt_symbols(config[CONF_RMT_SYMBOLS]))
