/*
  Arduino Nano ESP32 + Pololu QTR-8A
  Alle 8 analoge sensoren uitlezen
*/
const int sensorPins[8] = {
  A0, A1, A2, A3, A4, A5, A6, A7
};

int sensorWaarden[8];

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Pololu QTR-8A sensoruitlezing");
  Serial.println("S1   S2   S3   S4   S5   S6   S7   S8");
}

void loop() {
  // Lees alle 8 sensoren uit
  for (int i = 0; i < 8; i++) {
    sensorWaarden[i] = analogRead(sensorPins[i]);
  }

  // Toon de waarden in de Seriele Monitor
  for (int i = 0; i < 8; i++) {
    Serial.print(sensorWaarden[i]);
    Serial.print("\t");
  }

  Serial.println();

  delay(100);
}
