#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define PIN       D6
#define NUMPIXELS 50

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);
ESP8266WebServer server(80);

// ================= WIFI - CHANGE THESE =================
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ================= MODES =================
#define MODE_RANDOM 0
#define MODE_TEXT   1
#define MODE_LIVE   2
int currentMode = MODE_RANDOM;

// ================= TEXT SPELLING =================
String spellWord  = "";
int    spellIndex = 0;
unsigned long lastSpellTime = 0;
bool   spellGap   = false;
#define LETTER_ON_TIME  900
#define LETTER_GAP_TIME 300

// ================= RANDOM =================
unsigned long lastRandomTime = 0;
bool randomVisible = false;

// ================= AMBIENT =================
int ambientBrightness = 20;
int ambientDir        = 1;
unsigned long lastAmbientTime  = 0;
unsigned long lastAmbientShift = 0;

uint32_t ambientColors[] = {
  pixels.Color(80,  0,  0),
  pixels.Color(60,  0, 20),
  pixels.Color(40,  0, 40),
  pixels.Color(20,  0, 60),
  pixels.Color(0,   0, 80),
  pixels.Color(0,  40, 40),
  pixels.Color(60, 20,  0),
  pixels.Color(0,  60,  0),
};
#define NUM_AMBIENT_COLORS 8
int ambientColorIndex[25];

// ================= LIGHTNING =================
unsigned long lastLightningTime = 0;
unsigned long nextLightningIn   = 8000;
bool lightningActive = false;
unsigned long lightningEnd = 0;

