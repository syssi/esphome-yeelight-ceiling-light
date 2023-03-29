from esphome import automation
import esphome.codegen as cg
from esphome.components import esp32_ble_tracker, sensor
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

CODEOWNERS = ["@syssi"]

DEPENDENCIES = ["esp32_ble_tracker"]
AUTO_LOAD = ["xiaomi_ble", "sensor"]
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
    "XiaomiYLYK01YL", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

OnPressTrigger = xiaomi_ylyk01yl_ns.class_(
    "OnPressTrigger", automation.Trigger.template(cg.uint8)
)

OnLongPressTrigger = xiaomi_ylyk01yl_ns.class_(
    "OnLongPressTrigger", automation.Trigger.template(cg.uint8)
)

# pylint: disable=too-many-function-args
CONFIG_SCHEMA = (
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
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))

    if CONF_LAST_BUTTON_PRESSED in config:
        sens = await sensor.new_sensor(config[CONF_LAST_BUTTON_PRESSED])
        cg.add(var.set_keycode(sens))

    for action in ON_PRESS_ACTIONS:
        for conf in config.get(action, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            if CONF_KEYCODE in conf:
                cg.add(trigger.set_keycode(conf[CONF_KEYCODE]))
            await automation.build_automation(trigger, [(cg.uint8, "keycode")], conf)
