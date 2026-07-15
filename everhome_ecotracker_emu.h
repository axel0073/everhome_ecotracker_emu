#pragma once

#ifdef USE_ESP_IDF

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esp_http_server.h"

namespace esphome {
namespace everhome_ecotracker_emu {

class EverhomeEcoTrackerEmu : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Gibt den Namen der Komponente zurück (erforderlich für das ESPHome-Framework)
  const char *get_component_name() const { return "everhome_ecotracker_emu"; }

  // Bestimmt die Startreihenfolge im ESPHome-Framework (nach dem WLAN)
  float get_setup_priority() const override { return setup_priority::WIFI + 10.0f; }

  void set_sensors(esphome::sensor::Sensor *tot_p,
                   esphome::sensor::Sensor *v1, esphome::sensor::Sensor *v2, esphome::sensor::Sensor *v3,
                   esphome::sensor::Sensor *i1, esphome::sensor::Sensor *i2, esphome::sensor::Sensor *i3,
                   esphome::sensor::Sensor *e_in, esphome::sensor::Sensor *e_out);

  esphome::sensor::Sensor *total_power_sensor{nullptr};
  esphome::sensor::Sensor *v1_sensor{nullptr};
  esphome::sensor::Sensor *v2_sensor{nullptr};
  esphome::sensor::Sensor *v3_sensor{nullptr};
  esphome::sensor::Sensor *i1_sensor{nullptr};
  esphome::sensor::Sensor *i2_sensor{nullptr};
  esphome::sensor::Sensor *i3_sensor{nullptr};
  esphome::sensor::Sensor *energy_in_sensor{nullptr};
  esphome::sensor::Sensor *energy_out_sensor{nullptr};

  void set_serial_number(const std::string &serial) { this->serial_ = serial; }
  void set_ip(const std::string &ip) { this->ip_ = ip; }

 protected:
  httpd_handle_t server{nullptr};
  bool mdns_initialized_{false};

  std::string serial_;
  std::string hostname_; // Wird in der .cpp automatisch generiert
  std::string ip_;

  static esp_err_t ecotracker_json_handler(httpd_req_t *req);
};

}  // namespace everhome_ecotracker_emu
}  // namespace esphome

#endif  // USE_ESP_IDF
