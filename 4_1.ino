#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include "BluetoothSerial.h" 

// ===== CẤU HÌNH BLUETOOTH =====
BluetoothSerial SerialBT;

// =============================================
// ===== CẤU HÌNH TELEGRAM (BẮT BUỘC SỬA) =====
// =============================================
String BOT_TOKEN = "8240245709:AAEu_ciOfNLNTdn1ANamzCzhGSsT0UtrbPI"; 
String CHAT_ID = "5910648332"; 

// ===== CẤU HÌNH RFID (MFRC522) =====
#define SS_PIN 32   
#define RST_PIN 33 
MFRC522 rfid(SS_PIN, RST_PIN);
String uidString;

// ===== CẤU HÌNH SERIAL GIAO TIẾP ARDUINO (Serial2) =====
#define RXD2 16 // ESP32 RX2 (Nối với Arduino TX)
#define TXD2 17 // ESP32 TX2 (Nối với Arduino RX)

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
bool InOutState=0; //0: vào, 1:ra
const int ledIO = 4;
const int buzzer = 5; 

// Biến chống spam Telegram
unsigned long lastTelegramTime = 0;
const unsigned long telegramDelay = 3000; // 3 giây mới gửi 1 lần

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
  
  if(!readDataSheet()) Serial.println("Can't read data from google sheet!");

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
  Serial.println("Card UID: "+uidString);
  
  beep(1, 100);

  if(runMode==1)  writeUIDSheet();
  else if(runMode==2) writeLogSheet(); 

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void writeLogSheet() {

  SerialBT.end();
  delay(200);

  char charArray[uidString.length() + 1];
  uidString.toCharArray(charArray, uidString.length() + 1);
  char* studentName = getStudentNameById(charArray);

  if (studentName != nullptr) {
    // Gửi lệnh mở cửa
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
    Serial.println("Thẻ sai");
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

void writeUIDSheet(){
   String Send_Data_URL = Web_App_URL + "?sts=writeuid";
   Send_Data_URL += "&uid=" + uidString;
   HTTPClient http;
   http.begin(Send_Data_URL.c_str());
   http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
   http.GET(); 
   http.end();
}

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

////////



#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

// ================= CẤU HÌNH =================
#define BAUD_RATE 9600
 
// Địa chỉ I2C 0x27 là phổ biến
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// ======= QUANG (PIR, Còi/LED Báo động) =======
const int PIR_IN_QUANG = 2;
const int LOA_LED_QUANG = 3;
const int BUTTON_QUANG = 4; // Nút BẬT/TẮT CẢNH BÁO (systemOn)

// ======= THANH (LED, Nút LED, Quạt, Nút Quạt) =======
const int LED_Thanh = 5;
const int BTN_Thanh = 6;
const int QUAT_Thanh = 13;
const int QUAT_BTN_Thanh = 11;

// ======= VINH (TRIG_SIEU_AM,ECHO_SIEU_AM, Nút Servo, Servo) =======
const int TRIG_SIEU_AM = 7;
const int ECHO_SIEU_AM = 9; 
const int BUTTON_Servo_VINH = 8; 
const int SERVO_PIN = 10;
const int LED_VINH = 1; // Giả định pin LED báo RFID (nếu có)

// ======= DHT11 =======
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================= BIẾN TRẠNG THÁI =================
bool systemOn = false; // MẶC ĐỊNH PIR TẮT KHI KHỞI ĐỘNG
bool detected = false;
bool lastDetectedState = false;

bool ledState = false;
bool fanState = false;
bool manualServoState = false;

// ======= DEBOUNCE (CHỐNG RUNG PHÍM) =======
bool lastAlarmBtn = HIGH;
bool lastBtnLed = HIGH;
bool lastBtnFan = HIGH;
bool lastServoBtn = HIGH;

unsigned long lastAlarmTime = 0;
unsigned long lastLedTime = 0;
unsigned long lastFanButtonTime = 0;
unsigned long lastServoBtnTime = 0;
const unsigned long debounceDelay = 150;

// ======= TỰ ĐỘNG CỬA (AUTO DOOR) =======
unsigned long doorOpenTime = 0;
bool isDoorAutoOpen = false;
const unsigned long AUTO_CLOSE_DELAY = 5000; // 5 giây chờ đóng cửa
const int DISTANCE_THRESHOLD = 10; // Khoảng cách tối đa (cm) để giữ cửa mở (10cm)
unsigned long lastDistanceTime = 0;
const unsigned long distanceInterval = 500; // 500ms đọc siêu âm 1 lần

// ======= SERVO =======
Servo myservo;

// ======= UART =======
String incomingCommand = "";

// ================= LCD SCROLL =================
String lcdLine1 = "SmartHome-Thanh-Vinh-QA";
String lcdTemp = "";
int scrollPos = 0;
unsigned long lastScrollTime = 0;
const unsigned long scrollDelay = 300;


// ================= HÀM ĐO KHOẢNG CÁCH (SIÊU ÂM) =================
float readDistance() {
  digitalWrite(TRIG_SIEU_AM, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_SIEU_AM, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_SIEU_AM, LOW);

  long duration = pulseIn(ECHO_SIEU_AM, HIGH);
  float distanceCm = duration * 0.0343 / 2; 

  return distanceCm;
}


// ================= HÀM CẬP NHẬT LCD =================
void updateLCD(float nhietDo, float doAm) {
  // Tạo chuỗi hiển thị: Nhiệt độ -> Độ ẩm KK 
  lcdTemp = lcdLine1 + " T:" + String(nhietDo, 1) + (char)223 +
            "C H:" + String(doAm, 0) + "% ";

  if (millis() - lastScrollTime > scrollDelay) {
    lcd.setCursor(0, 0);
    if (scrollPos + 16 > lcdTemp.length()) {
      lcd.print(lcdTemp.substring(scrollPos) +
                lcdTemp.substring(0, 16 - (lcdTemp.length() - scrollPos)));
    } else {
      lcd.print(lcdTemp.substring(scrollPos, scrollPos + 16));
    }
    scrollPos++;
    if (scrollPos >= lcdTemp.length()) scrollPos = 0;
    lastScrollTime = millis();
  }

  lcd.setCursor(0, 1);
  lcd.print("CB:"); lcd.print(systemOn ? "O" : "F"); 
  lcd.print(" Q:"); lcd.print(fanState ? "O" : "F");
  lcd.print(" L:"); lcd.print(ledState ? "O" : "F");
  lcd.print(" D:"); lcd.print(manualServoState ? "O" : "F"); 
}

// ================= XỬ LÝ LỆNH TỪ ESP32 (UART) =================
void executeCommand(String cmd) {
  cmd.trim();

  // === CẢNH BÁO PIR ===
  if (cmd == "ALARM_ON") systemOn = true;
  else if (cmd == "ALARM_OFF") systemOn = false;
  
  // === LED ===
  else if (cmd == "LED_ON") ledState = true;
  else if (cmd == "LED_OFF") ledState = false;
  
  // === QUẠT ===
  else if (cmd == "FAN_ON") fanState = true;
  else if (cmd == "FAN_OFF") fanState = false;
  
  // === SERVO (THỦ CÔNG) ===
  else if (cmd == "DOOR_OPEN") {
    myservo.write(90);
    manualServoState = true;
    isDoorAutoOpen = false; 
  }
  else if (cmd == "DOOR_CLOSE") {
    myservo.write(0);
    manualServoState = false;
    isDoorAutoOpen = false; 
  }
  
  // === MỞ CỬA BẰNG THẺ (TỰ ĐÓNG THÔNG MINH) ===
  else if (cmd == "RFID_VALID") {
    digitalWrite(LED_VINH, HIGH); delay(100);
    digitalWrite(LED_VINH, LOW); delay(100);

    myservo.write(90); // Mở cửa
    manualServoState = true;

    isDoorAutoOpen = true; 
    doorOpenTime = millis(); 
    
    Serial.println("CUA MO: Kich hoat tu dong dong thong minh"); 
  }
  
  // === THẺ SAI ===
  else if (cmd == "RFID_INVALID") {
    // Có thể thêm báo hiệu thẻ sai ở đây
  }
}

// ================= HÀM LỌC TÍN HIỆU PIR =================
bool readPIR_Filter() {
  if (digitalRead(PIR_IN_QUANG) == HIGH) {
    delay(120); // Delay nhỏ để lọc nhiễu
    return digitalRead(PIR_IN_QUANG) == HIGH;
  }
  return false;
}

// ================= SETUP =================
void setup() {
  Wire.begin();
  Serial.begin(BAUD_RATE); 
  
  Serial.println("Arduino Ready! (Serial to PC)"); 

  pinMode(PIR_IN_QUANG, INPUT);
  pinMode(LOA_LED_QUANG, OUTPUT);
  pinMode(BUTTON_QUANG, INPUT_PULLUP);

  pinMode(LED_Thanh, OUTPUT);
  pinMode(BTN_Thanh, INPUT_PULLUP);
  pinMode(QUAT_Thanh, OUTPUT);
  pinMode(QUAT_BTN_Thanh, INPUT_PULLUP);

  pinMode(TRIG_SIEU_AM, OUTPUT);
  pinMode(ECHO_SIEU_AM, INPUT);
  pinMode(BUTTON_Servo_VINH, INPUT_PULLUP);
  pinMode(LED_VINH, OUTPUT); 

  lcd.init();
  lcd.backlight();
  lcd.clear();

  myservo.attach(SERVO_PIN);
  myservo.write(0);
  manualServoState = false; 
  systemOn = false; // ĐẢM BẢO PIR TẮT BAN ĐẦU

  dht.begin();
  digitalWrite(QUAT_Thanh, LOW);
  digitalWrite(LOA_LED_QUANG, LOW); 
  digitalWrite(LED_VINH, LOW);
}

// ================= LOOP =================
void loop() {

  // ---- 1. Nhận lệnh từ ESP32 (UART) ----
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      incomingCommand.trim();
      if (incomingCommand.length()) {
        executeCommand(incomingCommand);
        incomingCommand = "";
      }
    } else incomingCommand += c;
  }
  
  // ---- 2. Logic Tự động đóng cửa THÔNG MINH (RFID & SIÊU ÂM) ----
  if (isDoorAutoOpen) {
      
    if (millis() - lastDistanceTime >= distanceInterval) {
        float distance = readDistance();
        lastDistanceTime = millis();
        
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm");
        
        if (distance > 0 && distance <= DISTANCE_THRESHOLD) {
          doorOpenTime = millis(); 
          Serial.println("SIEU_AM_BLOCK"); 
        }
    }
    
    if (millis() - doorOpenTime > AUTO_CLOSE_DELAY) {
      myservo.write(0); 
      manualServoState = false;
      isDoorAutoOpen = false; 
      Serial.println("DOOR_CLOSED_AUTO");
    }
  }


  // ---- 3. NÚT BÁO ĐỘNG (PIN 4) ----
  if (digitalRead(BUTTON_QUANG) == LOW &&
      lastAlarmBtn == HIGH &&
      millis() - lastAlarmTime > debounceDelay) {
    systemOn = !systemOn; // Đảo trạng thái cảnh báo
    Serial.println(systemOn ? "ALARM_BUTTON_ON" : "ALARM_BUTTON_OFF"); // Gửi thông báo
    lastAlarmTime = millis();
  }
  lastAlarmBtn = digitalRead(BUTTON_QUANG);

  // ---- 4, 5, 6. NÚT LED, QUẠT, SERVO (Logic debounce giữ nguyên) ----
  if (digitalRead(BTN_Thanh) == LOW && lastBtnLed == HIGH && millis() - lastLedTime > debounceDelay) {
    ledState = !ledState; lastLedTime = millis();
  }
  lastBtnLed = digitalRead(BTN_Thanh);

  if (digitalRead(QUAT_BTN_Thanh) == LOW && lastBtnFan == HIGH && millis() - lastFanButtonTime > debounceDelay) {
    fanState = !fanState; lastFanButtonTime = millis();
  }
  lastBtnFan = digitalRead(QUAT_BTN_Thanh);

  if (digitalRead(BUTTON_Servo_VINH) == LOW && lastServoBtn == HIGH && millis() - lastServoBtnTime > debounceDelay) {
    manualServoState = !manualServoState;
    if (manualServoState) myservo.write(90);
    else myservo.write(0);
    isDoorAutoOpen = false;
    lastServoBtnTime = millis();
  }
  lastServoBtn = digitalRead(BUTTON_Servo_VINH);

  // ---- 7. PIR & BÁO ĐỘNG (LOGIC QUAN TRỌNG ĐÃ SỬA) ----
  detected = readPIR_Filter();
  
  // Kích hoạt còi/LED báo động (trên Arduino)
  digitalWrite(LOA_LED_QUANG, (systemOn && detected) ? HIGH : LOW);

  // CHỈ GỬI LỆNH "MOTION" KHI systemOn = true
  if (detected && !lastDetectedState) {
    if (systemOn) { 
        Serial.println("MOTION"); // Báo ESP32 
    }
  }
  lastDetectedState = detected;

  // ---- 8. OUTPUT (Thực thi LED & Quạt) ----
  digitalWrite(LED_Thanh, ledState ? HIGH : LOW);
  digitalWrite(QUAT_Thanh, fanState ? HIGH : LOW);

  // ---- 9. ĐỌC CẢM BIẾN & HIỂN THỊ LCD ----
  float doAmKK = dht.readHumidity();
  float nhietDo = dht.readTemperature();
  if (isnan(doAmKK)) doAmKK = 0;
  if (isnan(nhietDo)) nhietDo = 0;

  updateLCD(nhietDo, doAmKK);

  delay(50);
}