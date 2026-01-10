#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

// ================= CẤU HÌNH =================
#define BAUD_RATE 9600
 
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// ======= QUANG (PIR, Còi/LED Báo động) =======
const int PIR_IN_QUANG = 2;
const int LOA_LED_QUANG = 3;
const int BUTTON_QUANG = 4; 

// ======= THANH (LED, Nút LED, Quạt, Nút Quạt) =======
const int LED_Thanh = 5;      // Đèn LED chính
const int BTN_Thanh = 6;      // Nút điều khiển LED thủ công
const int LDR_PIN = A0;       // <<< CHÂN MỚI: Cảm biến ánh sáng LDR
const int QUAT_Thanh = 13; 
const int QUAT_BTN_Thanh = 11;

// ======= VINH (TRIG_SIEU_AM,ECHO_SIEU_AM, Nút Servo, Servo) =======
const int TRIG_SIEU_AM = 7;
const int ECHO_SIEU_AM = 9; 
const int BUTTON_Servo_VINH = 8; 
const int SERVO_PIN = 10;
const int LED_VINH = 1; 

// ======= DHT11 =======
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================= BIẾN TRẠNG THÁI =================
bool systemOn = false; 
bool detected = false;
bool lastDetectedState = false;

// --- LOGIC LED ---
bool ledState = false; // Trạng thái LED thủ công/từ xa
bool ledAutoState = false; // Trạng thái LED tự động (LDR)
bool ledManualOverride = false; // Cờ ghi đè LED (Manual > Auto)
const int LDR_THRESHOLD = 500; // Ngưỡng tối (tối: < 500)

// --- LOGIC QUẠT ---
bool fanState = false; 
bool fanAutoState = false; 
bool fanManualOverride = false; 
const float TEMP_THRESHOLD = 25.0; // Đổi ngưỡng nhiệt độ hợp lý hơn

bool manualServoState = false;

// ======= NON-BLOCKING TIMERS & DEBOUNCE STATES (Giữ nguyên) =======
const unsigned long debounceDelay = 150;
const unsigned long AUTO_CLOSE_DELAY = 5000; 
const unsigned long distanceInterval = 500; 
const unsigned long dhtReadInterval = 2000; 
const unsigned long ldrReadInterval = 500; // Đọc LDR 500ms 1 lần
const unsigned long scrollDelay = 300;

unsigned long lastAlarmTime = 0;
unsigned long lastLedTime = 0;
unsigned long lastFanButtonTime = 0;
unsigned long lastServoBtnTime = 0;
unsigned long doorOpenTime = 0;
unsigned long lastDistanceTime = 0;
unsigned long lastDhtReadTime = 0;
unsigned long lastLdrReadTime = 0; // Timer mới
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
  // Logic giữ nguyên
  digitalWrite(TRIG_SIEU_AM, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_SIEU_AM, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_SIEU_AM, LOW);

  long duration = pulseIn(ECHO_SIEU_AM, HIGH);
  float distanceCm = duration * 0.0343 / 2; 

  return distanceCm;
}

// ================= HÀM ĐỌC LDR (NON-BLOCKING) =================
void readLDRSensor() {
    if (millis() - lastLdrReadTime >= ldrReadInterval) {
        int ldrValue = analogRead(LDR_PIN);
        
        // Nếu giá trị LDR thấp (Tối), bật đèn
        if (ldrValue < LDR_THRESHOLD) {
            ledAutoState = true; 
        } else {
            ledAutoState = false;
        }

        Serial.print("LDR Value: ");
        Serial.println(ldrValue);
        
        lastLdrReadTime = millis();
    }
}


// ================= HÀM ĐỌC DHT (NON-BLOCKING) =================
void readDhtSensor() {
    if (millis() - lastDhtReadTime >= dhtReadInterval) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        if (!isnan(h)) currentHumidity = h;
        if (!isnan(t)) currentTemperature = t;

        if (currentTemperature > TEMP_THRESHOLD) {
            fanAutoState = true;
        } else if (currentTemperature <= TEMP_THRESHOLD) {
            fanAutoState = false;
        }
        
        lastDhtReadTime = millis();
    }
}


// ================= HÀM CẬP NHẬT LCD =================
void updateLCD() {
  // Logic giữ nguyên
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

  lcd.setCursor(0, 1);
  lcd.print("CB:"); lcd.print(systemOn ? "O" : "F"); 
  lcd.print(" Q:"); lcd.print(digitalRead(QUAT_Thanh) ? "O" : "F");
  // Hiển thị trạng thái VẬT LÝ của LED
  lcd.print(" L:"); lcd.print(digitalRead(LED_Thanh) ? "O" : "F");
  lcd.print(" D:"); lcd.print(manualServoState ? "O" : "F"); 
}

