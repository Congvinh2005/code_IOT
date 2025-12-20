#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Servo.h>

// ================= CẤU HÌNH =================
#define BAUD_RATE 9600

// OLED SH1106 0.96"
Adafruit_SH1106G oled(128, 64, &Wire, -1);

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

// ================= OLED =================
void updateOLED() {
    oled.clearDisplay();
    oled.setTextColor(SH110X_WHITE);

    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.println(" SmartHome Group 3");

    oled.setCursor(0, 14);
    oled.print("LED   : ");
    oled.println(ledState ? "ON" : "OFF");

    oled.setCursor(0, 24);
    oled.print("ALARM : ");
    oled.println(systemOn ? "ON" : "OFF");

    oled.setCursor(0, 34);
    oled.print("DOOR  : ");
    oled.println(manualServoState ? "OPEN" : "CLOSE");

    oled.setCursor(0, 44);
    oled.print("MODE  : ");
    oled.println(modeIN ? "IN" : "OUT");

    oled.setCursor(0, 54);
    oled.print("PIR   : ");
    oled.println(detected ? "DETECT" : "IDLE");

    oled.display();
}

// ================= GỬI TRẠNG THÁI =================
void sendStatusToESP32() {
    Serial.print("STATUS:");
    Serial.print(systemOn ? "1|" : "0|");
    Serial.print(ledState ? "1|" : "0|");
    Serial.print(modeIN ? "1|" : "0|");
    Serial.print(detected ? "1" : "0");
    Serial.println();
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
        digitalWrite(LED_VINH, LOW);  delay(150);
        executeCommand("DOOR_OPEN");
    }
    else if (cmd == "RFID_INVALID") {
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_VINH, HIGH); delay(150);
            digitalWrite(LED_VINH, LOW);  delay(150);
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

    // OLED init
    if (!oled.begin(0x3C, true)) {
        while (1); // không tìm thấy OLED
    }
    oled.clearDisplay();
    oled.display();

    myservo.attach(SERVO_PIN);
    myservo.write(0);

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

    // ---- OUTPUT ----
    digitalWrite(LED_Thanh, ledState ? HIGH : LOW);

    // ---- OLED ----
    updateOLED();
}
