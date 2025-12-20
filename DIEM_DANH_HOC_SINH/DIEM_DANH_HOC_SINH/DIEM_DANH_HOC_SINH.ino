// #include <SPI.h>
// #include <MFRC522.h>

// // ===== RFID =====
// #define SS_PIN 16
// #define RST_PIN 17
// MFRC522 rfid(SS_PIN, RST_PIN);

// // ===== IO =====
// #define BTN_IO 15
// #define LED_IO 4
// #define BUZZER 5
// #define ONBOARD_LED 2

// bool inOutState = 0;          // 0 = VÀO, 1 = RA
// bool btnState = HIGH;
// unsigned long lastBtnTime = 0;
// unsigned long lastReadTime = 0;

// String uidString;

// // ===== BEEP =====
// void beep(int n, int d) {
//   for (int i = 0; i < n; i++) {
//     digitalWrite(BUZZER, HIGH);
//     delay(d);
//     digitalWrite(BUZZER, LOW);
//     delay(d);
//   }
// }

// void setup() {
//   Serial.begin(115200);

//   pinMode(BTN_IO, INPUT_PULLUP);
//   pinMode(LED_IO, OUTPUT);
//   pinMode(BUZZER, OUTPUT);
//   pinMode(ONBOARD_LED, OUTPUT);

//   digitalWrite(BUZZER, LOW);
//   digitalWrite(LED_IO, LOW);
//   digitalWrite(ONBOARD_LED, LOW);

//   SPI.begin();
//   rfid.PCD_Init();

//   Serial.println();
//   Serial.println("================================");
//   Serial.println("        RFID TEST MODE           ");
//   Serial.println("================================");
//   Serial.println("Quet the RFID de hien UID");
//   Serial.println("Nhan nut de doi trang thai VAO/RA");
//   Serial.println("================================");
// }

// void loop() {
//   // ===== NUT BAM =====
//   if (digitalRead(BTN_IO) == LOW) {
//     if (btnState == HIGH && millis() - lastBtnTime > 300) {
//       inOutState = !inOutState;
//       digitalWrite(LED_IO, inOutState);

//       Serial.println();
//       Serial.println(">>> DOI TRANG THAI <<<");
//       Serial.print("TRANG THAI HIEN TAI: ");
//       Serial.println(inOutState == 0 ? "VÀO" : "RA");

//       beep(2, 100);
//       lastBtnTime = millis();
//       btnState = LOW;
//     }
//   } else {
//     btnState = HIGH;
//   }

//   // ===== DOC RFID =====
//   if (millis() - lastReadTime > 300) {
//     readUID();
//     lastReadTime = millis();
//   }
// }

// void readUID() {
//   if (!rfid.PICC_IsNewCardPresent()) return;
//   if (!rfid.PICC_ReadCardSerial()) return;

//   uidString = "";
//   for (byte i = 0; i < rfid.uid.size; i++) {
//     if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
//     uidString += String(rfid.uid.uidByte[i], HEX);
//   }
//   uidString.toUpperCase();

//   Serial.println();
//   Serial.println("================================");
//   Serial.println("        THE RFID DUOC QUET       ");
//   Serial.println("================================");
//   Serial.print("UID       : ");
//   Serial.println(uidString);
//   Serial.print("TRANG THAI: ");
//   Serial.println(inOutState == 0 ? "VÀO" : "RA");
//   Serial.println("================================");

//   digitalWrite(ONBOARD_LED, HIGH);
//   beep(1, 200);
//   delay(100);
//   digitalWrite(ONBOARD_LED, LOW);

//   rfid.PICC_HaltA();
//   rfid.PCD_StopCrypto1();
// }





// =============================================================



// #include <SPI.h>
// #include <MFRC522.h>
// #define SS_PIN 16
// #define RST_PIN 17
// MFRC522 rfid(SS_PIN, RST_PIN);
// String uidString;

// #include "WiFi.h"
// #include <HTTPClient.h>
// const char* ssid = "Long 2.4G 3"; 
// const char* password = "123456789";
// // Google script Web_App_URL.
// String Web_App_URL = "https://script.google.com/macros/s/AKfycbwtEVcFEvqSPvasEReOtsofOXwUg7YUCI-qi6xQ0z83BY77H9Z-Xd--f1Zx8GWtIthDeg/exec";
// #define On_Board_LED_PIN 2
// #define MAX_STUDENTS 10 // Số lượng học sinh tối đa
// struct Student {
//   String id;
//   char code[10];
//   char name[30];
// };
// Student students[MAX_STUDENTS];
// int studentCount = 0;
// int runMode=2;
// const int btnIO = 15;
// bool btnIOState = HIGH;
// unsigned long timeDelay=millis();
// unsigned long timeDelay2=millis();
// bool InOutState=0; //0: vào, 1:ra
// const int ledIO = 4;
// const int buzzer = 5;