// ================= XỬ LÝ LỆNH TỪ ESP32 (UART) =================
void executeCommand(String cmd) {
  cmd.trim();

  // --- PIR, QUẠT, SERVO (Logic giữ nguyên) ---
  if (cmd == "ALARM_ON") systemOn = true;
  else if (cmd == "ALARM_OFF") systemOn = false;

  // === LỆNH LED TỪ XA: BẬT CỜ GHI ĐÈ LED ===
  else if (cmd == "LED_ON") {
    ledState = true;
    ledManualOverride = true; // Bật cờ điều khiển thủ công LED
  }
  else if (cmd == "LED_OFF") {
    ledState = false;
    ledManualOverride = true; // Bật cờ điều khiển thủ công LED
  }
  
  // === LỆNH QUẠT TỪ XA: BẬT CỜ GHI ĐÈ QUẠT ===
  else if (cmd == "FAN_ON") {
      fanState = true;
      fanManualOverride = true; 
  }
  else if (cmd == "FAN_OFF") {
      fanState = false;
      fanManualOverride = true; 
  }
  
  // === SERVO & RFID (Logic giữ nguyên) ===
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
    // ...
  }
}

// ================= HÀM LỌC TÍN HIỆU PIR =================
bool readPIR_Filter() {
  if (digitalRead(PIR_IN_QUANG) == HIGH) {
    delay(120); 
    return digitalRead(PIR_IN_QUANG) == HIGH;
  }
  return false;
}

// ================= SETUP =================
void setup() {
  Wire.begin();
  Serial.begin(BAUD_RATE); 
  
  Serial.println("Arduino Ready! (With LDR/Light Auto)"); 

  // --- Pin Modes ---
  pinMode(PIR_IN_QUANG, INPUT);
  pinMode(LOA_LED_QUANG, OUTPUT);
  pinMode(BUTTON_QUANG, INPUT_PULLUP);

  pinMode(LED_Thanh, OUTPUT);
  pinMode(BTN_Thanh, INPUT_PULLUP);
  pinMode(QUAT_Thanh, OUTPUT);
  pinMode(QUAT_BTN_Thanh, INPUT_PULLUP);
  
  // CHÂN A0 KHÔNG CẦN KHAI BÁO PIN MODE (MẶC ĐỊNH LÀ INPUT)

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
  // Logic giữ nguyên
  if (isDoorAutoOpen) {
      if (millis() - lastDistanceTime >= distanceInterval) {
          float distance = readDistance();
          lastDistanceTime = millis();
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


  // ---- 3. NÚT BÁO ĐỘNG (Bật/Tắt systemOn) ----
  if (digitalRead(BUTTON_QUANG) == LOW && lastAlarmBtn == HIGH && millis() - lastAlarmTime > debounceDelay) {
    systemOn = !systemOn; 
    Serial.println(systemOn ? "ALARM_BUTTON_ON" : "ALARM_BUTTON_OFF"); 
    lastAlarmTime = millis();
  }
  lastAlarmBtn = digitalRead(BUTTON_QUANG);

  // ---- 4. NÚT LED (Bật/Tắt cờ ghi đè LED) ----
  if (digitalRead(BTN_Thanh) == LOW && lastBtnLed == HIGH && millis() - lastLedTime > debounceDelay) {
    ledState = !ledState; 
    ledManualOverride = true; // Bật cờ điều khiển thủ công
    lastLedTime = millis();
  }
  lastBtnLed = digitalRead(BTN_Thanh);

  // ---- 5. NÚT QUẠT (Bật/Tắt cờ ghi đè Quạt) ----
  if (digitalRead(QUAT_BTN_Thanh) == LOW && lastBtnFan == HIGH && millis() - lastFanButtonTime > debounceDelay) {
    fanState = !fanState; 
    fanManualOverride = true; 
    lastFanButtonTime = millis();
  }
  lastBtnFan = digitalRead(QUAT_BTN_Thanh);

  // ---- 6. NÚT SERVO (Thủ công) ----
  if (digitalRead(BUTTON_Servo_VINH) == LOW && lastServoBtn == HIGH && millis() - lastServoBtnTime > debounceDelay) {
    manualServoState = !manualServoState;
    if (manualServoState) myservo.write(140);
    else myservo.write(0);
    isDoorAutoOpen = false; 
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

  // ---- 8. ĐỌC CẢM BIẾN & XỬ LÝ LOGIC TỰ ĐỘNG ----
  readDhtSensor(); // Quạt Auto
  readLDRSensor(); // LED Auto

  // --- LOGIC XỬ LÝ XUNG ĐỘT LED (PRIORITY CONTROL) ---
  bool finalLedState;
  
  if (ledManualOverride) {
      finalLedState = ledState;
      // Nếu trạng thái tay điều khiển trùng với trạng thái tự động, hủy ghi đè
      if (ledState == ledAutoState) {
        ledManualOverride = false;
        ledState = ledAutoState; 
      }
  } else {
      finalLedState = ledAutoState;
      ledState = ledAutoState;
  }
  
  // --- LOGIC XỬ LÝ XUNG ĐỘT QUẠT (PRIORITY CONTROL) ---
  bool finalFanState;
  
  if (fanManualOverride) {
      finalFanState = fanState;
      if (fanState == fanAutoState) {
        fanManualOverride = false;
        fanState = fanAutoState; 
      }
  } else {
      finalFanState = fanAutoState;
      fanState = fanAutoState; 
  }
  
  // ---- 9. OUTPUT (Thực thi LED & Quạt) ----
  digitalWrite(LED_Thanh, finalLedState ? HIGH : LOW); // <<< ĐIỀU KHIỂN LED
  digitalWrite(QUAT_Thanh, finalFanState ? HIGH : LOW); 
  
  // ---- 10. HIỂN THỊ LCD ----
  updateLCD();

  delay(50); 
}