#include <WiFi.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define BUTTON_PIN 25
#define contol_led1 33
#define contol_led2 32
#define contol_led3 35
int ls_led1 = 0;
int ls_led2 = 0;
int ls_led3 = 0;

#define ledpin1 26
#define ledpin2 27
#define ledpin3 14

#define SSID_SIZE 32
#define PASS_SIZE 64
#define SSID_ADDR 0
#define PASS_ADDR (SSID_ADDR + SSID_SIZE)
#define TOKEN_SIZE 32
#define TOKEN_ADDR (PASS_ADDR + PASS_SIZE)

int st = 0;
const char* serverAddr = "172.24.160.51";
const int serverPort = 3000;
String ssid = "";
String password = "";
String webToken = "";

WebServer myServer(80);

void handleRoot() {
  String html = R"(
  <!DOCTYPE html>
  <html lang='en'>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>เชื่อมต่อ Wi-Fi</title>
    <style>
      select, input[type=text], input[type=password] {
        width: 30%;
        padding: 12px 20px;
        margin: 8px 0;
        box-sizing: border-box;
        border-radius: 4px;
        background-color: #f1f1f1;
      }
      .connectwifi {
        background-color: gray;
        width: 40%;
        height: 50%;
        padding: 20px;
        border-radius: 8px;
      }
      button[type=submit] {
        width: 110px;
        height: 50px;
        background-color: rgb(0, 115, 255);
        border: none;
        border-radius: 4px;
        color: white;
        font-size: 16px;
      }
    </style>
  </head>
  <body>
    <center>
      <form action='/connect' method='POST'>
        <div class="connectwifi">
          <h1>ตั้งค่า Wi-Fi</h1>
          <select name='ssid' id='ssid'>
            <option value=''>-- เลือก Wi-Fi --</option>)";

  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; ++i) {
    html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }

  html += R"(</select><br>
          <input type='password' name='password' placeholder='รหัสผ่าน'><br>
          <input type='text' name='token' value='' placeholder='Token'><br>
          <button type='submit'>ยืนยันการเชื่อมต่อ</button><br>
        </div>
      </form>
    </center>
  </body>
  </html>)";

  myServer.send(200, "text/html", html);
}

void handleConnect() {
  String ssid = myServer.arg("ssid");
  String password = myServer.arg("password");
  String webToken = myServer.arg("token");

  saveWiFi(ssid, password, webToken);
  connectToWiFi();
}

void handleDelete() {
  clearEEPROM();
  webToken = "";
  startAccessPoint();
  myServer.send(200, "text/plain", "ลบการตั้งค่า Wi-Fi เรียบร้อยแล้ว");
}

void saveWiFi(String ssid, String password, String webToken) {
  EEPROM.begin(512);
  char ssidBuf[SSID_SIZE];
  char passBuf[PASS_SIZE];
  char tokenBuf[TOKEN_SIZE];
  ssid.toCharArray(ssidBuf, SSID_SIZE);
  password.toCharArray(passBuf, PASS_SIZE);
  webToken.toCharArray(tokenBuf, TOKEN_SIZE);
  EEPROM.put(SSID_ADDR, ssidBuf);
  EEPROM.put(PASS_ADDR, passBuf);
  EEPROM.put(TOKEN_ADDR, tokenBuf);
  EEPROM.commit();
  EEPROM.end();
}

void clearEEPROM() {
  EEPROM.begin(512);
  for (int i = 0; i < SSID_SIZE; ++i) {
    EEPROM.write(SSID_ADDR + i, 0);
  }
  for (int i = 0; i < PASS_SIZE; ++i) {
    EEPROM.write(PASS_ADDR + i, 0);
  }
  for (int i = 0; i < TOKEN_SIZE; ++i) {
    EEPROM.write(TOKEN_ADDR + i, 0);
  }
  EEPROM.commit();
  EEPROM.end();
}

