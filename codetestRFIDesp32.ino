#include <SPI.h>
#include <MFRC522.h>

// ===== RFID =====
#define SS_PIN 16
#define RST_PIN 17
MFRC522 rfid(SS_PIN, RST_PIN);

// ===== IO =====
#define BTN_IO 15
#define LED_IO 4
#define BUZZER 5
#define ONBOARD_LED 2

bool inOutState = 0;          // 0 = VÀO, 1 = RA
bool btnState = HIGH;
unsigned long lastBtnTime = 0;
unsigned long lastReadTime = 0;

String uidString;

// ===== BEEP =====
void beep(int n, int d) {
  for (int i = 0; i < n; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(d);
    digitalWrite(BUZZER, LOW);
    delay(d);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_IO, INPUT_PULLUP);
  pinMode(LED_IO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_IO, LOW);
  digitalWrite(ONBOARD_LED, LOW);

  SPI.begin();
  rfid.PCD_Init();

  Serial.println();
  Serial.println("================================");
  Serial.println("        RFID TEST MODE           ");
  Serial.println("================================");
  Serial.println("Quet the RFID de hien UID");
  Serial.println("Nhan nut de doi trang thai VAO/RA");
  Serial.println("================================");
}

void loop() {
  // ===== NUT BAM =====
  if (digitalRead(BTN_IO) == LOW) {
    if (btnState == HIGH && millis() - lastBtnTime > 300) {
      inOutState = !inOutState;
      digitalWrite(LED_IO, inOutState);

      Serial.println();
      Serial.println(">>> DOI TRANG THAI <<<");
      Serial.print("TRANG THAI HIEN TAI: ");
      Serial.println(inOutState == 0 ? "VÀO" : "RA");

      beep(2, 100);
      lastBtnTime = millis();
      btnState = LOW;
    }
  } else {
    btnState = HIGH;
  }

  // ===== DOC RFID =====
  if (millis() - lastReadTime > 300) {
    readUID();
    lastReadTime = millis();
  }
}

void readUID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();

  Serial.println();
  Serial.println("================================");
  Serial.println("        THE RFID DUOC QUET       ");
  Serial.println("================================");
  Serial.print("UID       : ");
  Serial.println(uidString);
  Serial.print("TRANG THAI: ");
  Serial.println(inOutState == 0 ? "VÀO" : "RA");
  Serial.println("================================");

  digitalWrite(ONBOARD_LED, HIGH);
  beep(1, 200);
  delay(100);
  digitalWrite(ONBOARD_LED, LOW);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
