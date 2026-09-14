#pragma once
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "mqtt_client.h"

class MqttClient {
public:
  esp_err_t start();

private:
  static void eventHandler(void *context, esp_event_base_t base,
                           int32_t event_id, void *event_data);
  void handleEvent(esp_mqtt_event_t &event);
  void handleData(const esp_mqtt_event_t &event);
  void resetClient();

  esp_mqtt_client_handle_t client_{nullptr};
};
