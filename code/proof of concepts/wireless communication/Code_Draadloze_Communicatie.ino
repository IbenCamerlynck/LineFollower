/*
  Draadloze communicatie - proof of concept
  Arduino Nano ESP32 <-> smartphone via webbrowser
  (uit het linefollower-programma: alleen WiFi + webserver + knop-interrupt + LED)

  Drukknop: D2 (INPUT_PULLUP, ingedrukt = LOW)
  LED:      D12

  WiFi: de ESP32 maakt zelf een netwerk aan.
    Naam (SSID):  Linefollower
    Wachtwoord:   linefollower
    Webpagina:    http://192.168.4.1

  Telefoon -> ESP32:  LED aan/uit zetten  (/led?aan=1 of /led?aan=0)
  ESP32 -> telefoon:  knopstand, aantal keer ingedrukt en uptime (/status, elke 500 ms)
*/

#include <WiFi.h>
#include <WebServer.h>

// ===== WIFI =====
const char* WIFI_NAAM = "Linefollower";
const char* WIFI_WACHTWOORD = "linefollower";   // minstens 8 tekens

WebServer server(80);

// ===== PINNEN =====
const int KNOP = 2;     // D2
const int LED = 12;     // D12

const unsigned long DEBOUNCE_TIJD = 50;  // ms

// ===== TOESTAND =====
bool ledAan = false;
unsigned long aantalDrukken = 0;

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
<title>Draadloze POC</title>
<style>
  :root {
    --bg: #f4f5f7; --kaart: #ffffff; --tekst: #1d2330; --rand: #d5d9e0;
    --accent: #2563eb; --groen: #16a34a; --grijs: #6b7280;
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
  button { font: inherit; font-weight: 600; padding: 18px; border: 0; border-radius: 10px; width: 100%;
           color: #fff; background: var(--accent); cursor: pointer; font-size: 1.1rem; }
  .badge { padding: 4px 10px; border-radius: 999px; font-size: .85rem; font-weight: 600;
           background: var(--rand); color: var(--tekst); }
  .badge.ok { background: var(--groen); color: #fff; }
  .groot { font-size: 1.6rem; font-weight: 700; }
  #melding { min-height: 1.3em; font-size: .9rem; color: var(--grijs); margin-top: 8px; }
  .hint { font-size: .8rem; color: var(--grijs); margin-top: 8px; }
</style>
</head>
<body>
<div class="wrap">
  <h1>Draadloze communicatie</h1>

  <div class="kaart">
    <span id="verbinding" class="badge">Verbinden...</span>
    <div id="melding"></div>
  </div>

  <div class="kaart">
    <h2>Telefoon &rarr; ESP32</h2>
    <button id="bLed" onclick="zetLed()">LED aanzetten</button>
    <div class="hint">De LED op D12 volgt deze knop.</div>
  </div>

  <div class="kaart">
    <h2>ESP32 &rarr; telefoon</h2>
    <div>Fysieke knop: <span id="stand" class="badge">los</span></div>
    <div style="margin-top:8px">Aantal keer ingedrukt: <span id="teller" class="groot">0</span></div>
    <div style="margin-top:8px">Uptime: <span id="uptime">-</span> s</div>
    <div class="hint">Wordt elke 500 ms ververst.</div>
  </div>
</div>

<script>
const $ = id => document.getElementById(id);
let ledAan = false;

async function api(pad) {
  try {
    const r = await fetch(pad);
    $('melding').textContent = await r.text();
    status();
  } catch (e) {
    $('melding').textContent = 'Geen verbinding met de ESP32.';
  }
}

function zetLed() {
  api('/led?aan=' + (ledAan ? 0 : 1));
}

async function status() {
  try {
    const r = await fetch('/status');
    const s = await r.json();

    $('verbinding').textContent = 'Verbonden';
    $('verbinding').className = 'badge ok';

    ledAan = s.led === 1;
    $('bLed').textContent = ledAan ? 'LED uitzetten' : 'LED aanzetten';

    $('stand').textContent = s.ingedrukt ? 'ingedrukt' : 'los';
    $('stand').className = 'badge' + (s.ingedrukt ? ' ok' : '');
    $('teller').textContent = s.knop;
    $('uptime').textContent = s.up;
  } catch (e) {
    $('verbinding').textContent = 'Geen verbinding';
    $('verbinding').className = 'badge';
  }
}

setInterval(status, 500);
status();
</script>
</body>
</html>
)rawliteral";

// ===== WEBSERVER =====
void handleRoot() {
  server.send_P(200, "text/html", PAGINA);
}

void handleStatus() {
  String j = "{";
  j += "\"led\":" + String(ledAan ? 1 : 0);
  j += ",\"ingedrukt\":" + String(digitalRead(KNOP) == LOW ? 1 : 0);
  j += ",\"knop\":" + String(aantalDrukken);
  j += ",\"up\":" + String(millis() / 1000);
  j += "}";

  server.send(200, "application/json", j);
}

void handleLed() {
  if (server.hasArg("aan")) {
    ledAan = server.arg("aan").toInt() == 1;
    digitalWrite(LED, ledAan ? HIGH : LOW);
  }
  server.send(200, "text/plain", ledAan ? "LED staat aan." : "LED staat uit.");
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
  pinMode(KNOP, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  attachInterrupt(digitalPinToInterrupt(KNOP), knopISR, FALLING);

  // WiFi access point + webserver
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_NAAM, WIFI_WACHTWOORD);

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/led", handleLed);
  server.begin();
}

// ===== LOOP =====
void loop() {
  server.handleClient();

  if (knopVlag) {
    knopVlag = false;
    aantalDrukken++;
  }

  delay(2);
}
