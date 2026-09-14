#include "dht.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include "wifi.h"
#include <cmath>
#include <cstdint>

#define DHT_TAG "DHT"
#define DHT_DATA_PIN GPIO_NUM_3
#define TAG "main"

static void dht_collect(void *param) {
  int16_t hum = 0;
  int16_t temp = 0;

  while (true) {
    // ESP_LOGI(DHT_TAG, "Start loop");
    esp_err_t dht_err =
        dht_read_data(DHT_TYPE_DHT11, DHT_DATA_PIN, &hum, &temp);

    vTaskDelay(pdMS_TO_TICKS(5000));
    // dht_read_float_data(DHT_TYPE_DHT11, DHT_DATA_PIN, &hum_f, &temp_f);
    ESP_LOGI(DHT_TAG, "dht code: %s", esp_err_to_name(dht_err));
    if (dht_err == ESP_OK) {
      ESP_LOGI(DHT_TAG, "temp%d", temp / 10);
      ESP_LOGI(DHT_TAG, "hum%d", hum / 10);
    } else {
      ESP_LOGI(DHT_TAG, "Failed to read data %s", esp_err_to_name(dht_err));
    }
  }
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "Starting tutorial...");
  ESP_ERROR_CHECK(init());

  esp_err_t ret = connect(CONFIG_ENVIROTRACKER_WIFI_SSID,
                          CONFIG_ENVIROTRACKER_WIFI_PASSWORD);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to connect to Wi-Fi network");
  }

  wifi_ap_record_t ap_info;
  ret = esp_wifi_sta_get_ap_info(&ap_info);
  if (ret == ESP_ERR_WIFI_CONN) {
    ESP_LOGE(TAG, "Wi-Fi station interface not initialized");
  } else if (ret == ESP_ERR_WIFI_NOT_CONNECT) {
    ESP_LOGE(TAG, "Wi-Fi station is not connected");
  } else {
    ESP_LOGI(TAG, "--- Access Point Information ---");
    ESP_LOG_BUFFER_HEX("MAC Address", ap_info.bssid, sizeof(ap_info.bssid));
    ESP_LOG_BUFFER_CHAR("SSID", ap_info.ssid, sizeof(ap_info.ssid));
    ESP_LOGI(TAG, "Primary Channel: %d", ap_info.primary);
    ESP_LOGI(TAG, "RSSI: %d", ap_info.rssi);

    vTaskDelay(pdMS_TO_TICKS(5000));
  }

  // config is mostly handled by DHT-drivers but keeping this just in case.
  gpio_config_t dht_config = {
      .pin_bit_mask = 1ULL << GPIO_NUM_3,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  esp_err_t gpio_config_error = gpio_config(&dht_config);
  ESP_LOGI(DHT_TAG, "gpio config%d", gpio_config_error);

  BaseType_t result =
      xTaskCreate(dht_collect, "dht_collect", configMINIMAL_STACK_SIZE * 3,
                  nullptr, 5, nullptr);
  if (result != pdPASS) {
    ESP_LOGE(DHT_TAG, "Failed to create DHT task");
  }
}
