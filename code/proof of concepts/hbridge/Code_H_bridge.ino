// Rechter motor
const int AIN1 = D3;
const int AIN2 = D5;

// Linker motor
const int BIN1 = D6;
const int BIN2 = D9;


void setup() {

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  stopMotoren();
}

void loop() {
  // TEST LINKERMOTOR

  motorLinks(120, 1);
  delay(3000);

  motorLinks(200, 1);
  delay(3000);

  motorLinks(0, 0);
  delay(3000);

  motorLinks(120, -1);
  delay(3000);

  motorLinks(200, -1);
  delay(3000);

  motorLinks(0, 0);
  delay(3000);


  // TEST RECHTERMOTOR

  motorRechts(120, 1);
  delay(3000);

  motorRechts(200, 1);
  delay(3000);

  motorRechts(0, 0);
  delay(3000);

  motorRechts(120, -1);
  delay(3000);

  motorRechts(200, -1);
  delay(3000);

  motorRechts(0, 0);
  delay(3000);


  // BEIDE MOTOREN TEGENGESTELD
  // Linker motor vooruit
  // Rechter motor achteruit

  motorLinks(200, 1);
  motorRechts(200, -1);

  delay(3000);
  // Richtingen omdraaien:
  // Linker motor achteruit
  // Rechter motor vooruit
  motorLinks(200, -1);
  motorRechts(200, 1);

  delay(3000);
  stopMotoren();

  // Programma stopt definitief
  while (true) {
    delay(1000);
  }
}


//LINKERMOTOR

void motorLinks(int snelheid, int richting) {

  snelheid = constrain(snelheid, 0, 255);

  if (richting == 1) {
    analogWrite(BIN1, snelheid);
    analogWrite(BIN2, 0);
  }
  else if (richting == -1) {
    analogWrite(BIN1, 0);
    analogWrite(BIN2, snelheid);
  }
  else {
    analogWrite(BIN1, 0);
    analogWrite(BIN2, 0);
  }
}


//RECHTERMOTOR

void motorRechts(int snelheid, int richting) {

  snelheid = constrain(snelheid, 0, 255);

  if (richting == 1) {
    analogWrite(AIN1, snelheid);
    analogWrite(AIN2, 0);
  }
  else if (richting == -1) {
    analogWrite(AIN1, 0);
    analogWrite(AIN2, snelheid);
  }
  else {
    analogWrite(AIN1, 0);
    analogWrite(AIN2, 0);
  }
}

//BEIDE MOTOREN STOPPEN
void stopMotoren() {

  motorLinks(0, 0);
  motorRechts(0, 0);
}
