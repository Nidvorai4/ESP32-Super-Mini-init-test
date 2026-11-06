#pragma once

#if defined(IS_BOARD_C6)
  #define BOARD_NAME "ESP32-C6 Super Mini"
  #define RGB_LED_PIN 8
  #define BLUE_LED_PIN 15
  #define HAS_WIFI true

#elif defined(IS_BOARD_C3)
  #define BOARD_NAME "ESP32-C3 Super Mini"
  #define RGB_LED_PIN 8     // У некоторых ревизий C3 бывает пин 2
  #define BLUE_LED_PIN 10   // Синий диод статуса
  #define HAS_WIFI true

#elif defined(IS_BOARD_S3)
  #define BOARD_NAME "ESP32-S3 Super Mini"
  #define RGB_LED_PIN 48    // Классический пин для S3 Mini
  #define BLUE_LED_PIN 47
  #define HAS_WIFI true

#elif defined(IS_BOARD_H2)
  #define BOARD_NAME "ESP32-H2 Super Mini"
  #define RGB_LED_PIN 8
  #define BLUE_LED_PIN 12
  #define HAS_WIFI false    // На H2 нет Wi-Fi, код автоматически переключится на BLE

#endif
