

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
