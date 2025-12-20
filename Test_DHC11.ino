
// #include <DHT.h>

// #define DHTPIN 12      // Chân DATA nối vào D2
// #define DHTTYPE DHT11 // Khai báo loại cảm biến

// DHT dht(DHTPIN, DHTTYPE);

// void setup() {
//   Serial.begin(9600);
//   dht.begin();
//   Serial.println("Bat dau doc DHT11...");
// }

// void loop() {
//   float doAm = dht.readHumidity();      // Đọc độ ẩm
//   float nhietDo = dht.readTemperature(); // Đọc nhiệt độ (°C)

//   // Kiểm tra lỗi
//   if (isnan(doAm) || isnan(nhietDo)) {
//     Serial.println("Loi doc du lieu tu DHT11!");
//     return;
//   }

//   Serial.print("Nhiet do: ");
//   Serial.print(nhietDo);
//   Serial.print(" °C  |  Do am: ");
//   Serial.print(doAm);
//   Serial.println(" %");

//   delay(2000); // DHT11 đọc tối đa mỗi 2 giây
// }






#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

// ================= CẤU HÌNH =================
#define BAUD_RATE 9600

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======= QUANH =======
const int PIR_IN_QUANH = 2;
const int LOA_LED_QUANH = 3;
const int BUTTON_QUANH = 4;

// ======= THANH =======
const int LED_Thanh = 5;
const int BTN_Thanh = 6;

// ======= VINH =======
const int LED_MODE_VINH = 7;
const int LED_VINH = 8;
const int BUTTON_IN_OUT_VINH = 9;
const int SERVO_PIN = 10;

// ======= DHT11 =======
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================= BIẾN TRẠNG THÁI =================
bool systemOn = false;
bool detected = false;
bool ledState = false;
bool manualServoState = false;
bool modeIN = true;

// Debounce
bool lastAlarmBtn = HIGH;
bool lastBtnLed = HIGH;
bool lastButton = HIGH;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 150;

Servo myservo;
String incomingCommand = "";

// =====// ================= LCD SCROLL =================
String lcdLine1 = "   SmartHomeGroup3 - Thanh - Vinh - Quang   ";
String lcdTemp = ""; // thêm nhiệt độ + độ ẩm vào scroll
int scrollPos = 0;
unsigned long lastScrollTime = 0;
const unsigned long scrollDelay = 300;

// ================= HÀM LCD =================
void updateLCD(float nhietDo, float doAm) {
    // ----- Chuẩn bị dòng scroll -----
    lcdTemp = lcdLine1 + " T:" + String(nhietDo, 1) + (char)223 + "C H:" + String(doAm, 0) + "%   ";

    // ----- Dòng 1 scroll -----
    if (millis() - lastScrollTime > scrollDelay) {
        lcd.setCursor(0, 0);
        if (scrollPos + 16 > lcdTemp.length()) {
            lcd.print(lcdTemp.substring(scrollPos) + lcdTemp.substring(0, 16 - (lcdTemp.length() - scrollPos)));
        } else {
            lcd.print(lcdTemp.substring(scrollPos, scrollPos + 16));
        }
        scrollPos++;
        if (scrollPos >= lcdTemp.length()) scrollPos = 0;

        lastScrollTime = millis();
    }

    // ----- Dòng 2 cố định -----
    lcd.setCursor(0, 1);
    lcd.print("Led:");
    lcd.print(ledState ? "O" : "F");
    lcd.print(" CB:");
    lcd.print(systemOn ? "O" : "F");
    lcd.print(" Dr:");
    lcd.print(manualServoState ? "O" : "F");
}


// ================= XỬ LÝ LỆNH ESP32 =================
void executeCommand(String cmd) {
    if (cmd == "ALARM_ON") systemOn = true;
    else if (cmd == "ALARM_OFF") systemOn = false;
    else if (cmd == "LED_ON") ledState = true;
    else if (cmd == "LED_OFF") ledState = false;
    else if (cmd == "DOOR_OPEN") {
        myservo.write(93);
        manualServoState = true;
    }
    else if (cmd == "DOOR_CLOSE") {
        myservo.write(0);
        manualServoState = false;
    }
    else if (cmd == "MODE_IN") modeIN = true;
    else if (cmd == "MODE_OUT") modeIN = false;
    else if (cmd == "RFID_VALID") {
        digitalWrite(LED_VINH, HIGH); delay(150);
        digitalWrite(LED_VINH, LOW); delay(150);
        executeCommand("DOOR_OPEN");
    }
    else if (cmd == "RFID_INVALID") {
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_VINH, HIGH); delay(150);
            digitalWrite(LED_VINH, LOW); delay(150);
        }
    }
}

// ================= PIR FILTER =================
bool readPIR_Filter() {
    if (digitalRead(PIR_IN_QUANH) == HIGH) {
        delay(120);
        return digitalRead(PIR_IN_QUANH) == HIGH;
    }
    return false;
}

// ================= SETUP =================
void setup() {
    Wire.begin();
    Serial.begin(BAUD_RATE);

    pinMode(PIR_IN_QUANH, INPUT);
    pinMode(LOA_LED_QUANH, OUTPUT);
    pinMode(BUTTON_QUANH, INPUT_PULLUP);
    pinMode(LED_Thanh, OUTPUT);
    pinMode(BTN_Thanh, INPUT_PULLUP);
    pinMode(LED_MODE_VINH, OUTPUT);
    pinMode(LED_VINH, OUTPUT);
    pinMode(BUTTON_IN_OUT_VINH, INPUT_PULLUP);

    lcd.init();
    lcd.backlight();
    lcd.clear();

    myservo.attach(SERVO_PIN);
    myservo.write(0);

    dht.begin();

    Serial.println("Arduino System Ready");
}

// ================= LOOP =================
void loop() {
    // ---- Nhận lệnh ESP32 ----
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

    // ---- Nút cảnh báo ----
    bool alarmBtn = digitalRead(BUTTON_QUANH);
    if (alarmBtn == LOW && lastAlarmBtn == HIGH &&
        millis() - lastButtonTime > debounceDelay) {
        systemOn = !systemOn;
        lastButtonTime = millis();
    }
    lastAlarmBtn = alarmBtn;

    // ---- Nút LED ----
    bool btnLed = digitalRead(BTN_Thanh);
    if (btnLed == LOW && lastBtnLed == HIGH) ledState = !ledState;
    lastBtnLed = btnLed;

    // ---- Nút MODE ----
    bool modeBtn = digitalRead(BUTTON_IN_OUT_VINH);
    if (modeBtn == LOW && lastButton == HIGH) {
        modeIN = !modeIN;
        digitalWrite(LED_MODE_VINH, modeIN ? HIGH : LOW);
    }
    lastButton = modeBtn;

    // ---- PIR ----
    detected = readPIR_Filter();
    digitalWrite(LOA_LED_QUANH, (systemOn && detected) ? HIGH : LOW);

    // ---- Cập nhật OUTPUT ----
    digitalWrite(LED_Thanh, ledState ? HIGH : LOW);

    // ---- Đọc DHT11 ----
    float doAm = dht.readHumidity();
    float nhietDo = dht.readTemperature();
    if (isnan(doAm)) doAm = 0;
    if (isnan(nhietDo)) nhietDo = 0;

    // ---- LCD ----
    updateLCD(nhietDo, doAm);

    delay(500); // update LCD + DHT11 mỗi 0.5s
}

