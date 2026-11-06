#pragma once

#if defined(IS_BOARD_C6)
  #define BOARD_NAME "ESP32-C6 Super Mini"
  #define DEFAULT_RGB_PIN 8
  #define DEFAULT_BLUE_PIN 15
  #define HAS_WIFI true

#elif defined(IS_BOARD_C3)
  #define BOARD_NAME "ESP32-C3 Super Mini"
  #define DEFAULT_RGB_PIN 8     // Если у вас ревизия с GPIO 2, поменяйте тут или настройте через сайт
  #define DEFAULT_BLUE_PIN 10
  #define HAS_WIFI true

#elif defined(IS_BOARD_S3)
  #define BOARD_NAME "ESP32-S3 Super Mini"
  #define DEFAULT_RGB_PIN 48
  #define DEFAULT_BLUE_PIN 47
  #define HAS_WIFI true

#elif defined(IS_BOARD_H2)
  #define BOARD_NAME "ESP32-H2 Super Mini"
  #define DEFAULT_RGB_PIN 8
  #define DEFAULT_BLUE_PIN 12
  #define HAS_WIFI false
#endif