// void setup() {
//   Serial.begin(115200);
//   pinMode(buzzer,OUTPUT);
//   digitalWrite(buzzer,LOW);
//   pinMode(btnIO,INPUT_PULLUP);
//   pinMode(ledIO,OUTPUT);
//   pinMode(On_Board_LED_PIN,OUTPUT);
//   Serial.println();
//   Serial.println("-------------");
//   Serial.println("WIFI mode : STA");
//   Serial.println("-------------");
//   Serial.print("Connecting to ");
//   Serial.println(ssid);
//   WiFi.begin(ssid, password);
//   while(WiFi.status() != WL_CONNECTED) {
//     Serial.print(".");
//     delay(1000);
//   }
//   if(!readDataSheet()) Serial.println("Can't read data from google sheet!");

//   SPI.begin(); // Init SPI bus
//   rfid.PCD_Init(); // Init MFRC522
// }

// void loop() {
//   if(millis()-timeDelay2>500){
//     readUID();
//     timeDelay2=millis();
//   }
//   if(digitalRead(btnIO)==LOW){
//     if(btnIOState==HIGH){
//       if(millis()-timeDelay>500){
//         //Thực hiện lệnh
//         InOutState = !InOutState;
//         digitalWrite(ledIO,InOutState);
//         timeDelay=millis();
//       }
//       btnIOState=LOW;
//     }
//   }else{
//     btnIOState=HIGH;
//   }
// }
// void beep(int n, int d){
//   for(int i=0;i<n;i++){
//     digitalWrite(buzzer,HIGH);
//     delay(d);
//     digitalWrite(buzzer,LOW);
//     delay(d);
//   }
// }
// void readUID(){
//   // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
//   MFRC522::MIFARE_Key key;
//   for (byte i = 0; i < 6; i++) {
//     key.keyByte[i] = 0xFF;
//   }
//   // Look for new cards
//   if ( ! rfid.PICC_IsNewCardPresent()) {
//     return;
//   }

//   // Select one of the cards
//   if ( ! rfid.PICC_ReadCardSerial()) {
//     return;
//   }
//   // Now a card is selected. The UID and SAK is in mfrc522.uid.

//   uidString="";
//   for (byte i = 0; i < rfid.uid.size; i++) {
//     uidString.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
//     uidString.concat(String(rfid.uid.uidByte[i], HEX));
//   }
//   uidString.toUpperCase();
//   Serial.println("Card UID: "+uidString);
//   beep(1,200);
//   if(runMode==1)  writeUIDSheet();
//   else if(runMode==2) writeLogSheet();
//   // Dump PICC type
//   byte piccType = rfid.PICC_GetType(rfid.uid.sak);
//   //    Serial.print("PICC type: ");
//   //Serial.println(mfrc522.PICC_GetTypeName(piccType));
//   if (        piccType != MFRC522::PICC_TYPE_MIFARE_MINI
//               &&        piccType != MFRC522::PICC_TYPE_MIFARE_1K
//               &&        piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
//     //Serial.println("This sample only works with MIFARE Classic cards.");
//     return;
//   }
// }
// bool readDataSheet(){
//   if (WiFi.status() == WL_CONNECTED) {
//     digitalWrite(On_Board_LED_PIN, HIGH);

//     // Create a URL for reading or getting data from Google Sheets.
//     String Read_Data_URL = Web_App_URL + "?sts=read";

//     Serial.println();
//     Serial.println("-------------");
//     Serial.println("Read data from Google Spreadsheet...");
//     Serial.print("URL : ");
//     Serial.println(Read_Data_URL);

//     //::::::::::::::::::The process of reading or getting data from Google Sheets.
//       // Initialize HTTPClient as "http".
//       HTTPClient http;

//       // HTTP GET Request.
//       http.begin(Read_Data_URL.c_str());
//       http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

//       // Gets the HTTP status code.
//       int httpCode = http.GET(); 
//       Serial.print("HTTP Status Code : ");
//       Serial.println(httpCode);
  
