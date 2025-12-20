#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PIR_PIN     2
#define BUTTON_PIN  3
#define LED_PIN     4
#define BUZZER_PIN  5

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool canhBao = false;
bool lastButton = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(PIR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HE THONG SAN");
  lcd.setCursor(0, 1);
  lcd.print("SANG...");

  delay(2000);
  lcd.clear();
}

void loop() {
  // ===== BUTTON =====
  bool buttonState = digitalRead(BUTTON_PIN);
  if (lastButton == HIGH && buttonState == LOW) {
    canhBao = !canhBao;
    lcd.clear();
    delay(200); // chống dội
  }
  lastButton = buttonState;

  // ===== TRANG THAI =====
  lcd.setCursor(0, 0);
  if (canhBao) {
    lcd.print("CANH BAO: BAT ");
  } else {
    lcd.print("CANH BAO: TAT ");
  }

  // ===== PIR =====
  lcd.setCursor(0, 1);
  if (canhBao) {
    int pir = digitalRead(PIR_PIN);
    if (pir == HIGH) {
      lcd.print("CO NGUOI       ");
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      lcd.print("KHONG CO NGUOI ");
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
    }
  } else {
    lcd.print("HE THONG TAT   ");
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}
