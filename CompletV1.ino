#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <esp32-hal-ledc.h> // Explicitly include for LEDC functions

// Configuration du point d'accès WiFi
const char* ap_ssid = "Machine à plantes";
const char* ap_password = "12345678"; // Minimum 8 caractères

// Serveur web sur le port 80
WebServer server(80);

// Structure pour stocker un profil
struct Profile {
  char name[32];
  int humidity;    // 0-10 (valeur du slider, sera mappée si nécessaire pour la logique de la pompe)
  int brightness;  // 0-24
};

// Structure pour l'assignation des profils aux plantes
struct PlantAssignment {
  int profileIndex; // -1 pour profil vide, 0-49 pour les profils configurés
  char plantName[32]; // Nom personnalisé de la plante (peut être utilisé à l'avenir)
  char lightColor[8]; // Format #RRGGBB, e.g., "#FF0000" for red
};

Profile profiles[50];
PlantAssignment plantsA[5];  // 5 plantes

const int PROFILES_OFFSET = 0;
const int PLANTS_OFFSET = PROFILES_OFFSET + sizeof(profiles);
const int EEPROM_SIZE = PLANTS_OFFSET + sizeof(plantsA);

unsigned long rememberTimeHum=0;
extern volatile unsigned long timer0_millis;
unsigned long ZeroMillis=0;

int Humidité1;
int Humidité2;
int Humidité3;
int Humidité4;
int Humidité5;

const int PatteCapteur1=1;
const int PatteCapteur2=2;
const int PatteCapteur3=3;
const int PatteCapteur4=4;
const int PatteCapteur5=5;

bool Pompe1;
bool Pompe2;
bool Pompe3;
bool Pompe4;
bool Pompe5;

const int PattePompe1=10;
const int PattePompe2=11; // Assuming different pins for different pumps
const int PattePompe3=12;
const int PattePompe4=13;
const int PattePompe5=14;


//Variables de contrôle pour les DELs - Sera adapté pour RGB à l'étape 6
// int DEL1_J;
// int DEL1_R;
// ... autres DELs ...

// Placeholder for RGB LED pins (adjust as per your wiring)
const int LED_R_PIN_1 = 25; // Example pin for Plant 1 Red LED
const int LED_G_PIN_1 = 26; // Example pin for Plant 1 Green LED
const int LED_B_PIN_1 = 27; // Example pin for Plant 1 Blue LED
// Add more pins for other plants if they have separate RGB LEDs

bool Minuit;
bool État_Minuit;

struct PlantManualState {
  bool buttonState;
  int sliderValue;
};
PlantManualState plantManualStates[5];
bool Manuel = false;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadData();

  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Point d'accès WiFi créé!");
  Serial.print("SSID: "); Serial.println(ap_ssid);
  Serial.print("Mot de passe: "); Serial.println(ap_password);
  Serial.print("Adresse IP du point d'accès: "); Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/blue", handleBluePage); // Assignation
  server.on("/red", handleRedPage);   // Mode Manuel
  server.on("/green", handleGreenPage); // Configuration

  server.on("/assign", HTTP_POST, handleAssign);
  server.on("/getPlants", handleGetPlants);
  server.on("/getProfiles", handleGetProfiles);

  server.on("/save", HTTP_POST, handleSave);
  server.on("/get", handleGet);
  server.on("/style.css", handleCSS);

  server.on("/toggle/1", []() { togglePlantManual(0); });
  server.on("/toggle/2", []() { togglePlantManual(1); });
  server.on("/toggle/3", []() { togglePlantManual(2); });
  server.on("/toggle/4", []() { togglePlantManual(3); });
  server.on("/toggle/5", []() { togglePlantManual(4); });

  server.on("/slider", HTTP_GET, handleSliderUpdate);

  server.begin();
  Serial.println("Serveur web démarré");
}

