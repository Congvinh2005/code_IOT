// =============================================================
// =============== SMART HOME + PIR + Servor+BLUETOOTH ================
// =============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =============================================================
// =============== NHÓM CHÂN THEO TÊN THÀNH VIÊN ===============
// =============================================================

// ======= QUANH =======
const int PIR_IN_QUANH = 8;
const int LOA_LED_QUANH = 9;
const int BUTTON_QUANH = 7;

// ======= THANH =======
const int LED_Thanh = 11;
const int BTN_Thanh = 10;

// ======= VINH =======
const int LED_MODE_VINH = 5;
const int LED_VINH = 12;
const int BUTTON_IN_OUT_VINH = 6;
const int BUTTON_SERVOR_THU_CONG = 4;

// ======= BIẾN =======
bool systemOn = false;
bool detected = false;
bool ledState = false;

bool lastAlarmBtn = HIGH;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 150;

bool lastBtnLed = HIGH;

bool modeIN = true;
bool lastButton = HIGH;
bool printedInMsg = false;
bool printedOutMsg = false;

Servo myservo;
bool manualServoState = false;
bool lastManualBtn = HIGH;

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

// Beep LED RFID
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

    pinMode(PIR_IN_QUANH, INPUT);
    pinMode(LOA_LED_QUANH, OUTPUT);
    pinMode(BUTTON_QUANH, INPUT_PULLUP);

    pinMode(LED_Thanh, OUTPUT);
    pinMode(BTN_Thanh, INPUT_PULLUP);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Smart Home Group 3");

    myservo.attach(13);
    myservo.write(0);

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
    // 1) ĐỌC BLUETOOTH + RFID (CHỐNG TRÔI LỆNH)
    // =============================================================
    if (Serial.available()) {
        cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd.length() > 0) {

            // ====================== BLUETOOTH ======================
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

                if (cmd == "bat all") {
                    ledState = true;
                    manualServoState = true;
                    myservo.write(93);
                    systemOn = true;
                }
                if (cmd == "tat all") {
                    ledState = false;
                    manualServoState = false;
                    myservo.write(0);
                    systemOn = false;
                }
            }

            // ====================== RFID ======================
            else {
                String id = cmd;

                // HIỂN THỊ ID RFID LÊN LCD
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("RFID: ");
                lcd.print(id);

                // 2 giây rồi trả LCD về trạng thái cũ
                unsigned long t0 = millis();

                if (modeIN) {
                    Serial.print("IN - TAG: ");
                    Serial.println(id);

                    if (id == "AB123456789A" || id == "123456" || id == "HELLO1") {
                        Serial.println("VALID TAG");
                        beepRFID(1);
                        openGate();
                    }
                    else {
                        Serial.println("INVALID TAG");
                        beepRFID(3);
                    }
                }
                else {
                    Serial.print("OUT - TAG: ");
                    Serial.println(id);
                    openGate();
                }

                // Đợi LCD hiển thị xong rồi trả về
                while (millis() - t0 < 2000);

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Smart Home Group 3");
                
            }
        }
    }

    // =============================================================
    // 2) NÚT CẢNH BÁO (PIR)
    // =============================================================
    bool alarmBtn = digitalRead(BUTTON_QUANH);

    if (alarmBtn == LOW && lastAlarmBtn == HIGH) {
        if (millis() - lastButtonTime > debounceDelay) {
            systemOn = !systemOn;
            lastButtonTime = millis();
        }
    }
    lastAlarmBtn = alarmBtn;

    detected = readPIR_Filter();
    digitalWrite(LOA_LED_QUANH, (systemOn && detected) ? HIGH : LOW);

    lcd.setCursor(0, 1);
    lcd.print("By Vinh-Thanh-Quanh");
    
    lcd.setCursor(0, 2);
    lcd.print(ledState ? "LED:O " : "LED:F");

    lcd.setCursor(7, 2);
    lcd.print(systemOn ? "CB:O " : "CB:F");

    lcd.setCursor(13, 2);
    lcd.print(manualServoState ? "Door:O " : "Door:F"); // rút gọn vì hết chỗ

    // =============================================================
    // 3) NÚT BẬT/TẮT LED BLUETOOTH
    // =============================================================
    bool currentBtn = digitalRead(BTN_Thanh);
    if (currentBtn == LOW && lastBtnLed == HIGH) {
        ledState = !ledState;
        delay(150);
    }
    lastBtnLed = currentBtn;

    digitalWrite(LED_Thanh, ledState ? HIGH : LOW);

    // =============================================================
    // 4) MODE IN / OUT + SERVO THỦ CÔNG
    // =============================================================
    bool modeBtn = digitalRead(BUTTON_IN_OUT_VINH);
    bool manualBtn = digitalRead(BUTTON_SERVOR_THU_CONG);

    if (modeBtn == LOW && lastButton == HIGH) {
        modeIN = !modeIN;
        printedInMsg = false;
        printedOutMsg = false;

        Serial.print("MODE: ");
        Serial.println(modeIN ? "IN" : "OUT");
        digitalWrite(LED_MODE_VINH, modeIN ? LOW : HIGH);
        delay(200);
    }
    lastButton = modeBtn;

    if (manualBtn == LOW && lastManualBtn == HIGH) {
        manualServoState = !manualServoState;

        Serial.print("MANUAL SERVO : ");
        Serial.println(manualServoState ? "ON" : "OFF");

        lcd.setCursor(13, 2);
        lcd.print("Door:");
        lcd.print(manualServoState ? "O " : "F");

        if (manualServoState) myservo.write(93);
        else myservo.write(0);

        delay(200);
    }
    lastManualBtn = manualBtn;

    if (modeIN && !printedInMsg) {
        Serial.println("IN MODE - Waiting for RFID...");
        printedInMsg = true;
    }
    if (!modeIN && !printedOutMsg) {
        Serial.println("OUT MODE - Waiting for RFID...");
        printedOutMsg = true;
    }
    
}