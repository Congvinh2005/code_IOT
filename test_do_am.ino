
#define SOIL_PIN A0

void setup() {
  Serial.begin(9600);
}

void loop() {
  int soilValue = analogRead(SOIL_PIN); // 0–1023
  int soilPercent = map(soilValue, 1023, 300, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  Serial.print("Do am dat: ");
  Serial.print(soilPercent);
  Serial.println("%");

  delay(1000);
}