// ================================================
//  HTML PAGE
// ================================================
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
<title>The Wall</title>
<style>
  :root {
    --red:    #c0392b;
    --red2:   #ff2400;
    --darker: #050000;
    --dim:    #1a0000;
    --glow:   0 0 10px #c0392b, 0 0 30px #c0392b44;
    --glow2:  0 0 6px #ff240088;
  }
  * { margin:0; padding:0; box-sizing:border-box; }
  html, body {
    background: var(--darker);
    color: var(--red);
    font-family: 'Courier New', Courier, monospace;
    min-height: 100vh;
    overflow-x: hidden;
  }

  /* scanlines */
  body::before {
    content:'';
    position:fixed; inset:0;
    background: repeating-linear-gradient(
      0deg, transparent, transparent 2px,
      rgba(0,0,0,0.1) 2px, rgba(0,0,0,0.1) 4px
    );
    pointer-events:none; z-index:999;
  }

  .container {
    max-width:480px; margin:0 auto;
    padding:24px 16px 40px;
    position:relative; z-index:1;
  }

  /* ---- HEADER ---- */
  .header { text-align:center; margin-bottom:28px; }

  .title {
    font-family: 'Courier New', monospace;
    font-size: clamp(2rem, 10vw, 3.2rem);
    font-weight: bold;
    color: var(--red2);
    text-shadow: var(--glow);
    letter-spacing: 0.18em;
    text-transform: uppercase;
    animation: titleFlicker 8s infinite;
    line-height: 1;
  }

  .subtitle {
    font-size:0.6rem; letter-spacing:0.35em;
    color:#660000; margin-top:8px; text-transform:uppercase;
  }

  @keyframes titleFlicker {
    0%,100%{opacity:1;text-shadow:var(--glow);}
    92%{opacity:1;}
    93%{opacity:0.2;text-shadow:none;}
    94%{opacity:1;text-shadow:var(--glow);}
    96%{opacity:1;}
    97%{opacity:0.15;text-shadow:none;}
    98%{opacity:1;text-shadow:var(--glow);}
  }

  /* ---- STATUS ---- */
  .status-bar {
    border:1px solid #3a0000; background:var(--dim);
    border-radius:4px; padding:10px 14px; margin-bottom:22px;
    display:flex; align-items:center; gap:10px;
    font-size:0.7rem; letter-spacing:0.15em;
    text-transform:uppercase; color:#993300;
  }
  .pulse-dot {
    width:8px; height:8px; border-radius:50%;
    background:var(--red2); box-shadow:var(--glow2);
    animation:pulseDot 1.4s ease-in-out infinite; flex-shrink:0;
  }
  @keyframes pulseDot{
    0%,100%{opacity:1;transform:scale(1);}
    50%{opacity:0.3;transform:scale(0.5);}
  }

  /* ---- SECTION LABEL ---- */
  .section-label {
    font-size:0.55rem; letter-spacing:0.4em;
    text-transform:uppercase; color:#550000; margin-bottom:10px;
  }

  /* ---- MODE BUTTONS ---- */
  .mode-row { display:flex; gap:8px; margin-bottom:22px; }
  .mode-btn {
    flex:1; padding:12px 4px;
    background:var(--dim); border:1px solid #3a0000; border-radius:4px;
    color:#883300; font-family:'Courier New',monospace;
    font-size:0.65rem; letter-spacing:0.2em; text-transform:uppercase;
    cursor:pointer; transition:all 0.15s;
  }
  .mode-btn.active {
    border-color:var(--red2); color:var(--red2);
    text-shadow:var(--glow2);
    box-shadow:inset 0 0 10px #c0392b22, 0 0 8px #c0392b44;
  }
  .mode-btn:active { opacity:0.7; transform:scale(0.97); }

  /* ---- ALPHABET GRID ---- */
  .alphabet-display {
    display:grid; grid-template-columns:repeat(13,1fr);
    gap:3px; margin-bottom:22px; padding:10px;
    border:1px solid #1a0000; border-radius:4px;
    background:#030000;
  }
  .alpha-led {
    aspect-ratio:1; border-radius:50%;
    background:#0f0000; border:1px solid #1a0000;
    display:flex; align-items:center; justify-content:center;
    font-size:0.42rem; color:#2a0000;
    transition:all 0.1s; letter-spacing:0;
  }
  .alpha-led.lit {
    background:radial-gradient(circle,#ff2400 0%,#8b0000 100%);
    border-color:var(--red2); color:#fff;
    box-shadow:0 0 6px var(--red2), 0 0 14px #c0392b99;
    animation:ledPop 0.15s ease-out;
  }
  @keyframes ledPop{
    0%{transform:scale(0.7);}
    60%{transform:scale(1.2);}
    100%{transform:scale(1);}
  }

  .divider { border:none; border-top:1px solid #1a0000; margin:18px 0; }

  /* ---- INPUT ---- */
  .wall-input {
    width:100%; background:var(--dim);
    border:1px solid #3a0000; border-radius:4px;
    padding:13px 14px; color:var(--red2);
    font-family:'Courier New',monospace;
    font-size:1rem; letter-spacing:0.25em;
    text-transform:uppercase; outline:none;
    caret-color:var(--red2); margin-bottom:10px;
    transition:border-color 0.2s, box-shadow 0.2s;
  }
  .wall-input::placeholder { color:#3a0000; }
  .wall-input:focus {
    border-color:var(--red);
    box-shadow:0 0 0 2px #c0392b22, inset 0 0 8px #c0392b11;
  }

  .send-btn {
    width:100%; padding:13px;
    background:#120000; border:1px solid var(--red); border-radius:4px;
    color:var(--red2); font-family:'Courier New',monospace;
    font-size:0.85rem; font-weight:bold;
    letter-spacing:0.3em; text-transform:uppercase;
    cursor:pointer; text-shadow:var(--glow2);
    box-shadow:0 0 10px #c0392b33; transition:all 0.15s;
  }
  .send-btn:active {
    background:#2a0000;
    box-shadow:0 0 20px #c0392b77;
    transform:scale(0.98);
  }

  /* ---- KEYBOARD ---- */
  .keyboard { display:flex; flex-direction:column; gap:5px; margin-bottom:16px; }
  .key-row { display:flex; justify-content:center; gap:4px; }
  .key {
    width:100%; max-width:40px; aspect-ratio:1;
    background:var(--dim); border:1px solid #3a0000; border-radius:4px;
    color:#883300; font-family:'Courier New',monospace;
    font-size:0.8rem; cursor:pointer; transition:all 0.1s;
    display:flex; align-items:center; justify-content:center;
    -webkit-tap-highlight-color:transparent; user-select:none;
  }
  .key:active, .key.pressed {
    background:#3a0000; border-color:var(--red2);
    color:var(--red2); text-shadow:var(--glow2);
    box-shadow:0 0 8px #c0392b66; transform:scale(0.9);
  }

  /* ---- LIGHTNING ---- */
  .lightning-btn {
    width:100%; padding:12px;
    background:#00001a; border:1px solid #000077; border-radius:4px;
    color:#8888ff; font-family:'Courier New',monospace;
    font-size:0.7rem; letter-spacing:0.3em; text-transform:uppercase;
    cursor:pointer; transition:all 0.15s; margin-bottom:16px;
  }
  .lightning-btn:active {
    background:#00003a;
    box-shadow:0 0 20px #8888ff88;
  }

  /* ---- IP BOX ---- */
  .ip-box {
    text-align:center; font-size:0.58rem;
    letter-spacing:0.2em; color:#3a0000;
    padding:8px; border:1px solid #150000; border-radius:4px;
  }

  /* ---- TOAST ---- */
  .toast {
    position:fixed; bottom:24px; left:50%;
    transform:translateX(-50%) translateY(80px);
    background:#1a0000; border:1px solid var(--red); border-radius:4px;
    padding:10px 20px; font-size:0.68rem;
    letter-spacing:0.2em; text-transform:uppercase;
    color:var(--red2); text-shadow:var(--glow2);
    transition:transform 0.3s cubic-bezier(.34,1.56,.64,1);
    z-index:1000; white-space:nowrap;
  }
  .toast.show { transform:translateX(-50%) translateY(0); }
</style>
</head>
<body>
<div class="container">

  <div class="header">
    <div class="title">THE WALL</div>
    <div class="subtitle">Hawkins, Indiana &nbsp;&middot;&nbsp; 1983</div>
  </div>

  <div class="status-bar">
    <div class="pulse-dot"></div>
    <div id="statusText">Wall Active...</div>
  </div>

  <div class="section-label">// Select Mode</div>
  <div class="mode-row">
    <button class="mode-btn active" id="btnRandom" onclick="setMode('random')">Random</button>
    <button class="mode-btn"        id="btnLive"   onclick="setMode('live')">Live Key</button>
    <button class="mode-btn"        id="btnText"   onclick="setMode('text')">Spell</button>
  </div>

  <div class="section-label">// Wall Status</div>
  <div class="alphabet-display" id="alphaDisplay"></div>

  <hr class="divider">

  <div id="textSection">
    <div class="section-label">// Spell a Word</div>
    <input class="wall-input" id="wordInput" type="text"
           placeholder="TYPE HERE..." maxlength="25"
           oninput="this.value=this.value.toUpperCase()"
           onkeydown="if(event.key==='Enter')sendText()">
    <button class="send-btn" onclick="sendText()">&#9654; TRANSMIT</button>
    <br><br>
  </div>

  <div id="liveSection" style="display:none">
    <div class="section-label">// Live Keypad</div>
    <div class="keyboard" id="keyboard"></div>
  </div>

  <hr class="divider">

  <button class="lightning-btn" onclick="triggerLightning()">
    &#9889; &nbsp; TRIGGER LIGHTNING
  </button>

  <div class="ip-box" id="ipBox">Loading...</div>
</div>

<div class="toast" id="toast"></div>

<script>
  const LETTERS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';

  // Build alphabet LED grid
  const alphaEl = document.getElementById('alphaDisplay');
  LETTERS.split('').forEach(l => {
    const d = document.createElement('div');
    d.className = 'alpha-led';
    d.id = 'led_' + l;
    d.textContent = l;
    alphaEl.appendChild(d);
  });

  // Build keyboard
  const rows = ['QWERTYUIOP','ASDFGHJKL','ZXCVBNM'];
  const kb = document.getElementById('keyboard');
  rows.forEach(row => {
    const rowEl = document.createElement('div');
    rowEl.className = 'key-row';
    row.split('').forEach(l => {
      const k = document.createElement('button');
      k.className='key'; k.textContent=l; k.id='key_'+l;
      k.onclick = () => sendLive(l);
      rowEl.appendChild(k);
    });
    kb.appendChild(rowEl);
  });

  function setMode(mode) {
    document.getElementById('textSection').style.display = (mode==='text') ? 'block' : 'none';
    document.getElementById('liveSection').style.display = (mode==='live') ? 'block' : 'none';
    ['Random','Live','Text'].forEach(m => {
      document.getElementById('btn'+m).classList.toggle('active', mode===m.toLowerCase());
    });
    clearLeds();
    fetch('/mode?m='+mode)
      .then(r=>r.text()).then(setStatus)
      .catch(()=>setStatus('Connection error'));
  }

  function sendText() {
    const w = document.getElementById('wordInput').value.trim();
    if (!w) { showToast('Type something!'); return; }
    clearLeds();
    spellOnUI(w);
    fetch('/text?w='+encodeURIComponent(w))
      .then(r=>r.text()).then(setStatus)
      .catch(()=>setStatus('Error'));
  }

  function sendLive(letter) {
    clearLeds(); lightLed(letter);
    const k = document.getElementById('key_'+letter);
    if (k) { k.classList.add('pressed'); setTimeout(()=>k.classList.remove('pressed'),300); }
    fetch('/live?l='+letter)
      .then(r=>r.text()).then(setStatus)
      .catch(()=>setStatus('Error'));
  }

  function triggerLightning() {
    fetch('/lightning')
      .then(()=>showToast('LIGHTNING!'))
      .catch(()=>setStatus('Error'));
  }

  function clearLeds() {
    LETTERS.split('').forEach(l =>
      document.getElementById('led_'+l).classList.remove('lit'));
  }
  function lightLed(l) {
    const el = document.getElementById('led_'+l);
    if (el) el.classList.add('lit');
  }

  let spellTimer = null;
  function spellOnUI(word) {
    if (spellTimer) clearInterval(spellTimer);
    let i = 0;
    spellTimer = setInterval(() => {
      clearLeds();
      if (i < word.length) { lightLed(word[i]); i++; }
      else { clearInterval(spellTimer); setTimeout(clearLeds,1200); }
    }, 1200);
  }

  function setStatus(txt) { document.getElementById('statusText').textContent = txt; }
  function showToast(msg) {
    const t = document.getElementById('toast');
    t.textContent = msg; t.classList.add('show');
    setTimeout(()=>t.classList.remove('show'), 2200);
  }

  // Fetch and show IP
  fetch('/ip')
    .then(r=>r.text())
    .then(ip => { document.getElementById('ipBox').textContent = 'WALL IP: ' + ip; })
    .catch(()=>{ document.getElementById('ipBox').textContent = 'IP unavailable'; });

  // Init
  setStatus('Wall Active · Ready');
  setMode('random');
</script>
</body>
</html>
)rawhtml";

// ================================================
//  ZIGZAG LETTER MAPPING (ODD = Letters)
// ================================================

// This map translates the alphabet (A-Z) into the physical 
// logical positions along your S-curved LED strip.
const int letterMap[26] = {
  0,  1,  2,  3,  4,  5,  6,  7,   // A B C D E F G H (Left to Right)
  16, 15, 14, 13, 12, 11, 10, 9, 8,// I J K L M N O P Q (Right to Left)
  17, 18, 19, 20, 21, 22, 23, 24, 25 // R S T U V W X Y Z (Left to Right)
};

int letterToLed(char c) {
  c = toupper(c);
  if (c < 'A' || c > 'Z') return -1;
  
  // Get the logical position from our zigzag map
  int logicalPos = letterMap[c - 'A'];
  
  // Constrain to available LEDs (50 pixels = 25 letters max, 0-24 index)
  if (logicalPos > 24) {
    logicalPos = 24; 
  }
  
  return (logicalPos * 2) + 1; // Maps to Odd indices (1, 3, 5...)
}

void clearLetterLeds() {
  for (int i = 1; i < NUMPIXELS; i += 2) pixels.setPixelColor(i, 0);
}
void lightLetter(char c, uint32_t color) {
  int idx = letterToLed(c);
  if (idx >= 0) pixels.setPixelColor(idx, color);
}

// ================================================
//  AMBIENT (EVEN = Ambient)
// ================================================
void updateAmbient() {
  if (lightningActive) return;
  unsigned long now = millis();
  if (now - lastAmbientTime > 50) {
    lastAmbientTime = now;
    for (int i = 0; i < NUMPIXELS; i += 2) { 
      int slot    = i / 2; 
      int flicker = random(100);
      if (flicker > 85) {
        pixels.setPixelColor(i, 0);
      } else if (flicker > 70) {
        int burst = random(6);
        if (burst==0) pixels.setPixelColor(i, pixels.Color(random(100,255),0,0));
        if (burst==1) pixels.setPixelColor(i, pixels.Color(random(80,180),0,random(80,200)));
        if (burst==2) pixels.setPixelColor(i, pixels.Color(0,0,random(100,255)));
        if (burst==3) pixels.setPixelColor(i, pixels.Color(0,random(80,180),random(80,180)));
        if (burst==4) pixels.setPixelColor(i, pixels.Color(random(100,200),random(30,80),0));
        if (burst==5) pixels.setPixelColor(i, pixels.Color(random(50,120),0,random(100,255)));
      } else {
        uint32_t c = ambientColors[ambientColorIndex[slot]];
        uint8_t r = ((c>>16)&0xFF)*ambientBrightness/90;
        uint8_t g = ((c>>8) &0xFF)*ambientBrightness/90;
        uint8_t b = ( c     &0xFF)*ambientBrightness/90;
        pixels.setPixelColor(i, pixels.Color(r,g,b));
      }
    }
    ambientBrightness += ambientDir;
    if (ambientBrightness >= 90 || ambientBrightness <= 10) ambientDir *= -1;
  }
  if (now - lastAmbientShift > (unsigned long)random(200,800)) {
    lastAmbientShift = now;
    for (int j = 0; j < random(2,6); j++)
      ambientColorIndex[random(25)] = random(NUM_AMBIENT_COLORS);
  }
}

// ================================================
//  LIGHTNING (EVEN = Ambient)
// ================================================
void updateLightning() {
  unsigned long now = millis();
  if (!lightningActive && (now - lastLightningTime > nextLightningIn)) {
    lightningActive = true;
    lightningEnd    = now + random(60,250);
    for (int i = 0; i < NUMPIXELS; i += 2)
      pixels.setPixelColor(i, pixels.Color(220,230,255));
  }
  if (lightningActive && millis() > lightningEnd) {
    lightningActive   = false;
    lastLightningTime = millis();
    nextLightningIn   = random(6000,14000);
    for (int i = 0; i < NUMPIXELS; i += 2)
      pixels.setPixelColor(i, pixels.Color(ambientBrightness,0,0));
  }
}

// ================================================
//  TEXT SPELL
// ================================================
void handleTextMode() {
  unsigned long now = millis();
  if (spellIndex >= (int)spellWord.length()) {
    if (now - lastSpellTime > 2000) { currentMode=MODE_RANDOM; clearLetterLeds(); }
    return;
  }
  if (!spellGap) {
    if (now - lastSpellTime < LETTER_ON_TIME) {
      clearLetterLeds();
      lightLetter(spellWord[spellIndex], pixels.Color(255,0,0));
    } else {
      clearLetterLeds(); spellGap=true; lastSpellTime=now;
    }
  } else {
    if (now - lastSpellTime >= LETTER_GAP_TIME) {
      spellIndex++; spellGap=false; lastSpellTime=now;
    }
  }
}

// ================================================
//  RANDOM IDLE
// ================================================
void handleRandomMode() {
  unsigned long now = millis();
  if (now - lastRandomTime > (unsigned long)random(1800,4500)) {
    lastRandomTime = millis();
    clearLetterLeds();
    lightLetter('A' + random(26), pixels.Color(60,0,0));
    randomVisible = true;
  }
  if (randomVisible && now - lastRandomTime > 600) {
    clearLetterLeds(); randomVisible=false;
  }
}

// ================================================
//  WEB ROUTES
// ================================================
void handleRoot()  { server.send_P(200, "text/html", INDEX_HTML); }
void handleIP()    { server.send(200, "text/plain", WiFi.localIP().toString()); }

void handleMode() {
  String m = server.arg("m");
  clearLetterLeds();
  if      (m=="random") { currentMode=MODE_RANDOM; server.send(200,"text/plain","Mode: Random"); }
  else if (m=="live")   { currentMode=MODE_LIVE;   server.send(200,"text/plain","Mode: Live Key · Ready"); }
  else if (m=="text")   { currentMode=MODE_TEXT;   server.send(200,"text/plain","Mode: Spell · Send a word"); }
  else                    server.send(400,"text/plain","Unknown mode");
}

void handleText() {
  String w = server.arg("w");
  w.toUpperCase();
  if (w.length()==0) { server.send(400,"text/plain","No word"); return; }
  currentMode=MODE_TEXT; spellWord=w; spellIndex=0;
  spellGap=false; lastSpellTime=millis();
  clearLetterLeds();
  server.send(200,"text/plain","Spelling: "+w);
}

void handleLive() {
  String l = server.arg("l");
  if (l.length()==0) { server.send(400,"text/plain","No letter"); return; }
  char c = toupper(l[0]);
  clearLetterLeds();
  lightLetter(c, pixels.Color(255,0,0));
  pixels.show();
  server.send(200,"text/plain",String("Lit: ")+c);
}

void handleLightning() {
  lightningActive   = true;
  lightningEnd      = millis() + random(150,400);
  lastLightningTime = millis();
  for (int i = 0; i < NUMPIXELS; i += 2)
    pixels.setPixelColor(i, pixels.Color(220,230,255));
  pixels.show();
  server.send(200,"text/plain","ZAP");
}

// ================================================
//  SETUP
// ================================================
void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(255);
  pixels.clear();
  pixels.show();
  randomSeed(analogRead(A0));

  for (int i = 0; i < 25; i++)
    ambientColorIndex[i] = random(NUM_AMBIENT_COLORS);

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n=============================");
    Serial.println("      CONNECTED!");
    Serial.print("   Open on phone: http://");
    Serial.println(WiFi.localIP());
    Serial.println("=============================");
  } else {
    Serial.println("\n=============================");
    Serial.println("   FAILED TO CONNECT!");
    Serial.println("   Check SSID / password");
    Serial.println("   Must be 2.4GHz network");
    Serial.println("=============================");
  }

  server.on("/",          handleRoot);
  server.on("/ip",        handleIP);
  server.on("/mode",      handleMode);
  server.on("/text",      handleText);
  server.on("/live",      handleLive);
  server.on("/lightning", handleLightning);
  server.begin();
  Serial.println("Web server started.");
}

// ================================================
//  LOOP
// ================================================
void loop() {
  server.handleClient();
  updateAmbient();
  updateLightning();

  if      (currentMode == MODE_TEXT)   handleTextMode();
  else if (currentMode == MODE_RANDOM) handleRandomMode();

  pixels.show();

  // Print IP every 5 seconds
  static unsigned long lastIPPrint = 0;
  if (millis() - lastIPPrint > 5000) {
    lastIPPrint = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi disconnected!");
    }
  }
}