#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <Wire.h>
#include "OmniDrivers.h"

// --- GLOBALS ---
std::vector<Device*> devices;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
SemaphoreHandle_t mutex;

// --- UTILS ---
bool isI2CDriver(String type) {
    return (type == "INA219" || type == "BME280" || type == "BH1750" || type == "LCD_I2C" || type == "OLED");
}

bool isPinValid(int pin, String type) {
    if(isI2CDriver(type)) return (pin >= 1 && pin <= 127);
    if (pin < 0 || pin > 39) return false;
    if (pin == 1 || pin == 3 || (pin >= 6 && pin <= 11)) return false; 
    return true;
}

void clearDevices() {
    for(auto d : devices) delete d;
    devices.clear();
}

// --- SCANNER I2C ---
String scanI2C() {
    DynamicJsonDocument* doc = new DynamicJsonDocument(2048);
    JsonArray arr = doc->createNestedArray("i2c_devices");
    
    for(byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            JsonObject obj = arr.createNestedObject();
            obj["addr_dec"] = address;
            char hexStr[5]; sprintf(hexStr, "0x%02X", address);
            obj["addr_hex"] = String(hexStr);
            
            if(address == 0x27) obj["hint"] = "LCD 1602";
            else if(address == 0x3C) obj["hint"] = "OLED SSD1306";
            else if(address == 0x40) obj["hint"] = "INA219 Power";
            else if(address == 0x76) obj["hint"] = "BME280";
            else if(address == 0x23) obj["hint"] = "BH1750";
            else obj["hint"] = "Unknown";
        }
    }
    String output; serializeJson(*doc, output);
    delete doc;
    return output;
}

// --- CONFIG MANAGEMENT ---
void saveConfig() {
    File f = LittleFS.open("/config.json", "w");
    if(!f) return;
    
    DynamicJsonDocument* doc = new DynamicJsonDocument(8192);
    JsonArray devArr = doc->createNestedArray("devices");
    
    xSemaphoreTake(mutex, portMAX_DELAY);
    for(auto d : devices) {
        JsonObject obj = devArr.createNestedObject();
        obj["id"] = d->getId(); obj["driver"] = d->getDriver(); 
        obj["name"] = d->getName(); obj["pin"] = d->getPin();
    }
    xSemaphoreGive(mutex);

    serializeJson(*doc, f);
    delete doc;
    f.close();
}

void loadConfig() {
    if(!LittleFS.exists("/config.json")) return;
    File f = LittleFS.open("/config.json", "r");
    
    DynamicJsonDocument* doc = new DynamicJsonDocument(8192);
    deserializeJson(*doc, f);
    f.close();

    JsonArray arr = (*doc)["devices"];
    for(JsonObject obj : arr) {
        String type = obj["driver"]; int pin = obj["pin"];
        if(isPinValid(pin, type)) {
            Device* d = DeviceFactory::create(type, obj["id"], obj["name"], pin);
            if(d) { d->begin(); devices.push_back(d); }
        }
    }
    delete doc;
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    mutex = xSemaphoreCreateMutex();
    Wire.begin();
    
    if(!LittleFS.begin(true)) Serial.println("LITTLEFS Error");

    loadConfig();

    WiFiManager wm;
    wm.setClass("invert");
    if(!wm.autoConnect("OmniESP-V2", "admin1234")) ESP.restart();

    // --- API STATUS (STREAMING FOR STABILITY) ---
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req){
        AsyncResponseStream *res = req->beginResponseStream("application/json");
        res->print("{\"devices\":[");
        
        xSemaphoreTake(mutex, portMAX_DELAY);
        for(size_t i=0; i<devices.size(); i++) {
            Device* d = devices[i];
            res->print("{");
            res->printf("\"id\":\"%s\",\"name\":\"%s\",\"driver\":\"%s\",\"pin\":%d,\"val\":", 
                d->getId().c_str(), d->getName().c_str(), d->getDriver().c_str(), d->getPin());
            
            StaticJsonDocument<256> smallDoc;
            JsonObject val = smallDoc.to<JsonObject>();
            d->read(val);
            serializeJson(val, *res);
            
            res->print("}");
            if(i < devices.size()-1) res->print(",");
        }
        xSemaphoreGive(mutex);
        
        res->print("]}");
        req->send(res);
    });

    // --- API SCAN ---
    server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *req){
        xSemaphoreTake(mutex, portMAX_DELAY);
        String res = scanI2C();
        xSemaphoreGive(mutex);
        req->send(200, "application/json", res);
    });

    // --- API CONTROL ---
    server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *req){
        if(req->hasParam("id", true)) {
            String id = req->getParam("id", true)->value();
            xSemaphoreTake(mutex, portMAX_DELAY);
            for(auto d : devices) {
                if(d->getId() == id) {
                    if(req->hasParam("text", true)) d->writeText(req->getParam("text", true)->value());
                    else if(req->hasParam("cmd", true)) {
                        float v = req->hasParam("val", true) ? req->getParam("val", true)->value().toFloat() : 0;
                        d->write(req->getParam("cmd", true)->value(), v);
                    }
                }
            }
            xSemaphoreGive(mutex);
            req->send(200);
        } else req->send(400);
    });

    // --- API CONFIG ---
    server.onRequestBody([](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total){
        if(req->url() == "/api/config") {
            DynamicJsonDocument* doc = new DynamicJsonDocument(8192);
            DeserializationError error = deserializeJson(*doc, data);
            
            if (!error) {
                xSemaphoreTake(mutex, portMAX_DELAY);
                clearDevices(); 
                JsonArray arr = (*doc)["devices"];
                for(JsonObject obj : arr) {
                     String type = obj["driver"]; int pin = obj["pin"];
                     if (isPinValid(pin, type)) {
                        Device* d = DeviceFactory::create(type, obj["id"], obj["name"], pin);
                        if(d) { d->begin(); devices.push_back(d); }
                     }
                }
                xSemaphoreGive(mutex);
                saveConfig();
                req->send(200, "text/plain", "Saved");
            } else {
                req->send(400, "text/plain", "Invalid JSON");
            }
            delete doc;
        }
    });

    server.addHandler(&ws);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.begin();
}

// --- LOOP ---
void loop() {
    static unsigned long lastWsUpdate = 0;
    
    // WebSocket update every 2 seconds to avoid flooding
    if(millis() - lastWsUpdate > 2000) {
        lastWsUpdate = millis();
        
        if(ws.count() > 0) {
            DynamicJsonDocument* doc = new DynamicJsonDocument(4096);
            JsonArray arr = doc->createNestedArray("devices");
            
            if(xSemaphoreTake(mutex, (TickType_t)100) == pdTRUE) {
                for(auto d : devices) {
                    JsonObject obj = arr.createNestedObject();
                    obj["id"] = d->getId();
                    JsonObject val = obj.createNestedObject("val");
                    d->read(val);
                }
                xSemaphoreGive(mutex);
                
                String out; 
                serializeJson(*doc, out);
                ws.textAll(out);
            }
            delete doc;
        }
    }
}
