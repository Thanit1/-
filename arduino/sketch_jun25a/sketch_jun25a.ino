#include <WiFi.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define BUTTON_PIN 34
#define contol_led1 35
#define contol_led2 32
#define contol_led3 33

int lastSW1 = 0;
int lastSW2 = 0;
int lastSW3 = 0;

#define ledpin1 12
#define ledpin2 14
#define ledpin3 27

#define SSID_SIZE 32
#define PASS_SIZE 64
#define SSID_ADDR 0
#define PASS_ADDR (SSID_ADDR + SSID_SIZE)
#define TOKEN_SIZE 32
#define TOKEN_ADDR (PASS_ADDR + PASS_SIZE)
#define LED_ON_TIME_ADDR1 (TOKEN_ADDR + TOKEN_SIZE)
#define LED_ON_TIME_ADDR2 (LED_ON_TIME_ADDR1 + sizeof(unsigned long))
#define LED_ON_TIME_ADDR3 (LED_ON_TIME_ADDR2 + sizeof(unsigned long))
#define TEN_MINUTES 600000

int st = 0;
const char* serverAddr = "http://lcmweb.azurewebsites.net";
const int serverPort = 8080;
String ssid = "";
String password = "";
String webToken = "";

unsigned long ledOnTime1 = 0;
unsigned long ledOnTime2 = 0;
unsigned long ledOnTime3 = 0;

bool timeSent1 = false;
bool timeSent2 = false;
bool timeSent3 = false;
bool upStatus = false;
bool datatime = false;
bool apstatus = false;
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
        /* General styles */
        select,
        input[type=text],
        input[type=password] {
            width: 100%;
            max-width: 100%;
            padding: 12px 20px;
            margin: 8px 0;
            box-sizing: border-box;
            border-radius: 4px;
            background-color: #f1f1f1;
        }

        .connectwifi {
            box-shadow: rgba(0, 0, 0, 0.25) 0px 54px 55px, rgba(0, 0, 0, 0.12) 0px -12px 30px, rgba(0, 0, 0, 0.12) 0px 4px 6px, rgba(0, 0, 0, 0.17) 0px 12px 13px, rgba(0, 0, 0, 0.09) 0px -3px 5px;
            width: 90%;
            max-width: 400px;
            padding: 20px;
            border-radius: 8px;
            background-color: #fff;
        }

        /* Button styles */
        .button-58 {
            align-items: center;
            background-color: #06f;
            border: 2px solid #06f;
            box-sizing: border-box;
            color: #fff;
            cursor: pointer;
            display: inline-flex;
            fill: #000;
            font-family: Inter, sans-serif;
            font-size: 16px;
            font-weight: 600;
            height: 48px;
            justify-content: center;
            letter-spacing: -.8px;
            line-height: 24px;
            min-width: 140px;
            outline: 0;
            padding: 0 17px;
            text-align: center;
            text-decoration: none;
            transition: all .3s;
            user-select: none;
            -webkit-user-select: none;
            touch-action: manipulation;
            width: 100%;
            max-width: 100%;
        }

        .button-58:focus {
            color: #171e29;
        }

        .button-58:hover {
            background-color: #3385ff;
            border-color: #3385ff;
            fill: #06f;
        }

        .button-58:active {
            background-color: #3385ff;
            border-color: #3385ff;
            fill: #06f;
        }

        @media (min-width: 768px) {
            .connectwifi {
                width: 50%;
            }

            .button-58 {
                width: auto;
                max-width: 170px;
            }
        }
    </style>
</head>

<body>
    <center><br><br>
        <form action='/connect' method='POST'>
            <div class="connectwifi">
                <h1>ตั้งค่า Wi-Fi</h1>
                <select name='ssid' id='ssid'>
                    <option value=''>-- เลือก Wi-Fi --</option>)";

  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; ++i) {
    html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }

  html += R"(
                </select><br>
                <input type='password' name='password' placeholder='รหัสผ่าน'><br>
          <input type='text' name='token' value='' placeholder='Token'><br>
          <button class="button-58" type='submit'>ยืนยันการเชื่อมต่อ</button><br>
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
  ESP.restart();

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
  ESP.restart();
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
  if (ssid == "" || password == "" || webToken == "") {
    Serial.println("Missing WiFi or server credentials. Starting Access Point.");
    startAccessPoint();
    return;
  }

  WiFi.disconnect(true);
  delay(500);

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
    lambController(webToken);
    // ปิด Access Point
    apstatus = false;
    WiFi.softAPdisconnect(true);
    myServer.send(200, "text/plain", "เชื่อมต่อ Wi-Fi สำเร็จ");
  } else {
    Serial.println("\nFailed to connect to WiFi");
    startAccessPoint();
  }
}

