#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <functional>  // std::functionを使用するために必要
#include <string>

#define BRIDGE_COMMUNICATION_OK (1)

#define bufferSize (1024)

using CallbackFunction = std::function<void(int)>;

class Bridge {
public:
  Bridge(int uartNo, int baudRate, int serialParam, int rxdPin, int txdPin, int port);
  static void runner(void* params);
  void start();
  void tcpLoop();
  void udpLoop();
  void serialBegin(int baudRate, int serialParam);
  bool tcp;
  void registerCallback(CallbackFunction callback);

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
};
