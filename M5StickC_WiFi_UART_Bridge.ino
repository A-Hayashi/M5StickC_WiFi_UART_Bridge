#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "Bridge.h"

Preferences preferences;
WebServer server(80);

const char* ap_ssid = "M5StickC-AP";
const char* ap_password = "12345678";

Bridge EXT_IO(1, 9600, SERIAL_8E1, 0, 26, 8881);  //EXT IO
Bridge Grove(2, 9600, SERIAL_8E1, 32, 33, 8882);  //Grove
char comState[4] = { 'O', 'O', 'O', 'O' };

void updateComState(void* params);

void setup() {
  M5.begin();

  // 画面の回転を設定 (0, 1, 2, 3のいずれかで設定可能)
  M5.Lcd.setRotation(3);     // 90度回転
  M5.Lcd.setTextSize(2);     // 文字サイズを2倍に設定
  M5.Lcd.fillScreen(BLACK);  // 画面を黒でクリア

  M5.Lcd.setCursor(0, 0);  // カーソルを左上に設定
  M5.Lcd.println("Starting...");

  Serial.begin(115200);

  // フラッシュメモリからSSIDとパスワードを取得
  preferences.begin("wifi-config", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");

  // 保存されたSSIDとパスワードでWiFiに接続を試みる
  if (savedSSID.length() > 0 && savedPassword.length() > 0) {
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    Serial.println("Connecting to WiFi...");

    unsigned long startAttemptTime = millis();

    // 接続が成功するか、10秒経過するまで試みる
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
  }

  // 接続が成功しなかった場合
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect, starting AP mode...");
    M5.Lcd.println("Starting AP...");

    // APモードを開始
    WiFi.softAP(ap_ssid, ap_password);
    IPAddress IP = WiFi.softAPIP();

    Serial.print("AP SSID: ");
    Serial.println(ap_ssid);
    Serial.print("AP PASSWORD: ");
    Serial.println(ap_password);
    Serial.print("AP IP address: ");
    Serial.println(IP);

    M5.Lcd.print("AP SSID: ");
    M5.Lcd.println(ap_ssid);
    M5.Lcd.print("AP PASSWORD: ");
    M5.Lcd.println(ap_password);
    M5.Lcd.print("AP IP: ");
    M5.Lcd.println(IP);

    // WebサーバのルートでSSIDとパスワードを入力するフォームを提供
    server.on("/", HTTP_GET, []() {
      server.send(200, "text/html", "<form action='/set' method='POST'><input type='text' name='ssid' placeholder='SSID'><input type='text' name='password' placeholder='Password'><input type='submit'></form>");
    });

    // フォームから送信されたSSIDとパスワードを取得
    server.on("/set", HTTP_POST, []() {
      String newSSID = server.arg("ssid");
      String newPassword = server.arg("password");

      // フラッシュメモリに保存
      preferences.putString("ssid", newSSID);
      preferences.putString("password", newPassword);

      // WiFi接続を再試行するために再起動
      server.send(200, "text/plain", "Settings saved. Rebooting...");
      delay(1000);
      ESP.restart();
    });

    server.begin();
  } else {
    Serial.println("Connected to WiFi");
    M5.Lcd.println("Connected to WiFi");

    // 接続している無線LANルータのSSIDを取得して表示
    String connectedSSID = WiFi.SSID();
    Serial.print("Connected SSID: ");
    Serial.println(connectedSSID);
    M5.Lcd.println("SSID:");
    M5.Lcd.println(connectedSSID);

    // 割り振られたIPアドレスを取得して表示
    IPAddress localIP = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(localIP);
    M5.Lcd.println("IP Address:");
    M5.Lcd.println(localIP);
    M5.Lcd.print("EXT:W/S Grove:W/S");

    EXT_IO.registerCallback([](int code) {
      Serial.print("EXT_IO:");
      EXT_IO.dispCode(code);

      switch (code) {
        case BRIDGE_WIFI_OK:
          comState[0] = 'O';
          break;
        case BRIDGE_WIFI_ERROR:
          comState[0] = 'X';
          break;
        case BRIDGE_SERIAL_OK:
          comState[1] = 'O';
          break;
        case BRIDGE_SERIAL_ERROR:
          comState[1] = 'X';
          break;
      }
    });
    Grove.registerCallback([](int code) {
      Serial.print("Grove:");
      Grove.dispCode(code);

      switch (code) {
        case BRIDGE_WIFI_OK:
          comState[2] = 'O';
          break;
        case BRIDGE_WIFI_ERROR:
          comState[2] = 'X';
          break;
        case BRIDGE_SERIAL_OK:
          comState[3] = 'O';
          break;
        case BRIDGE_SERIAL_ERROR:
          comState[3] = 'X';
          break;
      }
    });
    EXT_IO.start();
    Grove.start();
  }

  xTaskCreatePinnedToCore(updateComState,   /* タスクの入口となる関数名 */
                          "updateComState", /* タスクの名称 */
                          1024,             /* スタックサイズ */
                          NULL,             /* パラメータのポインタ */
                          8,                /* プライオリティ */
                          NULL,             /* ハンドル構造体のポインタ */
                          0);               /* 割り当てるコア (0/1) */
}

void loop() {
  // ボタンの状態を更新
  M5.update();
  // Webサーバーの処理
  server.handleClient();

  // ボタンBが押されたかをチェック
  if (M5.BtnB.wasPressed()) {
    M5.Lcd.fillScreen(BLACK);  // 画面をクリア
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Clearing Wi-Fi credentials...");

    // フラッシュメモリのSSIDとパスワードを消去
    preferences.begin("wifi-config", false);
    preferences.clear();  // 全ての保存された設定を消去
    preferences.end();

    M5.Lcd.println("Rebooting...");
    delay(1000);

    // M5StickCを再起動
    ESP.restart();
  }

  if (M5.BtnA.wasPressed()) {
    delay(1000);
    ESP.restart();
  }
}

void updateComState(void* params) {
  for (;;) {
    M5.Lcd.fillRect(0, 119, 240, 135, BLACK);  // 最下部の領域を黒で塗りつぶす
    // 新しいテキストを描画
    M5.Lcd.setCursor(0, 119);    // カーソル位置を設定
    M5.Lcd.setTextColor(WHITE);  // テキストの色を設定
    M5.Lcd.print("    ");
    M5.Lcd.print(comState[0]);
    M5.Lcd.print("/");
    M5.Lcd.print(comState[1]);
    M5.Lcd.print("       ");
    M5.Lcd.print(comState[2]);
    M5.Lcd.print("/");
    M5.Lcd.print(comState[3]);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}