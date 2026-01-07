// code esp32

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


// code sheet 
function doGet(e) { 
  Logger.log(JSON.stringify(e));
  var result = 'Ok';
  if (e.parameter == 'undefined') {
    result = 'No Parameters';
  }
  else {
    var sheet_id = '1_ewZ39cee0FxRZO8mC_63_oOu7iiOoVyv8umdxcGn6A'; 	// Spreadsheet ID.
    var sheet_name = "DANHSACH";  // Sheet Name in Google Sheets.

    var sheet_open = SpreadsheetApp.openById(sheet_id);
    var sheet_target = sheet_open.getSheetByName(sheet_name);

    var newRow = sheet_target.getLastRow();

    var rowDataLog = [];

    var Curr_Date = Utilities.formatDate(new Date(), "Asia/Jakarta", 'dd/MM/yyyy');
    rowDataLog[0] = Curr_Date;  // Date will be written in column A (in the "DHT11 Sensor Data Logger" section).

    var Curr_Time = Utilities.formatDate(new Date(), "Asia/Jakarta", 'HH:mm:ss');
    rowDataLog[1] = Curr_Time;  // Time will be written in column B (in the "DHT11 Sensor Data Logger" section).

    var sts_val = '';

    for (var param in e.parameter) {
      Logger.log('In for loop, param=' + param);
      var value = stripQuotes(e.parameter[param]);
      Logger.log(param + ':' + e.parameter[param]);
      switch (param) {
        case 'sts':
          sts_val = value;
          break;

        case 'uid':
          rowDataLog[2] = value;  
          result += ', UID Written';
          break;

        case 'name':
          rowDataLog[3] = value; 
          result += ', Name Written';
          break; 

        case 'inout':
          rowDataLog[4] = value; 
          result += ', INOUT Written';
          break;       

        default:
          result += ", unsupported parameter";
      }
    }

    // Conditions for writing data received from ESP32 to Google Sheets.
    if (sts_val == 'writeuid') {
      // Writes data to the "DHT11 Sensor Data Logger" section.
      Logger.log(JSON.stringify(rowDataLog));
      
      // Ensure rowDataLog is an array and has at least 3 elements
      if (Array.isArray(rowDataLog) && rowDataLog.length > 2) {
        var RangeDataLatest = sheet_target.getRange('F1');
        RangeDataLatest.setValue(rowDataLog[2]);
        
        return ContentService.createTextOutput('Success');
      } else {
        Logger.log('Error: rowDataLog is not valid');
        return ContentService.createTextOutput('Error: Invalid data');
      }
    }
    
    // Conditions for writing data received from ESP32 to Google Sheets.
    if (sts_val == 'writelog') {
      sheet_name = "DIEMDANH";  // Sheet Name in Google Sheets.
      sheet_target = sheet_open.getSheetByName(sheet_name);
      // Writes data to the "DHT11 Sensor Data Logger" section.
      Logger.log(JSON.stringify(rowDataLog));
      // Insert a new row above the existing data.
      sheet_target.insertRows(2);
      var newRangeDataLog = sheet_target.getRange(2,1,1, rowDataLog.length);
      newRangeDataLog.setValues([rowDataLog]);
      //maxRowData(11);
      return ContentService.createTextOutput(result);
    }
    
    // Conditions for sending data to ESP32 when ESP32 reads data from Google Sheets.
    if (sts_val == 'read') {
      sheet_name = "DANHSACH";  // Sheet Name in Google Sheets.
      sheet_target = sheet_open.getSheetByName(sheet_name);

      // Use the line of code below if you want ESP32 to read data from columns I3 to O3 (Date,Time,Sensor Reading Status,Temperature,Humidity,Switch 1, Switch 2).
      var all_Data = sheet_target.getRange('A2:C11').getDisplayValues();
      
      // Use the line of code below if you want ESP32 to read data from columns K3 to O3 (Sensor Reading Status,Temperature,Humidity,Switch 1, Switch 2).
      //var all_Data = sheet_target.getRange('A2:C11').getValues();
      return ContentService.createTextOutput(all_Data);
    }
  }
}
function maxRowData(allRowsAfter) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet()
                              .getSheetByName('DATA')
  
  sheet.getRange(allRowsAfter+1, 1, sheet.getLastRow()-allRowsAfter, sheet.getLastColumn())
       .clearContent()

}
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}
//________________

