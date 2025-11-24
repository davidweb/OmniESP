#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <vector>

// --- LIBRARIES ---
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <Adafruit_BME280.h>
#include <Adafruit_INA219.h>
#include <BH1750.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_SSD1306.h>

enum DeviceType { SENSOR_BIN, SENSOR_VAL, ACTUATOR_BIN, ACTUATOR_VAL, DISPLAY_DEV };

class Device {
protected:
    String _id, _name, _driver;
    int _pin;
public:
    Device(String id, String name, String driver, int pin) 
        : _id(id), _name(name), _driver(driver), _pin(pin) {}
    virtual ~Device() {}

    String getId() { return _id; }
    String getName() { return _name; }
    String getDriver() { return _driver; } 
    int getPin() { return _pin; }
    
    virtual void begin() = 0;
    virtual void read(JsonObject& doc) = 0; 
    virtual void write(String cmd, float val) {} 
    virtual void writeText(String text) {} 
    virtual DeviceType getType() = 0;
};

// ==========================================
// 1. DRIVERS GPIO (DIGITAL / ANALOG)
// ==========================================

class Driver_Digital : public Device {
    bool _isOutput, _inverted, _state;
public:
    Driver_Digital(String id, String name, String type, int pin, bool out, bool inv) 
        : Device(id, name, type, pin), _isOutput(out), _inverted(inv), _state(false) {}
    
    void begin() override { 
        pinMode(_pin, _isOutput ? OUTPUT : INPUT_PULLUP); 
        if(_isOutput) apply(); 
    }
    
    void write(String cmd, float val) override { 
        if(!_isOutput) return; 
        _state = (cmd == "toggle") ? !_state : (val >= 1); 
        apply(); 
    }
    
    void apply() { digitalWrite(_pin, _inverted ? !_state : _state); }
    
    void read(JsonObject& doc) override {
        if(!_isOutput) {
            bool phy = digitalRead(_pin);
            _state = (phy == (_inverted ? LOW : HIGH));
        }
        doc["val"] = _state ? 1 : 0;
        doc["human"] = _state ? "ON" : "OFF";
    }
    DeviceType getType() override { return _isOutput ? ACTUATOR_BIN : SENSOR_BIN; }
};

class Driver_Analog : public Device {
public:
    Driver_Analog(String id, String name, String type, int pin) : Device(id, name, type, pin) {}
    void begin() override { pinMode(_pin, INPUT); }
    void read(JsonObject& doc) override {
        int raw = analogRead(_pin);
        doc["val"] = raw;
        doc["volts"] = (raw * 3.3) / 4095.0;
    }
    DeviceType getType() override { return SENSOR_VAL; }
};

// ==========================================
// 2. DRIVERS CAPTEURS COMPLEXES (NON-BLOQUANTS)
// ==========================================

class Driver_DHT : public Device {
    DHT* dht;
    float lastT = 0, lastH = 0;
    unsigned long lastRead = 0;
public:
    Driver_DHT(String id, String name, int pin, int type) 
        : Device(id, name, type==DHT11?"DHT11":"DHT22", pin) { dht = new DHT(pin, type); }
    ~Driver_DHT() { delete dht; }
    
    void begin() override { dht->begin(); }
    
    void read(JsonObject& doc) override {
        // NON-BLOCKING LOGIC: Read only every 2 seconds
        if(millis() - lastRead > 2000) {
            float t = dht->readTemperature();
            float h = dht->readHumidity();
            if(!isnan(t) && !isnan(h)) {
                lastT = t; lastH = h;
                lastRead = millis();
            }
        }
        doc["temp"] = lastT; 
        doc["hum"] = lastH;
    }
    DeviceType getType() override { return SENSOR_VAL; }
};

class Driver_Dallas : public Device {
    OneWire* oneWire; DallasTemperature* sensors;
    float lastT = -127;
    unsigned long lastRead = 0;
public:
    Driver_Dallas(String id, String name, int pin) : Device(id, name, "DS18B20", pin) { 
        oneWire = new OneWire(pin); 
        sensors = new DallasTemperature(oneWire); 
    }
    ~Driver_Dallas() { delete sensors; delete oneWire; }
    
