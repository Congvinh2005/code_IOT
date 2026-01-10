#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include "BluetoothSerial.h" 

BluetoothSerial SerialBT;

String BOT_TOKEN = "8240245709:AAEu_ciOfNLNTdn1ANamzCzhGSsT0UtrbPI"; 
String CHAT_ID = "5910648332"; 

#define SS_PIN 32   
#define RST_PIN 33 
MFRC522 rfid(SS_PIN, RST_PIN);
String uidString;

// ===== CẤU HÌNH SERIAL GIAO TIẾP ARDUINO (Serial2) =====
#define RXD2 16 
#define TXD2 17 

// ===== CẤU HÌNH WIFI =====
const char* ssid = "Long 2.4G 3"; 
const char* password = "123456789";
String Web_App_URL = "https://script.google.com/macros/s/AKfycbwtEVcFEvqSPvasEReOtsofOXwUg7YUCI-qi6xQ0z83BY77H9Z-Xd--f1Zx8GWtIthDeg/exec";

#define On_Board_LED_PIN 2
#define MAX_STUDENTS 10 
struct Student {
  String id;
  char code[10];
  char name[30];
};
Student students[MAX_STUDENTS];

int studentCount = 0;
int runMode=2;
const int btnIO = 15;
bool btnIOState = HIGH;
unsigned long timeDelay=millis();
unsigned long timeDelay2=millis();
bool InOutState=0; 
const int ledIO = 4;
const int buzzer = 5; 


unsigned long lastTelegramTime = 0;
const unsigned long telegramDelay = 3000;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  pinMode(btnIO, INPUT_PULLUP);
  pinMode(ledIO, OUTPUT);
  pinMode(On_Board_LED_PIN, OUTPUT);

  // Kết nối WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  
  if(!readDataSheet())
   Serial.println("Can't read data from google sheet!");

  SPI.begin(); 
  rfid.PCD_Init(); 

  Serial.println("Hệ thống sẵn sàng !");
  SerialBT.begin("SmartHome_Group3"); 
}

void loop() {
  if(millis()-timeDelay2>500){
    readUID();
    timeDelay2=millis();
  }
  
  if(digitalRead(btnIO)==LOW){
    if(btnIOState==HIGH){
      if(millis()-timeDelay>500){
        InOutState = !InOutState;
        digitalWrite(ledIO,InOutState);
        timeDelay=millis();
      }
      btnIOState=LOW;
    }
  } else {
    btnIOState=HIGH;
  }

  // --- 1. NHẬN LỆNH TỪ ĐIỆN THOẠI (BLUETOOTH) ---
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n'); 
    cmd.trim();
    cmd.toLowerCase();

    Serial.print("Bluetooth Recv: ");
    Serial.println(cmd);

    if (cmd == "o" || cmd == "open" || cmd == "mở cửa" || cmd == "mo cua") {
        Serial2.println("DOOR_OPEN");
    }
    else if (cmd == "c" || cmd == "close" || cmd == "đóng cửa" || cmd == "dong cua") {
        Serial2.println("DOOR_CLOSE");
    }
    else if (cmd == "l" || cmd == "bật led" || cmd == "led on") {
        Serial2.println("LED_ON");
    }
    else if (cmd == "tắt led" || cmd == "led off") {
        Serial2.println("LED_OFF");
    }
    else if (cmd == "f" || cmd == "bật quạt") {
        Serial2.println("FAN_ON");
    }
    else if (cmd == "tắt quạt") {
        Serial2.println("FAN_OFF");
    }
    else if (cmd == "bật cảnh báo") {
        Serial2.println("ALARM_ON");
    }
    else if (cmd == "tắt cảnh báo") {
        Serial2.println("ALARM_OFF");
    }
  }

  // --- 2. NHẬN TÍN HIỆU TỪ ARDUINO (CHUYỂN ĐỘNG / KHOẢNG CÁCH) ---
  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    msg.trim();
    
    // Nếu Arduino gửi "MOTION" (chỉ gửi khi PIR được bật)
    if (msg == "MOTION") {
       Serial.println("PIR ALARM from Arduino!");
       
       if (millis() - lastTelegramTime > telegramDelay) {
         sendTelegram("⚠️ CẢNH BÁO: Phát hiện người xâm nhập!");
         lastTelegramTime = millis();
       }
    } 
    // Phản hồi từ nút bấm cảnh báo
    else if (msg == "ALARM_BUTTON_ON") {
        Serial.println("PIR: Da bat canh bao bang nut");
    }
    else if (msg == "ALARM_BUTTON_OFF") {
        Serial.println("PIR: Da tat canh bao bang nut");
    }
    else {
        // Các tín hiệu khác (khoảng cách, cửa tự động)
        Serial.print("Arduino Info: ");
        Serial.println(msg);
    }
  }
}

