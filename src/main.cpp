#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> // Ajout WiFiManager
#include "OmniDrivers.h"

// --- VARIABLES GLOBALES ---
std::vector<Device*> devices;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
SemaphoreHandle_t mutex;

// Structure pour l'Automation
struct Rule {
    String srcId;     
    String param;     
    String op;        
    float threshold;  
    String tgtId;     
    float actionVal;  
};
std::vector<Rule> rules;

// --- FONCTIONS UTILITAIRES DE VALIDATION ---

// Vérifie si un PIN est valide sur l'ESP32 et son usage (Input Only vs Output)
bool isPinValid(int pin, bool outputRequired) {
    if (pin < 0 || pin > 39) return false;
    // Pins réservés Flash/UART (à éviter sauf expert)
    if (pin == 1 || pin == 3 || (pin >= 6 && pin <= 11)) return false; 
    // Pins Input Only (34, 35, 36, 39)
    if (outputRequired && (pin == 34 || pin == 35 || pin == 36 || pin == 39)) return false;
    return true;
}

// Vérifie si le PIN est déjà utilisé par un autre device
bool isPinUsed(int pin) {
    for(auto d : devices) {
        if(d->getPin() == pin) return true;
    }
    return false;
}

// Determine si le driver demande une sortie
bool isOutputDriver(String type) {
    return (type == "RELAY" || type == "VALVE" || type == "PUMP" || 
            type == "LOCK" || type == "SERVO" || type == "NEOPIXEL" || 
            type == "LIGHT_INV");
}

// --- GESTION MEMOIRE ---

// Supprime proprement tous les devices et libère la mémoire
void clearDevices() {
    for(auto d : devices) {
        delete d; // Appelle le destructeur virtuel
    }
    devices.clear();
}

// --- GESTION CONFIGURATION ---

void saveConfig() {
    File f = LittleFS.open("/config.json", "w");
    if(!f) { Serial.println("Failed to open config for writing"); return; }

    DynamicJsonDocument doc(8192);
    JsonArray devArr = doc.createNestedArray("devices");
    
    xSemaphoreTake(mutex, portMAX_DELAY);
    for(auto d : devices) {
        JsonObject obj = devArr.createNestedObject();
        obj["id"] = d->getId();
        obj["driver"] = d->getDriver(); 
        obj["name"] = d->getName(); 
        obj["pin"] = d->getPin();
    }
    xSemaphoreGive(mutex);

    JsonArray ruleArr = doc.createNestedArray("rules");
    for(auto r : rules) {
        JsonObject obj = ruleArr.createNestedObject();
        obj["src"] = r.srcId; obj["prm"] = r.param; 
        obj["op"] = r.op; obj["val"] = r.threshold;
        obj["tgt"] = r.tgtId; obj["act"] = r.actionVal;
    }

    serializeJson(doc, f);
    f.close();
    Serial.println("Config Saved");
}

void loadConfig() {
    if(!LittleFS.exists("/config.json")) return;
    File f = LittleFS.open("/config.json", "r");
    
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, f);
    f.close();

    if(error) {
        Serial.println("Failed to parse config file");
        return;
    }

    // Pas de mutex ici car appelé uniquement au setup, 
    // ou protégé par mutex dans l'appelant lors d'un reload
    
    JsonArray arr = doc["devices"];
    for(JsonObject obj : arr) {
        String type = obj["driver"];
        int pin = obj["pin"];
        String id = obj["id"];
        String name = obj["name"];

        // Validation lors du chargement
        if(!isPinValid(pin, isOutputDriver(type))) {
            Serial.printf("SKIP Invalid PIN %d for %s\n", pin, type.c_str());
            continue;
        }

        Device* d = DeviceFactory::create(type, id, name, pin);
        if(d) {
            d->begin();
            devices.push_back(d);
            Serial.printf("Loaded %s on GPIO %d\n", name.c_str(), pin);
        }
    }
    
    rules.clear();
    JsonArray rArr = doc["rules"];
    for(JsonObject obj : rArr) {
        rules.push_back({obj["src"], obj["prm"], obj["op"], obj["val"], obj["tgt"], obj["act"]});
    }
}