    void begin() override { sensors->begin(); }
    
    void read(JsonObject& doc) override {
        // NON-BLOCKING: Request conversion sparingly
        if(millis() - lastRead > 2000) {
            sensors->requestTemperatures();
            lastT = sensors->getTempCByIndex(0);
            lastRead = millis();
        }
        doc["temp"] = lastT;
    }
    DeviceType getType() override { return SENSOR_VAL; }
};

// ==========================================
// 3. ACTIONNEURS AVANCÉS
// ==========================================

class Driver_Servo : public Device {
    Servo servo; int _pos = 0;
public:
    Driver_Servo(String id, String name, int pin) : Device(id, name, "SERVO", pin) {}
    ~Driver_Servo() { servo.detach(); }
    
    void begin() override {
        // ESP32Servo specific setup
        servo.setPeriodHertz(50);
        servo.attach(_pin, 500, 2400); 
    }
    
    void write(String cmd, float val) override { 
        _pos = constrain((int)val, 0, 180); 
        servo.write(_pos); 
    }
    void read(JsonObject& doc) override { doc["angle"] = _pos; }
    DeviceType getType() override { return ACTUATOR_VAL; }
};

class Driver_Neo : public Device {
    Adafruit_NeoPixel* pixels; int _count;
public:
    Driver_Neo(String id, String name, int pin, int count) 
        : Device(id, name, "NEOPIXEL", pin), _count(count) { 
        pixels = new Adafruit_NeoPixel(count, pin, NEO_GRB + NEO_KHZ800); 
    }
    ~Driver_Neo() { delete pixels; }
    
    void begin() override { pixels->begin(); pixels->show(); }
    
    void write(String cmd, float val) override {
        uint32_t color = pixels->ColorHSV((uint16_t)val);
        for(int i=0; i<_count; i++) pixels->setPixelColor(i, color);
        pixels->show();
    }
    void read(JsonObject& doc) override { doc["status"] = "Active"; }
    DeviceType getType() override { return ACTUATOR_VAL; }
};

// ==========================================
// 4. DRIVERS I2C INDUSTRIELS
// ==========================================

class Driver_I2C_Base : public Device {
public:
    Driver_I2C_Base(String id, String name, String type, int addr) : Device(id, name, type, addr) {}
};

class Driver_INA219 : public Driver_I2C_Base {
    Adafruit_INA219* ina;
public:
    Driver_INA219(String id, String name, int addr) : Driver_I2C_Base(id, name, "INA219", addr) { ina = new Adafruit_INA219(addr); }
    ~Driver_INA219() { delete ina; }
    void begin() override { if(!ina->begin()) Serial.println("INA Fail"); }
    void read(JsonObject& doc) override {
        doc["volts"] = ina->getBusVoltage_V();
        doc["mA"] = ina->getCurrent_mA();
        doc["mW"] = ina->getPower_mW();
    }
    DeviceType getType() override { return SENSOR_VAL; }
};

class Driver_BME280 : public Driver_I2C_Base {
    Adafruit_BME280* bme;
public:
    Driver_BME280(String id, String name, int addr) : Driver_I2C_Base(id, name, "BME280", addr) { bme = new Adafruit_BME280(); }
    ~Driver_BME280() { delete bme; }
    void begin() override { bme->begin(_pin); }
    void read(JsonObject& doc) override {
        doc["temp"] = bme->readTemperature();
        doc["hum"] = bme->readHumidity();
        doc["pres"] = bme->readPressure() / 100.0F;
    }
    DeviceType getType() override { return SENSOR_VAL; }
};

class Driver_BH1750 : public Driver_I2C_Base {
    BH1750* lightMeter;
public:
    Driver_BH1750(String id, String name, int addr) : Driver_I2C_Base(id, name, "BH1750", addr) { lightMeter = new BH1750(addr); }
    ~Driver_BH1750() { delete lightMeter; }
    void begin() override { lightMeter->begin(); }
    void read(JsonObject& doc) override { doc["lux"] = lightMeter->readLightLevel(); }
    DeviceType getType() override { return SENSOR_VAL; }
};

