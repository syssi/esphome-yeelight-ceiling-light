import esphome.codegen as cg
from esphome.components import light, output
import esphome.config_validation as cv
from esphome.const import (
    CONF_COLD_WHITE,
    CONF_COLOR_TEMPERATURE,
    CONF_GAMMA_CORRECT,
    CONF_MIN_BRIGHTNESS,
    CONF_OUTPUT_ID,
    CONF_WARM_WHITE,
)

from . import yeelight_cct_ns

CODEOWNERS = ["@syssi"]

CONF_INTERPOLATION = "interpolation"
CONF_MIN_DUTY = "min_duty"
CONF_MIN_SHARE = "min_share"
CONF_MIXING_TABLE = "mixing_table"

# Python handles for the C++ class and enum, used to generate code that refers to them.
YeelightCCTLightOutput = yeelight_cct_ns.class_(
    "YeelightCCTLightOutput", light.LightOutput
)
Interpolation = yeelight_cct_ns.enum("Interpolation", is_class=True)
INTERPOLATIONS = {
    "none": Interpolation.NONE,
    "quadratic": Interpolation.QUADRATIC,
}

# One row of mixing_table. color_temperature is parsed to mireds, the shares to 0..1.
MIX_POINT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_COLOR_TEMPERATURE): cv.color_temperature,
        cv.Required(CONF_WARM_WHITE): cv.percentage,
        cv.Required(CONF_COLD_WHITE): cv.percentage,
    }
)


def kelvin(mireds):
    """Convert mireds to whole kelvin."""
    return round(1e6 / mireds)


def validate_mixing_table(value):
    """Require strictly rising colour temperatures."""
    temperatures = [kelvin(point[CONF_COLOR_TEMPERATURE]) for point in value]
    if temperatures != sorted(set(temperatures)):
        raise cv.Invalid(
            "mixing_table must be sorted by rising color_temperature without duplicates"
        )
    return value


# RGB_LIGHT_SCHEMA is the common schema of lights with outputs (name, transitions, effects, ...).
CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(YeelightCCTLightOutput),
        # PWM outputs of the two LED channels.
        cv.Required(CONF_WARM_WHITE): cv.use_id(output.FloatOutput),
        cv.Required(CONF_COLD_WHITE): cv.use_id(output.FloatOutput),
        cv.Required(CONF_MIXING_TABLE): cv.All(
            cv.ensure_list(MIX_POINT_SCHEMA),
            cv.Length(min=2),
            validate_mixing_table,
        ),
        cv.Optional(CONF_INTERPOLATION, default="quadratic"): cv.enum(
            INTERPOLATIONS, lower=True
        ),
        # quadratic only: keep the table point if a share would drop below this.
        cv.Optional(CONF_MIN_SHARE, default="0%"): cv.percentage,
        # Output at 1 % brightness.
        cv.Optional(CONF_MIN_BRIGHTNESS, default="0%"): cv.percentage,
        # Duty offset of every lit channel.
        cv.Optional(CONF_MIN_DUTY, default="0%"): cv.percentage,
        # The stock firmware scales the duty linearly with the brightness.
        cv.Optional(CONF_GAMMA_CORRECT, default=0.0): cv.positive_float,
    }
)


async def to_code(config):
    """Generate the C++ code that creates and configures the light output."""
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    cg.add(var.set_warm_white(await cg.get_variable(config[CONF_WARM_WHITE])))
    cg.add(var.set_cold_white(await cg.get_variable(config[CONF_COLD_WHITE])))
    for point in config[CONF_MIXING_TABLE]:
        cg.add(
            var.add_point(
                kelvin(point[CONF_COLOR_TEMPERATURE]),
                point[CONF_WARM_WHITE],
                point[CONF_COLD_WHITE],
            )
        )
    cg.add(var.set_interpolation(config[CONF_INTERPOLATION]))
    cg.add(var.set_min_share(config[CONF_MIN_SHARE]))
    cg.add(var.set_min_brightness(config[CONF_MIN_BRIGHTNESS]))
    cg.add(var.set_min_duty(config[CONF_MIN_DUTY]))
