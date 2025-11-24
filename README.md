# ⚡ OmniESP OS - Version 2.0 (Industrial Grade)

![Version](https://img.shields.io/badge/version-2.0.0-blue.svg) ![Platform](https://img.shields.io/badge/platform-ESP32-green.svg) ![License](https://img.shields.io/badge/license-MIT-orange.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Core-orange) ![Framework](https://img.shields.io/badge/Framework-Arduino%20%7C%20FreeRTOS-blue).


**OmniESP OS** est un firmware universel "No-Code" pour ESP32. Il transforme votre microcontrôleur en une centrale domotique autonome, configurable via une interface web moderne, sans jamais avoir besoin de recompiler le code.

La **Version 2.0** introduit une architecture de grade industriel, une gestion native du bus I2C avec scanner intégré, et le support de capteurs de précision.

---

## 🚀 Nouveautés de la V2.0

*   **🛡️ Stabilité Industrielle :** Gestion stricte de la mémoire (plus de fuites), Thread-safety (Mutex) et validation des broches GPIO/I2C.
*   **🔍 Scanner I2C Intégré :** Outil de détection automatique des adresses des capteurs (plus besoin de deviner si c'est `0x27` ou `0x3C`).
*   **🏭 Drivers Industriels :** Support natif pour :
    *   **INA219** : Monitoring de tension, courant et puissance (DC).
    *   **BME280/BMP280** : Température, Humidité et Pression atmosphérique de précision.
    *   **BH1750** : Luxmètre numérique haute résolution.
    *   **LCD I2C (1602/2004)** : Affichage de messages et valeurs en temps réel.
*   **📶 WiFiManager :** Portail captif pour la configuration du WiFi (plus d'identifiants codés en dur).

---

## ✨ Fonctionnalités Principales

*   **Interface Web Responsive :** Tableau de bord temps réel et panneau de configuration (Mobile & Desktop).
*   **WebSockets :** Mise à jour instantanée des valeurs sans rafraîchir la page.
*   **Persistance :** Configuration sauvegardée en mémoire Flash (LittleFS).
*   **API REST :** Interagir avec l'ESP32 depuis des systèmes tiers (Jeedom, Home Assistant, Node-RED).
*   **Hot-Plug (Logique) :** Ajoutez ou supprimez des composants via l'interface web, le système redémarre et applique la config.

---

## 📦 Matériel Compatible (Drivers)

### 🏭 Périphériques I2C (Nouveau V2)
| Type | Driver | Description |
| :--- | :--- | :--- |
| **Énergie** | `INA219` | Voltmètre/Ampèremètre/Wattmètre (ex: Panneaux solaires) |
| **Météo** | `BME280` | Température, Humidité, Pression (Baromètre) |
| **Lumière** | `BH1750` | Intensité lumineuse précise (Lux) |
| **Écran** | `LCD_I2C` | Écrans LCD 16x2 ou 20x4 avec backpack I2C |
| **Écran** | `OLED` | (Support expérimental SSD1306) |

### 🔌 GPIO Standard (Compatible V1)
| Type | Driver | Description |
| :--- | :--- | :--- |
| **Relais** | `RELAY` | Contrôle ON/OFF (Lumières, Prises) |
| **Vanne** | `VALVE` | Électrovannes pour l'arrosage |
| **Servo** | `SERVO` | Contrôle d'angle (0-180°) |
| **LED** | `NEOPIXEL` | Rubans LED RGB adressables (WS2812B) |
| **Temp** | `DHT11/22` | Capteurs T°/Humidité classiques |
| **Temp** | `DS18B20` | Sonde de température étanche (OneWire) |
| **Entrée** | `BUTTON` | Boutons poussoirs, Interrupteurs |
| **Sécu** | `PIR/DOOR` | Détecteurs de mouvement ou d'ouverture |
| **Analog** | `MQ2/LDR` | Gaz, Luminosité analogique, Sol, etc. |

---

## 🛠️ Installation

### Prérequis
*   **VS Code** avec l'extension **PlatformIO**.
*   Une carte **ESP32** (ESP32 DevKit V1 recommandé).

### Étapes
1.  **Cloner le dépôt :**
    ```bash
    git clone https://github.com/davidweb/OmniESP.git
    ```
2.  **Ouvrir dans PlatformIO :** Ouvrez le dossier du projet.
3.  **Compiler et Téléverser le Firmware :**
    *   Cliquez sur la flèche `➡️` (Upload) dans la barre d'outils PlatformIO.
4.  **Téléverser l'Interface Web (Filesystem) :**
    *   Allez dans l'onglet `PlatformIO` (tête d'alien sur la gauche).
    *   Dans `Project Tasks` -> `omniesp_v2_industrial` -> `Platform`.
    *   Cliquez sur **`Upload Filesystem Image`**.
    *   *(Cette étape copie les fichiers HTML/CSS dans la mémoire de l'ESP32)*.

---

## 📖 Guide de Démarrage

### 1. Première Connexion (WiFi)
Au premier démarrage, l'ESP32 crée un point d'accès WiFi.
1.  Connectez-vous au WiFi : **`OmniESP-V2`**.
2.  Le portail de configuration s'ouvre (sinon aller sur `192.168.4.1`).
3.  Cliquez sur **Configure WiFi**, choisissez votre box et entrez le mot de passe.
4.  L'ESP32 redémarre et se connecte à votre réseau.

### 2. Configuration des Périphériques
1.  Trouvez l'adresse IP de l'ESP32 (via le moniteur série ou votre routeur).
2.  Ouvrez l'interface web : `http://<IP_ESP32>`.
3.  Allez dans l'onglet **⚙️ Configuration**.
4.  **Pour les capteurs I2C** :
    *   Sélectionnez le type (ex: `BME280`).
    *   Cliquez sur **🔍 Scan I2C**.
    *   Sélectionnez l'adresse détectée (ex: `0x76`).
5.  **Pour les GPIO** :
    *   Sélectionnez le type (ex: `RELAY`).
    *   Choisissez le PIN GPIO (ex: `GPIO 23`).
6.  Cliquez sur **Ajouter** puis **💾 Sauvegarder & Redémarrer**.

---

## 🔌 API Documentation

OmniESP expose une API JSON complète pour l'intégration.

### 1. Statut Global
Récupère l'état de tous les périphériques.
*   **GET** `/api/status`
*   **Réponse :**
    ```json
    {
      "devices": [
        { "id": "relay_23", "name": "Lampe", "val": { "val": 1, "human": "ON" } },
        { "id": "bme_118", "name": "Météo", "val": { "temp": 24.5, "hum": 50, "pres": 1013 } }
      ]
    }
    ```

### 2. Contrôle
Piloter un actionneur.
*   **POST** `/api/control`
*   **Paramètres :**
    *   `id`: L'ID du composant (ex: `relay_23`).
    *   `cmd`: Commande (`toggle`, `set`).
    *   `val`: Valeur numérique (optionnel, pour Servo/Dimmer).
    *   `text`: Texte à afficher (optionnel, pour LCD uniquement).

### 3. Scan I2C
Scanner le bus I2C.
*   **GET** `/api/scan`
*   **Réponse :** Liste des adresses hexadécimales et décimales trouvées.

---

## 🏗️ Architecture Technique

Le projet repose sur le **Factory Pattern** en C++.
*   `OmniDrivers.h` : Contient la classe mère `Device` et toutes les classes filles (`Driver_Digital`, `Driver_INA219`, etc.).
*   `main.cpp` : Gère le serveur AsyncWebServer, les WebSockets, la boucle d'automatisation et le scanner.
*   **Thread Safety** : Utilisation de sémaphores FreeRTOS (`xSemaphoreTake`) pour éviter les conflits entre la lecture des capteurs (Loop) et les requêtes HTTP (Server).

---

## 📄 Licence

Ce projet est sous licence MIT. Vous êtes libre de l'utiliser, le modifier et le distribuer.
*Développé par NeoRak.*
