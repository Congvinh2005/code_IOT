// =============================================================
// =============== SMART HOME + PIR + BLUETOOTH ================
// =============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =============================================================
// =============== NHÓM CHÂN THEO TÊN THÀNH VIÊN ===============
// =============================================================

// ======= CHÂN CỦA QUANH =======
const int PIR_IN_QUANH = 8;      // PIR
const int LOA_LED_QUANH = 9;     // LED + LOA chung
const int BUTTON_QUANH = 7;      // Nút bật/tắt cảnh báo

// ======= CHÂN CỦA THANH (LED BLUETOOTH) =======
const int LED_Thanh = 11;
const int BTN_Thanh = 10;

// ======= CHÂN CỦA VINH (RFID + SERVO) =======
const int LED_MODE_VINH = 5;
const int LED_VINH = 12;               // LED Beep RFID
const int BUTTON_IN_OUT_VINH = 6;      // Đổi IN/OUT
const int BUTTON_SERVOR_THU_CONG = 4;  // Servo thủ công

// ======= BIẾN TRẠNG THÁI =======
bool systemOn = false;
bool detected = false;
bool ledState = false;

// Nút PIR
bool lastAlarmBtn = HIGH;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 150;

// Nút LED Bluetooth
bool lastBtnLed = HIGH;

// RFID
bool modeIN = true;
bool lastButton = HIGH;

bool printedInMsg = false;
bool printedOutMsg = false;

// Servo
Servo myservo;
bool manualServoState = false;
bool lastManualBtn = HIGH;

// Buffer Bluetooth/RFID
String cmd = "";

// =============================================================
// ======================= HÀM PHỤ ==============================
// =============================================================

// Lọc nhiễu PIR
bool readPIR_Filter() {
    if (digitalRead(PIR_IN_QUANH) == HIGH) {
        delay(120);
        return (digitalRead(PIR_IN_QUANH) == HIGH);
    }
    return false;
}

// Beep LED cho RFID
void beepRFID(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_VINH, HIGH);
        delay(150);
        digitalWrite(LED_VINH, LOW);
        delay(150);
    }
}

// Servo mở cổng
void openGate() {
    Serial.println("Gate Opening...");
    myservo.write(93);
    delay(2000);
    myservo.write(0);

    if (manualServoState)
        myservo.write(93);
}

// =============================================================
// ============================== SETUP =========================
// =============================================================

void setup() {
    Serial.begin(9600);

    // ======= QUANH =======
    pinMode(PIR_IN_QUANH, INPUT);
    pinMode(LOA_LED_QUANH, OUTPUT);
    pinMode(BUTTON_QUANH, INPUT_PULLUP);

    // ======= THANH =======
    pinMode(LED_Thanh, OUTPUT);
    pinMode(BTN_Thanh, INPUT_PULLUP);

    // ======= LCD =======
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Smart Home Group 3");

    // ======= SERVO =======
    myservo.attach(13);
    myservo.write(0);

    // ======= VINH =======
    pinMode(LED_MODE_VINH, OUTPUT);
    pinMode(LED_VINH, OUTPUT);
    pinMode(BUTTON_IN_OUT_VINH, INPUT_PULLUP);
    pinMode(BUTTON_SERVOR_THU_CONG, INPUT_PULLUP);

    Serial.println("System Ready...");
}

// =============================================================
// ============================== LOOP ==========================
// =============================================================

void loop() {

    // =============================================================
    // 1️⃣ ĐỌC LỆNH BLUETOOTH + RFID (CHUNG SERIAL)
    // =============================================================
    while (Serial.available()) {
    char c = Serial.read();

    // Nếu chưa gặp ENTER thì tiếp tục ghép chuỗi
    if (c != '\n' && c != '\r') {
        cmd += c;
    }
    else {
        // Khi gặp ENTER → xử lý toàn chuỗi
        cmd.trim();

        if (cmd.length() > 0) {

            // ====== LỆNH BLUETOOTH ======
            if (cmd.startsWith("BT:")) {
                cmd.remove(0, 3);

                if (cmd == "bat led") ledState = true;
                if (cmd == "tat led") ledState = false;

                if (cmd == "bat cua") {
                    manualServoState = true;
                    myservo.write(93);
                }

                if (cmd == "tat cua") {
                    manualServoState = false;
                    myservo.write(0);
                }

                if (cmd == "bat canh bao") systemOn = true;
                if (cmd == "tat canh bao") systemOn = false;
            }

            // ====== RFID ======
            else {
                String id = cmd;

                if (modeIN) {
                    Serial.print("IN - TAG: ");
                    Serial.println(id);

                    if (id == "AB123456789A" || id == "123456" || id == "HELLO1") {
                        Serial.println("VALID TAG");
                        beepRFID(1);
                        openGate();
                    } else {
                        Serial.println("INVALID TAG");
                        beepRFID(3);
                    }
                }
                else {
                    Serial.print("OUT - TAG: ");
                    Serial.println(id);
                    openGate();
                }
            }
        }

        cmd = "";  // reset cho lần nhập tiếp theo
    }
}

    // =============================================================
    // 2️⃣ NÚT BẬT/TẮT CẢNH BÁO PIR
    // =============================================================
    bool alarmBtn = digitalRead(BUTTON_QUANH);

    if (alarmBtn == LOW && lastAlarmBtn == HIGH) {
        if (millis() - lastButtonTime > debounceDelay) {
            systemOn = !systemOn;
            lastButtonTime = millis();
        }
    }
    lastAlarmBtn = alarmBtn;

    // PIR
    detected = readPIR_Filter();

    // LED+LOA chung
    digitalWrite(LOA_LED_QUANH, (systemOn && detected) ? HIGH : LOW);

    // LCD
    lcd.setCursor(0, 1);
    lcd.print(ledState ? "LED:ON " : "LED:OFF");

    lcd.setCursor(7, 1);
    lcd.print(systemOn ? "CB:ON " : "CB:OFF");

    // =============================================================
    // 3️⃣ NÚT LED BLUETOOTH (THANH)
    // =============================================================
    bool currentBtn = digitalRead(BTN_Thanh);
    if (currentBtn == LOW && lastBtnLed == HIGH) {
        ledState = !ledState;
        delay(150);
    }
    lastBtnLed = currentBtn;

    digitalWrite(LED_Thanh, ledState ? HIGH : LOW);

    // =============================================================
    // ======================== RFID + SERVO =======================
    // =============================================================

    bool modeBtn = digitalRead(BUTTON_IN_OUT_VINH);
    bool manualBtn = digitalRead(BUTTON_SERVOR_THU_CONG);

    // Đổi mode IN/OUT
    if (modeBtn == LOW && lastButton == HIGH) {
        modeIN = !modeIN;
        printedInMsg = false;
        printedOutMsg = false;

        Serial.print("MODE: ");
        Serial.println(modeIN ? "IN" : "OUT");
        digitalWrite(LED_MODE_VINH, modeIN ? HIGH : LOW);
        delay(200);
    }
    lastButton = modeBtn;

    // Servo thủ công
    if (manualBtn == LOW && lastManualBtn == HIGH) {
        manualServoState = !manualServoState;

        if (manualServoState) myservo.write(93);
        else myservo.write(0);

        delay(180);
    }
    lastManualBtn = manualBtn;

    // In log không spam
    if (modeIN) {
        if (!printedInMsg) {
            Serial.println("IN MODE - Waiting for RFID...");
            printedInMsg = true;
        }
    } else {
        if (!printedOutMsg) {
            Serial.println("OUT MODE - Waiting for RFID...");
            printedOutMsg = true;
        }
    }
}