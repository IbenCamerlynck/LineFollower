
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// ===== PINNEN =====
const int KNOP = 2;     // D2
const int LED = 12;     // D12

const unsigned long DEBOUNCE_TIJD = 50;  // ms

// ===== WIFI =====
const char* WIFI_NAAM = "Linefollower";
const char* WIFI_WACHTWOORD = "linefollower";   // minstens 8 tekens

// ===== OBJECTEN =====
WebServer server(80);

// ===== PARAMETERS (worden in het EEPROM bewaard) =====
struct param_t
{
  uint32_t magic;            // herkent of het EEPROM al geldige gegevens bevat
  unsigned long cycleTime;   // cyclustijd in microseconden
  int snelheid;              // basissnelheid 0-255
  float kp;
  float ki;
  float kd;
  /* andere parameters die in het eeprom geheugen moeten opgeslagen worden voeg je hier toe ... */
} params;

const uint32_t PARAM_MAGIC = 0x4C465231;   // "LFR1"
const int EEPROM_GROOTTE = 64;             // moet >= sizeof(param_t) zijn

// ===== TOESTAND EN METINGEN =====
bool robotActief = false;

unsigned long previous = 0;        // tijdstip van de vorige cyclus (micros)
unsigned long calculationTime = 0; // langste rekentijd van de cyclische code (us)
unsigned long periodMax = 0;       // langste werkelijke tijd tussen twee cycli (us)

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
<title>Linefollower basis</title>
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
  button.groen { background: var(--groen); }
  button.rood { background: var(--rood); }
  button.grijs { background: var(--grijs); }
  button.groot { padding: 18px; font-size: 1.15rem; }
  label { display: block; font-size: .85rem; color: var(--grijs); margin: 10px 0 4px; }
  input[type=number] { width: 100%; font: inherit; padding: 10px; border-radius: 8px;
                       border: 1px solid var(--rand); background: var(--bg); color: var(--tekst); }
  input[type=range] { width: 100%; }
  .badge { padding: 4px 10px; border-radius: 999px; font-size: .85rem; font-weight: 600;
           background: var(--rand); color: var(--tekst); }
  .badge.rijdt { background: var(--accent); color: #fff; }
  table { width: 100%; border-collapse: collapse; }
  td { padding: 5px 0; border-bottom: 1px solid var(--rand); }
  td:last-child { text-align: right; font-variant-numeric: tabular-nums; font-weight: 600; }
  #melding { min-height: 1.3em; font-size: .9rem; color: var(--grijs); margin-top: 8px; }
  .hint { font-size: .8rem; color: var(--grijs); margin-top: 8px; }
</style>
</head>
<body>
<div class="wrap">
  <h1>Linefollower basisprogramma</h1>

  <div class="kaart">
    <span id="toestand" class="badge">Verbinden...</span>
    <div id="melding"></div>
  </div>

  <div class="kaart">
    <div class="rij">
      <button class="groen groot" onclick="api('/start')">START</button>
      <button class="rood groot" onclick="api('/stop')">STOP</button>
    </div>
  </div>

  <div class="kaart">
    <h2>Cyclustijd</h2>
    <table>
      <tr><td>Ingestelde cyclustijd</td><td><span id="cyc">-</span> &micro;s</td></tr>
      <tr><td>Rekentijd (max)</td><td><span id="calc">-</span> &micro;s</td></tr>
      <tr><td>Werkelijke periode (max)</td><td><span id="per">-</span> &micro;s</td></tr>
    </table>
    <div style="height:12px"></div>
    <button class="grijs" style="width:100%" onclick="api('/reset')">Maxima resetten</button>
    <div class="hint">Een webverzoek kan een cyclus vertragen. Sluit de pagina tijdens het rijden.</div>
  </div>

  <div class="kaart">
    <h2>Instellingen</h2>
    <label>Cyclustijd (&micro;s)</label>
    <input type="number" id="cycle" step="1" min="100" max="1000000" inputmode="numeric">
    <label>Basissnelheid (PWM): <b id="vWaarde">0</b></label>
    <input type="range" id="speed" min="0" max="255" step="1" oninput="$('vWaarde').textContent=this.value">
    <div class="rij">
      <div><label>Kp</label><input type="number" id="kp" step="any" min="0" inputmode="decimal"></div>
      <div><label>Ki</label><input type="number" id="ki" step="any" min="0" inputmode="decimal"></div>
      <div><label>Kd</label><input type="number" id="kd" step="any" min="0" inputmode="decimal"></div>
    </div>
    <div style="height:12px"></div>
    <button style="width:100%" onclick="bewaar()">Toepassen en opslaan</button>
    <div class="hint">Wordt bewaard in het EEPROM en blijft na een reset behouden.</div>
  </div>
</div>

<script>
const $ = id => document.getElementById(id);
let geladen = false;

async function api(pad) {
  try {
    const r = await fetch(pad);
    $('melding').textContent = await r.text();
    status();
  } catch (e) {
    $('melding').textContent = 'Geen verbinding met de robot.';
  }
}

function bewaar() {
  api('/set?cycle=' + $('cycle').value + '&speed=' + $('speed').value +
      '&kp=' + $('kp').value + '&ki=' + $('ki').value + '&kd=' + $('kd').value);
}

async function status() {
  try {
    const r = await fetch('/status');
    const s = await r.json();

    $('toestand').textContent = s.act ? 'Rijden' : 'Stil';
    $('toestand').className = 'badge' + (s.act ? ' rijdt' : '');

    $('cyc').textContent = s.cycle;
    $('calc').textContent = s.calc;
    $('per').textContent = s.per;

    // Instellingen alleen de eerste keer invullen (zodat typen niet overschreven wordt)
    if (!geladen) {
      $('cycle').value = s.cycle;
      $('speed').value = s.speed; $('vWaarde').textContent = s.speed;
      $('kp').value = s.kp; $('ki').value = s.ki; $('kd').value = s.kd;
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

// ===== PARAMETERS =====
void standaardParameters() {
  params.magic = PARAM_MAGIC;
  params.cycleTime = 5000;   // 5 ms
  params.snelheid = 255;
  params.kp = 0.05;
  params.ki = 0.0;
  params.kd = 0.60;
}

void bewaarParameters() {
  params.magic = PARAM_MAGIC;
  EEPROM.put(0, params);
  EEPROM.commit();   // op de ESP32 wordt pas na commit() echt weggeschreven
}

// Geeft een parameter een nieuwe waarde (aangeroepen door de webpagina).
// Geeft false terug als de naam onbekend is.
bool zetParameter(const char* param, const char* value) {
  if (strcmp(param, "cycle") == 0) params.cycleTime = constrain(atol(value), 100L, 1000000L);
  else if (strcmp(param, "speed") == 0) params.snelheid = constrain(atoi(value), 0, 255);
  else if (strcmp(param, "kp") == 0) params.kp = atof(value);
  else if (strcmp(param, "ki") == 0) params.ki = atof(value);
  else if (strcmp(param, "kd") == 0) params.kd = atof(value);
  /* parameters een nieuwe waarde geven via de webpagina doe je hier ... */
  else return false;

  return true;
}

// ===== START / STOP =====
void startRobot() {
  robotActief = true;
  digitalWrite(LED, HIGH);
}

void stopRobot() {
  robotActief = false;
  digitalWrite(LED, LOW);
  /* motoren stoppen doe je hier ... */
}

// ===== WEBSERVER =====
void handleRoot() {
  server.send_P(200, "text/html", PAGINA);
}

void handleStatus() {
  String j = "{";
  j += "\"act\":" + String(robotActief ? 1 : 0);
  j += ",\"cycle\":" + String(params.cycleTime);
  j += ",\"calc\":" + String(calculationTime);
  j += ",\"per\":" + String(periodMax);
  j += ",\"speed\":" + String(params.snelheid);
  j += ",\"kp\":" + String(params.kp, 5);
  j += ",\"ki\":" + String(params.ki, 7);
  j += ",\"kd\":" + String(params.kd, 5);
  j += "}";

  server.send(200, "application/json", j);
}

void handleSet() {
  const char* namen[] = {"cycle", "speed", "kp", "ki", "kd"};
  bool gewijzigd = false;

  for (int i = 0; i < 5; i++) {
    if (server.hasArg(namen[i])) {
      zetParameter(namen[i], server.arg(namen[i]).c_str());
      gewijzigd = true;
    }
  }

  if (gewijzigd) bewaarParameters();
  server.send(200, "text/plain", "Instellingen opgeslagen.");
}

void handleStart() {
  startRobot();
  server.send(200, "text/plain", "Gestart.");
}

void handleStop() {
  stopRobot();
  server.send(200, "text/plain", "Gestopt.");
}

void handleReset() {
  calculationTime = 0;
  periodMax = 0;
  server.send(200, "text/plain", "Maxima gereset.");
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
void setup()
{
  pinMode(KNOP, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // EEPROM: parameters inlezen, of standaardwaarden nemen als het geheugen nog leeg is
  EEPROM.begin(EEPROM_GROOTTE);
  EEPROM.get(0, params);
  if (params.magic != PARAM_MAGIC) {
    standaardParameters();
    bewaarParameters();
  }

  // Interrupt op de knop
  attachInterrupt(digitalPinToInterrupt(KNOP), knopISR, FALLING);

  // WiFi access point + webserver
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_NAAM, WIFI_WACHTWOORD);

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/set", handleSet);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/reset", handleReset);
  server.begin();

  previous = micros();
}

// ===== LOOP =====
void loop()
{
  // Communicatie (niet-blokkerend)
  server.handleClient();

  // Knop (gezet door de interrupt)
  if (knopVlag) {
    knopVlag = false;
    if (robotActief) stopRobot();
    else startRobot();
  }

  // Cyclisch gedeelte: wordt iedere params.cycleTime microseconden uitgevoerd
  unsigned long current = micros();
  if (current - previous >= params.cycleTime)
  {
    unsigned long periode = current - previous;   // werkelijke tijd sinds de vorige cyclus
    if (periode > periodMax) periodMax = periode;
    previous = current;

    /* code die cyclisch moet uitgevoerd worden programmeer je hier ... */

    /* normaliseren en interpoleren sensor */

    /* pid regeling */

    /* aansturen motoren (alleen als robotActief true is) */

    // Rekentijd van deze cyclus meten
    unsigned long difference = micros() - current;
    if (difference > calculationTime) calculationTime = difference;
  }
}
