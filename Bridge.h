#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <functional>  // std::functionを使用するために必要
#include <string>

#define BRIDGE_WIFI_OK (0)
#define BRIDGE_WIFI_ERROR (1)
#define BRIDGE_SERIAL_OK (2)
#define BRIDGE_SERIAL_ERROR (3)

#define bufferSize (1024)

using CallbackFunction = std::function<void(int)>;

class Bridge {
public:
  Bridge(int uartNo, int baudRate, int serialParam, int rxdPin, int txdPin, int port);
  static void runner(void* params);
  static void periodicTask(void* params);

  void start();
  void tcpLoop();
  void udpLoop();
  void serialBegin(int baudRate, int serialParam);
  bool tcp = false;
  void registerCallback(CallbackFunction callback);

  void dispCode(int code) {
    static String strings[] = { "BRIDGE_WIFI_OK", "BRIDGE_WIFI_ERROR", "BRIDGE_SERIAL_OK", "BRIDGE_SERIAL_ERROR" };
    Serial.println(strings[code]);
  }
private:
  HardwareSerial serial;
  int port;
  int baudRate;
  int serialParam;
  int rxdPin;
  int txdPin;

  WiFiServer server;
  WiFiClient TCPClient;
  WiFiUDP udp;
  IPAddress remoteIp;
  uint16_t remotePort;

  uint8_t buf1[bufferSize];
  uint16_t i1;

  uint8_t buf2[bufferSize];
  uint16_t i2;

  void triggerEvent(int message);  // イベントが発生した際にコールバック関数を呼び出すメソッド
  CallbackFunction callback;       // コールバック関数を保持するメンバー変数

  int wifiErrorCnt = 5;
  bool wifiError = false;
  void wifiOK() {
    if (wifiError == true) {
      wifiError = false;
      triggerEvent(BRIDGE_WIFI_OK);
    }
    wifiErrorCnt = 5;
  }

  void checkWifiError() {
    if (wifiErrorCnt > 0) {
      wifiErrorCnt--;
    }
    if (wifiErrorCnt == 0) {
      if (wifiError == false) {
        wifiError = true;
        triggerEvent(BRIDGE_WIFI_ERROR);
      }
    }
  }

  int serialErrorCnt = 5;
  bool serialError = false;
  void serialOK() {
    if (serialError == true) {
      serialError = false;
      triggerEvent(BRIDGE_SERIAL_OK);
    }
    serialErrorCnt = 5;
  }

  void checkSerialError() {
    if (serialErrorCnt > 0) {
      serialErrorCnt--;
    }
    if (serialErrorCnt == 0) {
      if (serialError == false) {
        serialError = true;
        triggerEvent(BRIDGE_SERIAL_ERROR);
      }
    }
  }
};
