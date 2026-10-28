from esphome import automation
import esphome.codegen as cg
from esphome.components import ble_device_base, sensor, xiaomi_ble
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_MAC_ADDRESS,
    CONF_ON_PRESS,
    CONF_TRIGGER_ID,
    DEVICE_CLASS_EMPTY,
    ICON_EMPTY,
    UNIT_EMPTY,
)
from esphome.types import ConfigType

CODEOWNERS = ["@syssi"]
DOMAIN = "xiaomi_ylyk01yl"

AUTO_LOAD = ["ble_device_base", "xiaomi_ble", "sensor"]
MULTI_CONF = True

CONF_LAST_BUTTON_PRESSED = "last_button_pressed"
CONF_ON_LONG_PRESS = "on_long_press"
CONF_KEYCODE = "keycode"

ON_PRESS_ACTIONS = [
    CONF_ON_PRESS,
    CONF_ON_LONG_PRESS,
]

xiaomi_ylyk01yl_ns = cg.esphome_ns.namespace("xiaomi_ylyk01yl")
XiaomiYLYK01YL = xiaomi_ylyk01yl_ns.class_(
    "XiaomiYLYK01YL", xiaomi_ble.XiaomiDevice, cg.Component
)

OnPressTrigger = xiaomi_ylyk01yl_ns.class_(
    "OnPressTrigger", automation.Trigger.template(cg.uint8)
)

OnLongPressTrigger = xiaomi_ylyk01yl_ns.class_(
    "OnLongPressTrigger", automation.Trigger.template(cg.uint8)
)

# pylint: disable=too-many-function-args
CONFIG_SCHEMA = cv.All(
    ble_device_base.rename_legacy_hub_id("xiaomi_ylyk01yl"),
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiYLYK01YL),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Optional(CONF_LAST_BUTTON_PRESSED): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon=ICON_EMPTY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
            ),
            cv.Optional(CONF_ON_PRESS): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnPressTrigger),
                    cv.Optional(CONF_KEYCODE): cv.uint8_t,
                }
            ),
            cv.Optional(CONF_ON_LONG_PRESS): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnLongPressTrigger),
                    cv.Optional(CONF_KEYCODE): cv.uint8_t,
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(ble_device_base.BLE_DEVICE_SCHEMA),
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_device_base.register_ble_device(var, config)

    xiaomi_ble.setup_xiaomi_device(var, config)

    if CONF_LAST_BUTTON_PRESSED in config:
        sens = await sensor.new_sensor(config[CONF_LAST_BUTTON_PRESSED])
        cg.add(var.set_keycode(sens))

    for action in ON_PRESS_ACTIONS:
        for conf in config.get(action, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            if CONF_KEYCODE in conf:
                cg.add(trigger.set_keycode(conf[CONF_KEYCODE]))
            await automation.build_automation(trigger, [(cg.uint8, "keycode")], conf)
