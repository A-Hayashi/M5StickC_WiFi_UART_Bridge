#include "Bridge.h"

Bridge::Bridge(int uartNo, int baudRate, int serialParam, int rxdPin, int txdPin, int port)
  : serial(uartNo) {
  this->port = port;
  this->baudRate = baudRate;
  this->serialParam = serialParam;
  this->rxdPin = rxdPin;
  this->txdPin = txdPin;
}

void Bridge::start(void) {
  /* create task */
  xTaskCreatePinnedToCore(this->runner, /* タスクの入口となる関数名 */
                          "TASK1",      /* タスクの名称 */
                          1024 * 3,     /* スタックサイズ */
                          this,         /* パラメータのポインタ */
                          1,            /* プライオリティ */
                          NULL,         /* ハンドル構造体のポインタ */
                          0);           /* 割り当てるコア (0/1) */

  xTaskCreatePinnedToCore(this->periodicTask, /* タスクの入口となる関数名 */
                          "PeriodicTask",     /* タスクの名称 */
                          1024,               /* スタックサイズ */
                          this,               /* パラメータのポインタ */
                          2,                  /* プライオリティ */
                          NULL,               /* ハンドル構造体のポインタ */
                          0);                 /* 割り当てるコア (0/1) */
}

void Bridge::serialBegin(int baudRate, int serialParam) {
  this->baudRate = baudRate;
  this->serialParam = serialParam;
  serial.end();
  serial.begin(baudRate, serialParam, rxdPin, txdPin);
}

void Bridge::runner(void *params) {
  Bridge *ref = static_cast<Bridge *>(params);
  if (ref->tcp == true) {
    ref->tcpLoop();
  } else {
    ref->udpLoop();
  }
}

void Bridge::periodicTask(void *params) {
  Bridge *ref = static_cast<Bridge *>(params);
  for (;;) {
    ref->checkWifiError();
    ref->checkSerialError();
    // 1秒（1000ミリ秒）待機
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void Bridge::tcpLoop() {
  serial.end();
  serial.begin(baudRate, serialParam, rxdPin, txdPin);
  server.begin(port);  // start TCP server
  server.setNoDelay(true);

  Serial.print("start TCP server: ");
  Serial.println(port);
  while (1) {
    if (server.hasClient()) {
      //find free/disconnected spot
      if (!TCPClient || !TCPClient.connected()) {
        if (TCPClient) {
          TCPClient.stop();
        }
        TCPClient = server.available();
      }
      //no free/disconnected spot so reject
      WiFiClient TmpserverClient = server.available();
      TmpserverClient.stop();
    }

    if (TCPClient) {
      while (TCPClient.available()) {
        buf1[i1] = TCPClient.read();  // read char from client (LK8000 app)
        if (i1 < bufferSize - 1) {
          i1++;
        }
      }
      if (i1 > 0) {
        Serial.print("from\t");
        Serial.print(port);
        Serial.print(":");
        Serial.write(buf1, i1);
        Serial.println("");
        serial.write(buf1, i1);  // now send to UART(num):
        wifiOK();
      }
      i1 = 0;
    }

    if (serial.available()) {
      while (serial.available()) {
        buf2[i2] = serial.read();  // read char from UART(num)
        if (i2 < bufferSize - 1) {
          i2++;
        }
      }
      // now send to WiFi:
      if (i2 > 0) {
        if (TCPClient) {
          Serial.print("to\t");
          Serial.print(port);
          Serial.print(":");
          Serial.write(buf2, i2);
          Serial.println("");
          TCPClient.write(buf2, i2);
          serialOK();
        }
      }
      i2 = 0;
    }
    vTaskDelay(1);
  }
}

void Bridge::udpLoop() {
  serial.end();
  serial.begin(baudRate, serialParam, rxdPin, txdPin);
  udp.begin(port);  // start UDP server

  Serial.print("start UDP server: ");
  Serial.println(port);
  while (1) {
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
      remoteIp = udp.remoteIP();      // store the ip of the remote device
      remotePort = udp.remotePort();  // store the port of the remote device
      udp.read(buf1, bufferSize);

      Serial.print("from\t");
      Serial.print(port);
      Serial.print(":");
      Serial.write(buf1, packetSize);
      Serial.println("");
      serial.write(buf1, packetSize);  // now send to UART(num):
      wifiOK();
    }

    if (serial.available()) {
      while (serial.available()) {
        buf2[i2] = serial.read();  // read char from UART(num)
        if (i2 < bufferSize - 1) {
          i2++;
        }
      }
      // now send to WiFi:
      if (i2 > 0) {
        Serial.print("to\t");
        Serial.print(remotePort);
        Serial.print(":");
        Serial.write(buf2, i2);
        Serial.println("");

        udp.beginPacket(remoteIp, remotePort);  // remote IP and port
        udp.write(buf2, i2);
        udp.endPacket();
        serialOK();
      }
      i2 = 0;
    }
    vTaskDelay(1);
  }
}

// コールバック関数を登録するメソッド
void Bridge::registerCallback(CallbackFunction callback) {
  this->callback = callback;
}

// イベントが発生した際にコールバック関数を呼び出すメソッド
void Bridge::triggerEvent(int code) {
  if (callback) {  // コールバックが登録されている場合のみ呼び出し
    callback(code);
  }
}
