#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// Configuration du point d'accès WiFi
const char* ap_ssid = "Machine à plantes";
const char* ap_password = "12345678"; // Minimum 8 caractères

// Structure pour stocker un profil
struct Profile {
  char name[32];
  int humidity;    // 0-10
  int brightness;  // 0-24
};

// Structure pour l'assignation des profils aux plantes
struct PlantAssignment {
  int profileIndex; // -1 pour profil vide, 0-49 pour les profils configurés
  char plantName[32]; // Nom personnalisé de la plante
};

Profile profiles[50];
PlantAssignment plantsA[5];  // 5 plantes
const int PROFILES_OFFSET = 0;
const int PLANTS_OFFSET = sizeof(profiles);
const int EEPROM_SIZE = sizeof(profiles) + sizeof(plantsA);

// Variables pour stocker l'état des plantes
struct Plant {
  bool buttonState;
  int sliderValue;
};

Plant plants[5] = {
  {false, 50},
  {false, 50},
  {false, 50},
  {false, 50},
  {false, 50}
};

bool Manuel = false;

// Serveur web sur le port 80
WebServer server(80);

// Page HTML principale avec navigation
const char htmlMainPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Page principale</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
        }
        .container {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            text-align: center;
            max-width: 400px;
            width: 100%;
        }
        h1 {
            color: #333;
            margin-bottom: 30px;
            font-size: 2.5em;
        }
        .nav-button {
            display: block;
            width: 100%;
            padding: 15px;
            margin: 10px 0;
            border: none;
            border-radius: 10px;
            font-size: 1.2em;
            font-weight: bold;
            color: white;
            text-decoration: none;
            transition: all 0.3s ease;
            cursor: pointer;
        }
        .blue-btn {
            background: linear-gradient(45deg, #4169E1, #1E90FF);
        }
        .red-btn {
            background: linear-gradient(45deg, #DC143C, #FF6347);
        }
        .green-btn {
            background: linear-gradient(45deg, #32CD32, #228B22);
        }
        .nav-button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        .info {
            color: #666;
            font-size: 0.9em;
            margin-top: 20px;
        }
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
            <small>Réseau: ESP32-Plantes | IP: 192.168.4.1</small>
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
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Contrôle des Plantes</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        
        .container {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
            max-width: 1000px;
            width: 95%;
        }
        
        h1 {
            text-align: center;
            color: #333;
            margin-bottom: 30px;
            font-size: 2em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
        }
        
        .plants-grid {
            position: relative;
            height: 640px;
            margin-bottom: 30px;
        }
        
        .plant {
            position: absolute;
            background: linear-gradient(145deg, #2ecc71, #27ae60);
            border-radius: 15px;
            padding: 20px;
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
            width: 250px;
            transform: translateX(-50%);
            transition: all 0.3s ease;
        }
        
        .plant:hover {
            transform: translateX(-50%) translateY(-5px);
            box-shadow: 0 8px 25px rgba(0, 0, 0, 0.3);
        }
        
        .plant-1 { bottom: 0; left: 40%; }
        .plant-2 { bottom: 130px; left: 60%; }
        .plant-3 { bottom: 260px; left: 40%; }
        .plant-4 { bottom: 390px; left: 60%; }
        .plant-5 { bottom: 520px; left: 40%; }
        
        .plant h3 {
            color: white;
            margin: 0 0 15px 0;
            text-align: center;
            font-size: 1.3em;
            text-shadow: 1px 1px 2px rgba(0,0,0,0.3);
        }
        
        .controls {
            display: flex;
            flex-direction: column;
            gap: 12px;
        }
        
        .control-row {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .btn {
            background: linear-gradient(145deg, #3498db, #2980b9);
            color: white;
            border: none;
            padding: 12px 20px;
            border-radius: 25px;
            cursor: pointer;
            font-size: 14px;
            font-weight: bold;
            transition: all 0.3s ease;
            min-width: 80px;
        }
        
        .btn:hover {
            background: linear-gradient(145deg, #2980b9, #1f6391);
            transform: translateY(-2px);
        }
        
        .btn.active {
            background: linear-gradient(145deg, #e74c3c, #c0392b);
        }
        
        .slider {
            flex: 1;
            height: 8px;
            border-radius: 5px;
            background: rgba(255, 255, 255, 0.3);
            outline: none;
            -webkit-appearance: none;
        }
        
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: white;
            cursor: pointer;
            box-shadow: 0 2px 5px rgba(0,0,0,0.3);
        }
        
        .slider::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: white;
            cursor: pointer;
            box-shadow: 0 2px 5px rgba(0,0,0,0.3);
            border: none;
        }
        
        .slider-value {
            color: white;
            font-weight: bold;
            min-width: 35px;
            text-align: center;
        }
        
        .system-control {
            text-align: center;
            margin-top: 20px;
        }
        
        .system-btn {
            background: linear-gradient(145deg, #95a5a6, #7f8c8d);
            color: white;
            border: none;
            padding: 15px 40px;
            border-radius: 30px;
            font-size: 18px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .system-btn.on {
            background: linear-gradient(145deg, #2ecc71, #27ae60);
        }
        
        .system-btn:hover {
            transform: translateY(-3px);
            box-shadow: 0 6px 20px rgba(0, 0, 0, 0.3);
        }
        
        .status {
            margin-top: 20px;
            padding: 15px;
            background: rgba(52, 152, 219, 0.1);
            border-radius: 10px;
            text-align: center;
            font-weight: bold;
            color: #2c3e50;
        }
        .back-btn {
            background: #666;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            text-decoration: none;
            display: inline-block;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    </div>
        <a href="/" class="back-btn">← Retour</a>
    <div class="container">
        <h1>🌱 Mode Manuel 🌱 </h1>
        
        <div class="plants-grid">
            <div class="plant plant-1">
                <h3>🌿 Plante 1</h3>
                <div class="controls">
                    <div class="control-row">
                        <button class="btn" onclick="togglePlant(1)">Pompe</button>
                        <input type="range" class="slider" min="0" max="100" value="50" oninput="updateSlider(1, this.value)">
                        <span class="slider-value" id="value1">50</span>
                    </div>
                </div>
            </div>
            
            <div class="plant plant-2">
                <h3>🌺 Plante 2</h3>
                <div class="controls">
                    <div class="control-row">
                        <button class="btn" onclick="togglePlant(2)">Pompe</button>
                        <input type="range" class="slider" min="0" max="100" value="50" oninput="updateSlider(2, this.value)">
                        <span class="slider-value" id="value2">50</span>
                    </div>
                </div>
            </div>
            
            <div class="plant plant-3">
                <h3>🌸 Plante 3</h3>
                <div class="controls">
                    <div class="control-row">
                        <button class="btn" onclick="togglePlant(3)">Pompe</button>
                        <input type="range" class="slider" min="0" max="100" value="50" oninput="updateSlider(3, this.value)">
                        <span class="slider-value" id="value3">50</span>
                    </div>
                </div>
            </div>
            
            <div class="plant plant-4">
                <h3>🌻 Plante 4</h3>
                <div class="controls">
                    <div class="control-row">
                        <button class="btn" onclick="togglePlant(4)">Pompe</button>
                        <input type="range" class="slider" min="0" max="100" value="50" oninput="updateSlider(4, this.value)">
                        <span class="slider-value" id="value4">50</span>
                    </div>
                </div>
            </div>
            
            <div class="plant plant-5">
                <h3>🌹 Plante 5</h3>
                <div class="controls">
                    <div class="control-row">
                        <button class="btn" onclick="togglePlant(5)">Pompe</button>
                        <input type="range" class="slider" min="0" max="100" value="50" oninput="updateSlider(5, this.value)">
                        <span class="slider-value" id="value5">50</span>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="status" id="status">
            Mode Manuel - Contrôle des plantes selon les boutons
        </div>
    </div>

    <script>
        let systemState = false;
        
        function togglePlant(plantId) {
            fetch('/toggle/' + plantId)
                .then(response => response.text())
                .then(data => {
                    console.log('Plante ' + plantId + ' toggled');
                    updateStatus();
                });
        }
        
        function updateSlider(plantId, value) {
            document.getElementById('value' + plantId).textContent = value;
            fetch('/slider/' + plantId + '/' + value)
                .then(response => response.text())
                .then(data => {
                    console.log('Plante ' + plantId + ' slider: ' + value);
                });
        }

        // Animation d'entrée
        window.onload = function() {
            const plants = document.querySelectorAll('.plant');
            plants.forEach((plant, index) => {
                plant.style.opacity = '0';
                plant.style.transform = 'translateX(-50%) translateY(20px)';
                setTimeout(() => {
                    plant.style.transition = 'all 0.6s ease';
                    plant.style.opacity = '1';
                    plant.style.transform = 'translateX(-50%) translateY(0)';
                }, index * 200);
            });
        };
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Initialiser EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadData();

  // Configuration du point d'accès WiFi
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress IP = WiFi.softAPIP();
  Serial.println("Point d'accès WiFi créé!");
  Serial.print("SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Mot de passe: ");
  Serial.println(ap_password);
  Serial.print("Adresse IP du point d'accès: ");
  Serial.println(IP);
    
  // Configuration des routes du serveur web
  server.on("/", handleRoot);
  server.on("/blue", handleBluePage);
  server.on("/red", handleRedPage);
  server.on("/green", handleGreenPage);

  // Assignation
  server.on("/assign", HTTP_POST, handleAssign);
  server.on("/getPlants", handleGetPlants);
  server.on("/getProfiles", handleGetProfiles);

  // Configuration
  server.on("/save", HTTP_POST, handleSave);
  server.on("/get", handleGet);
  server.on("/style.css", handleCSS);

  // Mode Manuel
  server.on("/toggle/1", []() { togglePlant(0); });
  server.on("/toggle/2", []() { togglePlant(1); });
  server.on("/toggle/3", []() { togglePlant(2); });
  server.on("/toggle/4", []() { togglePlant(3); });
  server.on("/toggle/5", []() { togglePlant(4); });
  
  server.on("/slider/1/0", []() { updateSlider(0, 0); });
  server.on("/slider/1/10", []() { updateSlider(0, 10); });
  server.on("/slider/1/20", []() { updateSlider(0, 20); });
  server.on("/slider/1/30", []() { updateSlider(0, 30); });
  server.on("/slider/1/40", []() { updateSlider(0, 40); });
  server.on("/slider/1/50", []() { updateSlider(0, 50); });
  server.on("/slider/1/60", []() { updateSlider(0, 60); });
  server.on("/slider/1/70", []() { updateSlider(0, 70); });
  server.on("/slider/1/80", []() { updateSlider(0, 80); });
  server.on("/slider/1/90", []() { updateSlider(0, 90); });
  server.on("/slider/1/100", []() { updateSlider(0, 100); });
    
    // Démarrage du serveur
    server.begin();
    Serial.println("Serveur web démarré");
    Serial.println("Connectez-vous au réseau WiFi 'ESP32-LED-Controller'");
    Serial.println("Puis ouvrez votre navigateur et allez à http://192.168.4.1");
}

void loop() {
    server.handleClient();
}

// Gestion de la page principale
void handleRoot() {
    server.send(200, "text/html", htmlMainPage);
}

// Gestion des pages secondaires
void handleBluePage() {
  String html = R"(
<!DOCTYPE html>
<html lang='fr'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Attribution des Profils aux Plantes</title>
    <link rel='stylesheet' href='/style.css'>
</head>
<body>
    </div>
        <a href="/" class="back-btn">← Retour</a>
    <div class='container'>
        <h1>🌱 Attribution des Profils aux Plantes</h1>
        
        <div class='garden-layout'>
            <!-- Niveau supérieur -->
            <div class='top-row'>
                <div class='plant-container' id='plant-4'>
                    <div class='plant-visual'>🌿</div>
                    <div class='plant-info'>
                        <h3>Plante 5</h3>
                        <select id='profileSelect-4'>
                            <option value='-1'>Aucun profil</option>
                        </select>
                        <div class='plant-details' id='details-4'></div>
                        <button onclick='assignProfile(4)'>Assigner</button>
                    </div>
                </div>
            </div>
            
            <!-- Niveau moyen haut -->
            <div class='middle-high-row'>
                <div class='plant-container' id='plant-3'>
                    <div class='plant-visual'>🌺</div>
                    <div class='plant-info'>
                        <h3>Plante 4</h3>
                        <select id='profileSelect-3'>
                            <option value='-1'>Aucun profil</option>
                        </select>
                        <div class='plant-details' id='details-3'></div>
                        <button onclick='assignProfile(3)'>Assigner</button>
                    </div>
                </div>
            </div>
            
            <!-- Niveau moyen -->
            <div class='middle-row'>
                <div class='plant-container' id='plant-2'>
                    <div class='plant-visual'>🌱</div>
                    <div class='plant-info'>
                        <h3>Plante 3</h3>
                        <select id='profileSelect-2'>
                            <option value='-1'>Aucun profil</option>
                        </select>
                        <div class='plant-details' id='details-2'></div>
                        <button onclick='assignProfile(2)'>Assigner</button>
                    </div>
                </div>
                
                <div class='plant-container' id='plant-1'>
                    <div class='plant-visual'>🪴</div>
                    <div class='plant-info'>
                        <h3>Plante 2</h3>
                        <select id='profileSelect-1'>
                            <option value='-1'>Aucun profil</option>
                        </select>
                        <div class='plant-details' id='details-1'></div>
                        <button onclick='assignProfile(1)'>Assigner</button>
                    </div>
                </div>
            </div>
            
            <!-- Niveau inférieur -->
            <div class='bottom-row'>
                <div class='plant-container' id='plant-0'>
                    <div class='plant-visual'>🌳</div>
                    <div class='plant-info'>
                        <h3>Plante 1</h3>
                        <select id='profileSelect-0'>
                            <option value='-1'>Aucun profil</option>
                        </select>
                        <div class='plant-details' id='details-0'></div>
                        <button onclick='assignProfile(0)'>Assigner</button>
                    </div>
                </div>
            </div>
        </div>
        
        <div id='message' class='message'></div>
        
        <div class='control-panel'>
            <h2>📊 Tableau de Bord</h2>
            <div class='stats' id='stats'></div>
            <button onclick='refreshAll()' class='refresh-btn'>🔄 Actualiser Tout</button>
        </div>
    </div>

    <script>
        let profiles = [];
        let plants = [];
        
        function loadProfiles() {
            fetch('/getProfiles')
                .then(response => response.json())
                .then(data => {
                    profiles = data;
                    updateProfileSelects();
                })
                .catch(error => {
                    showMessage('Erreur lors du chargement des profils: ' + error, 'error');
                });
        }
        
        function loadPlants() {
            fetch('/getPlants')
                .then(response => response.json())
                .then(data => {
                    plants = data;
                    updatePlantDisplay();
                    updateStats();
                })
                .catch(error => {
                    showMessage('Erreur lors du chargement des plantes: ' + error, 'error');
                });
        }
        
        function updateProfileSelects() {
            for(let i = 0; i < 5; i++) {
                const select = document.getElementById('profileSelect-' + i);
                select.innerHTML = '<option value="-1">Aucun profil</option>';
                
                profiles.forEach((profile, index) => {
                    if(profile.name && profile.name.trim() !== '') {
                        const option = document.createElement('option');
                        option.value = index;
                        option.textContent = 'Profil ' + (index + 1) + ' - ' + profile.name;
                        select.appendChild(option);
                    }
                });
            }
        }
        
        function updatePlantDisplay() {
            for(let i = 0; i < 5; i++) {
                const plant = plants[i];
                document.getElementById('profileSelect-' + i).value = plant.profileIndex;
                
                updatePlantDetails(i);
            }
        }
        
        function updatePlantDetails(plantIndex) {
            const plant = plants[plantIndex];
            const detailsDiv = document.getElementById('details-' + plantIndex);
            
            if(plant.profileIndex >= 0 && plant.profileIndex < profiles.length) {
                const profile = profiles[plant.profileIndex];
                if(profile.name && profile.name.trim() !== '') {
                    detailsDiv.innerHTML = 
                        '<div class="detail-item">💧 Humidité: ' + profile.humidity + '</div>' +
                        '<div class="detail-item">💡 Luminosité: ' + profile.brightness + '</div>';
                    detailsDiv.style.display = 'block';
                } else {
                    detailsDiv.style.display = 'none';
                }
            } else {
                detailsDiv.style.display = 'none';
            }
        }
        
        function assignProfile(plantIndex) {
            const profileIndex = parseInt(document.getElementById('profileSelect-' + plantIndex).value);
            
            const formData = new FormData();
            formData.append('plant', plantIndex);
            formData.append('plantName', '');
            formData.append('profileIndex', profileIndex);
            
            fetch('/assign', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data => {
                if(data === 'OK') {
                    plants[plantIndex].plantName = '';
                    plants[plantIndex].profileIndex = profileIndex;
                    updatePlantDetails(plantIndex);
                    updateStats();
                    
                    const plantNum = plantIndex + 1;
                    showMessage('✅ Plante ' + plantNum + ' configurée avec succès!', 'success');
                } else {
                    showMessage('❌ Erreur lors de l\'attribution', 'error');
                }
            })
            .catch(error => {
                showMessage('❌ Erreur: ' + error, 'error');
            });
        }
        
        function updateStats() {
            const statsDiv = document.getElementById('stats');
            let assignedCount = 0;
            let statsHtml = '';
            
            for(let i = 0; i < 5; i++) {
                const plant = plants[i];
                const displayName = 'Plante ' + (i + 1);
                
                statsHtml += '<div class="stat-item">';
                statsHtml += '<strong>' + displayName + ':</strong> ';
                
                if(plant.profileIndex >= 0 && plant.profileIndex < profiles.length) {
                    const profile = profiles[plant.profileIndex];
                    if(profile.name && profile.name.trim() !== '') {
                        statsHtml += profile.name + ' (H:' + profile.humidity + ', L:' + profile.brightness + ')';
                        assignedCount++;
                    } else {
                        statsHtml += 'Profil vide';
                    }
                } else {
                    statsHtml += 'Non assignée';
                }
                
                statsHtml += '</div>';
            }
            
            statsHtml = '<div class="summary">📈 ' + assignedCount + '/5 plantes configurées</div>' + statsHtml;
            statsDiv.innerHTML = statsHtml;
        }
        
        function refreshAll() {
            loadProfiles();
            loadPlants();
            showMessage('🔄 Données actualisées', 'success');
        }
        
        function showMessage(text, type) {
            const messageDiv = document.getElementById('message');
            messageDiv.textContent = text;
            messageDiv.className = 'message ' + type;
            messageDiv.style.display = 'block';
            
            setTimeout(() => {
                messageDiv.style.display = 'none';
            }, 3000);
        }
        
        // Charger les données au démarrage
        window.onload = function() {
            loadProfiles();
            loadPlants();
        };
        
        // Actualiser les détails quand on change de profil
        for(let i = 0; i < 5; i++) {
            document.addEventListener('DOMContentLoaded', function() {
                const select = document.getElementById('profileSelect-' + i);
                if(select) {
                    select.addEventListener('change', function() {
                        updatePlantDetails(i);
                    });
                }
            });
        }
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}


void handleRedPage() {
    server.send(200, "text/html", htmlRedPage);
}

void handleGreenPage() {
   String html = R"(
<!DOCTYPE html>
<html lang='fr'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>🌱 Configuration des Plantes</title>
    <link rel='stylesheet' href='/style.css'>
</head>
<body>
    </div>
        <a href="/" class="back-btn">← Retour</a>
    <div class='container'>
        <h1>🌱 Configuration des Profils</h1>
        
        <div class='profile-selector'>
            <label for='profileSelect'>Sélectionner une plante:</label>
            <select id='profileSelect' onchange='loadProfile()'>
)";
  
  for(int i = 0; i < 50; i++) {
    html += "<option value='" + String(i) + "'>Profil " + String(i + 1) + "</option>";
  }
  
  html += R"(
            </select>
        </div>
        
        <form id='profileForm'>
            <div class='form-group'>
                <label for='profileName'>Nom de la plante:</label>
                <input type='text' id='profileName' maxlength='31' placeholder='Entrez le nom de la plante'>
            </div>
            
            <div class='form-group'>
                <label for='humidity'>Humidité: <span id='humidityValue'>5</span></label>
                <input type='range' id='humidity' min='0' max='10' value='5' 
                       oninput='updateSliderValue("humidity", "humidityValue")'>
                <div class='slider-labels'>
                    <span>0</span>
                    <span>10</span>
                </div>
            </div>
            
            <div class='form-group'>
                <label for='brightness'>Luminosité: <span id='brightnessValue'>12</span></label>
                <input type='range' id='brightness' min='0' max='24' value='12' 
                       oninput='updateSliderValue("brightness", "brightnessValue")'>
                <div class='slider-labels'>
                    <span>0</span>
                    <span>24</span>
                </div>
            </div>
            
            <div class='buttons'>
                <button type='button' onclick='saveProfile()'>Sauvegarder Profil</button>
                <button type='button' onclick='loadProfile()'>Recharger Profil</button>
            </div>
        </form>
        
        <div id='message' class='message'></div>
        
        <div class='profile-list'>
            <h2>Liste des Profils Configurées 🌱</h2>
            <div id='profilesList'></div>
        </div>
    </div>

    <script>
        let currentProfile = 0;
        
        function updateSliderValue(sliderId, valueId) {
            const slider = document.getElementById(sliderId);
            const valueSpan = document.getElementById(valueId);
            valueSpan.textContent = slider.value;
        }
        
        function loadProfile() {
            currentProfile = parseInt(document.getElementById('profileSelect').value);
            
            fetch('/get?profile=' + currentProfile)
                .then(response => response.json())
                .then(data => {
                    document.getElementById('profileName').value = data.name || '';
                    document.getElementById('humidity').value = data.humidity || 5;
                    document.getElementById('brightness').value = data.brightness || 12;
                    
                    updateSliderValue('humidity', 'humidityValue');
                    updateSliderValue('brightness', 'brightnessValue');
                    
                    showMessage('Profil ' + (currentProfile + 1) + ' chargé', 'success');
                })
                .catch(error => {
                    showMessage('Erreur lors du chargement: ' + error, 'error');
                });
        }
        
        function saveProfile() {
            const name = document.getElementById('profileName').value;
            const humidity = document.getElementById('humidity').value;
            const brightness = document.getElementById('brightness').value;
            
            if (!name.trim()) {
                showMessage('Veuillez entrer un nom pour le profil', 'error');
                return;
            }
            
            const formData = new FormData();
            formData.append('profile', currentProfile);
            formData.append('name', name);
            formData.append('humidity', humidity);
            formData.append('brightness', brightness);
            
            fetch('/save', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data => {
                showMessage('Profil ' + (currentProfile + 1) + ' sauvegardé avec succès!', 'success');
                updateProfileSelector();
                loadProfilesList();
            })
            .catch(error => {
                showMessage('Erreur lors de la sauvegarde: ' + error, 'error');
            });
        }
        
        function showMessage(text, type) {
            const messageDiv = document.getElementById('message');
            messageDiv.textContent = text;
            messageDiv.className = 'message ' + type;
            messageDiv.style.display = 'block';
            
            setTimeout(() => {
                messageDiv.style.display = 'none';
            }, 3000);
        }
        
        function updateProfileSelector() {
            const select = document.getElementById('profileSelect');
            const currentValue = select.value;
            
            // Vider les options existantes
            select.innerHTML = '';
            
            // Recréer toutes les options avec les noms mis à jour
            for(let i = 0; i < 50; i++) {
                fetch('/get?profile=' + i)
                    .then(response => response.json())
                    .then(data => {
                        const option = document.createElement('option');
                        option.value = i;
                        
                        if(data.name && data.name.trim() !== '') {
                            option.textContent = 'Profil ' + (i + 1) + ': ' + data.name;
                        } else {
                            option.textContent = 'Profil ' + (i + 1);
                        }
                        
                        select.appendChild(option);
                        
                        // Restaurer la sélection précédente
                        if(i == currentValue) {
                            select.value = currentValue;
                        }
                    });
            }
        }
        
        function loadProfilesList() {
            let listHtml = '';
            let processedProfiles = 0;
            
            for(let i = 0; i < 50; i++) {
                fetch('/get?profile=' + i)
                    .then(response => response.json())
                    .then(data => {
                        processedProfiles++;
                        if(data.name && data.name.trim() !== '') {
                            listHtml += '<div class="profile-item">';
                            listHtml += '<strong>Profil ' + (i + 1) + ':</strong> ' + data.name;
                            listHtml += ' (Humidité: ' + data.humidity + ', Luminosité: ' + data.brightness + ')';
                            listHtml += '</div>';
                        }
                        
                        // Mettre à jour l'affichage quand tous les profils sont traités
                        if(processedProfiles === 50) {
                            document.getElementById('profilesList').innerHTML = listHtml || '<p>Aucun profil configuré</p>';
                        }
                    });
            }
        }
        
        // Charger le premier profil au démarrage
        window.onload = function() {
            updateProfileSelector();
            setTimeout(() => {
                loadProfile();
                loadProfilesList();
            }, 500); // Attendre que les options soient chargées
        };
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

  //Fonctions du Mode Manuel
void togglePlant(int plantIndex) {
  plants[plantIndex].buttonState = !plants[plantIndex].buttonState;
  
  Serial.print("Plante ");
  Serial.print(plantIndex + 1);
  Serial.print(" bouton: ");
  Serial.println(plants[plantIndex].buttonState ? "ON" : "OFF");
  
  server.send(200, "text/plain", "OK");
}

void updateSlider(int plantIndex, int value) {
  plants[plantIndex].sliderValue = value;
  
  Serial.print("Plante ");
  Serial.print(plantIndex + 1);
  Serial.print(" slider: ");
  Serial.println(value);
  
  server.send(200, "text/plain", "OK");
}


void handleSave() {
  if (server.hasArg("profile") && server.hasArg("name") && 
      server.hasArg("humidity") && server.hasArg("brightness")) {
    
    int profileIndex = server.arg("profile").toInt();
    
    if (profileIndex >= 0 && profileIndex < 50) {
      // Copier le nom (avec protection contre débordement)
      strncpy(profiles[profileIndex].name, server.arg("name").c_str(), 31);
      profiles[profileIndex].name[31] = '\0';
      
      profiles[profileIndex].humidity = server.arg("humidity").toInt();
      profiles[profileIndex].brightness = server.arg("brightness").toInt();
      
      // Valider les valeurs
      if (profiles[profileIndex].humidity < 0) profiles[profileIndex].humidity = 0;
      if (profiles[profileIndex].humidity > 10) profiles[profileIndex].humidity = 10;
      if (profiles[profileIndex].brightness < 0) profiles[profileIndex].brightness = 0;
      if (profiles[profileIndex].brightness > 24) profiles[profileIndex].brightness = 24;
      
      saveProfiles1();
      
      Serial.printf("Profil %d sauvegardé: %s (H:%d, L:%d)\n", 
                    profileIndex + 1, 
                    profiles[profileIndex].name,
                    profiles[profileIndex].humidity,
                    profiles[profileIndex].brightness);
      
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Index de profil invalide");
    }
  } else {
    server.send(400, "text/plain", "Paramètres manquants");
  }
}

  //Fonctions pour Configuration
void handleGet() {
  if (server.hasArg("profile")) {
    int profileIndex = server.arg("profile").toInt();
    
    if (profileIndex >= 0 && profileIndex < 50) {
      String json = "{";
      json += "\"name\":\"" + String(profiles[profileIndex].name) + "\",";
      json += "\"humidity\":" + String(profiles[profileIndex].humidity) + ",";
      json += "\"brightness\":" + String(profiles[profileIndex].brightness);
      json += "}";
      
      server.send(200, "application/json", json);
    } else {
      server.send(400, "text/plain", "Index de profil invalide");
    }
  } else {
    server.send(400, "text/plain", "Paramètre profil manquant");
  }
}

void saveProfiles1() {
  EEPROM.put(0, profiles);
  EEPROM.commit();
}

void loadProfiles() {
  EEPROM.get(0, profiles);
  
  // Initialiser les profils vides si c'est la première utilisation
  for (int i = 0; i < 50; i++) {
    // Vérifier si le profil contient des données valides
    bool isEmpty = true;
    for (int j = 0; j < 32; j++) {
      if (profiles[i].name[j] != 0 && profiles[i].name[j] != 255) {
        isEmpty = false;
        break;
      }
    }
    
    if (isEmpty || profiles[i].humidity < 0 || profiles[i].humidity > 10 || 
        profiles[i].brightness < 0 || profiles[i].brightness > 24) {
      // Initialiser le profil
      memset(profiles[i].name, 0, 32);
      profiles[i].humidity = 5;
      profiles[i].brightness = 12;
    }
  }
}

  //Fonctions pour Assignations
void handleAssign() {
  if (server.hasArg("plant") && server.hasArg("profileIndex")) {
    int plantIndex = server.arg("plant").toInt();
    int profileIndex = server.arg("profileIndex").toInt();
    
    if (plantIndex >= 0 && plantIndex < 5) {
      // Vider le nom de la plante
      memset(plantsA[plantIndex].plantName, 0, 32);
      
      // Assigner le profil (-1 pour aucun profil)
      plantsA[plantIndex].profileIndex = profileIndex;
      
      savePlants();
      
      Serial.printf("Plante %d assignée au profil %d\n", 
                    plantIndex + 1, 
                    profileIndex);
      
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Index de plante invalide");
    }
  } else {
    server.send(400, "text/plain", "Paramètres manquants");
  }
}

void handleGetPlants() {
  String json = "[";
  for (int i = 0; i < 5; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"plantName\":\"" + String(plantsA[i].plantName) + "\",";
    json += "\"profileIndex\":" + String(plantsA[i].profileIndex);
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
  // Charger les profils
  EEPROM.get(PROFILES_OFFSET, profiles);
  
  // Charger les attributions de plantes
  EEPROM.get(PLANTS_OFFSET, plantsA);
  
  // Initialiser les profils vides si c'est la première utilisation
  for (int i = 0; i < 50; i++) {
    bool isEmpty = true;
    for (int j = 0; j < 32; j++) {
      if (profiles[i].name[j] != 0 && profiles[i].name[j] != 255) {
        isEmpty = false;
        break;
      }
    }
    
    if (isEmpty || profiles[i].humidity < 0 || profiles[i].humidity > 10 || 
        profiles[i].brightness < 0 || profiles[i].brightness > 24) {
      memset(profiles[i].name, 0, 32);
      profiles[i].humidity = 5;
      profiles[i].brightness = 12;
    }
  }
  
  // Initialiser les plantes si c'est la première utilisation
  for (int i = 0; i < 5; i++) {
    bool isEmpty = true;
    for (int j = 0; j < 32; j++) {
      if (plantsA[i].plantName[j] != 0 && plantsA[i].plantName[j] != 255) {
        isEmpty = false;
        break;
      }
    }
    
    if (isEmpty || plantsA[i].profileIndex < -1 || plantsA[i].profileIndex >= 50) {
      memset(plantsA[i].plantName, 0, 32);
      plantsA[i].profileIndex = -1; // Aucun profil assigné
    }
  }
}

  //Style CSS pour Assignation et Configuration
void handleCSS() {
  String css = R"(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, #4CAF50 0%, #8BC34A 50%, #CDDC39 100%);
    min-height: 100vh;
    padding: 20px;
}

.container {
    max-width: 1200px;
    margin: 0 auto;
    background: rgba(255, 255, 255, 0.95);
    border-radius: 20px;
    box-shadow: 0 25px 40px rgba(0,0,0,0.1);
    padding: 30px;
    backdrop-filter: blur(10px);
}

h1 {
    text-align: center;
    color: #2E7D32;
    margin-bottom: 30px;
    font-size: 2.5em;
    text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
}

h2 {
    color: #388E3C;
    margin-bottom: 20px;
    font-size: 1.8em;
}
.garden-layout {
    display: flex;
    flex-direction: column;
    gap: 30px;
    margin-bottom: 40px;
}

.top-row, .middle-high-row, .middle-row, .bottom-row {
    display: flex;
    justify-content: center;
    gap: 30px;
    flex-wrap: wrap;
}

.plant-container {
    background: white;
    border-radius: 15px;
    padding: 20px;
    box-shadow: 0 8px 25px rgba(0,0,0,0.1);
    transition: all 0.3s ease;
    border: 3px solid #E8F5E8;
    min-width: 280px;
    max-width: 350px;
}

.plant-container:hover {
    transform: translateY(-5px);
    box-shadow: 0 15px 35px rgba(0,0,0,0.15);
    border-color: #4CAF50;
}

.plant-visual {
    text-align: center;
    font-size: 3em;
    margin-bottom: 15px;
    filter: drop-shadow(2px 2px 4px rgba(0,0,0,0.2));
}

.plant-info h3 {
    text-align: center;
    color: #2E7D32;
    margin-bottom: 15px;
    font-size: 1.3em;
}

.plant-info select {
    width: 100%;
    padding: 10px;
    border: 2px solid #E0E0E0;
    border-radius: 8px;
    margin-bottom: 15px;
    font-size: 14px;
    background: white;
    cursor: pointer;
}

.plant-details {
    background: #F1F8E9;
    border-radius: 8px;
    padding: 10px;
    margin-bottom: 15px;
    border-left: 4px solid #4CAF50;
    display: none;
}

.detail-item {
    margin-bottom: 5px;
    font-size: 14px;
    color: #2E7D32;
}

.plant-info button {
    width: 100%;
    padding: 12px;
    background: linear-gradient(45deg, #4CAF50, #66BB6A);
    color: white;
    border: none;
    border-radius: 8px;
    font-size: 16px;
    font-weight: bold;
    cursor: pointer;
    transition: all 0.3s ease;
}

.plant-info button:hover {
    background: linear-gradient(45deg, #388E3C, #4CAF50);
    transform: translateY(-2px);
    box-shadow: 0 5px 15px rgba(76, 175, 80, 0.3);
}

.message {
    display: none;
    padding: 15px;
    border-radius: 10px;
    margin: 20px 0;
    font-weight: bold;
    text-align: center;
    font-size: 16px;
}

.message.success {
    background: #D4EDDA;
    color: #155724;
    border: 2px solid #C3E6CB;
}

.message.error {
    background: #F8D7DA;
    color: #721C24;
    border: 2px solid #F5C6CB;
}

.control-panel {
    background: #F8F9FA;
    border-radius: 15px;
    padding: 25px;
    border: 2px solid #E9ECEF;
}

.control-panel h2 {
    color: #2E7D32;
    margin-bottom: 20px;
    text-align: center;
}

.stats {
    margin-bottom: 20px;
}

.summary {
    background: #E8F5E8;
    padding: 15px;
    border-radius: 8px;
    margin-bottom: 15px;
    text-align: center;
    font-weight: bold;
    font-size: 18px;
    color: #2E7D32;
}

.stat-item {
    background: white;
    padding: 12px;
    margin-bottom: 8px;
    border-radius: 6px;
    border-left: 4px solid #4CAF50;
    box-shadow: 0 2px 4px rgba(0,0,0,0.05);
}

.refresh-btn {
    width: 100%;
    padding: 15px;
    background: linear-gradient(45deg, #FF9800, #FFA726);
    color: white;
    border: none;
    border-radius: 10px;
    font-size: 18px;
    font-weight: bold;
    cursor: pointer;
    transition: all 0.3s ease;
}

.refresh-btn:hover {
    background: linear-gradient(45deg, #F57C00, #FF9800);
    transform: translateY(-2px);
    box-shadow: 0 5px 15px rgba(255, 152, 0, 0.3);
}
.plants-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    grid-template-rows: auto auto auto;
    gap: 30px;
    max-width: 800px;
    margin: 0 auto 40px;
    height: 600px;
}

.plant-card {
    background: linear-gradient(145deg, #ffffff, #f0f0f0);
    border-radius: 20px;
    padding: 25px;
    text-align: center;
    box-shadow: 0 10px 30px rgba(0,0,0,0.1);
    transition: all 0.3s ease;
    border: 3px solid transparent;
    position: relative;
    overflow: hidden;
}

.plant-card::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: linear-gradient(45deg, transparent, rgba(76, 175, 80, 0.1), transparent);
    opacity: 0;
    transition: opacity 0.3s ease;
}

.plant-card:hover::before {
    opacity: 1;
}

.plant-card:hover {
    transform: translateY(-5px) scale(1.02);
    box-shadow: 0 20px 40px rgba(0,0,0,0.15);
}

.plant-card.assigned {
    border-color: #4CAF50;
    background: linear-gradient(145deg, #E8F5E8, #C8E6C9);
}

.plant-card.assigned::before {
    background: linear-gradient(45deg, transparent, rgba(76, 175, 80, 0.2), transparent);
}

/* Positionnement spécifique des plantes */
.plant-1 {
    grid-column: 1;
    grid-row: 3;
}

.plant-2 {
    grid-column: 1;
    grid-row: 2;
}

.plant-3 {
    grid-column: 2;
    grid-row: 2;
}

.plant-4 {
    grid-column: 1;
    grid-row: 1;
}

.plant-5 {
    grid-column: 2;
    grid-row: 1;
}

.plant-icon {
    font-size: 3em;
    margin-bottom: 15px;
    filter: drop-shadow(2px 2px 4px rgba(0,0,0,0.1));
}

.plant-card h3 {
    color: #2E7D32;
    margin-bottom: 15px;
    font-size: 1.4em;
}

.plant-info {
    margin-bottom: 20px;
    padding: 15px;
    background: rgba(255, 255, 255, 0.8);
    border-radius: 12px;
    backdrop-filter: blur(5px);
}

.profile-name {
    font-weight: bold;
    color: #1B5E20;
    font-size: 1.1em;
    margin-bottom: 10px;
}

.profile-details {
    display: flex;
    justify-content: space-around;
    font-size: 0.9em;
    color: #388E3C;
}

.humidity, .brightness {
    background: rgba(76, 175, 80, 0.2);
    padding: 5px 10px;
    border-radius: 15px;
    font-weight: bold;
}

.profile-select {
    width: 100%;
    padding: 12px 15px;
    border: 2px solid #4CAF50;
    border-radius: 12px;
    font-size: 14px;
    background: white;
    cursor: pointer;
    transition: all 0.3s ease;
}

.profile-select:focus {
    outline: none;
    border-color: #2E7D32;
    box-shadow: 0 0 10px rgba(76, 175, 80, 0.3);
}

.profile-select:hover {
    background: #F1F8E9;
}

.message {
    display: none;
    padding: 15px 25px;
    border-radius: 12px;
    margin: 20px 0;
    font-weight: bold;
    text-align: center;
    backdrop-filter: blur(10px);
}

.message.success {
    background: rgba(200, 230, 201, 0.9);
    color: #1B5E20;
    border: 2px solid #4CAF50;
}

.message.error {
    background: rgba(255, 205, 210, 0.9);
    color: #C62828;
    border: 2px solid #F44336;
}

.summary {
    margin-top: 40px;
    padding: 30px;
    background: linear-gradient(145deg, #E8F5E8, #C8E6C9);
    border-radius: 20px;
    box-shadow: inset 0 5px 15px rgba(0,0,0,0.1);
}

.summary-item {
    background: rgba(255, 255, 255, 0.8);
    padding: 15px 20px;
    margin-bottom: 15px;
    border-radius: 12px;
    border-left: 5px solid #4CAF50;
    backdrop-filter: blur(5px);
}

.summary-details {
    display: block;
    font-size: 0.9em;
    color: #388E3C;
    margin-top: 5px;
}

.no-assignments {
    text-align: center;
    color: #757575;
    font-style: italic;
    padding: 20px;
}

.profile-selector {
    margin-bottom: 30px;
    text-align: center;
}

.profile-selector label {
    display: block;
    margin-bottom: 10px;
    font-weight: bold;
    color: #555;
}

.profile-selector select {
    padding: 12px 20px;
    border: 2px solid #ddd;
    border-radius: 8px;
    font-size: 16px;
    background: white;
    cursor: pointer;
    min-width: 200px;
}

.form-group {
    margin-bottom: 25px;
}

.form-group label {
    display: block;
    margin-bottom: 8px;
    font-weight: bold;
    color: #555;
}

input[type='text'] {
    width: 100%;
    padding: 12px 15px;
    border: 2px solid #ddd;
    border-radius: 8px;
    font-size: 16px;
    transition: border-color 0.3s ease;
}

input[type='text']:focus {
    outline: none;
    border-color: #667eea;
}

input[type='range'] {
    width: 100%;
    height: 8px;
    border-radius: 4px;
    background: #ddd;
    outline: none;
    margin: 10px 0;
    cursor: pointer;
}

input[type='range']::-webkit-slider-thumb {
    appearance: none;
    width: 20px;
    height: 20px;
    border-radius: 50%;
    background: linear-gradient(45deg, #667eea, #764ba2);
    cursor: pointer;
    box-shadow: 0 2px 6px rgba(0,0,0,0.2);
}

input[type='range']::-moz-range-thumb {
    width: 20px;
    height: 20px;
    border-radius: 50%;
    background: linear-gradient(45deg, #667eea, #764ba2);
    cursor: pointer;
    border: none;
}

.slider-labels {
    display: flex;
    justify-content: space-between;
    font-size: 12px;
    color: #777;
    margin-top: 5px;
}

.buttons {
    display: flex;
    gap: 15px;
    justify-content: center;
    margin-top: 30px;
}

button {
    padding: 12px 25px;
    border: none;
    border-radius: 8px;
    font-size: 16px;
    font-weight: bold;
    cursor: pointer;
    transition: all 0.3s ease;
    background: linear-gradient(45deg, #667eea, #764ba2);
    color: white;
}

button:hover {
    transform: translateY(-2px);
    box-shadow: 0 5px 15px rgba(0,0,0,0.2);
}

.message {
    display: none;
    padding: 15px;
    border-radius: 8px;
    margin: 20px 0;
    font-weight: bold;
    text-align: center;
}

.message.success {
    background: rgba(200, 230, 201, 0.9);
    color: #1B5E20;
    border: 2px solid #4CAF50;
}

.message.error {
    background: rgba(255, 205, 210, 0.9);
    color: #C62828;
    border: 2px solid #F44336;
}

.profile-list {
    margin-top: 40px;
    padding-top: 30px;
    border-top: 2px solid #eee;
}

.profile-item {
    background: #f8f9fa;
    padding: 12px 15px;
    margin-bottom: 10px;
    border-radius: 6px;
    border-left: 4px solid #667eea;
}
.back-btn {
    background: #666;
    color: white;
    padding: 10px 20px;
    border: none;
    border-radius: 5px;
    text-decoration: none;
    display: inline-block;
    margin-top: 20px;
    }
@media (max-width: 600px) {
    .container {
        padding: 20px;
    }
    
    h1 {
        font-size: 2em;
    }
    
    .buttons {
        flex-direction: column;
    }
    
    button {
        width: 100%;
    }
}
@media (max-width: 768px) {
    .container {
        padding: 20px;
    }
    
    h1 {
        font-size: 2em;
    }
    
    .plant-container {
        min-width: 100%;
        max-width: 100%;
    }
    
    .top-row, .middle-row, .bottom-row {
        flex-direction: column;
        align-items: center;
    }
}

@media (max-width: 480px) {
    .garden-layout {
        gap: 20px;
    }
    
    .plant-container {
        padding: 15px;
    }
    
    .plant-visual {
        font-size: 2.5em;
    }
}
)";

  server.send(200, "text/css", css);
}