// code arduino



#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

// ================= CẤU HÌNH =================
#define BAUD_RATE 9600
 
// Địa chỉ I2C 0x27 là phổ biến, nếu không hoạt động, thử 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// ======= QUANG (PIR, Còi/LED Báo động) =======
const int PIR_IN_QUANG = 2;
const int LOA_LED_QUANG = 3;
const int BUTTON_QUANG = 4; // Nút BẬT/TẮT CẢNH BÁO (systemOn)

// ======= THANH (LED, Nút LED, Quạt, Nút Quạt) =======
const int LED_Thanh = 5;
const int BTN_Thanh = 6;
// Chân 13 là QUAT_Thanh (OUTPUT tới Transistor điều khiển Quạt 5V)
const int QUAT_Thanh = 13; 
const int QUAT_BTN_Thanh = 11;

// ======= VINH (TRIG_SIEU_AM,ECHO_SIEU_AM, Nút Servo, Servo) =======
const int TRIG_SIEU_AM = 7;
const int ECHO_SIEU_AM = 9; 
const int BUTTON_Servo_VINH = 8; 
const int SERVO_PIN = 10;
const int LED_VINH = 1; // Giả định pin LED báo RFID

// ======= DHT11 =======
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================= BIẾN TRẠNG THÁI =================
bool systemOn = false; 
bool detected = false;
bool lastDetectedState = false;

bool ledState = false;
bool fanState = false; // Trạng thái quạt do Nút bấm hoặc ESP32 điều khiển (Manual State)
bool manualServoState = false;

// ======= LOGIC TỰ ĐỘNG & XUNG ĐỘT =======
bool fanAutoState = false; // Trạng thái quạt do Nhiệt độ quyết định (Auto State)
bool fanManualOverride = false; // Cờ ghi đè: TRUE nếu đang điều khiển thủ công/từ xa
const float TEMP_THRESHOLD = 20.0; // Ngưỡng nhiệt độ (25 độ C)

// ======= NON-BLOCKING TIMERS =======
const unsigned long debounceDelay = 150;
const unsigned long AUTO_CLOSE_DELAY = 5000; 
const unsigned long distanceInterval = 500; 
const unsigned long dhtReadInterval = 2000; // 2 giây đọc DHT11 1 lần
const unsigned long scrollDelay = 300;

unsigned long lastAlarmTime = 0;
unsigned long lastLedTime = 0;
unsigned long lastFanButtonTime = 0;
unsigned long lastServoBtnTime = 0;
unsigned long doorOpenTime = 0;
unsigned long lastDistanceTime = 0;
unsigned long lastDhtReadTime = 0;
unsigned long lastScrollTime = 0;

// ======= TRẠNG THÁI HIỆN TẠI =======
float currentTemperature = 0.0;
float currentHumidity = 0.0;
bool isDoorAutoOpen = false;
const int DISTANCE_THRESHOLD = 10; 

// ======= DEBOUNCE STATES =======
bool lastAlarmBtn = HIGH;
bool lastBtnLed = HIGH;
bool lastBtnFan = HIGH;
bool lastServoBtn = HIGH;

// ======= SERVO & UART & LCD =======
Servo myservo;
String incomingCommand = "";
String lcdLine1 = "SmartHome-Thanh-Vinh-QA";
String lcdTemp = "";
int scrollPos = 0;


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

