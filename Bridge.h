#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <string>

#define bufferSize (1024)

class Bridge {
public:
  Bridge(int uartNo, int baudRate, int serialParam, int rxdPin, int txdPin, int port);
  static void runner(void* params);
  static void periodicTask(void* params);

  void start();
  void tcpLoop();
  void udpLoop();
  bool tcp = false;

  typedef void (*CallbackFunction)(String);
  void registerCallback(CallbackFunction callback);

  bool getWifiError() {
    return wifiError;
  }
  bool getSerialError() {
    return serialError;
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

  void triggerEvent(String message);  // イベントが発生した際にコールバック関数を呼び出すメソッド
  CallbackFunction callback;          // コールバック関数を保持するメンバー変数

  int wifiErrorCnt = 5;
  bool wifiError = false;
  void wifiOK() {
    if (wifiError == true) {
      wifiError = false;
      triggerEvent("BRIDGE_WIFI_OK");
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
        triggerEvent("BRIDGE_WIFI_ERROR");
      }
    }
  }

  int serialErrorCnt = 5;
  bool serialError = false;
  void serialOK() {
    if (serialError == true) {
      serialError = false;
      triggerEvent("BRIDGE_SERIAL_OK");
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
        triggerEvent("BRIDGE_SERIAL_ERROR");
      }
    }
  }
};