// --- LOGIQUE AUTOMATION ---
void checkRules() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    StaticJsonDocument<1024> doc; // Augmenté pour sécurité
    
    for(auto& r : rules) {
        Device* src = nullptr;
        Device* tgt = nullptr;
        for(auto d : devices) {
            if(d->getId() == r.srcId) src = d;
            if(d->getId() == r.tgtId) tgt = d;
        }

        if(src && tgt) {
            doc.clear();
            JsonObject obj = doc.to<JsonObject>();
            src->read(obj);
            
            if(obj.containsKey(r.param)) {
                float val = obj[r.param];
                bool trig = false;
                if(r.op == ">" && val > r.threshold) trig = true;
                if(r.op == "<" && val < r.threshold) trig = true;
                
                if(trig) tgt->write("set", r.actionVal);
            }
        }
    }
    xSemaphoreGive(mutex);
}

// --- SETUP & LOOP ---
void setup() {
    Serial.begin(115200);
    mutex = xSemaphoreCreateMutex();
    
    if(!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    
    // --- CHARGEMENT CONFIG INITIAL ---
    loadConfig();

    // --- WIFIMANAGER ---
    WiFiManager wm;
    // Si la connexion échoue, il lance un AP "OmniESP-Setup"
    // IP par défaut : 192.168.4.1
    bool res = wm.autoConnect("OmniESP-Setup", "admin1234"); 

    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 
    Serial.println("WiFi Connected!");
    Serial.println(WiFi.localIP());

    // --- API STATUS ---
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req){
        AsyncResponseStream *res = req->beginResponseStream("application/json");
        DynamicJsonDocument doc(8192);
        JsonArray arr = doc.createNestedArray("devices");
        
        xSemaphoreTake(mutex, portMAX_DELAY);
        for(auto d : devices) {
            JsonObject obj = arr.createNestedObject();
            obj["id"] = d->getId();
            obj["name"] = d->getName();
            obj["driver"] = d->getDriver();
            obj["pin"] = d->getPin(); // Utile pour UI
            JsonObject val = obj.createNestedObject("val");
            d->read(val);
        }
        xSemaphoreGive(mutex);
        
        serializeJson(doc, *res);
        req->send(res);
    });

    // --- API CONTROL ---
    server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *req){
        if(req->hasParam("id", true) && req->hasParam("cmd", true)) {
            String id = req->getParam("id", true)->value();
            String cmd = req->getParam("cmd", true)->value();
            float v = req->hasParam("val", true) ? req->getParam("val", true)->value().toFloat() : 0;
            
            xSemaphoreTake(mutex, portMAX_DELAY);
            for(auto d : devices) if(d->getId() == id) d->write(cmd, v);
            xSemaphoreGive(mutex);
            req->send(200);
        } else req->send(400);
    });

    // --- API CONFIG (Dynamique) ---
    server.onRequestBody([](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total){
        if(req->url() == "/api/config") {
            DynamicJsonDocument doc(8192);
            DeserializationError error = deserializeJson(doc, data);
            
            if (error) {
                req->send(400, "text/plain", "Invalid JSON");
                return;
            }

            // Section critique : Reconstruction totale
            xSemaphoreTake(mutex, portMAX_DELAY);
            
            // 1. Nettoyage propre (Memory Leak Fix)
            clearDevices(); 

            // 2. Reconstruction
            JsonArray arr = doc["devices"];
            for(JsonObject obj : arr) {
                 String type = obj["driver"];
                 int pin = obj["pin"];
                 String id = obj["id"];
                 String name = obj["name"];
                 
                 // Validation stricte
                 if (isPinValid(pin, isOutputDriver(type)) && !isPinUsed(pin)) {
                    Device* d = DeviceFactory::create(type, id, name, pin);
                    if(d) { d->begin(); devices.push_back(d); }
                 } else {
                     Serial.printf("Rejection: Invalid/Used Pin %d for %s\n", pin, name.c_str());
                 }
            }
            xSemaphoreGive(mutex);
            
            saveConfig();
            req->send(200, "text/plain", "Saved");
        }
    });

    server.addHandler(&ws);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.begin();
}

void loop() {
    checkRules(); 
    
    // Broadcast WS toutes les 2s
    static unsigned long last = 0;
    if(millis() - last > 2000) {
        last = millis();
        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.createNestedArray("devices");
        
        if(xSemaphoreTake(mutex, (TickType_t)100) == pdTRUE) { // TryLock pour ne pas bloquer le loop
            for(auto d : devices) {
                JsonObject obj = arr.createNestedObject();
                obj["id"] = d->getId();
                JsonObject val = obj.createNestedObject("val");
                d->read(val);
            }
            xSemaphoreGive(mutex);
            String out; serializeJson(doc, out);
            ws.textAll(out);
        }
    }
}