// ================= HÀM ĐỌC DHT (NON-BLOCKING) =================
void readDhtSensor() {
    if (millis() - lastDhtReadTime >= dhtReadInterval) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        // Chỉ cập nhật nếu giá trị đọc được là hợp lệ (không phải NaN)
        if (!isnan(h)) currentHumidity = h;
        if (!isnan(t)) currentTemperature = t;

        // --- LOGIC QUẠT TỰ ĐỘNG THEO NHIỆT ĐỘ ---
        if (currentTemperature > TEMP_THRESHOLD) {
            fanAutoState = true;
        } else if (currentTemperature <= TEMP_THRESHOLD) { // Dùng <= để bao quát cả trường hợp t<25 và t=0 (lỗi đọc)
            fanAutoState = false;
        }
        // ---------------------------------------------
        
        lastDhtReadTime = millis();
    }
}


// ================= HÀM CẬP NHẬT LCD =================
void updateLCD() {
  // Tạo chuỗi hiển thị: Nhiệt độ -> Độ ẩm KK 
  lcdTemp = lcdLine1 + " T:" + String(currentTemperature, 1) + (char)223 +
            "C H:" + String(currentHumidity, 0) + "% ";

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

  // Dòng 2: Hiện trạng thái Quạt là trạng thái VẬT LÝ cuối cùng (digitalRead)
  lcd.setCursor(0, 1);
  lcd.print("CB:"); lcd.print(systemOn ? "O" : "F"); 
  
  // Trạng thái hiển thị Quạt là trạng thái vật lý thực tế
  lcd.print(" Q:"); lcd.print(digitalRead(QUAT_Thanh) ? "O" : "F");
  
  lcd.print(" L:"); lcd.print(ledState ? "O" : "F");
  lcd.print(" D:"); lcd.print(manualServoState ? "O" : "F"); 
}

// ================= XỬ LÝ LỆNH TỪ ESP32 (UART) =================
void executeCommand(String cmd) {
  cmd.trim();

  if (cmd == "ALARM_ON") systemOn = true;
  else if (cmd == "ALARM_OFF") systemOn = false;
  
  else if (cmd == "LED_ON") ledState = true;
  else if (cmd == "LED_OFF") ledState = false;
  
  // === LỆNH QUẠT TỪ XA: BẬT CỜ GHI ĐÈ ===
  else if (cmd == "FAN_ON") {
      fanState = true;
      fanManualOverride = true; 
  }
  else if (cmd == "FAN_OFF") {
      fanState = false;
      fanManualOverride = true; 
  }
  
  // === SERVO & RFID ===
  else if (cmd == "DOOR_OPEN") {
    myservo.write(140);
    manualServoState = true;
    isDoorAutoOpen = false; 
  }
  else if (cmd == "DOOR_CLOSE") {
    myservo.write(0);
    manualServoState = false;
    isDoorAutoOpen = false; 
  }
  else if (cmd == "RFID_VALID") {
    digitalWrite(LED_VINH, HIGH); delay(100);
    digitalWrite(LED_VINH, LOW); delay(100);

    myservo.write(140); 
    manualServoState = true;
    isDoorAutoOpen = true; 
    doorOpenTime = millis(); 
    
    Serial.println("CUA MO: Kich hoat tu dong dong thong minh"); 
  }
  else if (cmd == "RFID_INVALID") {
    // Có thể thêm báo hiệu thẻ sai ở đây
  }
}

// ================= HÀM LỌC TÍN HIỆU PIR =================
bool readPIR_Filter() {
  if (digitalRead(PIR_IN_QUANG) == HIGH) {
    // delay(120) vẫn được giữ lại vì PIR thường cần thời gian ổn định
    // và việc này KHÔNG ảnh hưởng lớn đến tổng thể non-blocking loop
    delay(120); 
    return digitalRead(PIR_IN_QUANG) == HIGH;
  }
  return false;
}