//       // Getting response from google sheet.
//       String payload;
//       studentCount=0;
//       if (httpCode > 0) {
//         payload = http.getString();
//         Serial.println("Payload : " + payload); 

//          // Tách dữ liệu
//           char charArray[payload.length() + 1];
//           payload.toCharArray(charArray, payload.length() + 1);
//           // Đếm số phần tử
//           int numberOfElements = countElements(charArray, ',');
//           Serial.println("Number of elements: "+String(numberOfElements));
//           char *token = strtok(charArray, ",");
//           while (token != NULL && studentCount < numberOfElements/3) {
//             students[studentCount].id = atoi(token); // Chuyển đổi ID từ chuỗi sang số nguyên
//             token = strtok(NULL, ",");
//             strcpy(students[studentCount].code, token); // Sao chép mã học sinh
//             token = strtok(NULL, ",");
//             strcpy(students[studentCount].name, token); // Sao chép tên học sinh
            
//             studentCount++;
//             token = strtok(NULL, ",");
//           }
          
//           // In ra danh sách học sinh
//           for (int i = 0; i < studentCount; i++) {
//             Serial.print("ID: ");
//             Serial.println(students[i].id);
//             Serial.print("Code: ");
//             Serial.println(students[i].code);
//             Serial.print("Name: ");
//             Serial.println(students[i].name);
//           }
//       }
  
//       http.end();
//     //::::::::::::::::::
    
//     digitalWrite(On_Board_LED_PIN, LOW);
//     Serial.println("-------------");
//     if(studentCount>0) return true;
//     else return false;
//   }
// }
// void writeUIDSheet(){
//   String Send_Data_URL = Web_App_URL + "?sts=writeuid";
//   Send_Data_URL += "&uid=" + uidString;
//   Serial.println();
//   Serial.println("-------------");
//   Serial.println("Send data to Google Spreadsheet...");
//   Serial.print("URL : ");
//   Serial.println(Send_Data_URL);
//   // Initialize HTTPClient as "http".
//   HTTPClient http;

//   // HTTP GET Request.
//   http.begin(Send_Data_URL.c_str());
//   http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

//   // Gets the HTTP status code.
//   int httpCode = http.GET(); 
//   Serial.print("HTTP Status Code : ");
//   Serial.println(httpCode);

//   // Getting response from google sheets.
//   String payload;
//   if (httpCode > 0) {
//     payload = http.getString();
//     Serial.println("Payload : " + payload);    
//   }
  
//   http.end();
// }
// void writeLogSheet(){
//   char charArray[uidString.length() + 1];
//   uidString.toCharArray(charArray, uidString.length() + 1);
//   char* studentName = getStudentNameById(charArray);
//   if (studentName != nullptr) {
//     Serial.print("Tên học sinh với ID ");
//     Serial.print(uidString);
//     Serial.print(" là: ");
//     Serial.println(studentName);

//     String Send_Data_URL = Web_App_URL + "?sts=writelog";
//     Send_Data_URL += "&uid=" + uidString;
//     Send_Data_URL += "&name=" + urlencode(String(studentName));
//     if(InOutState==0){
//       Send_Data_URL += "&inout="+urlencode("VÀO");
//     }else{
//       Send_Data_URL += "&inout="+urlencode("RA");
//     }


//     Serial.println();
//     Serial.println("-------------");
//     Serial.println("Send data to Google Spreadsheet...");
//     Serial.print("URL : ");
//     Serial.println(Send_Data_URL);
//     // Initialize HTTPClient as "http".
//     HTTPClient http;

//     // HTTP GET Request.
//     http.begin(Send_Data_URL.c_str());
//     http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

//     // Gets the HTTP status code.
//     int httpCode = http.GET(); 
//     Serial.print("HTTP Status Code : ");
//     Serial.println(httpCode);

//     // Getting response from google sheets.
//     String payload;
//     if (httpCode > 0) {
//       payload = http.getString();
//       Serial.println("Payload : " + payload);    
//     }
    
