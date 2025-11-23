# ⚡ OmniESP OS

> **Le Framework IoT Industriel pour ESP32 : Modulaire, Extensible et No-Code.**

![PlatformIO](https://img.shields.io/badge/PlatformIO-Core-orange) ![Framework](https://img.shields.io/badge/Framework-Arduino%20%7C%20FreeRTOS-blue) ![License](https://img.shields.io/badge/License-MIT-green) ![Version](https://img.shields.io/badge/Version-1.0-red)

**OmniESP** est un firmware open-source pour ESP32 conçu pour transformer n'importe quelle carte de développement en une centrale domotique intelligente. Contrairement aux solutions rigides, OmniESP repose sur une architecture **Objet (C++)** et une configuration **JSON dynamique**, permettant d'ajouter, configurer et contrôler plus de 50 types de capteurs et actionneurs **sans écrire une seule ligne de code**.

---

## 🌟 Fonctionnalités Clés

*   **🔌 Architecture No-Code :** Assignez vos capteurs (Relais, DHT22, Boutons...) directement depuis l'interface Web.
*   **🚀 Extensibilité Maximale :** Architecture logicielle basée sur le Polymorphisme. Un noyau unique gère une infinité de drivers.
*   **💾 Persistance Intelligente :** Configuration stockée en **JSON** via **LittleFS**. Redémarrage à chaud sans perte de config.
*   **📱 Interface Web SPA :** Tableau de bord moderne, réactif (WebSocket) et mobile-friendly.
*   **⚡ Sécurité Hardware :** Protection logicielle contre les conflits de PINs (GPIO).
*   **🌐 API REST & WebSocket :** Intégration facile avec des tiers (Applications mobiles, Scripts Python, etc.).
*   **🛠️ 50+ Drivers Prêts à l'emploi :** Support natif des protocoles Digital, Analog, PWM, I2C, OneWire.

---

## 📦 Matériel Supporté

*   **MCU :** ESP32 (DevKit V1, NodeMCU, ESP32-S, etc.).
*   **Mémoire Flash :** Minimum 4MB recommandé.
*   **Système de fichiers :** LittleFS (Partitionnement `huge_app` ou `min_spiffs` recommandé).

---

## 🛠️ Installation et Flashement

Ce projet utilise **PlatformIO** (extension pour VS Code).

### 1. Prérequis
*   Visual Studio Code
*   Extension PlatformIO IDE

### 2. Configuration (`platformio.ini`)
Assurez-vous que votre fichier de configuration contient les dépendances nécessaires :

```ini
[env:omniesp_prod]
platform = espressif32
board = esp32dev
framework = arduino
board_build.filesystem = littlefs
board_build.partitions = huge_app.csv
monitor_speed = 115200

lib_deps =
    bblanchon/ArduinoJson
    esphome/ESPAsyncWebServer-esphome
    adafruit/Adafruit Unified Sensor
    adafruit/DHT sensor library
    adafruit/Adafruit NeoPixel
    paulstoffregen/OneWire
    milesburton/DallasTemperature
    roboticsbrno/ServoESP32
```

### 3. Téléversement (Important !)

OmniESP nécessite deux étapes de téléversement : le code (Firmware) et l'interface Web (Filesystem).

1.  Connectez votre ESP32 en USB.
2.  Ouvrez l'onglet **PlatformIO** (Tête d'Alien à gauche).
3.  Exécutez **`Platform > Upload Filesystem Image`** (Envoie `data/index.html`).
4.  Exécutez **`General > Upload`** (Envoie le Firmware compilé).

---

## 📖 Guide Utilisateur (Mode "Papy")

Une fois flashé, connectez-vous au WiFi configuré et accédez à l'adresse IP de l'ESP32 (ex: `http://192.168.1.50`).

### 1. Tableau de Bord (Dashboard)
C'est la vue principale. Elle affiche l'état de tous vos appareils en temps réel.
*   **Relais/Lumières :** Cliquez pour Allumer/Éteindre.
*   **Capteurs (Temp, Gaz...) :** Les valeurs s'actualisent automatiquement.
*   **Servos/LEDs :** Utilisez les curseurs pour ajuster.

### 2. Configuration (Ajout de Périphériques)
Allez dans l'onglet **"Appareils"**.

