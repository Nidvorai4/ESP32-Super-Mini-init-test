#include <Arduino.h>
#include <Preferences.h> // Для работы с энергонезависимой памятью NVS
#include "pin_config.h"

// Глобальные переменные пинов, которые будут меняться на лету
int rgbLedPin;
int blueLedPin;

Preferences prefs;

// Переменные таймеров и состояний диодов
unsigned long lastLedUpdate = 0;
unsigned long ledInterval = 3000; // Режим 1: старт с 3 секунд
int ledState = 0;                 

unsigned long startTime = 0;
bool radioActivated = false;
bool clientConnected = false;
const unsigned long activationTimeout = 20000; // 20 секунд ожидания

unsigned long lastLogTime = 0;
const unsigned long logInterval = 5000; // Логи раз в 5 секунд
const char* deviceName = "ESP32-SuperMini-TEST";

// Функция проверки Serial-команд для динамической смены пинов
void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    // Ожидаем команду вида SET_PINS:15,8 (Синий_Пин, RGB_Пин)
    if (cmd.startsWith("SET_PINS:")) {
      String pins = cmd.substring(9);
      int commaIndex = pins.indexOf(',');
      if (commaIndex != -1) {
        int newBlue = pins.substring(0, commaIndex).toInt();
        int newRgb = pins.substring(commaIndex + 1).toInt();
        
        // Сохраняем новые пины в память платы
        prefs.begin("dev_pins", false);
        prefs.putInt("blue", newBlue);
        prefs.putInt("rgb", newRgb);
        prefs.end();
        
        blueLedPin = newBlue;
        rgbLedPin = newRgb;
        
        pinMode(blueLedPin, OUTPUT);
        Serial.printf("\n[PREFS] Пины успешно сохранены! Синий: %d, RGB: %d\n", blueLedPin, rgbLedPin);
      }
    }
  }
}

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
  html += "<style>body{font-family:sans-serif; background:#eef2f7; text-align:center; padding:20px;} .card{background:white; max-width:450px; margin:30px auto; padding:25px; border-radius:12px; box-shadow:0 4px 15px rgba(0,0,0,0.1);} h1{color:#28a745;} ul{text-align:left; background:#f8f9fa; padding:15px 15px 15px 30px; border-radius:8px;} input{padding:8px; width:60px; text-align:center; margin:5px;} button{padding:10px 15px; background:#007bff; color:white; border:none; border-radius:5px; cursor:pointer; font-weight:bold;}</style></head>";
  html += "<body><div class='card'><h1>" + String(BOARD_NAME) + " OK!</h1><p>Плата успешно прошла Wi-Fi экспресс-тест.</p><hr>";
  html += "<h3>Параметры:</h3><ul><li><b>Модель:</b> " + String(ESP.getChipModel()) + "</li><li><b>Частота CPU:</b> " + String(ESP.getCpuFreqMHz()) + " МГц</li><li><b>Flash:</b> " + String(ESP.getFlashChipSize() / 1024 / 1024) + " МБ</li></ul>";
  
  // Интерактивная форма смены пинов прямо на веб-странице Wi-Fi
  html += "<h3>Корректировка пинов светодиодов:</h3>";
  html += "<p>Если диоды не мигают, введите новые GPIO и сохраните:</p>";
  html += "<form action='/set_pins' method='POST'>";
  html += "Status LED Pin: <input type='number' name='blue' value='" + String(blueLedPin) + "'><br>";
  html += "RGB LED Pin: <input type='number' name='rgb' value='" + String(rgbLedPin) + "'><br><br>";
  html += "<button type='submit'>Сохранить в память</button></form></div></body></html>";
  
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "text/html", html);
}

// Обработчик формы смены пинов через веб-интерфейс Wi-Fi
void handleSetPins() {
  if (server.hasArg("blue") && server.hasArg("rgb")) {
    int newBlue = server.arg("blue").toInt();
    int newRgb = server.arg("rgb").toInt();
    
    prefs.begin("dev_pins", false);
    prefs.putInt("blue", newBlue);
    prefs.putInt("rgb", newRgb);
    prefs.end();
    
    blueLedPin = newBlue;
    rgbLedPin = newRgb;
    pinMode(blueLedPin, OUTPUT);
    
    Serial.printf("\n[WEB PREFS] Пины изменены через браузер! Синий: %d, RGB: %d\n", blueLedPin, rgbLedPin);
    
    // Перенаправляем пользователя обратно на главную страницу
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Redirecting...");
  }
}
#endif