//     http.end();
//   } else {
//     Serial.print("Không tìm thấy học sinh với ID ");
//     Serial.println(uidString);
//     beep(3,500);
//   }
// }
// String urlencode(String str) {
//   String encodedString = "";
//   char c;
//   char code0;
//   char code1;
//   char code2;
//   for (int i = 0; i < str.length(); i++) {
//     c = str.charAt(i);
//     if (c == ' ') {
//       encodedString += '+';
//     } else if (isalnum(c)) {
//       encodedString += c;
//     } else {
//       code1 = (c & 0xf) + '0';
//       if ((c & 0xf) > 9) {
//         code1 = (c & 0xf) - 10 + 'A';
//       }
//       c = (c >> 4) & 0xf;
//       code0 = c + '0';
//       if (c > 9) {
//         code0 = c - 10 + 'A';
//       }
//       code2 = '\0';
//       encodedString += '%';
//       encodedString += code0;
//       encodedString += code1;
//     }
//     yield();
//   }
//   return encodedString;
// }
// char* getStudentNameById(char* uid) {
//   for (int i = 0; i < studentCount; i++) {
//     if (strcmp(students[i].code, uid) == 0) {
//       return students[i].name;
//     }
//   }
//   return nullptr; // Trả về nullptr nếu không tìm thấy
// }
// int countElements(const char* data, char delimiter) {
//   // Tạo một bản sao của chuỗi dữ liệu để tránh thay đổi chuỗi gốc
//   char dataCopy[strlen(data) + 1];
//   strcpy(dataCopy, data);
  
//   int count = 0;
//   char* token = strtok(dataCopy, &delimiter);
//   while (token != NULL) {
//     count++;
//     token = strtok(NULL, &delimiter);
//   }
//   return count;
// }




// =============================================================
// =============== ESP32: CONTROLLER (Wi-Fi, RFID) =============
// =============================================================

#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include <HTTPClient.h>

// ======================= CẤU HÌNH SERIAL (Giao tiếp với Arduino) =========================
#define RXD2 16 // Nối với TX (D1) của Arduino UNO
#define TXD2 17 // Nối với RX (D0) của Arduino UNO
#define BAUD_RATE 9600

// ======================= CẤU HÌNH WIFI & GOOGLE SHEET =========================
const char* ssid = "Long 2.4G 3"; // Thay bằng tên WiFi của bạn
const char* password = "123456789"; // Thay bằng mật khẩu WiFi của bạn
String Web_App_URL = "https://script.google.com/macros/s/AKfycbwtEVcFEvqSPvasEReOtsofOXwUg7YUCI-qi6xQ0z83BY77H9Z-Xd--f1Zx3GWtIthDeg/exec";

#define MAX_STUDENTS 10
struct Student {
    String id;
    char code[10];
    char name[30];
};
Student students[MAX_STUDENTS];
int studentCount = 0;
String uidString = "";

// ======================= CẤU HÌNH RFID MFRC522 =======================
#define SS_PIN  32 // Chân Slave Select (CS)
#define RST_PIN 33 // Chân Reset
MFRC522 rfid(SS_PIN, RST_PIN);

// Buzzer (GPIO 2 - LED onboard)
const int buzzer = 2; 

// Biến lưu trữ trạng thái I/O của Arduino (Được cập nhật qua Serial)
bool systemOn = false;
bool ledState = false;
bool modeIN = true; // Rất quan trọng cho logic VÀO/RA
bool detected = false;
String incomingSerial2 = "";

unsigned long lastStatusRequest = 0;
const unsigned long statusRequestInterval = 500; // Yêu cầu trạng thái mỗi 500ms

// ======================= HÀM GIAO TIẾP VÀ XỬ LÝ DỮ LIỆU =========================

void sendCommandToArduino(String cmd) {
    Serial2.println(cmd);
}

void beep(int n, int d){
    for(int i=0;i<n;i++){
        digitalWrite(buzzer,HIGH);
        delay(d);
        digitalWrite(buzzer,LOW);
        delay(d);
    }
}

void parseStatus(String statusData) {
    // FORMAT: 1|0|1|0 (systemOn|ledState|modeIN|detected)
    int p1 = statusData.indexOf('|');
    int p2 = statusData.indexOf('|', p1 + 1);
    int p3 = statusData.indexOf('|', p2 + 1);

    systemOn = (statusData.substring(0, p1) == "1");
    ledState = (statusData.substring(p1 + 1, p2) == "1");
    modeIN   = (statusData.substring(p2 + 1, p3) == "1");
    detected = (statusData.substring(p3 + 1) == "1");
}

void handleArduinoData(String data) {
    if (data.startsWith("STATUS:")) {
        parseStatus(data.substring(7));
    }
}

char* getStudentNameById(char* uid) {
    for (int i = 0; i < studentCount; i++) {
        if (strcmp(students[i].code, uid) == 0) {
            return students[i].name;
        }
    }
    return nullptr;
}

// ======================= HÀM GOOGLE SHEET =========================

