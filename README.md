# everhome_ecotracker_emu
everHome EcoTracker Emulator for ESPHome.

Can be used with balcony storage battery like from Growatt.

Add to your config yaml:
```yaml
esphome:
  name: ecotracker-112233aabbcc

external_components:
  - source:
      type: local
      path: my_components
    components: [ everhome_ecotracker_emu ]

wifi:
  manual_ip:
    static_ip: 192.168.0.10

everhome_ecotracker_emu:
  serial_number: "112233aabbcc"
  own_ip: "192.168.0.10"
  total_power_sensor: current_power_w     # Aktuelle Wirkleistung (W)
  v1_sensor: voltage_l1            # Spannung Phase 1 (V)
  v2_sensor: voltage_l2            # Spannung Phase 2 (V)
  v3_sensor: voltage_l3            # Spannung Phase 3 (V)
  i1_sensor: current_l1            # Stromstärke Phase 1 (A)
  i2_sensor: current_l2            # Stromstärke Phase 2 (A)
  i3_sensor: current_l3            # Stromstärke Phase 3 (A)
  energy_in_sensor: total_energy               # Zählerstand Bezug (kWh) -> wird intern zu Wh
  energy_out_sensor: total_energy_exported     # Zählerstand Einspeisung (kWh) -> wird intern zu Wh
```

