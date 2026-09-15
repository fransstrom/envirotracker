#include "mqttclient.h"

#include <cstring>

#include "esp_err.h"
#include "mqtt_client.h"

#define TAG "mqtt"

esp_err_t MqttClient::start() {

  esp_mqtt_client_config_t mqtt_cfg{};
  mqtt_cfg.broker.address.uri = CONFIG_ENVIROTRACKER_MQTT_HOST_URI;
  mqtt_cfg.broker.address.port = CONFIG_ENVIROTRACKER_MQTT_PORT;
  mqtt_cfg.credentials.username = CONFIG_ENVIROTRACKER_MQTT_USERNAME;
  mqtt_cfg.credentials.authentication.password =
      CONFIG_ENVIROTRACKER_MQTT_PASSWORD;
  if (CONFIG_ENVIROTRACKER_MQTT_TLS) {
    mqtt_cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
  }

  client_ = esp_mqtt_client_init(&mqtt_cfg);

  if (client_ == NULL) {
    return ESP_FAIL;
  }

  if (event_group_ == nullptr) {
    event_group_ = xEventGroupCreate();
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

esp_err_t MqttClient::waitForConnected(uint32_t timeout_ms) {
  if (event_group_ == nullptr) {
    return ESP_FAIL;
  }

  EventBits_t bits =
      xEventGroupWaitBits(event_group_, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE,
                          pdMS_TO_TICKS(timeout_ms));
  if (bits & MQTT_CONNECTED_BIT) {
    connected_ = true;
    return ESP_OK;
  }

  return ESP_FAIL;
}

int MqttClient::publish(const char *json_data, const char *topic) {
  if (!connected_) {
    return ESP_FAIL;
  }
  esp_mqtt_client_publish(client_, topic, json_data, 0, 1, 0);
  return ESP_OK;
}

void MqttClient::eventHandler(void *context, esp_event_base_t base,
                              int32_t event_id, void *event_data) {
  auto *client = static_cast<MqttClient *>(context);
  auto *event = static_cast<esp_mqtt_event_t *>(event_data);
  if (client != nullptr && event != nullptr) {
    client->handleEvent(*event);
  }
}

void MqttClient::handleEvent(esp_mqtt_event_t &event) {
  switch (event.event_id) {
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "PUBLISHED EVENT");
    break;
  case MQTT_EVENT_BEFORE_CONNECT:
    ESP_LOGI(TAG, "Connecting to MQTT broker...");
    break;
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "Connected to MQTT broker");
    connected_ = true;
    if (event_group_ != nullptr) {
      xEventGroupSetBits(event_group_, MQTT_CONNECTED_BIT);
    }
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "Disconnected from MQTT broker");
    connected_ = false;
    break;
  case MQTT_EVENT_ERROR:
    if (event.error_handle == nullptr) {
      ESP_LOGE(TAG, "MQTT error without diagnostic details");
      break;
    }
    if (event.error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
      ESP_LOGE(TAG,
               "MQTT transport error: esp-tls=%s (0x%x), TLS stack=0x%x, "
               "socket errno=%d (%s), certificate flags=0x%x",
               esp_err_to_name(event.error_handle->esp_tls_last_esp_err),
               event.error_handle->esp_tls_last_esp_err,
               event.error_handle->esp_tls_stack_err,
               event.error_handle->esp_transport_sock_errno,
               std::strerror(event.error_handle->esp_transport_sock_errno),
               event.error_handle->esp_tls_cert_verify_flags);
    } else if (event.error_handle->error_type ==
               MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
      ESP_LOGE(TAG, "MQTT broker refused connection: code=%d",
               event.error_handle->connect_return_code);
    } else {
      ESP_LOGE(TAG, "MQTT error type=%d", event.error_handle->error_type);
    }
    break;
  default:
    ESP_LOGD(TAG, "MQTT event: %d", event.event_id);
    break;
  }
}

void MqttClient::resetClient() {
  if (client_ != nullptr) {
    esp_mqtt_client_destroy(client_);
    client_ = nullptr;
  }
}
