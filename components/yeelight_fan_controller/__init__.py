import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@syssi"]

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["fan"]
MULTI_CONF = True

CONF_YEELIGHT_FAN_CONTROLLER_ID = "yeelight_fan_controller_id"

yeelight_fan_controller_ns = cg.esphome_ns.namespace("yeelight_fan_controller")
YeelightFanController = yeelight_fan_controller_ns.class_(
    "YeelightFanController", cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(YeelightFanController),
    }
).extend(uart.UART_DEVICE_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