// Page HTML principale avec navigation
const char htmlMainPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Page principale</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); margin: 0; padding: 20px; min-height: 100vh; display: flex; justify-content: center; align-items: center; }
        .container { background: rgba(255, 255, 255, 0.95); border-radius: 20px; padding: 40px; box-shadow: 0 20px 40px rgba(0,0,0,0.1); text-align: center; max-width: 400px; width: 100%; }
        h1 { color: #333; margin-bottom: 30px; font-size: 2.5em; }
        .nav-button { display: block; width: 100%; padding: 15px; margin: 10px 0; border: none; border-radius: 10px; font-size: 1.2em; font-weight: bold; color: white; text-decoration: none; transition: all 0.3s ease; cursor: pointer; }
        .blue-btn { background: linear-gradient(45deg, #4169E1, #1E90FF); }
        .red-btn { background: linear-gradient(45deg, #DC143C, #FF6347); }
        .green-btn { background: linear-gradient(45deg, #32CD32, #228B22); }
        .nav-button:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }
        .info { color: #666; font-size: 0.9em; margin-top: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌱 Page pincipale</h1>
        <a href="/blue" class="nav-button blue-btn">Assignation</a>
        <a href="/green" class="nav-button green-btn">Configuration</a>
        <a href="/red" class="nav-button red-btn">Mode manuel</a>
        <div class="info">
            Machine à Plantes<br>
            <small>Réseau: Machine à plantes | IP: 192.168.4.1</small>
        </div>
    </div>
</body>
</html>
)rawliteral";

// Page HTML pour Mode Manuel
const char htmlRedPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Contrôle des Plantes</title>
    <style>
        body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); margin: 0; padding: 20px; min-height: 100vh; display: flex; flex-direction: column; align-items: center; }
        .container { background: rgba(255, 255, 255, 0.95); border-radius: 20px; padding: 30px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); max-width: 1000px; width: 95%; }
        h1 { text-align: center; color: #333; margin-bottom: 30px; font-size: 2em; text-shadow: 2px 2px 4px rgba(0,0,0,0.1); }
        .plants-grid { position: relative; height: 640px; margin-bottom: 30px; }
        .plant { position: absolute; background: linear-gradient(145deg, #2ecc71, #27ae60); border-radius: 15px; padding: 20px; box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2); width: 250px; transform: translateX(-50%); transition: all 0.3s ease; }
        .plant:hover { transform: translateX(-50%) translateY(-5px); box-shadow: 0 8px 25px rgba(0, 0, 0, 0.3); }
        .plant-1 { bottom: 0; left: 40%; } .plant-2 { bottom: 130px; left: 60%; } .plant-3 { bottom: 260px; left: 40%; } .plant-4 { bottom: 390px; left: 60%; } .plant-5 { bottom: 520px; left: 40%; }
        .plant h3 { color: white; margin: 0 0 15px 0; text-align: center; font-size: 1.3em; text-shadow: 1px 1px 2px rgba(0,0,0,0.3); }
        .controls { display: flex; flex-direction: column; gap: 12px; }
        .control-row { display: flex; align-items: center; gap: 10px; }
        .btn { background: linear-gradient(145deg, #3498db, #2980b9); color: white; border: none; padding: 12px 20px; border-radius: 25px; cursor: pointer; font-size: 14px; font-weight: bold; transition: all 0.3s ease; min-width: 80px; }
        .btn:hover { background: linear-gradient(145deg, #2980b9, #1f6391); transform: translateY(-2px); }
        .btn.active { background: linear-gradient(145deg, #e74c3c, #c0392b); }
        .slider { flex: 1; height: 8px; border-radius: 5px; background: rgba(255, 255, 255, 0.3); outline: none; -webkit-appearance: none; }
        .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 20px; height: 20px; border-radius: 50%; background: white; cursor: pointer; box-shadow: 0 2px 5px rgba(0,0,0,0.3); }
        .slider::-moz-range-thumb { width: 20px; height: 20px; border-radius: 50%; background: white; cursor: pointer; box-shadow: 0 2px 5px rgba(0,0,0,0.3); border: none; }
        .slider-value { color: white; font-weight: bold; min-width: 35px; text-align: center; }
        .status { margin-top: 20px; padding: 15px; background: rgba(52, 152, 219, 0.1); border-radius: 10px; text-align: center; font-weight: bold; color: #2c3e50; }
        .back-btn { background: #666; color: white; padding: 10px 20px; border: none; border-radius: 5px; text-decoration: none; display: inline-block; margin-top: 20px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <a href="/" class="back-btn">← Retour</a>
    <div class="container">
        <h1>🌱 Mode Manuel 🌱 </h1>
        <div class="plants-grid">
            <div class="plant plant-1"><h3>🌿 Plante 1</h3><div class="controls"><div class="control-row"><button class="btn" id="btnPump1" onclick="togglePump(0)">Pompe</button><input type="range" class="slider" id="sliderLight1" min="0" max="100" value="50" oninput="updateLightSlider(0, this.value)"><span class="slider-value" id="valueLight1">50</span></div></div></div>
            <div class="plant plant-2"><h3>🌺 Plante 2</h3><div class="controls"><div class="control-row"><button class="btn" id="btnPump2" onclick="togglePump(1)">Pompe</button><input type="range" class="slider" id="sliderLight2" min="0" max="100" value="50" oninput="updateLightSlider(1, this.value)"><span class="slider-value" id="valueLight2">50</span></div></div></div>
            <div class="plant plant-3"><h3>🌸 Plante 3</h3><div class="controls"><div class="control-row"><button class="btn" id="btnPump3" onclick="togglePump(2)">Pompe</button><input type="range" class="slider" id="sliderLight3" min="0" max="100" value="50" oninput="updateLightSlider(2, this.value)"><span class="slider-value" id="valueLight3">50</span></div></div></div>
            <div class="plant plant-4"><h3>🌻 Plante 4</h3><div class="controls"><div class="control-row"><button class="btn" id="btnPump4" onclick="togglePump(3)">Pompe</button><input type="range" class="slider" id="sliderLight4" min="0" max="100" value="50" oninput="updateLightSlider(3, this.value)"><span class="slider-value" id="valueLight4">50</span></div></div></div>
            <div class="plant plant-5"><h3>🌹 Plante 5</h3><div class="controls"><div class="control-row"><button class="btn" id="btnPump5" onclick="togglePump(4)">Pompe</button><input type="range" class="slider" id="sliderLight5" min="0" max="100" value="50" oninput="updateLightSlider(4, this.value)"><span class="slider-value" id="valueLight5">50</span></div></div></div>
        </div>
        <div class="status" id="status">Mode Manuel - Contrôlez les pompes et lumières.</div>
    </div>
    <script>
        function togglePump(plantIndex) { fetch('/toggle/' + (plantIndex + 1)).then(response => response.text()).then(data => { console.log('Plante ' + (plantIndex+1) + ' pompe: ' + data); document.getElementById('btnPump'+(plantIndex+1)).classList.toggle('active', data === 'ON'); }); }
        function updateLightSlider(plantIndex, value) { document.getElementById('valueLight' + (plantIndex + 1)).textContent = value; fetch('/slider?plant=' + (plantIndex + 1) + '&value=' + value); }
        window.onload = function() { const plants = document.querySelectorAll('.plant'); plants.forEach((plant, index) => { plant.style.opacity='0'; plant.style.transform='translateX(-50%) translateY(20px)'; setTimeout(() => { plant.style.transition='all 0.6s ease'; plant.style.opacity='1'; plant.style.transform='translateX(-50%) translateY(0)';}, index * 150); }); /* TODO: Fetch initial states */ };
    </script>
</body>
</html>
)rawliteral";

void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();

  // Plant watering cycle logic
  static int currentPlantToWater = 0;
  static unsigned long lastWaterCheckTime = 0;

  if (currentMillis - lastWaterCheckTime >= 60000) { // Every 60 seconds
    lastWaterCheckTime = currentMillis;
    switch (currentPlantToWater) {
        case 0: Plante1(); break;
        case 1: Plante2(); break;
        case 2: Plante3(); break;
        case 3: Plante4(); break;
        case 4: Plante5(); break;
    }
    currentPlantToWater = (currentPlantToWater + 1) % 5;
  }

  if (Minuit && (Minuit != État_Minuit)){
    Serial.println("Minuit détecté, logique setMillis désactivée/problématique.");
  }
  État_Minuit = Minuit;

  // Update pump states based on their individual PompeX variables (set in PlanteX or by manual override)
  digitalWrite(PattePompe1, Pompe1);
  digitalWrite(PattePompe2, Pompe2);
  digitalWrite(PattePompe3, Pompe3);
  digitalWrite(PattePompe4, Pompe4);
  digitalWrite(PattePompe5, Pompe5);

  // Light control logic
  if (Manuel) {
    // Manual light control
    for (int i = 0; i < 5; i++) {
      if (i == 0) { // Only plant 1 has LED pins defined for now
        int manualBrightnessSetting = map(plantManualStates[i].sliderValue, 0, 100, 0, 24); // map slider 0-100 to profile brightness 0-24
        // Use last assigned color for manual mode, or a default like white if no color assigned yet.
        const char* colorToUse = (plantsA[i].lightColor[0] == '#' ? plantsA[i].lightColor : "#FFFFFF");
        setPlantLedColor(i, colorToUse, manualBrightnessSetting);
      }
      // Else: Off for other plants or implement their LED controls
      // else { setPlantLedColor(i, "#000000", 0); } // Example: turn others off
    }
  } else {
    // Automatic light control based on profiles
    // Only Plant 1 (index 0) for now
    if (plantsA[0].profileIndex != -1) {
      int profileIdx = plantsA[0].profileIndex;
      if (profileIdx >= 0 && profileIdx < 50) {
          setPlantLedColor(0, plantsA[0].lightColor, profiles[profileIdx].brightness);
      } else {
          setPlantLedColor(0, "#000000", 0); // Turn off if profile is invalid
      }
    } else {
      setPlantLedColor(0, "#000000", 0); // Turn off if no profile assigned
    }
    // Extend for other plants if they have LEDs and profiles
  }
}

// Helper function to parse hex color string to R, G, B values
void parseHexColor(const char* hexColor, int &r, int &g, int &b) {
    long colorValue = strtol(hexColor + 1, NULL, 16); // Skip '#'
    r = (colorValue >> 16) & 0xFF;
    g = (colorValue >> 8) & 0xFF;
    b = colorValue & 0xFF;
}

// PWM Channels (0-15 for ESP32) - Use different channels for each color of each LED if controlled independently.
// For simplicity, let's assign channels for one RGB LED first (Plant 1)
const int PWM_FREQ = 5000; // PWM frequency in Hz
const int PWM_RESOLUTION = 8; // 8-bit resolution (0-255)

// Add more channels for other plants' LEDs if needed

void setupLedPwm() {
    // Plant 1 LED - Using new API: ledcAttach(pin, freq, resolution)
    // Channels are managed internally by the new API.
    ledcAttach(LED_R_PIN_1, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(LED_G_PIN_1, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(LED_B_PIN_1, PWM_FREQ, PWM_RESOLUTION);

    // Setup other plants' LEDs here if they exist
    // e.g., ledcAttach(LED_R_PIN_2, PWM_FREQ, PWM_RESOLUTION);
    //       ledcAttach(LED_G_PIN_2, PWM_FREQ, PWM_RESOLUTION);
    //       ledcAttach(LED_B_PIN_2, PWM_FREQ, PWM_RESOLUTION);
}

// Function to set the color and brightness for a plant's LED
// plantIndex: 0-4
void setPlantLedColor(int plantIndex, const char* hexColor, int brightnessPercent) {
    int r, g, b;
    parseHexColor(hexColor, r, g, b);

    // Adjust RGB values by brightness (0-24 from profile, map to 0-100% for LEDs)
    // Brightness 0 from profile should be OFF, 24 should be max.
    // Let's map 0-24 from profile to 0-255 duty cycle multiplier.
    // float brightnessFactor = map(brightnessPercent, 0, 24, 0, 100) / 100.0;
    // Max brightness from profile is 24. If we treat this as 100% for PWM (255 value)
    // then a brightness of 0 means 0 PWM, 12 means ~50% PWM (127 value)

    // Brightness from profile is 0-24. We can map this to a scale of 0-255 for PWM duty cycle.
    // If brightnessPercent is 0, LED is off. If 24, LED is at color's full intensity.
    if (brightnessPercent == 0) {
        r = 0; g = 0; b = 0;
    } else {
        // Scale RGB by brightness. brightnessPercent (0-24)
        // Let's make brightness=1 map to a dim light, and 24 to full.
        // A simple scaling: multiply by brightnessPercent / 24.0
        float factor = (float)brightnessPercent / 24.0;
        r = (int)(r * factor);
        g = (int)(g * factor);
        b = (int)(b * factor);
    }

    // Ensure values are within 0-255
    r = constrain(r, 0, 255);
    g = constrain(g, 0, 255);
    b = constrain(b, 0, 255);

    if (plantIndex == 0) { // For Plant 1 - Using new API: ledcWrite(pin, value)
        ledcWrite(LED_R_PIN_1, r);
        ledcWrite(LED_G_PIN_1, g);
        ledcWrite(LED_B_PIN_1, b);
    }
    // Add else if for other plants:
    // else if (plantIndex == 1) {
    //   ledcWrite(LED_R_PIN_2, r); // Assuming LED_R_PIN_2 is defined
    //   ...
    // }
    // Serial.printf("Plant %d LED: #%s, Bright: %d%% -> R:%d G:%d B:%d\n", plantIndex+1, hexColor, brightnessPercent, r, g, b);
}


void Plante1() {
  Pompe2=LOW;Pompe3=LOW;Pompe4=LOW;Pompe5=LOW; // Assure que les autres pompes sont éteintes
  if (plantsA[0].profileIndex != -1 && !Manuel) { // Vérifie si un profil est assigné et que le mode manuel n'est pas actif
    Humidité1 = analogRead(PatteCapteur1);
    // TODO: Affiner le mapping pour targetHumidity. La valeur du profil (0-10) doit correspondre à la plage du capteur.
    // Exemple: si 0 est très humide et 10 très sec pour le profil.
    // Et le capteur donne (ex: 4095 sec, 1000 humide).
    // int mappedTarget = map(profiles[plantsA[0].profileIndex].humidity, 0, 10, 1000, 4095);
    int targetHumiditySetting = profiles[plantsA[0].profileIndex].humidity; // 0-10
    // Cette logique suppose que Humidité1 diminue quand c'est plus humide.
    // Et que targetHumiditySetting (0-10) est un seuil où 0 = ne pas arroser, 10 = arroser très tôt.
    // Il faut une conversion claire. Pour l'instant, on garde la logique originale avec un facteur.
    int mappedTarget = targetHumiditySetting * 300; // Valeur arbitraire, à ajuster. (0-3000)
                                                 // Plus la valeur du profil est élevée, plus le seuil d'humidité est bas (plus sec)

    if (Humidité1 > mappedTarget + 200) { // Si c'est plus sec que la consigne + marge
      Pompe1=HIGH;
    } else if (Humidité1 < mappedTarget) { // Si c'est plus humide que la consigne
      Pompe1=LOW;
    } // Sinon, reste dans la bande morte
    if (Humidité1 < 50) Pompe1=LOW; // Si capteur débranché/très humide (valeur très basse)
  } else {
    Pompe1 = Manuel ? plantManualStates[0].buttonState : LOW; // Mode manuel ou pas de profil
  }
}
void Plante2() {
  Pompe1=LOW;Pompe3=LOW;Pompe4=LOW;Pompe5=LOW;
  if (plantsA[1].profileIndex != -1 && !Manuel) {
    Humidité2 = analogRead(PatteCapteur2);
    int targetHumiditySetting = profiles[plantsA[1].profileIndex].humidity;
    int mappedTarget = targetHumiditySetting * 300;
    if (Humidité2 > mappedTarget + 200) { Pompe2=HIGH; }
    else if (Humidité2 < mappedTarget) { Pompe2=LOW; }
    if (Humidité2 < 50) Pompe2=LOW;
  } else {
    Pompe2 = Manuel ? plantManualStates[1].buttonState : LOW;
  }
}
void Plante3() {
  Pompe1=LOW;Pompe2=LOW;Pompe4=LOW;Pompe5=LOW;
   if (plantsA[2].profileIndex != -1 && !Manuel) {
    Humidité3 = analogRead(PatteCapteur3);
    int targetHumiditySetting = profiles[plantsA[2].profileIndex].humidity;
    int mappedTarget = targetHumiditySetting * 300;
    if (Humidité3 > mappedTarget + 200) { Pompe3=HIGH; }
    else if (Humidité3 < mappedTarget) { Pompe3=LOW; }
    if (Humidité3 < 50) Pompe3=LOW;
  } else {
    Pompe3 = Manuel ? plantManualStates[2].buttonState : LOW;
  }
}
void Plante4() {
  Pompe1=LOW;Pompe2=LOW;Pompe3=LOW;Pompe5=LOW;
  if (plantsA[3].profileIndex != -1 && !Manuel) {
    Humidité4 = analogRead(PatteCapteur4);
    int targetHumiditySetting = profiles[plantsA[3].profileIndex].humidity;
    int mappedTarget = targetHumiditySetting * 300;
    if (Humidité4 > mappedTarget + 200) { Pompe4=HIGH; }
    else if (Humidité4 < mappedTarget) { Pompe4=LOW; }
    if (Humidité4 < 50) Pompe4=LOW;
  } else {
    Pompe4 = Manuel ? plantManualStates[3].buttonState : LOW;
  }
}
void Plante5() {
  Pompe1=LOW;Pompe2=LOW;Pompe3=LOW;Pompe4=LOW;
  if (plantsA[4].profileIndex != -1 && !Manuel) {
    Humidité5 = analogRead(PatteCapteur5);
    int targetHumiditySetting = profiles[plantsA[4].profileIndex].humidity;
    int mappedTarget = targetHumiditySetting * 300;
    if (Humidité5 > mappedTarget + 200) { Pompe5=HIGH; }
    else if (Humidité5 < mappedTarget) { Pompe5=LOW; }
    if (Humidité5 < 50) Pompe5=LOW;
  } else {
    Pompe5 = Manuel ? plantManualStates[4].buttonState : LOW;
  }
}

void setMillis(unsigned long new_millis) {
  // uint8_t oldSREG = SREG; // SREG et cli() sont spécifiques à AVR, pas ESP32
  // cli();
  // timer0_millis = new_millis;
  // SREG = oldSREG;
  Serial.println("setMillis function called, but it's not recommended for ESP32.");
}

// --- BEGIN Web Server Handlers and HTML/CSS ---

void handleRoot() {
    server.send(200, "text/html", htmlMainPage);
}

void handleBluePage() { // Assignation Page
  String html = R"(
<!DOCTYPE html>
<html lang='fr'>
<head>
    <meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Attribution des Profils</title><link rel='stylesheet' href='/style.css'>
</head>
<body>
    <a href="/" class="back-btn">← Retour</a>
    <div class='container'>
        <h1>🌱 Attribution des Profils aux Plantes</h1>
        <div class='garden-layout'>
)";
  for (int i = 0; i < 5; ++i) {
    String plant_icons[] = {"🌳", "🪴", "🌱", "🌺", "🌿"};
    html += "<div class='plant-container' id='plant-" + String(i) + "'>";
    html += "<div class='plant-visual'>" + plant_icons[i % 5] + "</div>";
    html += "<div class='plant-info'><h3>Plante " + String(i + 1) + "</h3>";
    html += "<select id='profileSelect-" + String(i) + "' onchange='updatePlantDetails(" + String(i) + ")'><option value='-1'>Aucun profil</option></select>";
    html += "<div class='color-picker-container'>Couleur: <input type='color' id='colorPicker-" + String(i) + "' value='#FFFFFF'></div>";
    html += "<div class='plant-details' id='details-" + String(i) + "'></div>";
    html += "<button onclick='assignProfile(" + String(i) + ")'>Assigner</button>";
    html += "</div></div>";
  }
html += R"(
        </div><div id='message' class='message'></div>
        <div class='control-panel'><h2>📊 Tableau de Bord</h2><div class='stats' id='stats'></div>
            <button onclick='refreshAll()' class='refresh-btn'>🔄 Actualiser Tout</button>
        </div>
    </div>
    <script>
        let profiles_data = []; let plants_data = [];
        function loadProfiles() { fetch('/getProfiles').then(r => r.json()).then(data => { profiles_data = data; updateProfileSelects(); loadPlants(); }).catch(e => showMessage('Erreur chargement profils: ' + e, 'error')); }
        function loadPlants() { fetch('/getPlants').then(r => r.json()).then(data => { plants_data = data; updatePlantDisplay(); updateStats(); }).catch(e => showMessage('Erreur chargement plantes: ' + e, 'error')); }
        function updateProfileSelects() { for(let i=0; i<5; i++) { const sel = document.getElementById('profileSelect-'+i); sel.innerHTML = '<option value="-1">Aucun profil</option>'; profiles_data.forEach((p,idx) => { if(p.name && p.name.trim()!=='') { const opt = document.createElement('option'); opt.value=idx; opt.textContent='Profil '+(idx+1)+' - '+p.name; sel.appendChild(opt); }}); }}
        function updatePlantDisplay() { for(let i=0; i<5; i++) { if(plants_data[i]) { document.getElementById('profileSelect-'+i).value = plants_data[i].profileIndex; document.getElementById('colorPicker-'+i).value = plants_data[i].lightColor || "#FFFFFF"; updatePlantDetails(i); }}}
        function updatePlantDetails(plantIndex) { const p_assign = plants_data[plantIndex]; const detDiv = document.getElementById('details-'+plantIndex); if(p_assign && p_assign.profileIndex >=0 && p_assign.profileIndex < profiles_data.length) { const prof = profiles_data[p_assign.profileIndex]; if(prof.name && prof.name.trim()!=='') { let dets='<div class="detail-item">💧 Hum: '+prof.humidity+'</div><div class="detail-item">💡 Lum: '+prof.brightness+'</div>'; if(p_assign.lightColor) { dets+='<div class="detail-item">🎨 Coul: <span style="background-color:'+p_assign.lightColor+';padding:0 10px;">&nbsp;</span> '+p_assign.lightColor+'</div>';} detDiv.innerHTML=dets; detDiv.style.display='block'; } else {detDiv.style.display='none';} } else {detDiv.style.display='none';}}
        function assignProfile(plantIdx) { const profIdx = parseInt(document.getElementById('profileSelect-'+plantIdx).value); const colorVal = document.getElementById('colorPicker-'+plantIdx).value; const formData = new FormData(); formData.append('plant', plantIdx); formData.append('profileIndex', profIdx); formData.append('color', colorVal); fetch('/assign',{method:'POST',body:formData}).then(r=>r.text()).then(d=>{ if(d==='OK'){ if(!plants_data[plantIdx]) plants_data[plantIdx]={}; plants_data[plantIdx].profileIndex=profIdx; plants_data[plantIdx].lightColor=colorVal; updatePlantDetails(plantIdx); updateStats(); showMessage('✅ Plante '+(plantIdx+1)+' configurée!', 'success');} else {showMessage('❌ Erreur attribution', 'error');}}).catch(e => showMessage('❌ Erreur: '+e, 'error'));}
        function updateStats() { const statsDiv = document.getElementById('stats'); let assigned=0; let html=''; for(let i=0; i<5; i++) { const p_assign=plants_data[i]; html+='<div class="stat-item"><strong>Plante '+(i+1)+':</strong> '; if(p_assign && p_assign.profileIndex >=0 && p_assign.profileIndex < profiles_data.length){ const prof=profiles_data[p_assign.profileIndex]; if(prof.name && prof.name.trim()!==''){ html+=prof.name+' (H:'+prof.humidity+', L:'+prof.brightness+')'; if(p_assign.lightColor) {html+=' <span style="background-color:'+p_assign.lightColor+';padding:0 5px;">&nbsp;</span>';} assigned++; } else {html+='Profil vide';}} else {html+='Non assignée';} html+='</div>';} html='<div class="summary">📈 '+assigned+'/5 plantes configurées</div>'+html; statsDiv.innerHTML=html;}
        function refreshAll() { loadProfiles(); /* loadPlants() is called by loadProfiles's success */ showMessage('🔄 Données actualisées', 'info'); }
        function showMessage(text, type) { const msgDiv=document.getElementById('message'); msgDiv.textContent=text; msgDiv.className='message '+type; msgDiv.style.display='block'; setTimeout(()=>{msgDiv.style.display='none';},3000);}
        window.onload = loadProfiles;
    </script>
</body></html>
)";
  server.send(200, "text/html", html);
}

void handleRedPage() { // Mode Manuel page
    server.send(200, "text/html", htmlRedPage);
}

void handleGreenPage() { // Configuration Page
   String html = R"(
<!DOCTYPE html>
<html lang='fr'>
<head>
    <meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>🌱 Configuration des Plantes</title><link rel='stylesheet' href='/style.css'>
</head>
<body>
    <a href="/" class="back-btn">← Retour</a>
    <div class='container'><h1>🌱 Configuration des Profils</h1>
        <div class='profile-selector'><label for='profileSelectCfg'>Sélectionner un profil:</label>
            <select id='profileSelectCfg' onchange='loadProfileForEdit()'>
)";
  for(int i = 0; i < 50; i++) { html += "<option value='" + String(i) + "'>Profil " + String(i + 1) + "</option>"; }
html += R"(
            </select>
        </div>
        <form id='profileForm'>
            <div class='form-group'><label for='profileName'>Nom de la plante:</label><input type='text' id='profileName' maxlength='31' placeholder='Entrez le nom'></div>
            <div class='form-group'><label for='humidity'>Humidité: <span id='humidityValue'>5</span></label><input type='range' id='humidity' min='0' max='10' value='5' oninput='updateSliderValue("humidity", "humidityValue")'><div class='slider-labels'><span>0</span><span>10</span></div></div>
            <div class='form-group'><label for='brightness'>Luminosité: <span id='brightnessValue'>12</span></label><input type='range' id='brightness' min='0' max='24' value='12' oninput='updateSliderValue("brightness", "brightnessValue")'><div class='slider-labels'><span>0</span><span>24</span></div></div>
            <div class='buttons'><button type='button' onclick='saveProfileData()'>Sauvegarder Profil</button><button type='button' onclick='loadProfileForEdit()'>Recharger</button></div>
        </form>
        <div id='messageCfg' class='message'></div>
        <div class='profile-list'><h2>Liste des Profils Configurés 🌱</h2><div id='profilesList'></div></div>
    </div>
    <script>
        let currentProfileIdx = 0;
        function updateSliderValue(sliderId, valueId) { document.getElementById(valueId).textContent = document.getElementById(sliderId).value; }
        function loadProfileForEdit() { currentProfileIdx = parseInt(document.getElementById('profileSelectCfg').value); fetch('/get?profile='+currentProfileIdx).then(r=>r.json()).then(d=>{document.getElementById('profileName').value=d.name||''; document.getElementById('humidity').value=d.humidity||5; document.getElementById('brightness').value=d.brightness||12; updateSliderValue('humidity','humidityValue'); updateSliderValue('brightness','brightnessValue'); showCfgMessage('Profil '+(currentProfileIdx+1)+' chargé','success');}).catch(e=>showCfgMessage('Erreur chargement: '+e,'error'));}
        function saveProfileData() { const name=document.getElementById('profileName').value; const hum=document.getElementById('humidity').value; const bright=document.getElementById('brightness').value; if(!name.trim()){showCfgMessage('Nom requis','error');return;} const fd=new FormData(); fd.append('profile',currentProfileIdx); fd.append('name',name); fd.append('humidity',hum); fd.append('brightness',bright); fetch('/save',{method:'POST',body:fd}).then(r=>r.text()).then(d=>{showCfgMessage('Profil '+(currentProfileIdx+1)+' sauvegardé!','success'); updateProfileSelectorOptions(); loadProfilesList();}).catch(e=>showCfgMessage('Erreur sauvegarde: '+e,'error'));}
        function showCfgMessage(text,type){const m=document.getElementById('messageCfg');m.textContent=text;m.className='message '+type;m.style.display='block';setTimeout(()=>{m.style.display='none';},3000);}
        function updateProfileSelectorOptions(){ const sel=document.getElementById('profileSelectCfg'); const curVal=sel.value; sel.innerHTML=''; for(let i=0;i<50;i++){fetch('/get?profile='+i).then(r=>r.json()).then(d=>{const opt=document.createElement('option');opt.value=i; opt.textContent='Profil '+(i+1)+(d.name&&d.name.trim()!==''?': '+d.name:''); sel.appendChild(opt); if(i==curVal)sel.value=curVal;});}}
        function loadProfilesList() { let listHtml=''; let processed=0; for(let i=0;i<50;i++){fetch('/get?profile='+i).then(r=>r.json()).then(d=>{processed++; if(d.name&&d.name.trim()!==''){listHtml+='<div class="profile-item"><strong>Profil '+(i+1)+':</strong> '+d.name+' (H:'+d.humidity+', L:'+d.brightness+')</div>';} if(processed===50)document.getElementById('profilesList').innerHTML=listHtml||'<p>Aucun profil.</p>';});}}
        window.onload=()=>{updateProfileSelectorOptions(); setTimeout(()=>{loadProfileForEdit(); loadProfilesList();},500);};
    </script>
</body></html>
)";
  server.send(200, "text/html", html);
}

void togglePlantManual(int plantIndex) {
  plantManualStates[plantIndex].buttonState = !plantManualStates[plantIndex].buttonState;
  Serial.printf("Plante %d (idx %d) pompe manuelle: %s\n", plantIndex+1, plantIndex, plantManualStates[plantIndex].buttonState ? "ON" : "OFF");
  server.send(200, "text/plain", plantManualStates[plantIndex].buttonState ? "ON" : "OFF");
}

void updateSlider(int plantIndex, int value) { // For manual mode light slider
  plantManualStates[plantIndex].sliderValue = value;
  Serial.printf("Plante %d (idx %d) slider manuel: %d\n", plantIndex+1, plantIndex, value);
  server.send(200, "text/plain", "OK");
}

void handleSliderUpdate() {
  if (server.hasArg("plant") && server.hasArg("value")) {
    int plantIndex = server.arg("plant").toInt() - 1;
    int value = server.arg("value").toInt();
    if (plantIndex >= 0 && plantIndex < 5) {
      updateSlider(plantIndex, value);
    } else { server.send(400, "text/plain", "Index plante invalide"); }
  } else { server.send(400, "text/plain", "Paramètres manquants"); }
}

void handleSave() { // Save profile configuration
  if (server.hasArg("profile") && server.hasArg("name") && server.hasArg("humidity") && server.hasArg("brightness")) {
    int profileIndex = server.arg("profile").toInt();
    if (profileIndex >= 0 && profileIndex < 50) {
      strncpy(profiles[profileIndex].name, server.arg("name").c_str(), 31);
      profiles[profileIndex].name[31] = '\0';
      profiles[profileIndex].humidity = server.arg("humidity").toInt();
      profiles[profileIndex].brightness = server.arg("brightness").toInt();
      if (profiles[profileIndex].humidity < 0) profiles[profileIndex].humidity = 0;
      if (profiles[profileIndex].humidity > 10) profiles[profileIndex].humidity = 10;
      if (profiles[profileIndex].brightness < 0) profiles[profileIndex].brightness = 0;
      if (profiles[profileIndex].brightness > 24) profiles[profileIndex].brightness = 24;
      saveProfiles();
      Serial.printf("Profil %d sauvegardé: %s (H:%d, L:%d)\n", profileIndex + 1, profiles[profileIndex].name, profiles[profileIndex].humidity, profiles[profileIndex].brightness);
      server.send(200, "text/plain", "OK");
    } else { server.send(400, "text/plain", "Index profil invalide"); }
  } else { server.send(400, "text/plain", "Paramètres manquants"); }
}

void handleGet() { // Get specific profile data
  if (server.hasArg("profile")) {
    int profileIndex = server.arg("profile").toInt();
    if (profileIndex >= 0 && profileIndex < 50) {
      String json = "{";
      json += "\"name\":\"" + String(profiles[profileIndex].name) + "\",";
      json += "\"humidity\":" + String(profiles[profileIndex].humidity) + ",";
      json += "\"brightness\":" + String(profiles[profileIndex].brightness);
      json += "}";
      server.send(200, "application/json", json);
    } else { server.send(400, "text/plain", "Index profil invalide"); }
  } else { server.send(400, "text/plain", "Paramètre profil manquant"); }
}

// Fonctions pour Assignations
void handleAssign() {
  if (server.hasArg("plant") && server.hasArg("profileIndex") && server.hasArg("color")) {
    int plantIndex = server.arg("plant").toInt(); // Expecting 0-4 from JavaScript
    int profileIndex = server.arg("profileIndex").toInt();
    String colorStr = server.arg("color");

    if (plantIndex >= 0 && plantIndex < 5) {
      plantsA[plantIndex].profileIndex = profileIndex;
      strncpy(plantsA[plantIndex].lightColor, colorStr.c_str(), 7);
      plantsA[plantIndex].lightColor[7] = '\0'; // Ensure null termination

      savePlants(); // Save the updated plantsA array to EEPROM

      Serial.printf("Plante %d (JS index %d) assignée au profil %d, Couleur: %s\n",
                    plantIndex + 1, plantIndex, profileIndex, plantsA[plantIndex].lightColor);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Index de plante invalide.");
    }
  } else {
    server.send(400, "text/plain", "Paramètres manquants. 'plant', 'profileIndex', et 'color' sont requis.");
  }
}

void handleGetPlants() {
  String json = "[";
  for (int i = 0; i < 5; i++) {
    if (i > 0) {
      json += ",";
    }
    json += "{";
    json += "\"plantName\":\"" + String(plantsA[i].plantName) + "\",";
    json += "\"profileIndex\":" + String(plantsA[i].profileIndex) + ",";
    // Ensure lightColor is initialized before sending
    if (plantsA[i].lightColor[0] == '\0' || plantsA[i].lightColor[0] == (char)255 || plantsA[i].lightColor[0] != '#') {
        json += "\"lightColor\":\"#FFFFFF\""; // Default to white if not set or invalid
    } else {
        json += "\"lightColor\":\"" + String(plantsA[i].lightColor) + "\"";
    }
    json += "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleGetProfiles() {
  String json = "[";
  for (int i = 0; i < 50; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"name\":\"" + String(profiles[i].name) + "\",";
    json += "\"humidity\":" + String(profiles[i].humidity) + ",";
    json += "\"brightness\":" + String(profiles[i].brightness);
    json += "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void saveProfiles() {
  EEPROM.put(PROFILES_OFFSET, profiles);
  EEPROM.commit();
}

void savePlants() {
  EEPROM.put(PLANTS_OFFSET, plantsA);
  EEPROM.commit();
}

void loadData() {
  EEPROM.get(PROFILES_OFFSET, profiles);
  EEPROM.get(PLANTS_OFFSET, plantsA);

  for (int i = 0; i < 50; i++) {
    bool isEmptyName = true;
    for (int j = 0; j < 32; j++) {
      if (profiles[i].name[j] != 0 && profiles[i].name[j] != (char)255) {
        isEmptyName = false;
        break;
      }
    }
    bool isLikelyUninitialized = profiles[i].humidity < 0 || profiles[i].humidity > 10 ||
                               profiles[i].brightness < 0 || profiles[i].brightness > 24;
    if (isEmptyName || isLikelyUninitialized) {
      memset(profiles[i].name, 0, 32);
      profiles[i].humidity = 5;
      profiles[i].brightness = 12;
    }
  }

  for (int i = 0; i < 5; i++) {
    bool nameIsEmpty = true;
    for (int j = 0; j < 32; j++) {
      if (plantsA[i].plantName[j] != 0 && plantsA[i].plantName[j] != (char)255) {
        nameIsEmpty = false;
        break;
      }
    }
    if (nameIsEmpty || plantsA[i].profileIndex < -1 || plantsA[i].profileIndex >= 50) {
      memset(plantsA[i].plantName, 0, 32);
      plantsA[i].profileIndex = -1;
    }
    if (plantsA[i].lightColor[0] == '\0' || plantsA[i].lightColor[0] == (char)255 || plantsA[i].lightColor[0] != '#') {
        strncpy(plantsA[i].lightColor, "#FFFFFF", 7);
        plantsA[i].lightColor[7] = '\0';
    }
  }
}

void handleCSS() {
  String css = R"(
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif; background:linear-gradient(135deg,#4CAF50 0%,#8BC34A 50%,#CDDC39 100%); min-height:100vh; padding:20px; }
.container { max-width:1200px; margin:0 auto; background:rgba(255,255,255,0.95); border-radius:20px; box-shadow:0 25px 40px rgba(0,0,0,0.1); padding:30px; backdrop-filter:blur(10px); }
h1 { text-align:center; color:#2E7D32; margin-bottom:30px; font-size:2.5em; text-shadow:2px 2px 4px rgba(0,0,0,0.1); }
h2 { color:#388E3C; margin-bottom:20px; font-size:1.8em; }
.garden-layout { display:flex; flex-direction:column; gap:20px; margin-bottom:30px; }
.plant-container { background:white; border-radius:15px; padding:15px; box-shadow:0 8px 25px rgba(0,0,0,0.1); transition:all 0.3s ease; border:3px solid #E8F5E8; min-width:250px; }
.plant-container:hover { transform:translateY(-5px); box-shadow:0 15px 35px rgba(0,0,0,0.15); border-color:#4CAF50; }
.plant-visual { text-align:center; font-size:2.5em; margin-bottom:10px; }
.plant-info h3 { text-align:center; color:#2E7D32; margin-bottom:10px; font-size:1.2em; }
.plant-info select { width:100%; padding:8px; border:2px solid #E0E0E0; border-radius:8px; margin-bottom:10px; font-size:14px; background:white; cursor:pointer; }
.color-picker-container { margin-bottom:10px; text-align:center; }
.color-picker-container input[type='color'] { width:40px; height:25px; border:1px solid #ddd; border-radius:5px; cursor:pointer; vertical-align:middle; }
.plant-details { background:#F1F8E9; border-radius:8px; padding:8px; margin-bottom:10px; border-left:4px solid #4CAF50; display:none; font-size:0.9em; }
.detail-item { margin-bottom:3px; color:#2E7D32;}
.plant-info button { width:100%; padding:10px; background:linear-gradient(45deg,#4CAF50,#66BB6A); color:white; border:none; border-radius:8px; font-size:15px; font-weight:bold; cursor:pointer; transition:all 0.3s ease; }
.plant-info button:hover { background:linear-gradient(45deg,#388E3C,#4CAF50); transform:translateY(-2px); box-shadow:0 5px 15px rgba(76,175,80,0.3); }
.message { display:none; padding:12px; border-radius:10px; margin:15px 0; font-weight:bold; text-align:center; font-size:15px; }
.message.success { background:#D4EDDA; color:#155724; border:2px solid #C3E6CB; }
.message.error { background:#F8D7DA; color:#721C24; border:2px solid #F5C6CB; }
.message.info { background:#D1ECF1; color:#0C5460; border:2px solid #BEE5EB; }
.control-panel { background:#F8F9FA; border-radius:15px; padding:20px; border:2px solid #E9ECEF; }
.control-panel h2 { color:#2E7D32; margin-bottom:15px; text-align:center; }
.stats { margin-bottom:15px; font-size:0.95em; }
.summary { background:#E8F5E8; padding:12px; border-radius:8px; margin-bottom:10px; text-align:center; font-weight:bold; font-size:1.1em; color:#2E7D32; }
.stat-item { background:white; padding:10px; margin-bottom:6px; border-radius:6px; border-left:4px solid #4CAF50; box-shadow:0 2px 4px rgba(0,0,0,0.05); }
.refresh-btn { width:100%; padding:12px; background:linear-gradient(45deg,#FF9800,#FFA726); color:white; border:none; border-radius:10px; font-size:16px; font-weight:bold; cursor:pointer; transition:all 0.3s ease; }
.refresh-btn:hover { background:linear-gradient(45deg,#F57C00,#FF9800); transform:translateY(-2px); box-shadow:0 5px 15px rgba(255,152,0,0.3); }
.profile-selector { margin-bottom:25px; text-align:center; }
.profile-selector label { display:block; margin-bottom:8px; font-weight:bold; color:#555; }
.profile-selector select { padding:10px 15px; border:2px solid #ddd; border-radius:8px; font-size:15px; background:white; cursor:pointer; min-width:180px; }
.form-group { margin-bottom:20px; }
.form-group label { display:block; margin-bottom:6px; font-weight:bold; color:#555; }
input[type='text']{width:100%;padding:10px 12px;border:2px solid #ddd;border-radius:8px;font-size:15px;transition:border-color 0.3s ease;}
input[type='text']:focus{outline:none;border-color:#667eea;}
input[type='range']{width:100%;height:8px;border-radius:4px;background:#ddd;outline:none;margin:8px 0;cursor:pointer;}
input[type='range']::-webkit-slider-thumb{appearance:none;width:18px;height:18px;border-radius:50%;background:linear-gradient(45deg,#667eea,#764ba2);cursor:pointer;box-shadow:0 2px 6px rgba(0,0,0,0.2);}
input[type='range']::-moz-range-thumb{width:18px;height:18px;border-radius:50%;background:linear-gradient(45deg,#667eea,#764ba2);cursor:pointer;border:none;}
.slider-labels{display:flex;justify-content:space-between;font-size:12px;color:#777;margin-top:4px;}
.buttons{display:flex;gap:12px;justify-content:center;margin-top:25px;}
button{padding:10px 20px;border:none;border-radius:8px;font-size:15px;font-weight:bold;cursor:pointer;transition:all 0.3s ease;background:linear-gradient(45deg,#667eea,#764ba2);color:white;}
button:hover{transform:translateY(-2px);box-shadow:0 5px 15px rgba(0,0,0,0.2);}
.profile-list{margin-top:30px;padding-top:25px;border-top:2px solid #eee;}
.profile-item{background:#f8f9fa;padding:10px 12px;margin-bottom:8px;border-radius:6px;border-left:4px solid #667eea; font-size:0.95em;}
.back-btn{background:#666;color:white;padding:8px 15px;border:none;border-radius:5px;text-decoration:none;display:inline-block;margin-bottom:15px; font-size:14px;}
@media (max-width:768px){.container{padding:20px;}h1{font-size:2em;}.plant-container{min-width:100%;}}
@media (max-width:600px){.buttons{flex-direction:column;}button{width:100%;}}
)";
  server.send(200, "text/css", css);
}

// --- END Web Server Handlers and HTML/CSS ---
