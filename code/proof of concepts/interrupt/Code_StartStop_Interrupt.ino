// ===== PINNEN =====
const int KNOP = D2;    // drukknop (INPUT_PULLUP, ingedrukt = LOW)
const int LED  = D12;

// Rechter motor
const int AIN1 = D3;
const int AIN2 = D5;

// Linker motor
const int BIN1 = D6;
const int BIN2 = D9;

// ===== INSTELLINGEN =====
const int SNELHEID = 120;                // 0-255
const unsigned long DEBOUNCE_TIJD = 50;  // ms

// ===== VARIABELEN =====
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

volatile bool dalendeFlank = false;      // knop net ingedrukt
volatile unsigned long laatsteFlankTijd = 0;

bool robotActief = false;

// ===== INTERRUPT (flankdetectie) =====
void IRAM_ATTR knopInterrupt() {
  unsigned long nu = millis();

  // Ontdenderen: te snel na de vorige flank = contactdender
  if (nu - laatsteFlankTijd < DEBOUNCE_TIJD) return;
  laatsteFlankTijd = nu;

  // Niveau na de flank bepaalt welke flank het was
  if (digitalRead(KNOP) == LOW) {
    // Dalende flank: knop ingedrukt
    portENTER_CRITICAL_ISR(&mux);
    dalendeFlank = true;
    portEXIT_CRITICAL_ISR(&mux);
  }
  // Stijgende flank (loslaten): bewust genegeerd
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(KNOP, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  digitalWrite(LED, LOW);
  stopMotoren();

  attachInterrupt(digitalPinToInterrupt(KNOP), knopInterrupt, CHANGE);

  Serial.println("Klaar. Druk op de knop.");
}

// ===== LOOP =====
void loop() {
  bool gedrukt = false;

  portENTER_CRITICAL(&mux);
  if (dalendeFlank) {
    gedrukt = true;
    dalendeFlank = false;
  }
  portEXIT_CRITICAL(&mux);

  if (gedrukt) {
    robotActief = !robotActief;

    if (robotActief) {
      digitalWrite(LED, HIGH);
      motorenVooruit();
      Serial.println("AAN");
    } else {
      stopMotoren();
      digitalWrite(LED, LOW);
      Serial.println("UIT");
    }
  }
}

// ===== MOTOREN =====
void motorenVooruit() {
  analogWrite(AIN1, SNELHEID);
  analogWrite(AIN2, 0);

  analogWrite(BIN1, SNELHEID);
  analogWrite(BIN2, 0);
}

void stopMotoren() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);

  analogWrite(BIN1, 0);
  analogWrite(BIN2, 0);
}