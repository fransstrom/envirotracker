#include "mqttclient.h"
#include "esp_err.h"
#include "mqtt_client.h"
// void MqttClient::eventHandler(void *context, esp_event_base_t base,
//                               int32_t event_id, void *event_data) {}
//
esp_err_t MqttClient::start() {

  esp_mqtt_client_config_t mqtt_cfg{};
  mqtt_cfg.broker.address.hostname = CONFIG_ENVIROTRACKER_MQTT_HOST_URI;
  mqtt_cfg.broker.address.port = CONFIG_ENVIROTRACKER_MQTT_PORT;
  mqtt_cfg.credentials.username = CONFIG_ENVIROTRACKER_MQTT_USERNAME;
  mqtt_cfg.credentials.authentication.password =
      CONFIG_ENVIROTRACKER_MQTT_PASSWORD;
  mqtt_cfg.broker.address.transport = CONFIG_ENVIROTRACKER_MQTT_TLS
                                          ? MQTT_TRANSPORT_OVER_SSL
                                          : MQTT_TRANSPORT_OVER_TCP;
  mqtt_cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;

  client_ = esp_mqtt_client_init(&mqtt_cfg);

  if (client_ == NULL) {
    return ESP_FAIL;
  }
  constexpr auto MQTT_ANY_EVENT =
      static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID);

  esp_err_t err = esp_mqtt_client_register_event(client_, MQTT_ANY_EVENT,
                                                 eventHandler, this);
  if (err != ESP_OK) {
    resetClient();
    return err;
  }

  err = esp_mqtt_client_start(client_);
  if (err != ESP_OK) {
    resetClient();
    return err;
  }

  return err;
}

void MqttClient::resetClient() {
  if (client_ != nullptr) {
    esp_mqtt_client_destroy(client_);
    client_ = nullptr;
  }
}
