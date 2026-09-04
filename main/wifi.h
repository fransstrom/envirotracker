#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

esp_err_t init(void);

esp_err_t connect(char *wifi_ssid, char *wifi_password);

esp_err_t disconnect(void);

esp_err_t deinit(void);
