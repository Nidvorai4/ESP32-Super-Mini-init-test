#include <Arduino.h>
#include "pin_config.h"

// Переменные таймеров и состояний диодов (Общие для всех плат)
unsigned long lastLedUpdate = 0;
unsigned long ledInterval = 3000; // Режим 1: старт с 3 секунд
int ledState = 0;                 

unsigned long startTime = 0;
bool radioActivated = false;
bool clientConnected = false;
const unsigned long activationTimeout = 20000; // 20 секунд ожидания

unsigned long lastLogTime = 0;
const unsigned long logInterval = 5000; // Логи раз в 5 секунд

// Имя сети / BLE устройства
const char* deviceName = "ESP32-SuperMini-TEST";

// ============================================================================
// УСЛОВНАЯ КОМПИЛЯЦИЯ ДЛЯ ПЛАТ С WI-FI (C3, C6, S3)
// ============================================================================
#if HAS_WIFI
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

const char* apPassword = "12345678Super"; 
WebServer server(80);
DNSServer dnsServer;
IPAddress clientIP;
String clientUA = "";

void handleRoot() {
  if (!clientConnected) {
    clientConnected = true;
    ledInterval = 200; // Режим 3: пользователь на странице (мигание 0.2 сек)
  }
  clientIP = server.client().remoteIP();
  clientUA = server.header("User-Agent");

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Тест " + String(BOARD_NAME) + "</title>";
  html += "<style>body{font-family:sans-serif; background:#eef2f7; text-align:center; padding:20px;} .card{background:white; max-width:450px; margin:30px auto; padding:25px; border-radius:12px; box-shadow:0 4px 15px rgba(0,0,0,0.1);} h1{color:#28a745;} ul{text-align:left; background:#f8f9fa; padding:15px 15px 15px 30px; border-radius:8px;}</style></head>";
  html += "<body><div class='card'><h1>" + String(BOARD_NAME) + " OK!</h1><p>Плата успешно прошла Wi-Fi экспресс-тест.</p><hr>";
  html += "<h3>Параметры:</h3><ul><li><b>Модель:</b> " + String(ESP.getChipModel()) + "</li><li><b>Частота CPU:</b> " + String(ESP.getCpuFreqMHz()) + " МГц</li><li><b>Flash:</b> " + String(ESP.getFlashChipSize() / 1024 / 1024) + " МБ</li></ul>";
  html += "<h3>Ваш девайс:</h3><ul><li><b>Ваш IP:</b> " + clientIP.toString() + "</li><li><b>User-Agent:</b> " + clientUA.substring(0,40) + "...</li></ul></div></body></html>";
  
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "text/html", html);
}
#endif

// ============================================================================
// УСЛОВНАЯ КОМПИЛЯЦИЯ ДЛЯ ПЛАТЫ БЕЗ WI-FI (ESP32-H2 через BLE)
// ============================================================================
#if !HAS_WIFI
#include <BLEDevice.h>
#include <BLEServer.h>

BLEServer* pServer = NULL;
int lastRSSI = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      clientConnected = true;
      ledInterval = 200; // Режим 3: клиент подключился по BLE (мигание 0.2 сек)
      Serial.println("\n[BLE] Клиент подключился!");
    }
    void onDisconnect(BLEServer* pServer) {
      clientConnected = false;
      ledInterval = 1000; // Возврат в Режим 2: ожидание подключения (мигание 1 сек)
      Serial.println("\n[BLE] Клиент отключился. Ожидание...");
      BLEDevice::startAdvertising(); // Перезапуск вещания
    }
};
#endif

// ============================================================================
// ОСНОВНЫЕ ФУНКЦИИ ARDUINO
// ============================================================================
void setup() {
  Serial.begin(115200);
  while(!Serial) { delay(10); }

  Serial.printf("\n=== Запущен тест для платы: %s ===\n", BOARD_NAME);
  Serial.println("[INFO] Режим 1: Ожидание 20 секунд (Мигание 3 сек с логами)");

  pinMode(BLUE_LED_PIN, OUTPUT);
  startTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // --- Инициализация радиомодулей через 20 секунд ---
  if (!radioActivated && (currentMillis - startTime >= activationTimeout)) {
    radioActivated = true;
    ledInterval = 1000; // Режим 2: Радио активно, клиентов нет (мигание 1 сек)
    lastLogTime = currentMillis;

    #if HAS_WIFI
      WiFi.softAP(deviceName, apPassword);
      dnsServer.start(53, "*", WiFi.softAPIP());
      const char* headerkeys[]  = {"User-Agent"};
      server.collectHeaders(headerkeys, 1);
      server.onNotFound(handleRoot);
      server.begin();
      Serial.println("[SYSTEM] Режим 2: Wi-Fi AP запущена. Мигание 1с без логов.");
    #else
      BLEDevice::init(deviceName);
      pServer = BLEDevice::createServer();
      pServer->setCallbacks(new MyServerCallbacks());
      
      // Создаем пустой сервис, чтобы плату было видно при сканировании
      BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x180A)); 
      pService->start();
      
      BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
      pAdvertising->addServiceUUID(pService->getUUID());
      pAdvertising->setScanResponse(true);
      BLEDevice::startAdvertising();
      Serial.println("[SYSTEM] Режим 2: BLE маньяк запущен. Мигание 1с без логов.");
    #endif
  }

  // --- Обслуживание активных сетевых сервисов ---
  if (radioActivated) {
    #if HAS_WIFI
      dnsServer.processNextRequest();
      server.handleClient();
      
      // Периодический лог Wi-Fi
      if (currentMillis - lastLogTime >= logInterval) {
        lastLogTime = currentMillis;
        if (!clientConnected) {
          Serial.printf("[AP LOG] Сеть: %s | Пароль: %s | IP: %s\n", deviceName, apPassword, WiFi.softAPIP().toString().c_str());
        } else {
          Serial.printf("[CLIENT LOG] Юзер на сайте. IP: %s | Браузер: %s\n", clientIP.toString().c_str(), clientUA.c_str());
        }
      }
    #else
      // Периодический лог BLE
      if (currentMillis - lastLogTime >= logInterval) {
        lastLogTime = currentMillis;
        if (!clientConnected) {
          Serial.printf("[BLE LOG] Ожидание BLE подключения. Имя устройства: %s\n", deviceName);
        } else {
          // Выводим статус, если клиент подключен
          Serial.println("[CLIENT LOG] Активная BLE сессия с телефоном.");
        }
      }
    #endif
  }

  // --- Управление светодиодами (Неблокирующее) ---
  if (currentMillis - lastLedUpdate >= ledInterval) {
    lastLedUpdate = currentMillis;

    int r = 0, g = 0, b = 0;
    bool blueState = LOW;

    if (ledState == 0) {
      r = 64; blueState = HIGH;
      if (!radioActivated) Serial.println("[LED LOG] Цвет: КРАСНЫЙ");
      ledState = 1;
    } 
    else if (ledState == 1) {
      g = 64; blueState = LOW;
      if (!radioActivated) Serial.println("[LED LOG] Цвет: ЗЕЛЕНЫЙ");
      ledState = 2;
    } 
    else if (ledState == 2) {
      b = 64; blueState = HIGH;
      if (!radioActivated) Serial.println("[LED LOG] Цвет: СИНИЙ");
      ledState = 0;
    }

    rgbLedWrite(RGB_LED_PIN, r, g, b);
    digitalWrite(BLUE_LED_PIN, blueState);
  }
}
