
#include <Wire.h>

#include <LiquidCrystal_I2C.h>

#include <Servo.h>

#include <DHT.h>



// ================= CẤU HÌNH =================

#define BAUD_RATE 9600 // Tốc độ giao tiếp với ESP32ZZZZ



LiquidCrystal_I2C lcd(0x27, 16, 2);



// ======= QUANH (PIR, Còi/LED Báo động) =======

const int PIR_IN_QUANH = 2;

const int LOA_LED_QUANH = 3;

const int BUTTON_QUANH = 4;



// ======= THANH (LED, Nút LED, Quạt, Nút Quạt) =======

const int LED_Thanh = 5;

const int BTN_Thanh = 6;

const int QUAT_Thanh = 13;

const int QUAT_BTN_Thanh = 11;



// ======= VINH (LED Mode, LED, Nút Mode, Servo) =======

const int LED_MODE_VINH = 7;

const int LED_VINH = 8;

const int BUTTON_IN_OUT_VINH = 9;

const int SERVO_PIN = 10;



// ======= DHT11 =======

#define DHTPIN 12

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);



// ================= BIẾN TRẠNG THÁI =================

bool systemOn = false;  // Báo động

bool detected = false;  // PIR phát hiện

bool ledState = false;  // Đèn

bool fanState = false;  // Quạt

bool manualServoState = false;

bool modeIN = true;



// Debounce (Chống rung nút nhấn)

bool lastAlarmBtn = HIGH;

bool lastBtnLed = HIGH;

bool lastBtnFan = HIGH;

bool lastButton = HIGH;

unsigned long lastButtonTime = 0;

unsigned long lastFanButtonTime = 0;

const unsigned long debounceDelay = 150;



// === BIẾN CHO CỬA TỰ ĐỘNG (QUAN TRỌNG) ===

unsigned long doorOpenTime = 0;

bool isDoorAutoOpen = false;



Servo myservo;

String incomingCommand = "";



// ================= LCD SCROLL =================

String lcdLine1 = "   SmartHome-Thanh-Vinh-QA";

String lcdTemp = "";

int scrollPos = 0;

unsigned long lastScrollTime = 0;

const unsigned long scrollDelay = 300;



// ================= HÀM LCD =================

void updateLCD(float nhietDo, float doAm) {

    lcdTemp = lcdLine1 + " T:" + String(nhietDo, 1) + (char)223 + "C H:" + String(doAm, 0) + "%   ";



    // Scroll dòng 1

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



    // Hiển thị trạng thái dòng 2

    lcd.setCursor(0, 1);

    lcd.print("L:");

    lcd.print(ledState ? "O" : "F");

    lcd.print(" CB:");

    lcd.print(systemOn ? "O" : "F");

    lcd.print(" D:");

    lcd.print(manualServoState ? "O" : "F");

    // Thêm hiển thị trạng thái Quạt

    lcd.print(" F:");

    lcd.print(fanState ? "O" : "F");

}



// ================= XỬ LÝ LỆNH TỪ ESP32 =================

void executeCommand(String cmd) {

    if (cmd == "ALARM_ON") systemOn = true;

    else if (cmd == "ALARM_OFF") systemOn = false;

    else if (cmd == "LED_ON") ledState = true;

    else if (cmd == "LED_OFF") ledState = false;

    else if (cmd == "FAN_ON") fanState = true;

    else if (cmd == "FAN_OFF") fanState = false;

   

    // Mở cửa thủ công (không tự đóng)

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

   

    // === MỞ CỬA BẰNG THẺ (TỰ ĐÓNG) ===

    else if (cmd == "RFID_VALID") {

        digitalWrite(LED_VINH, HIGH); delay(100);

        digitalWrite(LED_VINH, LOW); delay(100);

       

        myservo.write(90); // Mở cửa

        manualServoState = true;

       

        // Kích hoạt bộ đếm thời gian

        isDoorAutoOpen = true;

        doorOpenTime = millis();

    }

    // === THẺ SAI ===

    else if (cmd == "RFID_INVALID") {

        for (int i = 0; i < 3; i++) {

            digitalWrite(LED_VINH, HIGH); delay(100);

            digitalWrite(LED_VINH, LOW); delay(100);

        }

    }

}



// ================= SETUP =================

void setup() {

    Wire.begin();

    Serial.begin(BAUD_RATE); // Giao tiếp ESP32



    pinMode(PIR_IN_QUANH, INPUT);

    pinMode(LOA_LED_QUANH, OUTPUT);

    pinMode(BUTTON_QUANH, INPUT_PULLUP);

   

    pinMode(LED_Thanh, OUTPUT);

    pinMode(BTN_Thanh, INPUT_PULLUP);

    pinMode(QUAT_Thanh, OUTPUT);

    pinMode(QUAT_BTN_Thanh, INPUT_PULLUP);



    pinMode(LED_MODE_VINH, OUTPUT);

    pinMode(LED_VINH, OUTPUT);

    pinMode(BUTTON_IN_OUT_VINH, INPUT_PULLUP);

   

    lcd.init();

    lcd.backlight();

    lcd.clear();



    myservo.attach(SERVO_PIN);

    myservo.write(0); // Đóng cửa ban đầu



    dht.begin();

    digitalWrite(QUAT_Thanh, LOW);

}



// ================= LOOP =================

void loop() {

    // 1. Nhận lệnh từ ESP32 (UART)

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



    // 2. Logic Tự động đóng cửa (Nằm ngoài Serial)

    if (isDoorAutoOpen && (millis() - doorOpenTime > 5000)) { // 5000ms = 5 giây

        myservo.write(0); // Đóng cửa

        manualServoState = false;

        isDoorAutoOpen = false; // Tắt bộ đếm

    }



    // 3. Xử lý nút nhấn Báo động

    bool alarmBtn = digitalRead(BUTTON_QUANH);

    if (alarmBtn == LOW && lastAlarmBtn == HIGH && millis() - lastButtonTime > debounceDelay) {

        systemOn = !systemOn;

        lastButtonTime = millis();

    }

    lastAlarmBtn = alarmBtn;



    // 4. Xử lý nút nhấn Đèn

    bool btnLed = digitalRead(BTN_Thanh);

    if (btnLed == LOW && lastBtnLed == HIGH && millis() - lastButtonTime > debounceDelay) {

        ledState = !ledState;

        lastButtonTime = millis();

    }

    lastBtnLed = btnLed;



    // 5. Xử lý nút nhấn Quạt

    bool btnFan = digitalRead(QUAT_BTN_Thanh);

    if (btnFan == LOW && lastBtnFan == HIGH && millis() - lastFanButtonTime > debounceDelay) {

        fanState = !fanState;

        lastFanButtonTime = millis();

    }

    lastBtnFan = btnFan;



    // 6. Logic hệ thống

    if(digitalRead(PIR_IN_QUANH) == HIGH) detected = true;

    else detected = false;

   

    // Nếu Bật báo động VÀ Có người -> Hú còi

    if (systemOn && detected) digitalWrite(LOA_LED_QUANH, HIGH);

    else digitalWrite(LOA_LED_QUANH, LOW);



    digitalWrite(LED_Thanh, ledState ? HIGH : LOW);

    digitalWrite(QUAT_Thanh, fanState ? HIGH : LOW);



    // 7. LCD & DHT

    float doAm = dht.readHumidity();

    float nhietDo = dht.readTemperature();

    if (isnan(doAm)) doAm = 0;

    if (isnan(nhietDo)) nhietDo = 0;

   

    updateLCD(nhietDo, doAm);



    delay(100);
}