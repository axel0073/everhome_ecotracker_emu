import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

# Registriere den Namensraum und die C++ Klasse
everhome_ecotracker_emu_ns = cg.esphome_ns.namespace("everhome_ecotracker_emu")
EverhomeEcoTrackerEmu = everhome_ecotracker_emu_ns.class_("EverhomeEcoTrackerEmu", cg.Component)

CONF_TOTAL_POWER = "total_power_sensor"
CONF_V1 = "v1_sensor"
CONF_V2 = "v2_sensor"
CONF_V3 = "v3_sensor"
CONF_I1 = "i1_sensor"
CONF_I2 = "i2_sensor"
CONF_I3 = "i3_sensor"
CONF_ENERGY_IN = "energy_in_sensor"
CONF_ENERGY_OUT = "energy_out_sensor"
CONF_SERIAL_NUMBER = "serial_number"
CONF_OWN_IP = "own_ip"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EverhomeEcoTrackerEmu),
    cv.Required(CONF_TOTAL_POWER): cv.use_id(sensor.Sensor),
    cv.Required(CONF_V1): cv.use_id(sensor.Sensor),
    cv.Required(CONF_V2): cv.use_id(sensor.Sensor),
    cv.Required(CONF_V3): cv.use_id(sensor.Sensor),
    cv.Required(CONF_I1): cv.use_id(sensor.Sensor),
    cv.Required(CONF_I2): cv.use_id(sensor.Sensor),
    cv.Required(CONF_I3): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_ENERGY_IN): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_ENERGY_OUT): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_SERIAL_NUMBER, default="112233aabbcc"): cv.string,
    cv.Optional(CONF_OWN_IP, default="192.168.0.10"): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    cg.add_global(cg.RawStatement('#include "esphome/components/everhome_ecotracker_emu/everhome_ecotracker_emu.h"'))

    # Erstelle die Instanz der Komponente
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Hole die C++ Variablen-Zeiger der einzelnen Sensoren
    tot_p = await cg.get_variable(config[CONF_TOTAL_POWER])
    v1 = await cg.get_variable(config[CONF_V1])
    v2 = await cg.get_variable(config[CONF_V2])
    v3 = await cg.get_variable(config[CONF_V3])
    i1 = await cg.get_variable(config[CONF_I1])
    i2 = await cg.get_variable(config[CONF_I2])
    i3 = await cg.get_variable(config[CONF_I3])
    
    e_in = await cg.get_variable(config[CONF_ENERGY_IN]) if CONF_ENERGY_IN in config else None
    e_out = await cg.get_variable(config[CONF_ENERGY_OUT]) if CONF_ENERGY_OUT in config else None

    # Übergib die Sensoren an die set_sensors-Methode im C++ Code
    cg.add(var.set_sensors(tot_p, v1, v2, v3, i1, i2, i3, e_in, e_out))

    cg.add(var.set_serial_number(config[CONF_SERIAL_NUMBER]))
    cg.add(var.set_ip(config[CONF_OWN_IP]))
