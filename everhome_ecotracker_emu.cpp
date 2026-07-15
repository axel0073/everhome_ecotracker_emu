#include "everhome_ecotracker_emu.h"

#ifdef USE_ESP_IDF

#include "esphome/core/log.h"
#include "mdns.h"
#include <cmath>
#include <algorithm>

namespace esphome {
namespace everhome_ecotracker_emu {

static const char *const TAG = "everhome_ecotracker_emu";

// Falls du in ESPHome auch die Energiezähler (Wh/kWh) mitgeben willst, 
// kannst du diese Zuweisungsfunktion in deinem Header erweitern.
void EverhomeEcoTrackerEmu::set_sensors(esphome::sensor::Sensor *tot_p,
                                        esphome::sensor::Sensor *v1, esphome::sensor::Sensor *v2, esphome::sensor::Sensor *v3,
                                        esphome::sensor::Sensor *i1, esphome::sensor::Sensor *i2, esphome::sensor::Sensor *i3,
                                        esphome::sensor::Sensor *e_in, esphome::sensor::Sensor *e_out) {
  this->total_power_sensor = tot_p;
  this->v1_sensor = v1;
  this->v2_sensor = v2;
  this->v3_sensor = v3;
  this->i1_sensor = i1;
  this->i2_sensor = i2;
  this->i3_sensor = i3;
  this->energy_in_sensor = e_in;   // optionaler Zählerstand Bezug
  this->energy_out_sensor = e_out; // optionaler Zählerstand Einspeisung
}

esp_err_t custom_404_handler(httpd_req_t *req, httpd_err_code_t err) {
  ESP_LOGW(TAG, "404 - Unbekannter EcoTracker-Pfad: %s", req->uri);
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
  return ESP_OK;
}

// Zentraler Handler für die lokale REST-API des EcoTrackers
esp_err_t EverhomeEcoTrackerEmu::ecotracker_json_handler(httpd_req_t *req) {
  auto *self = static_cast<EverhomeEcoTrackerEmu*>(req->user_ctx);

  // NEU: Loggt den GET-Aufruf inklusive Pfad
  ESP_LOGI(TAG, "Erfolgreicher GET-Aufruf auf Pfad: %s", req->uri);
  
  // 1. Auslesen der Rohwerte und Schutz vor NaN
  float total_p = (self->total_power_sensor && self->total_power_sensor->has_state() && !std::isnan(self->total_power_sensor->state)) ? self->total_power_sensor->state : 0.0f;
  float v1 = (self->v1_sensor && self->v1_sensor->has_state() && !std::isnan(self->v1_sensor->state)) ? self->v1_sensor->state : 230.0f;
  float v2 = (self->v2_sensor && self->v2_sensor->has_state() && !std::isnan(self->v2_sensor->state)) ? self->v2_sensor->state : 230.0f;
  float v3 = (self->v3_sensor && self->v3_sensor->has_state() && !std::isnan(self->v3_sensor->state)) ? self->v3_sensor->state : 230.0f;
  float i1 = (self->i1_sensor && self->i1_sensor->has_state() && !std::isnan(self->i1_sensor->state)) ? self->i1_sensor->state : 0.0f;
  float i2 = (self->i2_sensor && self->i2_sensor->has_state() && !std::isnan(self->i2_sensor->state)) ? self->i2_sensor->state : 0.0f;
  float i3 = (self->i3_sensor && self->i3_sensor->has_state() && !std::isnan(self->i3_sensor->state)) ? self->i3_sensor->state : 0.0f;

  // Optionale kWh-Zählerstände auslesen und direkt im Code in Wh (Ganzzahl) umrechnen
  float energy_in_kwh = (self->energy_in_sensor && self->energy_in_sensor->has_state() && !std::isnan(self->energy_in_sensor->state)) ? self->energy_in_sensor->state : 0.0f;
  float energy_out_kwh = (self->energy_out_sensor && self->energy_out_sensor->has_state() && !std::isnan(self->energy_out_sensor->state)) ? self->energy_out_sensor->state : 0.0f;

  // Umrechnung von kWh in Wh mit kaufmännischer Rundung
  long energy_in_wh = (long)std::round(energy_in_kwh * 1000.0f);
  long energy_out_wh = (long)std::round(energy_out_kwh * 1000.0f);

  // 2. Mathematische Phasen-Leistungsberechnung via Scheinleistungsgewichtung
  float va1 = v1 * i1;
  float va2 = v2 * i2;
  float va3 = v3 * i3;
  float va_total = va1 + va2 + va3;

  float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;

  if (va_total > 0.1f) {
    p1 = (va1 / va_total) * total_p;
    p2 = (va2 / va_total) * total_p;
    p3 = (va3 / va_total) * total_p;
  } else {
    p1 = total_p / 3.0f; p2 = total_p / 3.0f; p3 = total_p / 3.0f;
  }

  // 3. JSON-Ausgabe im offiziellen EcoTracker-Format zusammenbauen
  auto json_response = std::make_unique<char[]>(384);

  snprintf(json_response.get(), 384,
    "{"
      "\"power\":%.1f,"
      "\"powerAvg\":%.1f,"
      "\"powerPhase1\":%.1f,"
      "\"powerPhase2\":%.1f,"
      "\"powerPhase3\":%.1f,"
      "\"energyCounterIn\":%ld,"
      "\"energyCounterInT1\":%ld,"
      "\"energyCounterInT2\":0,"
      "\"energyCounterOut\":%ld"
    "}",
    total_p,       
    total_p,       
    p1, p2, p3,    
    energy_in_wh,     // Jetzt korrekt in Wh übergeben
    energy_in_wh,     // Tarif 1 spiegelt ebenfalls die Wh wider
    energy_out_wh     // Einspeisung in Wh übergeben
  );

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, json_response.get());
  return ESP_OK;
}

void EverhomeEcoTrackerEmu::setup() {
  // Hostnamen automatisch aus "ecotracker-" + Serial in UPPERCASE generieren
  std::string serial_upper = this->serial_;
  std::transform(serial_upper.begin(), serial_upper.end(), serial_upper.begin(), ::toupper);
  this->hostname_ = "ecotracker-" + serial_upper;

  // mDNS Responder für Everhome initialisieren
  esp_err_t err = mdns_init();
  if (err == ESP_OK) {
    // Standard-Namensschema für EcoTracker im Netzwerk
    mdns_hostname_set(this->hostname_.c_str());
    mdns_instance_name_set(this->hostname_.c_str());
    
    // WICHTIG: EcoTracker wird unter dem Service "_everhome" angekündigt
    mdns_service_add(this->hostname_.c_str(), "_everhome", "_tcp", 80, nullptr, 0);

    mdns_txt_item_t serviceTxtData[] = {
      {"ip", this->ip_.c_str()},
      {"serial", this->serial_.c_str()},
      {"productid", "1137"}
    };
    mdns_service_txt_set("_everhome", "_tcp", serviceTxtData, 3);
    this->mdns_initialized_ = true;
    ESP_LOGI(TAG, "mDNS erfolgreich registriert. Hostname: %s, IP: %s", this->hostname_.c_str(), this->ip_.c_str());
  } else {
    this->mdns_initialized_ = false;
    ESP_LOGE(TAG, "mDNS Initialisierung fehlgeschlagen.");
  }

  // Konfiguration für den separaten EcoTracker-Server auf Port 80
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32775;
  config.max_open_sockets = 7;
  // Erzwingt das Freigeben alter Sockets bei Ressourcenknappheit
  config.lru_purge_enable = true; 
  // Geschlossene Sockets werden sofort verworfen, anstatt im TIME-WAIT-Zustand zu verharren
  config.enable_so_linger = true;

  if (httpd_start(&this->server, &config) == ESP_OK) {
    httpd_uri_t ecotracker_uri = { 
      .uri = "/v1/json", 
      .method = HTTP_GET, 
      .handler = EverhomeEcoTrackerEmu::ecotracker_json_handler, 
      .user_ctx = this 
    };
    httpd_register_uri_handler(this->server, &ecotracker_uri);
    httpd_register_err_handler(this->server, HTTPD_404_NOT_FOUND, custom_404_handler);

    ESP_LOGI(TAG, "Separater EcoTracker-Server erfolgreich auf Port 80 gestartet.");
  } else {
    ESP_LOGE(TAG, "Fehler beim Starten des EcoTracker-Webservers auf Port 80.");
  }
}

void EverhomeEcoTrackerEmu::loop() {
  if (!this->mdns_initialized_) return;

  static uint32_t last_mdns_check = 0;
  uint32_t now = millis();

  // Alle 30 Minuten mDNS-Ankündigung auffrischen
  if (now - last_mdns_check > 1800000) {
    last_mdns_check = now;

    mdns_txt_item_t serviceTxtData[] = {
      {"ip", this->ip_.c_str()},
      {"serial", this->serial_.c_str()}, // Korrekte konfigurierte Seriennummer
      {"productid", "1137"}
    };
    
    mdns_service_txt_set("_everhome", "_tcp", serviceTxtData, 3);
    ESP_LOGD(TAG, "mDNS-Präsenz für EcoTracker erneuert. IP: %s, Serial: %s", this->ip_.c_str(), this->serial_.c_str());
  }
}

void EverhomeEcoTrackerEmu::dump_config() {
  ESP_LOGCONFIG(TAG, "Everhome EcoTracker Emulator aktiv auf Port 80 unter /v1/json.");
}

} // namespace everhome_ecotracker_emu
} // namespace esphome
#endif