// ================= SETUP =================
void setup() {
  Wire.begin();
  Serial.begin(BAUD_RATE); 
  
  Serial.println("Arduino Ready!"); 

  // --- Pin Modes ---
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

  // --- Khởi tạo Thiết bị ---
  lcd.init();
  lcd.backlight();
  lcd.clear();

  myservo.attach(SERVO_PIN);
  myservo.write(0);

  dht.begin();
  
  // Đảm bảo tất cả OUTPUT ở trạng thái LOW ban đầu
  digitalWrite(QUAT_Thanh, LOW);
  digitalWrite(LOA_LED_QUANG, LOW); 
  digitalWrite(LED_VINH, LOW);
  digitalWrite(LED_Thanh, LOW);
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
        
        if (distance > 0 && distance <= DISTANCE_THRESHOLD) {
          doorOpenTime = millis(); // Reset timer khi có người cản
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


  // ---- 3. NÚT BÁO ĐỘNG (Bật/Tắt systemOn) ----
  if (digitalRead(BUTTON_QUANG) == LOW &&
      lastAlarmBtn == HIGH &&
      millis() - lastAlarmTime > debounceDelay) {
    systemOn = !systemOn; 
    Serial.println(systemOn ? "ALARM_BUTTON_ON" : "ALARM_BUTTON_OFF"); 
    lastAlarmTime = millis();
  }
  lastAlarmBtn = digitalRead(BUTTON_QUANG);

  // ---- 4. NÚT LED ----
  if (digitalRead(BTN_Thanh) == LOW && lastBtnLed == HIGH && millis() - lastLedTime > debounceDelay) {
    ledState = !ledState; lastLedTime = millis();
  }
  lastBtnLed = digitalRead(BTN_Thanh);

  // ---- 5. NÚT QUẠT (Bật/Tắt cờ ghi đè) ----
  if (digitalRead(QUAT_BTN_Thanh) == LOW && lastBtnFan == HIGH && millis() - lastFanButtonTime > debounceDelay) {
    fanState = !fanState; 
    fanManualOverride = true; // Bật cờ điều khiển thủ công
    lastFanButtonTime = millis();
  }
  lastBtnFan = digitalRead(QUAT_BTN_Thanh);

  // ---- 6. NÚT SERVO (Thủ công) ----
  if (digitalRead(BUTTON_Servo_VINH) == LOW && lastServoBtn == HIGH && millis() - lastServoBtnTime > debounceDelay) {
    manualServoState = !manualServoState;
    if (manualServoState) myservo.write(140);
    else myservo.write(0);
    isDoorAutoOpen = false; // Hủy chế độ tự động
    lastServoBtnTime = millis();
  }
  lastServoBtn = digitalRead(BUTTON_Servo_VINH);

  // ---- 7. PIR & BÁO ĐỘNG ----
  detected = readPIR_Filter();
  
  digitalWrite(LOA_LED_QUANG, (systemOn && detected) ? HIGH : LOW);

  if (detected && !lastDetectedState) {
    if (systemOn) { 
        Serial.println("MOTION"); 
    }
  }
  lastDetectedState = detected;

  // ---- 8. ĐỌC CẢM BIẾN & XỬ LÝ LOGIC QUẠT TỰ ĐỘNG ----
  readDhtSensor(); // Non-blocking: Cập nhật currentTemperature và fanAutoState

  // --- LOGIC XỬ LÝ XUNG ĐỘT QUẠT (PRIORITY CONTROL) ---
  bool finalFanState;
  
  if (fanManualOverride) {
      // PRIORITY 1: Điều khiển thủ công/từ xa
      finalFanState = fanState;
      
      // Nếu trạng thái tay điều khiển trùng với trạng thái tự động, hủy ghi đè
      if (fanState == fanAutoState) {
        fanManualOverride = false;
        fanState = fanAutoState; // Đồng bộ trạng thái logic
      }
  } else {
      // PRIORITY 2: Điều khiển tự động theo nhiệt độ
      finalFanState = fanAutoState;
      fanState = fanAutoState; // Đồng bộ trạng thái logic
  }
  
  // ---- 9. OUTPUT (Thực thi LED & Quạt) ----
  digitalWrite(LED_Thanh, ledState ? HIGH : LOW);
  digitalWrite(QUAT_Thanh, finalFanState ? HIGH : LOW); // Điều khiển quạt
  
  // ---- 10. HIỂN THỊ LCD ----
  updateLCD();

  // Cho phép 50ms giữa các lần loop
  delay(50); 
}