// (Các hàm readDataSheet, urlencode, countElements được đặt ở cuối)

void writeLogSheet(char* studentName) {
    String nameForURL = urlencode(String(studentName));
    
    String Send_Data_URL = Web_App_URL + "?sts=writelog";
    Send_Data_URL += "&uid=" + uidString;
    Send_Data_URL += "&name=" + nameForURL;
    
    // Sử dụng trạng thái modeIN đã được đồng bộ từ Arduino UNO
    Send_Data_URL += "&inout=" + urlencode(modeIN ? "VÀO" : "RA");

    HTTPClient http;
    http.begin(Send_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET(); 
    
    if (httpCode > 0) {
      Serial.println("GSHEET Log Success.");    
    } else {
      Serial.println("GSHEET Log Failed.");
    }
    
    http.end();
}

// ======================= HÀM LOGIC RFID CHÍNH =========================

void readUID() {
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    if ( ! rfid.PICC_IsNewCardPresent() || ! rfid.PICC_ReadCardSerial()) {
        return;
    }

    // Đọc UID thẻ
    uidString = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        uidString.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
        uidString.concat(String(rfid.uid.uidByte[i], HEX));
    }
    uidString.toUpperCase();
    Serial.println("Card UID: " + uidString);

    // Tìm tên học sinh
    char charArray[uidString.length() + 1];
    uidString.toCharArray(charArray, uidString.length() + 1);
    char* studentName = getStudentNameById(charArray);

    bool isValidTag = (studentName != nullptr);

    // ==================== ÁP DỤNG LOGIC THEO MODE ====================

    if (modeIN) { // CHẾ ĐỘ VÀO: Cần xác thực
        if (isValidTag) {
            // Hợp lệ: Mở cổng và ghi log
            sendCommandToArduino("RFID_VALID"); 
            writeLogSheet(studentName); 
        } else {
            // Không hợp lệ: Không mở cổng, báo lỗi
            sendCommandToArduino("RFID_INVALID");
            beep(3, 300); 
        }
    } else { // CHẾ ĐỘ RA: Không cần xác thực, chỉ cần nhận thẻ để mở
        
        // Luôn luôn mở cổng ở chế độ RA
        sendCommandToArduino("RFID_VALID"); 
        
        // Ghi log (sử dụng tên nếu có, nếu không thì ghi UID/Unknown)
        if (isValidTag) {
            writeLogSheet(studentName); 
        } else {
            // Ghi log UID không xác định (tùy chọn: chỉ ghi UID trong Google Sheet)
            writeLogSheet("Unknown Tag"); 
        }
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}

// ======================= SETUP VÀ LOOP =========================

void setup() {
    Serial.begin(115200);
    Serial2.begin(BAUD_RATE, SERIAL_8N1, RXD2, TXD2); 

    pinMode(buzzer, OUTPUT);
    digitalWrite(buzzer, LOW);

    // Khởi tạo WIFI
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi connected!");

    // Đọc danh sách UID hợp lệ từ Google Sheet
    if(!readDataSheet()) Serial.println("Can't read data from google sheet!");

    // Khởi tạo RFID MFRC522
    SPI.begin();
    rfid.PCD_Init();
    Serial.println("ESP32 System Ready - Gateway Active.");
}

void loop() {
    // 1) ĐỌC VÀ XỬ LÝ DỮ LIỆU TỪ ARDUINO UNO
    while (Serial2.available()) {
        char c = Serial2.read();
        if (c == '\n') {
            incomingSerial2.trim();
            if (incomingSerial2.length() > 0) {
                handleArduinoData(incomingSerial2);
                incomingSerial2 = "";
            }
        } else incomingSerial2 += c;
    }

    // 2) ĐỊNH KỲ YÊU CẦU TRẠNG THÁI TỪ ARDUINO UNO (Để đồng bộ modeIN)
    if (millis() - lastStatusRequest >= statusRequestInterval) {
        sendCommandToArduino("STATUS_REQ"); 
        lastStatusRequest = millis();
    }
    
    // 3) ĐỌC RFID VÀ XỬ LÝ LOGIC CHÍNH
    readUID(); 
}

// ======================= CÁC HÀM GOOGLE SHEET ĐẦY ĐỦ (Cần đặt sau loop()) =========================

// CÁC HÀM readDataSheet, urlencode, countElements, v.v. CẦN ĐƯỢC THÊM VÀO ĐẦY ĐỦ Ở ĐÂY
