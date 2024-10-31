#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <FS.h>

const int ledPin = D1; // ตั้งค่าขาที่ใช้ควบคุมไฟ
const int resetButtonPin = D3; // ขาที่เชื่อมต่อปุ่มรีเซ็ต
const int EEPROM_SIZE = 512; // ขนาด EEPROM
int address = 0; // ตำแหน่งของข้อมูลใน EEPROM

ESP8266WebServer server(80);

void handleRoot() {
  if (server.hasArg("light")) {
    String command = server.arg("light");
    if (command == "on") {
      // สั่งเปิดไฟ
      digitalWrite(ledPin, HIGH);
    } else if (command == "off") {
      // สั่งปิดไฟ
      digitalWrite(ledPin, LOW);
    }
  }

  String page = "<html><head><title>ESP8266 Light Control</title></head>";
  page += "<body><h1>ESP8266 Light Control</h1>";
  page += "<form action='/' method='GET'>";
  page += "<button type='submit' name='light' value='on'>Turn On Light</button>";
  page += "<button type='submit' name='light' value='off'>Turn Off Light</button>";
  page += "</form></body></html>";

  server.send(200, "text/html", page); 
}

void resetWiFi() {
  Serial.println("Resetting WiFi configuration...");
  if (SPIFFS.begin()) {
    SPIFFS.format(); // ลบข้อมูลทั้งหมดใน SPIFFS
    SPIFFS.end();
  }
  Serial.println("WiFi configuration reset. Restarting configuration portal...");
  delay(2000);
  ESP.reset();
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT); // กำหนดขา ledPin เป็นเอาต์พุต
  pinMode(resetButtonPin, INPUT_PULLUP); // กำหนดขา resetButtonPin เป็นขา Input แบบ Pull-Up
  WiFiManager wifiManager;

  // เชื่อมต่อ Wi-Fi
  if (!wifiManager.autoConnect("All")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  // ตรวจสอบการกดปุ่มรีเซ็ต Wi-Fi
  if (digitalRead(resetButtonPin) == LOW) {
    delay(50); // รอสักครู่เพื่อป้องกันการกระชาก
    if (digitalRead(resetButtonPin) == LOW) {
      resetWiFi(); // เรียกฟังก์ชันเมื่อปุ่มถูกกด
    }
  }
}