void startAccessPoint() {
  apstatus = true;
  IPAddress apIP(192, 168, 1, 1);
  WiFi.softAP("esp32", "12345678", 5, false, 50);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
}
void checkButton() {
  static unsigned long buttonPressTime = 0;
  static bool buttonPressed = false;

  if (digitalRead(BUTTON_PIN) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressTime = millis();
    } else {
      if (millis() - buttonPressTime >= 5000) {
        // Button held for 5 seconds
        handleDelete();
      }
    }
  } else {
    if (buttonPressed) {
      if (millis() - buttonPressTime < 5000) {
        ESP.restart();
      }
      buttonPressed = false;
    }
  }
}
volatile bool sw1Changed = false;
volatile bool sw2Changed = false;
volatile bool sw3Changed = false;

void IRAM_ATTR handleSwitch1Interrupt() {
  sw1Changed = true;
}

void IRAM_ATTR handleSwitch2Interrupt() {
  sw2Changed = true;
}

void IRAM_ATTR handleSwitch3Interrupt() {
  sw3Changed = true;
}

void setup() {
  pinMode(contol_led1, INPUT_PULLUP);
  pinMode(contol_led2, INPUT_PULLUP);
  pinMode(contol_led3, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(ledpin1, OUTPUT);
  pinMode(ledpin2, OUTPUT);
  pinMode(ledpin3, OUTPUT);
  digitalWrite(ledpin1, LOW);
  digitalWrite(ledpin2, LOW);
  digitalWrite(ledpin3, LOW);
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


  Serial.println("Server started");
   attachInterrupt(digitalPinToInterrupt(contol_led1), handleSwitch1Interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(contol_led2), handleSwitch2Interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(contol_led3), handleSwitch3Interrupt, CHANGE);
}

void loop() {
 
    myServer.handleClient();
  
  checkButton();
if (sw1Changed) {
    handleSwitches1();
    sw1Changed = false;
  }
  if (sw2Changed) {
    handleSwitches2();
    sw2Changed = false;
  }
  if (sw3Changed) {
    handleSwitches3();
    sw3Changed = false;
  }
  addTimeONled();

  if (upStatus == false) {
    lambController(webToken);
  } else {
    swcontol(webToken);
  }

}
unsigned long lastLedOnTime1 = 0;
unsigned long lastLedOnTime2 = 0;
unsigned long lastLedOnTime3 = 0;

void addTimeONled() {
  if (!datatime) {
    EEPROM.begin(512); // เริ่มต้นการทำงานของ EEPROM ขนาด 512 ไบต์

    unsigned long currentTime = millis();  // เก็บเวลาปัจจุบัน

    // ตรวจสอบว่า LED1 ถูกเปิดนานเกิน 1 นาทีหรือไม่
    if (digitalRead(ledpin1) == HIGH) {
      if (currentTime - lastLedOnTime1 >= 60000) {  // นานเกิน 1 นาที
        ledOnTime1 += 1;  // เพิ่มเวลา 1 นาที
        EEPROM.put(LED_ON_TIME_ADDR1, ledOnTime1);
        lastLedOnTime1 = currentTime;  // รีเซ็ตเวลาที่บันทึก
      }
    } else {
      lastLedOnTime1 = currentTime;  // ถ้า LED ปิดให้รีเซ็ตเวลาเริ่มต้นใหม่
    }

    // ตรวจสอบว่า LED2 ถูกเปิดนานเกิน 1 นาทีหรือไม่
    if (digitalRead(ledpin2) == HIGH) {
      if (currentTime - lastLedOnTime2 >= 60000) {
        ledOnTime2 += 1;  // เพิ่มเวลา 1 นาที
        EEPROM.put(LED_ON_TIME_ADDR2, ledOnTime2);
        lastLedOnTime2 = currentTime;
      }
    } else {
      lastLedOnTime2 = currentTime;
    }

    // ตรวจสอบว่า LED3 ถูกเปิดนานเกิน 1 นาทีหรือไม่
    if (digitalRead(ledpin3) == HIGH) {
      if (currentTime - lastLedOnTime3 >= 60000) {
        ledOnTime3 += 1;  // เพิ่มเวลา 1 นาที
        EEPROM.put(LED_ON_TIME_ADDR3, ledOnTime3);
        lastLedOnTime3 = currentTime;
      }
    } else {
      lastLedOnTime3 = currentTime;
    }

    EEPROM.commit();
    EEPROM.end();
  } else {
    // ส่งเวลาไปยังเซิร์ฟเวอร์
    timetoserver(webToken);
  }
}


void timetoserver(String webToken) {
  EEPROM.begin(512);
  unsigned long storedTime1;
  unsigned long storedTime2;
  unsigned long storedTime3;

  // อ่านเวลาที่ถูกสะสมจาก EEPROM
  EEPROM.get(LED_ON_TIME_ADDR1, storedTime1);
  EEPROM.get(LED_ON_TIME_ADDR2, storedTime2);
  EEPROM.get(LED_ON_TIME_ADDR3, storedTime3);

  // พิมพ์ค่าเพื่อเช็คความถูกต้อง
  Serial.print("Time for LED 1: ");
  Serial.println(storedTime1);
  Serial.print("Time for LED 2: ");
  Serial.println(storedTime2);
  Serial.print("Time for LED 3: ");
  Serial.println(storedTime3);

  HTTPClient http;
  http.begin("http://lcmweb.azurewebsites.net/addTimeONled");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // แก้ไขการสร้าง string สำหรับข้อมูล post
  String postData = "token=" + webToken + "&pin1=" + String(storedTime1) + "&pin2=" + String(storedTime2) + "&pin3=" + String(storedTime3);

  int httpResponseCode = http.POST(postData);

  // ตรวจสอบการตอบกลับจากเซิร์ฟเวอร์
  if (httpResponseCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    // รีเซ็ตเวลาหลังจากส่งข้อมูลสำเร็จ
    ledOnTime1 = 0;
    ledOnTime2 = 0;
    ledOnTime3 = 0;

    EEPROM.put(LED_ON_TIME_ADDR1, ledOnTime1);
    EEPROM.put(LED_ON_TIME_ADDR2, ledOnTime2);
    EEPROM.put(LED_ON_TIME_ADDR3, ledOnTime3);
    EEPROM.commit(); // บันทึกการเปลี่ยนแปลงใน EEPROM
    EEPROM.end();
    int responseStatus = doc["status"];
    Serial.print("Response Status: ");
    Serial.println(responseStatus);
  } else {
    // พิมพ์โค้ด error เมื่อมีปัญหา
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  datatime = false;
  http.end();
}


void handleSwitches1() {
  if (digitalRead(contol_led1) != lastSW1) {
    Serial.println("sw1");
    digitalWrite(ledpin1, !digitalRead(ledpin1)); // Toggle LED state
    lastSW1 = digitalRead(contol_led1);
    upStatus = true;
  }
}

void handleSwitches2() {
  if (digitalRead(contol_led2) != lastSW2) {
    Serial.println("sw2");
    digitalWrite(ledpin2, !digitalRead(ledpin2)); // Toggle LED state
    lastSW2 = digitalRead(contol_led2);
    upStatus = true;
  }
}

void handleSwitches3() {
  if (digitalRead(contol_led3) != lastSW3) {
    Serial.println("sw3");
    digitalWrite(ledpin3, !digitalRead(ledpin3)); // Toggle LED state
    lastSW3 = digitalRead(contol_led3);
    upStatus = true;
  }
}

void swcontol(String webToken) {
   String pin1 =  String(digitalRead(ledpin1));
   String pin2 =  String(digitalRead(ledpin2));
   String pin3 =  String(digitalRead(ledpin3));
   HTTPClient http;
   http.begin("http://lcmweb.azurewebsites.net/swcontrol1");
   http.addHeader("Content-Type", "application/x-www-form-urlencoded");
   String postData = "token=" + webToken + "&pin1=" + pin1 + "&pin2=" + pin2 + "&pin3=" + pin3;
   int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
  String payload = http.getString();
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("swcontol deserializeJson() failed: ");
      Serial.println(error.c_str());
      upStatus = false;
      return;
    }
    upStatus = false;
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void lambController(String webToken) {
  HTTPClient http;
  http.begin("http://lcmweb.azurewebsites.net/lambController");
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


      for (JsonObject item : doc.as<JsonArray>()) {
        int pin = item["pin"];
        String status = item["status"];
        if ((pin == 1 || pin == 2 || pin == 3) && (datatime == false) ) {

          if (item["upgdatetime"] == 1) {
            datatime = true;
            Serial.println("upgdatetime:");
          } else {
            datatime = false;
          }
        }
        
        if (pin == 1) {
          digitalWrite(ledpin1, status == "1" ? HIGH : LOW);
        } else if (pin == 2) {
          digitalWrite(ledpin2, status == "1" ? HIGH : LOW);
        } else if (pin == 3) {
          digitalWrite(ledpin3, status == "1" ? HIGH : LOW);
        }
      }
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