void loadee() {
  EEPROM.begin(512);
  char ssidBuf[SSID_SIZE];
  char passBuf[PASS_SIZE];
  char tokenBuf[TOKEN_SIZE];
  EEPROM.get(SSID_ADDR, ssidBuf);
  EEPROM.get(PASS_ADDR, passBuf);
  EEPROM.get(TOKEN_ADDR, tokenBuf);
  ssid = String(ssidBuf);
  password = String(passBuf);
  webToken = String(tokenBuf);
  EEPROM.end();
}

void connectToWiFi() {
  WiFi.disconnect(true);
  delay(1000);

  Serial.print("Attempting to connect to ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // ปิด Access Point
    WiFi.softAPdisconnect(true);
    myServer.send(200, "text/plain", "เชื่อมต่อ Wi-Fi สำเร็จ");
  } else {
    Serial.println("\nFailed to connect to WiFi");
    startAccessPoint();
  }
}

void startAccessPoint() {
  IPAddress apIP(192, 168, 1, 1);
  WiFi.softAP("espS2", "12345678", 5, false, 50);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros());
  EEPROM.begin(512);

  loadee();
  if (ssid == "" && password == "" && webToken == "") {
    Serial.println("Starting Access Point");
    startAccessPoint();
  } else {
    Serial.println("Connecting to WiFi");
    connectToWiFi();
  }

  myServer.on("/", handleRoot);
  myServer.on("/connect", handleConnect);
  myServer.on("/delete", handleDelete);
  myServer.begin();

  pinMode(contol_led1, INPUT_PULLUP);
  pinMode(contol_led2, INPUT_PULLUP);
  pinMode(contol_led3, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(ledpin1, OUTPUT);
  pinMode(ledpin2, OUTPUT);
  pinMode(ledpin3, OUTPUT);
  Serial.println("Server started");
}

void loop() {
  myServer.handleClient();

  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      handleDelete();
    }
  }

  static int lastSW1 = HIGH;
  static int lastSW2 = HIGH;
  static int lastSW3 = HIGH;

  int currentSW1 = digitalRead(contol_led1);
  if (currentSW1 != lastSW1) {
    if (currentSW1 == LOW) {
      ls_led1 = !ls_led1;
      digitalWrite(ledpin1, ls_led1);
      String ls_led1_str = String(ls_led1);
      swcontol(webToken, "1", ls_led1_str);
    }
    lastSW1 = currentSW1;
    delay(250);
  }

int currentSW2 = digitalRead(contol_led2);
  if (currentSW2 != lastSW2) {
    if (currentSW2 == LOW) {
      ls_led2 = !ls_led2;
      digitalWrite(ledpin2, ls_led2);
      String ls_led2_str = String(ls_led2);
      swcontol(webToken, "2", ls_led2_str);
    }
    lastSW2 = currentSW2;
    delay(250);
  }

  int currentSW3 = digitalRead(contol_led3);
  if (currentSW3 != lastSW3) {
    if (currentSW3 == LOW) {
      ls_led3 = !ls_led3;
      digitalWrite(ledpin3, ls_led3);
      String ls_led3_str = String(ls_led3);
      swcontol(webToken, "3", ls_led3_str);
    }
    lastSW3 = currentSW3;
    delay(250);
  }
  //ledcontrol(webToken);
}
void swcontol(String webToken, String pinled, String statusled) {
  HTTPClient http;
  http.begin(serverAddr, serverPort, "/swcontrol");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postData = "token=" + webToken + "&pin=" + pinled + "&status=" + statusled;
  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }


    int responseStatus = doc["status"];
    Serial.print("Response Status: ");
    Serial.println(responseStatus);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void ledcontrol(String webToken) {
  delay(500);
  HTTPClient http;
  http.begin(serverAddr, serverPort, "/ledcontrol");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.POST("token=" + webToken);

  if (httpResponseCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc; 
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    } else {
      Serial.println("JSON Document:");
      serializeJsonPretty(doc, Serial);
      Serial.println();

    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
