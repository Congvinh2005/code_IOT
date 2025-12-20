

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

// ================= CẤU HÌNH =================
#define BAUD_RATE 9600
#define SOIL_PIN A0  

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======= QUANG (PIR, Còi/LED Báo động) =======
const int PIR_IN_QUANG = 2;
const int LOA_LED_QUANG = 3;
const int BUTTON_QUANG = 4;

// ======= THANH (LED, Nút LED, Quạt, Nút Quạt) =======
const int LED_Thanh = 5;
const int BTN_Thanh = 6;
const int QUAT_Thanh = 13;
const int QUAT_BTN_Thanh = 11;

// ======= VINH (LED Mode, LED, Nút Servo, Servo) =======
const int LED_MODE_VINH = 7;
// const int LED_VINH = 9;

const int BUTTON_Servor_VINH = 8; 
const int SERVO_PIN = 10;

// ======= DHT11 =======
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================= BIẾN TRẠNG THÁI =================
bool systemOn = false;
bool detected = false;
bool lastDetectedState = false;

bool ledState = false;
bool fanState = false;
bool manualServoState = false;

// ======= DEBOUNCE =======
bool lastAlarmBtn = HIGH;
bool lastBtnLed = HIGH;
bool lastBtnFan = HIGH;
bool lastServoBtn = HIGH;

unsigned long lastButtonTime = 0;
unsigned long lastFanButtonTime = 0;
unsigned long lastServoBtnTime = 0;
const unsigned long debounceDelay = 150;

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

// ================= LCD UPDATE (ĐÃ SỬA) =================
// Thêm tham số doAmDat vào hàm
void updateLCD(float nhietDo, float doAm, int doAmDat) {
  // Tạo chuỗi hiển thị: Nhiệt độ -> Độ ẩm KK -> Độ ẩm Đất (Dat)
  lcdTemp = lcdLine1 + " T:" + String(nhietDo, 1) + (char)223 +
            "C H:" + String(doAm, 0) + "% Dat:" + String(doAmDat) + "% ";

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

// ================= XỬ LÝ LỆNH TỪ ESP32 =================
void executeCommand(String cmd) {
  cmd.trim();

  if (cmd == "ALARM_ON") systemOn = true;
  else if (cmd == "ALARM_OFF") systemOn = false;
  else if (cmd == "LED_ON") ledState = true;
  else if (cmd == "LED_OFF") ledState = false;
  else if (cmd == "FAN_ON") fanState = true;
  else if (cmd == "FAN_OFF") fanState = false;
  else if (cmd == "DOOR_OPEN") {
    myservo.write(90);
    manualServoState = true;
  }
  else if (cmd == "DOOR_CLOSE") {
    myservo.write(0);
    manualServoState = false;
  }
}

// ================= PIR FILTER =================
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

  pinMode(PIR_IN_QUANG, INPUT);
  pinMode(LOA_LED_QUANG, OUTPUT);
  pinMode(BUTTON_QUANG, INPUT_PULLUP);

  pinMode(LED_Thanh, OUTPUT);
  pinMode(BTN_Thanh, INPUT_PULLUP);
  pinMode(QUAT_Thanh, OUTPUT);
  pinMode(QUAT_BTN_Thanh, INPUT_PULLUP);

  pinMode(LED_MODE_VINH, OUTPUT);
//   pinMode(LED_VINH, OUTPUT);
  pinMode(BUTTON_Servor_VINH, INPUT_PULLUP); // Sửa tên biến

  // Setup cảm biến đất (không bắt buộc pinMode với Analog nhưng tốt cho rõ ràng)
  // pinMode(SOIL_PIN, INPUT); 

  lcd.init();
  lcd.backlight();
  lcd.clear();

  myservo.attach(SERVO_PIN);
  myservo.write(0);

  dht.begin();
  digitalWrite(QUAT_Thanh, LOW);
}

// ================= LOOP =================
void loop() {
  // ---- 1. Nhận lệnh từ ESP32 ----
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      executeCommand(incomingCommand);
      incomingCommand = "";
    } else {
      incomingCommand += c;
    }
  }

  // ---- 2. NÚT BÁO ĐỘNG ----
  if (digitalRead(BUTTON_QUANG) == LOW &&
      lastAlarmBtn == HIGH &&
      millis() - lastButtonTime > debounceDelay) {
    systemOn = !systemOn;
    lastButtonTime = millis();
  }
  lastAlarmBtn = digitalRead(BUTTON_QUANG);

  // ---- 3. NÚT LED ----
  if (digitalRead(BTN_Thanh) == LOW &&
      lastBtnLed == HIGH &&
      millis() - lastButtonTime > debounceDelay) {
    ledState = !ledState;
    lastButtonTime = millis();
  }
  lastBtnLed = digitalRead(BTN_Thanh);

  // ---- 4. NÚT QUẠT ----
  if (digitalRead(QUAT_BTN_Thanh) == LOW &&
      lastBtnFan == HIGH &&
      millis() - lastFanButtonTime > debounceDelay) {
    fanState = !fanState;
    lastFanButtonTime = millis();
  }
  lastBtnFan = digitalRead(QUAT_BTN_Thanh);
// ---- 5. NÚT SERVO THỦ CÔNG (PIN 9) ----
if (digitalRead(BUTTON_Servor_VINH) == LOW &&  lastServoBtn == HIGH &&
    millis() - lastServoBtnTime > debounceDelay) {

  manualServoState = !manualServoState;

  if (manualServoState) myservo.write(90);
  else myservo.write(0);

  lastServoBtnTime = millis();
}
lastServoBtn = digitalRead(BUTTON_Servor_VINH); 

  // ---- 6. PIR & BÁO ĐỘNG ----
  detected = readPIR_Filter();
  digitalWrite(LOA_LED_QUANG, (systemOn && detected) ? HIGH : LOW);

  if (detected && !lastDetectedState) {
    Serial.println("MOTION");
  }
  lastDetectedState = detected;

  // ---- 7. OUTPUT ----
  digitalWrite(LED_Thanh, ledState ? HIGH : LOW);
  digitalWrite(QUAT_Thanh, fanState ? HIGH : LOW);

  // ---- 8. ĐỌC CẢM BIẾN & HIỂN THỊ LCD ----
  
  // Đọc DHT11
  float doAmKK = dht.readHumidity();
  float nhietDo = dht.readTemperature();
  if (isnan(doAmKK)) doAmKK = 0;
  if (isnan(nhietDo)) nhietDo = 0;

  // Đọc Cảm biến độ ẩm đất (Code mới tích hợp)
  int soilValue = analogRead(SOIL_PIN); // 0–1023
  int soilPercent = map(soilValue, 1023, 300, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
  
  // In ra Serial (để debug nếu cần, nhưng cẩn thận không spam quá nhiều)
  // Serial.print("Do am dat: "); Serial.println(soilPercent); 

  // Cập nhật LCD với thông số mới
  updateLCD(nhietDo, doAmKK, soilPercent);

  delay(50);
}