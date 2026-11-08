/*
  Linefollower met webserver - Arduino Nano ESP32 + DRV8833 + QTR-8A

  Sensoren:  A7 = meest LINKS ... A0 = meest RECHTS
  Rechter motor: AIN1 = D3, AIN2 = D5
  Linker motor:  BIN1 = D6, BIN2 = D9
  Drukknop: D2 (INPUT_PULLUP, ingedrukt = LOW)
  LED:      D12

  WiFi: de robot maakt zelf een netwerk aan.
    Naam (SSID):  Linefollower
    Wachtwoord:   linefollower
    Webpagina:    http://192.168.4.1

  Via de webpagina kan je:
    - zwart en wit kalibreren (de robot kan pas starten na beide)
    - basissnelheid, Kp, Ki, Kd en startvertraging aanpassen (wordt bewaard)
    - de robot starten en stoppen
    - de sensorwaarden live bekijken

  Drukknop: 1e druk = starten (na de startvertraging), 2e druk = direct stoppen.
  Op een kruispunt rijdt de robot rechtdoor.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// ===== WIFI =====
const char* WIFI_NAAM = "Linefollower";
const char* WIFI_WACHTWOORD = "linefollower";   // minstens 8 tekens

WebServer server(80);

// ===== INSTELLINGEN =====
const bool LIJN_ZWART = true;      // true = zwarte lijn op wit, false = witte lijn op zwart

// Deze waarden zijn de standaardwaarden; ze worden via de webpagina aangepast en bewaard.
float Kp = 0.05;                   // proportioneel
float Ki = 0.0;                    // integraal
float Kd = 0.60;                   // differentieel
int basisSnelheid = 255;           // 0-255 (PWM)
int startVertraging = 3;          // seconden tussen start en rijden

const int ZOEK_SNELHEID = 255;      // draaisnelheid als de lijn kwijt is
const int MAX_SNELHEID = 255;

const int KRUISPUNT_AANTAL = 8;    // zoveel sensoren op de lijn = kruispunt
const int LIJN_WAARDE = 600;       // genormaliseerde waarde (0-1000) vanaf wanneer een sensor "op de lijn" staat
const int RUIS_WAARDE = 200;       // lagere waarden worden genegeerd
const float I_MAX = 100.0;         // maximale bijdrage van de I-term (anti-windup)

const int AANTAL_METINGEN = 50;    // metingen per sensor bij het kalibreren
const unsigned long DEBOUNCE_TIJD = 50;  // ms

// ===== PINNEN =====
const int KNOP = 2;     // D2
const int LED = 12;     // D12

const int AANTAL_SENSOREN = 8;

// Index 0 = A0 = meest rechts ... index 7 = A7 = meest links
const int sensor[AANTAL_SENSOREN] = {
  A0, A1, A2, A3, A4, A5, A6, A7
};

const int AIN1 = D3;  // rechts
const int AIN2 = D5;
const int BIN1 = D6;  // links
const int BIN2 = D9;

// ===== OPSLAG (EEPROM) =====
const int EEPROM_GROOTTE = 128;
const uint32_t OPSLAG_MAGIC = 0x51545239;

struct Opslag {
  int black[AANTAL_SENSOREN];
  int white[AANTAL_SENSOREN];
  float kp;
  float ki;
  float kd;
  int snelheid;
  int vertraging;
  uint8_t zwartOK;
  uint8_t witOK;
  uint32_t magic;
};

Opslag opslag;

bool zwartOK = false;
bool witOK = false;

// ===== TOESTAND =====
enum Toestand { STIL, WACHTEN, RIJDEN };
Toestand toestand = STIL;
unsigned long startMoment = 0;

enum Meting { LIJN, KRUISPUNT, KWIJT };

float vorigeFout = 0;
float integraal = 0;
int laatsteRichting = 0;  // -1 = lijn was links, +1 = lijn was rechts

// Knop (interrupt)
volatile bool knopVlag = false;
volatile unsigned long laatsteInterrupt = 0;

// ===== WEBPAGINA =====
const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Linefollower</title>
<style>
  :root {
    --bg: #f4f5f7; --kaart: #ffffff; --tekst: #1d2330; --rand: #d5d9e0;
    --accent: #2563eb; --groen: #16a34a; --rood: #dc2626; --grijs: #6b7280;
  }
  @media (prefers-color-scheme: dark) {
    :root { --bg: #12151c; --kaart: #1c212b; --tekst: #e8ebf1; --rand: #333a49; --grijs: #98a1b3; }
  }
  * { box-sizing: border-box; }
  body { margin: 0; padding: 12px; background: var(--bg); color: var(--tekst);
         font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; }
  .wrap { max-width: 520px; margin: 0 auto; display: grid; gap: 12px; }
  h1 { font-size: 1.3rem; margin: 4px 0; }
  h2 { font-size: 1rem; margin: 0 0 10px; }
  .kaart { background: var(--kaart); border: 1px solid var(--rand); border-radius: 12px; padding: 14px; }
  .rij { display: flex; gap: 10px; }
  .rij > * { flex: 1; }
  button { font: inherit; font-weight: 600; padding: 12px; border: 0; border-radius: 10px;
           color: #fff; background: var(--accent); cursor: pointer; }
  button:disabled { opacity: .4; cursor: not-allowed; }
  button.groen { background: var(--groen); }
  button.rood { background: var(--rood); }
  button.grijs { background: var(--grijs); }
  button.groot { padding: 18px; font-size: 1.15rem; }
  label { display: block; font-size: .85rem; color: var(--grijs); margin: 10px 0 4px; }
  input[type=number] { width: 100%; font: inherit; padding: 10px; border-radius: 8px;
                       border: 1px solid var(--rand); background: var(--bg); color: var(--tekst); }
  input[type=range] { width: 100%; }
  .status { display: flex; justify-content: space-between; align-items: center; gap: 8px; flex-wrap: wrap; }
  .badge { padding: 4px 10px; border-radius: 999px; font-size: .85rem; font-weight: 600;
           background: var(--rand); color: var(--tekst); }
  .badge.ok { background: var(--groen); color: #fff; }
  .badge.rijdt { background: var(--accent); color: #fff; }
  .badge.wacht { background: #d97706; color: #fff; }
  .sensoren { display: grid; grid-template-columns: repeat(8, 1fr); gap: 6px; align-items: end; height: 110px; }
  .sensor { display: flex; flex-direction: column; justify-content: flex-end; height: 100%; }
  .balk { background: var(--accent); border-radius: 4px 4px 0 0; min-height: 2px; }
  .labels { display: grid; grid-template-columns: repeat(8, 1fr); gap: 6px; text-align: center;
            font-size: .72rem; color: var(--grijs); margin-top: 4px; }
  .labels b { display: block; color: var(--tekst); font-weight: 600; }
  #melding { min-height: 1.3em; font-size: .9rem; color: var(--grijs); }
  .hint { font-size: .8rem; color: var(--grijs); margin-top: 8px; }
</style>
</head>
<body>
<div class="wrap">
  <h1>Linefollower</h1>

  <div class="kaart">
    <div class="status">
      <span id="toestand" class="badge">Verbinden...</span>
      <span id="zw" class="badge">Zwart: nee</span>
      <span id="wi" class="badge">Wit: nee</span>
    </div>
    <div id="melding"></div>
  </div>

  <div class="kaart">
    <div class="rij">
      <button id="bStart" class="groen groot" onclick="api('/start')">START</button>
      <button id="bStop" class="rood groot" onclick="api('/stop')">STOP</button>
    </div>
  </div>

  <div class="kaart">
    <h2>Kalibratie</h2>
    <div class="rij">
      <button id="bZwart" class="grijs" onclick="api('/zwart')">Kalibreer ZWART</button>
      <button id="bWit" class="grijs" onclick="api('/wit')">Kalibreer WIT</button>
    </div>
    <div class="hint">Houd alle sensoren boven de juiste ondergrond en druk dan op de knop. Eerst zwart, dan wit. Daarna kan de robot starten.</div>
  </div>

  <div class="kaart">
    <h2>Instellingen</h2>
    <label>Basissnelheid (PWM): <b id="vWaarde">255</b></label>
    <input type="range" id="v" min="0" max="255" step="1" oninput="$('vWaarde').textContent=this.value">
    <div class="rij">
      <div><label>Kp</label><input type="number" id="kp" step="any" min="0" inputmode="decimal"></div>
      <div><label>Ki</label><input type="number" id="ki" step="any" min="0" inputmode="decimal"></div>
      <div><label>Kd</label><input type="number" id="kd" step="any" min="0" inputmode="decimal"></div>
    </div>
    <label>Startvertraging (seconden)</label>
    <input type="number" id="d" step="1" min="0" max="60" inputmode="numeric">
    <div style="height:12px"></div>
    <button style="width:100%" onclick="bewaar()">Toepassen en opslaan</button>
    <div class="hint">Wijzigingen werken meteen, ook terwijl de robot rijdt.</div>
  </div>

  <div class="kaart">
    <h2>Sensoren (links naar rechts)</h2>
    <div class="sensoren" id="balken"></div>
    <div class="labels" id="labels"></div>
  </div>
</div>

<script>
const $ = id => document.getElementById(id);
let geladen = false;

// Balken opbouwen: links = A7 ... rechts = A0
for (let i = 7; i >= 0; i--) {
  $('balken').insertAdjacentHTML('beforeend', '<div class="sensor"><div class="balk" id="b' + i + '"></div></div>');
  $('labels').insertAdjacentHTML('beforeend', '<div>A' + i + '<b id="r' + i + '">-</b></div>');
}

async function api(pad) {
  try {
    const r = await fetch(pad);
    $('melding').textContent = await r.text();
    status();
  } catch (e) {
    $('melding').textContent = 'Geen verbinding met de robot.';
  }
}

async function bewaar() {
  const q = '/set?v=' + $('v').value + '&kp=' + $('kp').value + '&ki=' + $('ki').value +
            '&kd=' + $('kd').value + '&d=' + $('d').value;
  api(q);
}

async function status() {
  try {
    const r = await fetch('/status');
    const s = await r.json();

    const namen = ['Stil', 'Wachten', 'Rijden'];
    const t = $('toestand');
    t.textContent = s.t === 1 ? 'Start over ' + s.rest + ' s' : namen[s.t];
    t.className = 'badge' + (s.t === 2 ? ' rijdt' : s.t === 1 ? ' wacht' : '');

    $('zw').textContent = 'Zwart: ' + (s.zw ? 'ja' : 'nee');
    $('zw').className = 'badge' + (s.zw ? ' ok' : '');
    $('wi').textContent = 'Wit: ' + (s.wi ? 'ja' : 'nee');
    $('wi').className = 'badge' + (s.wi ? ' ok' : '');

    const stil = s.t === 0;
    $('bStart').disabled = !(stil && s.zw && s.wi);
    $('bZwart').disabled = !stil;
    $('bWit').disabled = !stil;

    // Sensoren: s.raw en s.norm staan van links naar rechts (A7 ... A0)
    for (let k = 0; k < 8; k++) {
      const i = 7 - k;
      $('b' + i).style.height = Math.max(2, s.norm[k] / 10) + '%';
      $('r' + i).textContent = s.raw[k];
    }

    // Instellingen alleen de eerste keer invullen (zodat typen niet overschreven wordt)
    if (!geladen) {
      $('v').value = s.v; $('vWaarde').textContent = s.v;
      $('kp').value = s.kp; $('ki').value = s.ki; $('kd').value = s.kd; $('d').value = s.d;
      geladen = true;
    }
  } catch (e) {
    $('toestand').textContent = 'Geen verbinding';
    $('toestand').className = 'badge';
  }
}

setInterval(status, 500);
status();
</script>
</body>
</html>
)rawliteral";

// ===== MOTORSTURING =====
// snelheid: -255 (achteruit) ... +255 (vooruit)
void zetMotor(int pinIn1, int pinIn2, int snelheid) {
  snelheid = constrain(snelheid, -MAX_SNELHEID, MAX_SNELHEID);

  if (snelheid >= 0) {
    analogWrite(pinIn1, snelheid);
    analogWrite(pinIn2, 0);
  } else {
    analogWrite(pinIn1, 0);
    analogWrite(pinIn2, -snelheid);
  }
}

void zetMotoren(int links, int rechts) {
  zetMotor(BIN1, BIN2, links);
  zetMotor(AIN1, AIN2, rechts);
}

// ===== INSTELLINGEN OPSLAAN / LADEN =====
void bewaarInstellingen() {
  opslag.kp = Kp;
  opslag.ki = Ki;
  opslag.kd = Kd;
  opslag.snelheid = basisSnelheid;
  opslag.vertraging = startVertraging;
  opslag.zwartOK = zwartOK ? 1 : 0;
  opslag.witOK = witOK ? 1 : 0;
  opslag.magic = OPSLAG_MAGIC;

  EEPROM.put(0, opslag);
  EEPROM.commit();
}

void laadInstellingen() {
  EEPROM.get(0, opslag);

  if (opslag.magic == OPSLAG_MAGIC) {
    Kp = opslag.kp;
    Ki = opslag.ki;
    Kd = opslag.kd;
    basisSnelheid = constrain(opslag.snelheid, 0, 255);
    startVertraging = constrain(opslag.vertraging, 0, 60);
    zwartOK = opslag.zwartOK == 1;
    witOK = opslag.witOK == 1;
  } else {
    // Geen geldige gegevens: begin met lege kalibratie
    memset(&opslag, 0, sizeof(opslag));
    zwartOK = false;
    witOK = false;
  }
}

// ===== SENSOREN =====
// Genormaliseerd: 0 = wit, 1000 = zwart (omgekeerd bij een witte lijn)
int normaliseer(int i, int raw) {
  int zwart = opslag.black[i];
  int wit = opslag.white[i];

  if (!zwartOK || !witOK || zwart <= wit) return 0;

  long waarde = (long)(raw - wit) * 1000 / (zwart - wit);
  waarde = constrain(waarde, 0, 1000);

  if (!LIJN_ZWART) waarde = 1000 - waarde;

  return waarde;
}

// Bepaalt wat de sensoren zien. Bij LIJN wordt 'positie' ingevuld:
// 0 (uiterst links) ... 7000 (uiterst rechts).
Meting leesSensoren(int &positie) {
  long gewogenSom = 0;
  long som = 0;
  int aantalOpLijn = 0;

  for (int i = 0; i < AANTAL_SENSOREN; i++) {
    int waarde = normaliseer(i, analogRead(sensor[i]));

    if (waarde > LIJN_WAARDE) aantalOpLijn++;

    // A7 (i = 7) is links => positie 0, A0 (i = 0) is rechts => positie 7000
    int sensorPositie = (7 - i) * 1000;

    // Kleine waarden negeren (ruis)
    if (waarde > RUIS_WAARDE) {
      gewogenSom += (long)waarde * sensorPositie;
      som += waarde;
    }
  }

  // Veel sensoren tegelijk op de lijn: kruispunt (of T-splitsing) => rechtdoor
  if (aantalOpLijn >= KRUISPUNT_AANTAL) {
    return KRUISPUNT;
  }

  if (som == 0) return KWIJT;

  positie = gewogenSom / som;
  return LIJN;
}

// ===== LIJNVOLGEN =====
void volgLijn() {
  int positie = 3500;
  Meting meting = leesSensoren(positie);

  if (meting == LIJN) {
    // Fout: negatief = lijn links, positief = lijn rechts
    float fout = positie - 3500;
    float afgeleide = fout - vorigeFout;
    vorigeFout = fout;

    // Integraal met anti-windup
    float iTerm = 0;
    if (Ki > 0) {
      integraal += fout;
      iTerm = Ki * integraal;
      if (iTerm > I_MAX) {
        iTerm = I_MAX;
        integraal = I_MAX / Ki;
      } else if (iTerm < -I_MAX) {
        iTerm = -I_MAX;
        integraal = -I_MAX / Ki;
      }
    } else {
      integraal = 0;
    }

    float correctie = Kp * fout + iTerm + Kd * afgeleide;

    // Lijn rechts => linker motor sneller, rechter motor trager
    int snelheidLinks = basisSnelheid + correctie;
    int snelheidRechts = basisSnelheid - correctie;

    zetMotoren(snelheidLinks, snelheidRechts);

    if (fout < -1000) laatsteRichting = -1;
    else if (fout > 1000) laatsteRichting = 1;
  }
  else if (meting == KRUISPUNT) {
    // Rechtdoor, zonder bij te sturen
    vorigeFout = 0;
    integraal = 0;
    zetMotoren(basisSnelheid, basisSnelheid);
  }
  else {
    // Lijn kwijt: draai naar de kant waar de lijn het laatst was
    integraal = 0;
    if (laatsteRichting < 0) {
      zetMotoren(-ZOEK_SNELHEID, ZOEK_SNELHEID);
    } else {
      zetMotoren(ZOEK_SNELHEID, -ZOEK_SNELHEID);
    }
  }
}

// ===== START / STOP =====
String startRobot() {
  if (toestand != STIL) {
    return "De robot is al gestart.";
  }
  if (!zwartOK || !witOK) {
    return "Kalibreer eerst zwart en wit.";
  }

  toestand = WACHTEN;
  startMoment = millis();
  digitalWrite(LED, HIGH);

  return "Gestart. De robot rijdt na " + String(startVertraging) + " s.";
}

void stopRobot() {
  toestand = STIL;
  zetMotoren(0, 0);
  digitalWrite(LED, LOW);
}

void knipperLed(int keer) {
  for (int i = 0; i < keer; i++) {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
  }
}

// ===== KALIBRATIE =====
String kalibreer(bool zwart) {
  if (toestand != STIL) {
    return "Stop de robot eerst voor je kalibreert.";
  }

  delay(500);

  long som[AANTAL_SENSOREN] = {0};

  for (int meting = 0; meting < AANTAL_METINGEN; meting++) {
    for (int i = 0; i < AANTAL_SENSOREN; i++) {
      som[i] += analogRead(sensor[i]);
    }
    delay(5);
  }

  for (int i = 0; i < AANTAL_SENSOREN; i++) {
    int gemiddelde = som[i] / AANTAL_METINGEN;

    if (zwart) {
      opslag.black[i] = gemiddelde;
    } else {
      opslag.white[i] = gemiddelde;
    }
  }

  if (zwart) zwartOK = true;
  else witOK = true;

  bewaarInstellingen();

  String melding = zwart ? "Zwart gekalibreerd." : "Wit gekalibreerd.";

  // Controle: zwart moet bij elke sensor hoger zijn dan wit
  if (zwartOK && witOK) {
    for (int i = 0; i < AANTAL_SENSOREN; i++) {
      if (opslag.black[i] <= opslag.white[i]) {
        melding += " Let op: bij sensor A" + String(i) + " is zwart niet hoger dan wit. Kalibreer opnieuw.";
        return melding;
      }
    }
    melding += " Klaar om te starten.";
  } else {
    melding += zwart ? " Kalibreer nu wit." : " Kalibreer nu zwart.";
  }

  return melding;
}

// ===== WEBSERVER =====
void handleRoot() {
  server.send_P(200, "text/html", PAGINA);
}

void handleStatus() {
  int resterend = 0;
  if (toestand == WACHTEN) {
    unsigned long verlopen = millis() - startMoment;
    unsigned long totaal = (unsigned long)startVertraging * 1000UL;
    if (verlopen < totaal) {
      resterend = (totaal - verlopen + 999) / 1000;
    }
  }

  int raw[AANTAL_SENSOREN];
  for (int i = 0; i < AANTAL_SENSOREN; i++) {
    raw[i] = analogRead(sensor[i]);
  }

  String j = "{";
  j += "\"t\":" + String((int)toestand);
  j += ",\"rest\":" + String(resterend);
  j += ",\"zw\":" + String(zwartOK ? 1 : 0);
  j += ",\"wi\":" + String(witOK ? 1 : 0);
  j += ",\"v\":" + String(basisSnelheid);
  j += ",\"kp\":" + String(Kp, 5);
  j += ",\"ki\":" + String(Ki, 7);
  j += ",\"kd\":" + String(Kd, 5);
  j += ",\"d\":" + String(startVertraging);

  // Van links naar rechts: A7 ... A0
  j += ",\"raw\":[";
  for (int i = AANTAL_SENSOREN - 1; i >= 0; i--) {
    j += String(raw[i]);
    if (i > 0) j += ",";
  }
  j += "],\"norm\":[";
  for (int i = AANTAL_SENSOREN - 1; i >= 0; i--) {
    j += String(normaliseer(i, raw[i]));
    if (i > 0) j += ",";
  }
  j += "]}";

  server.send(200, "application/json", j);
}

void handleSet() {
  if (server.hasArg("v")) {
    basisSnelheid = constrain(server.arg("v").toInt(), 0, 255);
  }
  if (server.hasArg("kp")) {
    Kp = constrain(server.arg("kp").toFloat(), 0.0f, 10.0f);
  }
  if (server.hasArg("ki")) {
    Ki = constrain(server.arg("ki").toFloat(), 0.0f, 1.0f);
    integraal = 0;
  }
  if (server.hasArg("kd")) {
    Kd = constrain(server.arg("kd").toFloat(), 0.0f, 20.0f);
  }
  if (server.hasArg("d")) {
    startVertraging = constrain(server.arg("d").toInt(), 0, 60);
  }

  bewaarInstellingen();
  server.send(200, "text/plain", "Instellingen opgeslagen.");
}

void handleStart() {
  server.send(200, "text/plain", startRobot());
}

void handleStop() {
  stopRobot();
  server.send(200, "text/plain", "Gestopt.");
}

void handleZwart() {
  server.send(200, "text/plain", kalibreer(true));
}

void handleWit() {
  server.send(200, "text/plain", kalibreer(false));
}

// ===== KNOP =====
// Interrupt-routine: wordt aangeroepen bij een dalende flank (knop ingedrukt).
// Hier wordt alleen een vlag gezet; de verwerking gebeurt in loop().
void IRAM_ATTR knopISR() {
  unsigned long nu = millis();
  if (nu - laatsteInterrupt > DEBOUNCE_TIJD) {  // ontdenderen
    knopVlag = true;
    laatsteInterrupt = nu;
  }
}

// ===== SETUP =====
void setup() {
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(KNOP, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  zetMotoren(0, 0);
  digitalWrite(LED, LOW);

  EEPROM.begin(EEPROM_GROOTTE);
  laadInstellingen();

  attachInterrupt(digitalPinToInterrupt(KNOP), knopISR, FALLING);

  // WiFi access point + webserver
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_NAAM, WIFI_WACHTWOORD);

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/set", handleSet);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/zwart", handleZwart);
  server.on("/wit", handleWit);
  server.begin();
}

// ===== LOOP =====
void loop() {
  server.handleClient();

  if (knopVlag) {
    knopVlag = false;

    if (toestand == STIL) {
      if (zwartOK && witOK) {
        startRobot();
      } else {
        knipperLed(3);  // nog niet gekalibreerd
      }
    } else {
      stopRobot();
    }
  }

  if (toestand == WACHTEN) {
    if (millis() - startMoment >= (unsigned long)startVertraging * 1000UL) {
      vorigeFout = 0;
      integraal = 0;
      laatsteRichting = 0;
      toestand = RIJDEN;
    }
  }

  if (toestand == RIJDEN) {
    volgLijn();
  }

  delay(2);
}
