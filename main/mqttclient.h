#pragma once
#include "mqtt_client.h"

class MqttClient {
public:
  esp_err_t start();
  esp_err_t waitForConnected(uint32_t timeout_ms);
  int publish(const char *json_data, const char *topic);
  bool isConnected() const { return connected_; }
  ~MqttClient();

private:
  static const int MQTT_CONNECTED_BIT = BIT0;
  static void eventHandler(void *context, esp_event_base_t base,
                           int32_t event_id, void *event_data);
  void handleEvent(esp_mqtt_event_t &event);
  void handleData(const esp_mqtt_event_t &event);
  void resetClient();

  esp_mqtt_client_handle_t client_{nullptr};
  EventGroupHandle_t event_group_{nullptr};
  bool connected_{false};
};
