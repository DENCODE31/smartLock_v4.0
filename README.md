# 🔐 SmartLock 4.0 — Puertardino Next Gen

> Cerradura inteligente con **reconocimiento facial**, **huella dactilar**, **NFC**, **PIN** y **control por voz (Alexa)** — diseñada desde cero en Altium Designer, firmware en C puro con ESP-IDF, carcasa impresa en 3D.

![Estado](https://img.shields.io/badge/estado-en%20desarrollo-yellow)
![MCU](https://img.shields.io/badge/MCU-ESP32--WROOM--32E-blue)
![Framework](https://img.shields.io/badge/framework-ESP--IDF%20v5.3.2-red)
![Fase](https://img.shields.io/badge/fase-F1%20validaci%C3%B3n-orange)

---

## ¿Qué es SmartLock 4.0?

Sistema de control de acceso embebido de **4 factores de autenticación** construido sobre hardware diseñado a medida. No es un módulo comercial, no es un Arduino con librerías random — es un sistema completo diseñado con criterios de ingeniería profesional: PCB en Altium, firmware en C con FreeRTOS, integración cloud real.

Es la cuarta generación de un sistema que lleva más de 3 años funcionando en producción, evolucionando desde un teclado matricial hasta un sistema IoT con visión por computadora.

---

## ✨ Características

| Feature | Tecnología |
|---|---|
| 👁️ Reconocimiento facial | ESP32-S3-EYE + ESP-WHO + ESP-DL |
| 🖐️ Huella dactilar | Sensor GROW R503 (UART) |
| 📡 NFC / RFID | PN532 (SPI) — compatible con iPhone |
| 🔢 PIN táctil | Nextion 5" HMI |
| 🗣️ Alexa / App móvil | ESP-RainMaker (iOS + Android) |
| 🔄 OTA | Dual-bank OTA via ESP-RainMaker |
| 🔔 Notificaciones | Push via ESP-RainMaker cloud |
| 📊 Log de accesos | SPIFFS — registro por usuario, método y timestamp |

---

## 🏗️ Arquitectura del sistema

```
                       12V DC ENTRADA
                            │
                  ┌─────────▼─────────┐
                  │   Buck LM2596      │
                  │   12V → 5V 3A     │
                  └─────────┬─────────┘
                            │
               ┌────────────┼──────────────────────┐
               ▼            ▼                        ▼
         ┌──────────┐ ┌──────────┐       ┌──────────────────┐
         │ TP4056   │ │  ESP32   │       │   Nextion 5"     │
         │ Li-ion   │ │ WROOM-32 │       │   HMI táctil     │
         └────┬─────┘ │ (core)   │       └──────────────────┘
              ▼       └────┬─────┘
        ┌────────┐         │
        │ 18650  │         │
        └────────┘         │
              ┌────────────┼──────────────────────┐
              ▼            ▼                        ▼
         ┌────────┐ ┌─────────────┐    ┌──────────────────┐
         │  R503  │ │ ESP32-S3-EYE│    │     PN532        │
         │ Huella │ │  Visión IA  │    │   NFC / RFID     │
         └────────┘ └─────────────┘    └──────────────────┘

              ┌──────────┐  ┌──────────┐  ┌──────────┐
              │  MOSFET  │  │  Buzzer  │  │ Reed SW  │
              │ IRLZ44N  │  │  PWM     │  │  Puerta  │
              └──────────┘  └──────────┘  └──────────┘

                    ☁️  ESP-RainMaker — Alexa + App + OTA
```

---

## 📁 Estructura del repositorio

```
smartlock 4.0/
├── SMARTLOCK_4.0.md          ← Documentación maestra del proyecto
├── FIRMWARE.md               ← Pines, build, arquitectura, feature flags
├── CARCASA.md                ← Diseño 3D, impresión, tolerancias
├── datasheets/               ← Datasheets de todos los componentes
├── nextion_ui/               ← Proyectos HMI Nextion (.HMI, .tft)
├── carcasa_3d/               ← Archivos Fusion 360 + STL
├── altium/                   ← Proyecto Altium Designer (PCB)
└── firmware/
    └── smartlock_v4/         ← Proyecto ESP-IDF
        ├── main/
        │   ├── main.c
        │   ├── app_config.h  ← Pines + feature flags
        │   ├── managers/     ← Lógica de cada periférico
        │   └── hal/          ← Drivers de bajo nivel
        └── partitions.csv    ← OTA dual + SPIFFS
```

---

## 🔩 Hardware

| Componente | Modelo | Función |
|---|---|---|
| MCU principal | ESP32-WROOM-32E-N8 | Core del sistema |
| Visión facial | ESP32-S3-EYE | Reconocimiento facial con ESP-WHO |
| Huella | GROW R503 | Sensor óptico UART |
| NFC | PN532 | Lector NFC/RFID SPI |
| Display | Nextion 5" | HMI táctil |
| Buck converter | LM2596S-ADJ | 12V → 5V 3A |
| Cargador batería | TP4056 | Carga Li-ion 18650 |
| Driver cerradura | IRLZ44N (MOSFET) | Control 12V logic-level |
| PCB | JLCPCB 2 capas | Diseño en Altium Designer |
| Carcasa | PLA+ / PETG | Impresión en Anycubic Kobra 2 Neo |

---

## 🧠 Arquitectura de firmware

Diseñada en capas: HAL → Managers → App. Cada periférico tiene su propio manager, el `auth_manager` orquesta todos los métodos de autenticación y el `lock_manager` maneja la máquina de estados de la cerradura.

```
app_main()
    └── lock_manager       ← Máquina de estados: LOCKED / UNLOCKING
         └── auth_manager  ← Orquesta: huella | NFC | PIN | facial | Alexa
              ├── fingerprint_manager  (R503 UART)
              ├── nfc_manager          (PN532 SPI)
              ├── face_manager         (S3-EYE UART)
              ├── display_manager      (Nextion UART)
              ├── cloud_manager        (ESP-RainMaker)
              └── log_manager          (SPIFFS)
         └── hal/
              ├── relay.c   (MOSFET driver)
              ├── buzzer.c  (PWM)
              └── button.c  (debounce)
```

---

## 🚀 Cómo compilar

### Requisitos
- ESP-IDF v5.3.2
- VS Code + ESP-IDF Extension

### Build & Flash

```powershell
# Exportar entorno (PowerShell)
C:\Espressif\frameworks\esp-idf-v5.3.2\export.ps1

# Build
cd firmware\smartlock_v4
idf.py build

# Flash
idf.py -p COM10 flash

# Monitor serial
idf.py -p COM10 monitor
```

### Prototipo vs PCB final

El proyecto soporta dos targets mediante feature flag en `CMakeLists.txt`:

| Target | Board | Flag |
|---|---|---|
| Prototipo F1/F2 | Heltec WiFi LoRa 32 V3 | `-DHELTEC_V3` |
| PCB Final | ESP32-WROOM-32E-N8 | *(sin flag)* |

---

## 📅 Fases del proyecto

| Fase | Descripción | Estado |
|---|---|---|
| **F1** | Validación en protoboard — periférico por periférico | 🟡 En progreso |
| **F2** | Integración completa — todos los periféricos + RainMaker | ⬜ Pendiente |
| **F3** | PCB en Altium + fabricación JLCPCB (5 uds) | ⬜ Pendiente |
| **F4** | Bring-up PCB + pruebas funcionales | ⬜ Pendiente |
| **F5** | Carcasa 3D en Fusion 360 + impresión | ⬜ Pendiente |
| **F6** | Release — documentación, fotos, video demo | ⬜ Pendiente |

---

## 🛠️ Stack tecnológico

| Área | Herramienta |
|---|---|
| Firmware | ESP-IDF v5.3.2 · C · FreeRTOS |
| PCB | Altium Designer · JLCPCB |
| CAD mecánico | Fusion 360 |
| Impresión 3D | Anycubic Kobra 2 Neo |
| HMI | Nextion Editor |
| Cloud | ESP-RainMaker · Alexa |
| Control de versiones | Git · GitHub |

---

## 📜 Historial del proyecto

| Versión | Plataforma | Auth | Estado |
|---|---|---|---|
| PUERTA_2.0 | Arduino | Teclado 4x4 | Prototipo |
| PUERTARDUINO V1 | Arduino Uno | Huella AS608 + LCD | ✅ 3 años en producción |
| PUERTARDUINO 4.0 | Arduino Mega + Nextion | Huella + modos auto/manual | ❌ Incompleto |
| **SmartLock 4.0** | **ESP32 + S3-EYE** | **Facial + Huella + NFC + PIN + Alexa** | **🚀 En desarrollo** |

---

*Proyecto de ingeniería electrónica — diseño hardware + firmware embebido desde cero.*
