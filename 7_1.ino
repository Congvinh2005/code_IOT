
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
    myservo.write(140);
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

    myservo.write(140); // Mở cửa
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
    if (manualServoState) myservo.write(140);
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