1.  **Type :** Choisissez votre composant dans la liste (ex: `Relais`, `DHT22`, `Servo`).
2.  **Nom :** Donnez un nom convivial (ex: "Lampe Salon").
3.  **Pin (GPIO) :** Sélectionnez le numéro du Pin où vous avez branché le fil sur l'ESP32.
    *   *Note : Le système empêche d'utiliser deux fois le même Pin pour éviter les courts-circuits.*
4.  Cliquez sur **Ajouter**.
5.  Une fois terminé, cliquez sur **💾 SAUVEGARDER & REDÉMARRER**.

---

## 📚 Catalogue des Drivers (v1.0)

OmniESP embarque une factory intelligente capable de piloter ces périphériques :

### 🟢 Actionneurs (Sorties)
| Type | Description | Usage Typique |
| :--- | :--- | :--- |
| **RELAY** | Sortie ON/OFF standard | Lampes, Prises, Relais |
| **VALVE** | Sortie ON/OFF | Électrovannes d'arrosage |
| **LOCK** | Sortie Impulsionnelle | Gâches électriques |
| **SERVO** | PWM (0-180°) | Bras robotiques, Verrous méca |
| **NEOPIXEL** | LED Adressables | Rubans LED RGB (WS2812B) |
| **LIGHT_INV** | Relais Inversé (Active LOW) | Modules relais chinois |

### 🔵 Capteurs (Entrées Numériques)
| Type | Description | Usage Typique |
| :--- | :--- | :--- |
| **BUTTON** | Entrée Pullup | Boutons poussoirs |
| **DOOR** | Contact Magnétique | Sécurité porte/fenêtre |
| **PIR** | Infrarouge Passif | Détection de mouvement |
| **VIBRATION** | Capteur SW-420 | Détection de chocs/bris de glace |

### 🟠 Capteurs (Entrées Analogiques)
| Type | Description | Usage Typique |
| :--- | :--- | :--- |
| **MQ2** | Gaz / Fumée | Sécurité Incendie |
| **SOIL** | Humidité Capacitive | Plantes / Jardin |
| **LDR** | Photo-résistance | Détection Jour/Nuit |
| **VOLTAGE** | Pont diviseur | Mesure batterie (0-3.3V) |

### 🟣 Capteurs Spécifiques (Bus)
| Type | Description | Protocole |
| :--- | :--- | :--- |
| **DHT11/22** | Température & Humidité | Digital One-Wire |
| **DS18B20** | Température Étanche | OneWire (Dallas) |

---

## 💻 API pour les Développeurs

OmniESP expose une API RESTful complète pour l'intégration.

### 1. Obtenir l'état (`GET`)
**Endpoint :** `/api/status`
**Réponse :** JSON contenant tous les devices et leurs valeurs.
```json
{
  "devices": [
    { "id": "relay_23", "name": "Salon", "driver": "RELAY", "pin": 23, "val": { "val": 1, "human": "ON" } },
    { "id": "dht_4", "name": "Temp", "driver": "DHT22", "pin": 4, "val": { "temp": 24.5, "hum": 60 } }
  ]
}
```

### 2. Contrôler un appareil (`POST`)
**Endpoint :** `/api/control`
**Paramètres (Query ou Body) :**
*   `id` : L'identifiant du device (ex: `relay_23`).
*   `cmd` : La commande (`toggle`, `set`).
*   `val` : (Optionnel) La valeur (ex: `1` pour ON, `90` pour Servo).

**Exemple cURL :**
```bash
# Allumer la lampe
curl -X POST "http://ip-esp/api/control?id=relay_23&cmd=set&val=1"
```

### 3. Configuration Système (`POST`)
**Endpoint :** `/api/config`
**Body :** JSON complet de la configuration (Devices + Settings).
Utilisé par l'interface Web pour la sauvegarde.

---

## 📂 Structure du Projet

```text
OmniESP/
├── data/                  # Fichiers Web (Interface)
│   └── index.html         # Le Dashboard (HTML/JS/CSS)
├── src/
│   ├── main.cpp           # Point d'entrée, WebServer, API
│   └── OmniDrivers.h      # Le Cœur : Classes Drivers & Factory
├── platformio.ini         # Configuration du Build & Libs
└── README.md              # Ce fichier
```

---

## 🤝 Contribution

Les contributions sont les bienvenues ! Pour ajouter un nouveau driver :
1.  Définissez la classe dans `OmniDrivers.h` (héritez de `Device`).
2.  Ajoutez la condition dans `DeviceFactory::create`.
3.  Ajoutez l'option dans le `<select>` du fichier `index.html`.
4.  Compilez !

**Auteur :** NeoRak

**Licence :** MIT