// ============================================================================
// УСЛОВНАЯ КОМПИЛЯЦИЯ ДЛЯ ПЛАТЫ БЕЗ WI-FI (ESP32-H2 через BLE)
// ============================================================================
#if !HAS_WIFI
#include <BLEDevice.h>
#include <BLEServer.h>

BLEServer* pServer = NULL;

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
      BLEDevice::startAdvertising(); 
    }
};
#endif

// ============================================================================
// ОСНОВНЫЕ ФУНКЦИИ ARDUINO
// ============================================================================
void setup() {
  Serial.begin(115200);
  while(!Serial) { delay(10); }

  // Инициализируем Preferences и читаем сохраненные пины из NVS флеш-памяти
  prefs.begin("dev_pins", true);
  blueLedPin = prefs.getInt("blue", DEFAULT_BLUE_PIN);
  rgbLedPin = prefs.getInt("rgb", DEFAULT_RGB_PIN);
  prefs.end();

  Serial.printf("\n=== Запущен тестер для платы: %s ===\n", BOARD_NAME);
  Serial.printf("[INFO] Активные GPIO диодов: Синий/Status = %d | Адресный RGB = %d\n", blueLedPin, rgbLedPin);
  Serial.println("[INFO] Изменить пины в любой момент через Serial: SET_PINS:синий,rgb");
  Serial.println("[INFO] Режим 1: Ожидание 20 секунд (Мигание 3 сек с логами)");

  pinMode(blueLedPin, OUTPUT);
  startTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // Постоянно проверяем, не пришла ли команда настройки пинов по кабелю
  checkSerialCommands();

  // --- Инициализация радиомодулей через 20 секунд ---
  if (!radioActivated && (currentMillis - startTime >= activationTimeout)) {
    radioActivated = true;
    ledInterval = 1000; // Режим 2: Радио активно, клиентов нет (мигание 1 сек)
    lastLogTime = currentMillis;

    #if HAS_WIFI
      WiFi.softAP(deviceName, apPassword);
      dnsServer.start(53, "*", WiFi.softAPIP());
      const char* headerkeys[] = {"User-Agent"};
      server.collectHeaders(headerkeys, 1);
      server.on("/", handleRoot);
      server.on("/set_pins", HTTP_POST, handleSetPins); // Маршрут для обработки формы
      server.onNotFound(handleRoot);
      server.begin();
      Serial.println("[SYSTEM] Режим 2: Wi-Fi AP запущена. Мигание 1с без логов.");
    #else
      BLEDevice::init(deviceName);
      pServer = BLEDevice::createServer();
      pServer->setCallbacks(new MyServerCallbacks());
      
      BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x180A)); 
      pService->start();
      
      BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
      pAdvertising->addServiceUUID(pService->getUUID());
      pAdvertising->setScanResponse(true);
      BLEDevice::startAdvertising();
      Serial.println("[SYSTEM] Режим 2: BLE запущен. Мигание 1с без логов.");
    #endif
  }

  // --- Обслуживание активных сетевых сервисов ---
  if (radioActivated) {
    #if HAS_WIFI
      dnsServer.processNextRequest();
      server.handleClient();
      
      if (currentMillis - lastLogTime >= logInterval) {
        lastLogTime = currentMillis;
        if (!clientConnected) {
          Serial.printf("[AP LOG] Сеть: %s | Пароль: %s | IP: %s\n", deviceName, apPassword, WiFi.softAPIP().toString().c_str());
        } else {
          Serial.printf("[CLIENT LOG] Юзер на сайте. IP: %s | Устройство: %s\n", clientIP.toString().c_str(), clientUA.substring(0,30).c_str());
        }
      }
    #else
      if (currentMillis - lastLogTime >= logInterval) {
        lastLogTime = currentMillis;
        if (!clientConnected) {
          Serial.printf("[BLE LOG] Ожидание BLE подключения. Имя устройства: %s\n", deviceName);
        } else {
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

    // Подаем сигналы на динамически настроенные пины
    rgbLedWrite(rgbLedPin, r, g, b);
    digitalWrite(blueLedPin, blueState);
  }
}
