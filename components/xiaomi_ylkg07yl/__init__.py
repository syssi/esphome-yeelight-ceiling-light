from esphome import automation
import esphome.codegen as cg
from esphome.components import esp32_ble_tracker, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_BINDKEY,
    CONF_ID,
    CONF_MAC_ADDRESS,
    CONF_ON_PRESS,
    CONF_TRIGGER_ID,
    DEVICE_CLASS_EMPTY,
    ICON_EMPTY,
    UNIT_EMPTY,
)

AUTO_LOAD = ["xiaomi_ble", "sensor"]
CODEOWNERS = ["@syssi"]
DEPENDENCIES = ["esp32_ble_tracker"]
MULTI_CONF = True

CONF_KEYCODE = "keycode"
CONF_ENCODER_VALUE = "encoder_value"
CONF_ACTION_TYPE = "action_type"

SENSORS = [
    CONF_KEYCODE,
    CONF_ENCODER_VALUE,
    CONF_ACTION_TYPE,
]

# CONF_ON_PRESS = "on_press"
CONF_ON_PRESS_AND_ROTATE = "on_press_and_rotate"
CONF_ON_ROTATE = "on_rotate"

ON_PRESS_ACTIONS = [
    CONF_ON_PRESS,
    CONF_ON_PRESS_AND_ROTATE,
    CONF_ON_ROTATE,
]

xiaomi_ylkg07yl_ns = cg.esphome_ns.namespace("xiaomi_ylkg07yl")
XiaomiYLKG07YL = xiaomi_ylkg07yl_ns.class_(
    "XiaomiYLKG07YL", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

OnPressTrigger = xiaomi_ylkg07yl_ns.class_(
    "OnPressTrigger", automation.Trigger.template()
)

OnPressAndRotateTrigger = xiaomi_ylkg07yl_ns.class_(
    "OnPressAndRotateTrigger", automation.Trigger.template()
)

OnRotateTrigger = xiaomi_ylkg07yl_ns.class_(
    "OnRotateTrigger", automation.Trigger.template()
)


def validate_short_bind_key(value):
    value = cv.string_strict(value)
    parts = [value[i : i + 2] for i in range(0, len(value), 2)]
    if len(parts) != 12:
        raise cv.Invalid("Bind key must consist of 12 hexadecimal numbers")
    parts_int = []
    if any(len(part) != 2 for part in parts):
        raise cv.Invalid("Bind key must be format XX")
    for part in parts:
        try:
            parts_int.append(int(part, 16))
        except ValueError:
            # pylint: disable=raise-missing-from
            raise cv.Invalid("Bind key must be hex values from 00 to FF")

    return "".join(f"{part:02X}" for part in parts_int)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiYLKG07YL),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Required(CONF_BINDKEY): validate_short_bind_key,
            cv.Optional(CONF_KEYCODE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon=ICON_EMPTY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
            ),
            cv.Optional(CONF_ENCODER_VALUE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon=ICON_EMPTY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
            ),
            cv.Optional(CONF_ACTION_TYPE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon=ICON_EMPTY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
            ),
            cv.Optional(CONF_ON_PRESS): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnPressTrigger),
                }
            ),
            cv.Optional(CONF_ON_PRESS_AND_ROTATE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        OnPressAndRotateTrigger
                    ),
                }
            ),
            cv.Optional(CONF_ON_ROTATE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnRotateTrigger),
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
    cg.add(var.set_bindkey(config[CONF_BINDKEY]))

    for key in SENSORS:
        if key in config:
            conf = config[key]
            sens = await sensor.new_sensor(conf)
            cg.add(getattr(var, f"set_{key}_sensor")(sens))

    for action in ON_PRESS_ACTIONS:
        for conf in config.get(action, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [(cg.int_, "x")], conf)