// =============================================
// ============ HÀM PHỤ TRỢ (ESP32) =============
// =============================================

void sendTelegram(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    
    SerialBT.end(); 
    delay(100);

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + urlencode(message);
    
    Serial.println("Đang gửi Telegram...");
    http.begin(url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 
    
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Gửi Telegram thành công!");
    } else {
      Serial.println("Lỗi gửi Telegram: " + String(httpCode));
    }
    http.end();

    delay(100);
    SerialBT.begin("SmartHome_Group3");
    
  } else {
    Serial.println("WiFi mất kết nối, không thể gửi Telegram");
  }
}

void beep(int n, int d){
  for(int i=0;i<n;i++){
    digitalWrite(buzzer,HIGH);
    delay(d);
    digitalWrite(buzzer,LOW);
    delay(d);
  }
}

void readUID(){
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  if ( ! rfid.PICC_IsNewCardPresent()) return;
  if ( ! rfid.PICC_ReadCardSerial()) return;

  uidString="";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidString.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    uidString.concat(String(rfid.uid.uidByte[i], HEX));
  }
  uidString.toUpperCase();
  Serial.println("RFID_READ:" + uidString);
  beep(1, 100);

  // if(runMode==1)  writeUIDSheet(); //Ghi UID mới vào Sheet
  else if(runMode==2) writeLogSheet(); //Điểm danh + kiểm tra đúng/sai

  rfid.PICC_HaltA(); // dừng giao tiếp 
  rfid.PCD_StopCrypto1();// cho phép đọc thẻ mới tiếp theo
}



void writeLogSheet() {

  SerialBT.end();
  delay(200);

  char charArray[uidString.length() + 1];
  uidString.toCharArray(charArray, uidString.length() + 1);
  char* studentName = getStudentNameById(charArray);

  if (studentName != nullptr) {
    // Gửi lệnh mở cửa
    Serial.println("RFID_DUNG: " + uidString);
    Serial.print("TEN CHU THE: ");
    Serial.println(studentName);
    Serial2.println("RFID_VALID"); 
  
    String Send_Data_URL = Web_App_URL + "?sts=writelog";
    Send_Data_URL += "&uid=" + uidString;
    Send_Data_URL += "&name=" + urlencode(String(studentName));
    Send_Data_URL += (InOutState == 0) ? "&inout="+urlencode("VÀO") : "&inout="+urlencode("RA");

    HTTPClient http;
    http.begin(Send_Data_URL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    Serial.println("HTTP code: " + String(httpCode));

    http.end();
  }
  else {
    Serial.println("Thẻ sai:" + uidString);
    Serial2.println("RFID_INVALID");
    
    beep(3, 200);
  }

  delay(200);
  SerialBT.begin("SmartHome_Group3");
}

bool readDataSheet(){
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(On_Board_LED_PIN, HIGH);
    String Read_Data_URL = Web_App_URL + "?sts=read";
    HTTPClient http;
    http.begin(Read_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET(); 
    String payload;
    studentCount=0;
    if (httpCode > 0) {
        payload = http.getString();
        char charArray[payload.length() + 1];
        payload.toCharArray(charArray, payload.length() + 1);
        int numberOfElements = countElements(charArray, ',');
        char *token = strtok(charArray, ",");
        while (token != NULL && studentCount < numberOfElements/3) {
            students[studentCount].id = String(token);
            token = strtok(NULL, ",");
            if(token) strcpy(students[studentCount].code, token);
            token = strtok(NULL, ",");
            if(token) strcpy(students[studentCount].name, token);
            studentCount++;
            token = strtok(NULL, ",");
        }
    }
    http.end();
    digitalWrite(On_Board_LED_PIN, LOW);
    if(studentCount>0) return true;
    else return false;
  }
  return false;
}

// void writeUIDSheet(){
//    String Send_Data_URL = Web_App_URL + "?sts=writeuid";
//    Send_Data_URL += "&uid=" + uidString;
//    HTTPClient http;
//    http.begin(Send_Data_URL.c_str());
//    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
//    http.GET(); 
//    http.end();
// }

String urlencode(String str) {
  String encodedString = "";
  char c, code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') encodedString += '+';
    else if (isalnum(c)) encodedString += c;
    else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
}

char* getStudentNameById(char* uid) {
  for (int i = 0; i < studentCount; i++) {
    if (strcmp(students[i].code, uid) == 0) {
      return students[i].name;
    }
  }
  return nullptr;
}

int countElements(const char* data, char delimiter) {
  char dataCopy[strlen(data) + 1];
  strcpy(dataCopy, data);
  int count = 0;
  char* token = strtok(dataCopy, &delimiter);
  while (token != NULL) {
    count++;
    token = strtok(NULL, &delimiter);
  }
  return count;
}