class Driver_LCD : public Driver_I2C_Base {
    LiquidCrystal_I2C* lcd;
    String _txt = "Ready";
public:
    Driver_LCD(String id, String name, int addr) : Driver_I2C_Base(id, name, "LCD_I2C", addr) { lcd = new LiquidCrystal_I2C(addr, 16, 2); }
    ~Driver_LCD() { delete lcd; }
    void begin() override { lcd->init(); lcd->backlight(); lcd->setCursor(0,0); lcd->print("OmniESP V2"); }
    void writeText(String text) override {
        lcd->clear();
        lcd->setCursor(0,0); lcd->print(_name.substring(0,16));
        lcd->setCursor(0,1); lcd->print(text.substring(0,16));
        _txt = text;
    }
    void write(String cmd, float val) override { writeText(String(val)); }
    void read(JsonObject& doc) override { doc["display"] = _txt; }
    DeviceType getType() override { return DISPLAY_DEV; }
};

// --- DRIVER OLED (NOUVEAU) ---
class Driver_OLED : public Driver_I2C_Base {
    Adafruit_SSD1306* display;
    String _txt = "Ready";
public:
    Driver_OLED(String id, String name, int addr) : Driver_I2C_Base(id, name, "OLED", addr) {
        display = new Adafruit_SSD1306(128, 64, &Wire, -1);
    }
    ~Driver_OLED() { delete display; }
    
    void begin() override {
        if(!display->begin(SSD1306_SWITCHCAPVCC, _pin)) { Serial.println("OLED Fail"); return; }
        display->clearDisplay();
        display->setTextSize(1); display->setTextColor(SSD1306_WHITE);
        display->setCursor(0,0); display->println("OmniESP V2");
        display->println("Industrial"); display->display();
    }
    
    void writeText(String text) override {
        display->clearDisplay();
        display->setTextSize(1); display->setCursor(0,0); display->println(_name);
        display->drawLine(0, 10, 128, 10, SSD1306_WHITE);
        display->setTextSize(2); display->setCursor(0, 20); display->println(text);
        display->display();
        _txt = text;
    }
    void write(String cmd, float val) override { writeText(String(val)); }
    void read(JsonObject& doc) override { doc["display"] = _txt; }
    DeviceType getType() override { return DISPLAY_DEV; }
};

// ==========================================
// FACTORY
// ==========================================
class DeviceFactory {
public:
    static Device* create(String type, String id, String name, int pin) {
        // Digital
        if (type == "RELAY" || type == "VALVE" || type == "LOCK") return new Driver_Digital(id, name, type, pin, true, false);
        if (type == "BUTTON" || type == "DOOR") return new Driver_Digital(id, name, type, pin, false, true);
        if (type == "PIR") return new Driver_Digital(id, name, type, pin, false, false);
        
        // Analog / Specific
        if (type == "LDR" || type == "SOIL" || type == "MQ2") return new Driver_Analog(id, name, type, pin);
        if (type == "DHT22") return new Driver_DHT(id, name, pin, DHT22);
        if (type == "DHT11") return new Driver_DHT(id, name, pin, DHT11);
        if (type == "DS18B20") return new Driver_Dallas(id, name, pin);
        if (type == "SERVO") return new Driver_Servo(id, name, pin);
        if (type == "NEOPIXEL") return new Driver_Neo(id, name, pin, 16);

        // I2C
        if (type == "INA219") return new Driver_INA219(id, name, pin);
        if (type == "BME280") return new Driver_BME280(id, name, pin);
        if (type == "BH1750") return new Driver_BH1750(id, name, pin);
        if (type == "LCD_I2C") return new Driver_LCD(id, name, pin);
        if (type == "OLED") return new Driver_OLED(id, name, pin);

        return nullptr;
    }
};
