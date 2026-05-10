# SmartLock 4.0 — Firmware

> Documentación específica de firmware. Para visión general del proyecto ver `SMARTLOCK_4.0.md`.

---

## MCU Principal — ESP32-WROOM-32E-N8

| Especificación | Valor |
|---|---|
| Módulo | ESP32-WROOM-32E-N8 |
| Chip | ESP32-D0WD-V3 (ECO V3) |
| Flash | 8MB Quad SPI |
| CPU | Xtensa LX6 dual-core @ 240MHz |
| Interfaces | 3x UART, 3x SPI, 2x I2C, I2S, PWM |
| IDF target | `idf.py set-target esp32` |

> Durante F1/F2 se valida con Heltec WiFi LoRa 32 V3 (ESP32-S3, target `esp32s3`).

---

## Toolchain

- **IDE:** VS Code + ESP-IDF Extension
- **Framework:** ESP-IDF nativo v5.3.2 (NO PlatformIO)
- **Target proto:** `idf.py set-target esp32s3` (Heltec V3)
- **Target final:** `idf.py set-target esp32` (WROOM-32E-N8)

### Build & Flash

```powershell
# Export entorno (PowerShell)
C:\Espressif\frameworks\esp-idf-v5.3.2\export.ps1

# Build
cd firmware\smartlock_v4
idf.py build

# Flash
idf.py -p COM10 flash

# Monitor serial
idf.py -p COM10 monitor
```

---

## Estructura del proyecto

```
firmware/smartlock_v4/
├── CMakeLists.txt                  # Proyecto ESP-IDF root
├── partitions.csv                  # Tabla particiones (OTA dual)
├── sdkconfig                       # Config ESP-IDF (generado)
├── build/                          # Build output
├── main/
│   ├── CMakeLists.txt              # Compila con -DHELTEC_V3
│   ├── main.c                      # Entry point + heartbeat
│   ├── app_config.h                # Pines + feature flags
│   ├── managers/
│   │   ├── lock_manager.c/h        # Máquina de estados cerradura
│   │   ├── auth_manager.c/h        # Orquesta autenticación
│   │   ├── fingerprint_manager.c/h # R503 UART driver
│   │   ├── nfc_manager.c/h         # PN532 driver
│   │   ├── face_manager.c/h        # Comunicación UART S3-EYE
│   │   ├── display_manager.c/h     # Nextion protocolo UART
│   │   ├── cloud_manager.c/h       # ESP-RainMaker
│   │   └── log_manager.c/h         # Eventos en flash
│   └── hal/
│       ├── relay.c/h               # Driver MOSFET cerradura
│       ├── buzzer.c/h              # PWM buzzer
│       └── button.c/h              # Debounce pulsadores
└── components/
    └── esp-rainmaker/              # Componente ESP-RainMaker
```

---

## Mapeo de pines

### Heltec WiFi LoRa 32 V3 (F1/F2 — prototipo)

Activado con `#define HELTEC_V3` en `main/CMakeLists.txt`.

| Periférico | Protocolo | Pines | Notas |
|---|---|---|---|
| Debug serial | UART0 | GPIO43(TX), GPIO44(RX) | Fijo en Heltec V3 |
| Nextion 5" | UART1 | GPIO4(TX), GPIO5(RX) | 115200 baud |
| R503 Huella | UART2 | GPIO6(TX), GPIO7(RX) | 57600 baud |
| ESP32-S3-EYE | UART1 | GPIO19(TX), GPIO20(RX) | Comparte UART1 con Nextion (test por separado) |
| PN532 NFC | I2C0 | GPIO41(SDA), GPIO42(SCL) | SPI ocupado por LoRa |
| MOSFET cerradura | GPIO | GPIO47 | OUT |
| Buzzer | GPIO | GPIO48 | PWM |
| LED onboard | GPIO | GPIO35 | Active low |
| Vext enable | GPIO | GPIO36 | Set LOW para Vext |
| Pulsador interior | GPIO | GPIO1 | INPUT_PULLUP |
| Reed switch | GPIO | GPIO2 | INPUT_PULLUP |

### PCB Final — ESP32-WROOM-32E-N8

| Periférico | Protocolo | Pines | Notas |
|---|---|---|---|
| Debug serial | UART0 | GPIO1(TX), GPIO3(RX) | |
| Nextion 5" | UART2 | GPIO17(TX), GPIO16(RX) | 115200 baud |
| R503 Huella | UART1 | GPIO14(TX), GPIO15(RX) | 57600 baud |
| ESP32-S3-EYE | UART1 | GPIO10(TX), GPIO9(RX) | ⚠️ Conflicto con R503 (mismo UART_NUM_1) |
| PN532 NFC | SPI | CS=GPIO5, MOSI=GPIO23, MISO=GPIO19, SCK=GPIO18 | |
| MOSFET cerradura | GPIO | GPIO32 | OUT |
| Buzzer | GPIO | GPIO33 | PWM |
| LED RGB | GPIO | GPIO25(R), GPIO26(G), GPIO27(B) | |
| Pulsador interior | GPIO | GPIO34 | INPUT_PULLUP |
| Reed switch | GPIO | GPIO35 | INPUT_PULLUP |

---

## Arquitectura

### Máquina de estados — Lock Manager

```
         ┌─────────┐
         │ LOCKED  │◄────────────────────────────┐
         └────┬────┘                              │
              │                                    │
    ┌─────────┼─────────┐                          │
    ▼         ▼         ▼                          │
┌──────┐ ┌──────┐ ┌──────┐                       │
│AUTH  │ │TIMER │ │REMOTE│                       │
│PASS  │ │AUTO  │ │ALEXA │                       │
└──┬───┘ └──────┘ └──┬───┘                       │
   │                  │                            │
   └──────┬───────────┘                            │
          ▼                                        │
    ┌──────────┐                                  │
    │UNLOCKING │── 3s delay ───────────────────────┘
    └──────────┘   → LOCKED
```

### Secuencia de autenticación
1. Evento: huella / NFC / PIN / rostro / Alexa
2. `auth_manager` valúa contra whitelist local
3. Válido → `lock_manager.unlock()`
4. `log_manager.record(user_id, method, timestamp)`
5. `cloud_manager.notify()`

---

## Protocolo UART ESP32 ↔ ESP32-S3-EYE

```
→ ESP32 solicita:      "SCAN\n"
← ESP32-S3-EYE:        "OK:3\n"     (user_id reconocido)
                       "UNKNOWN\n"  (rostro no registrado)
                       "NOFACE\n"   (no hay rostro)
```

---

## Bring-up order (F1)

| # | Periférico | Validación |
|---|---|---|
| 1 | LED + UART debug | Heartbeat + log serial |
| 2 | Nextion | Display functional |
| 3 | MOSFET cerradura | Lock control |
| 4 | Buzzer | PWM tone |
| 5 | R503 | Fingerprint scan |
| 6 | PN532 | NFC tag read |
| 7 | ESP32-S3-EYE | Face recognition |
| 8 | ESP-RainMaker | Cloud + Alexa + OTA |

---

## Feature flags (`app_config.h`)

```c
#define ENABLE_DEBUG_LOG        1
#define ENABLE_NEXTION          0
#define ENABLE_FINGERPRINT      0
#define ENABLE_NFC              0
#define ENABLE_FACE             0
#define ENABLE_RAINMAKER        0
```

Se activan uno por uno durante F1/F2. Solo debug log está activo por defecto.

---

## Configuración de UARTs

| Periférico | Heltec V3 | PCB Final |
|---|---|---|
| Debug | UART0 (43/44) | UART0 (1/3) |
| Nextion | UART1 (4/5) | UART2 (16/17) |
| R503 | UART2 (6/7) | UART1 (14/15) |
| S3-EYE | UART1 (19/20) | UART1 (9/10) |

⚠️ **Conflicto conocido:** R503 y S3-EYE comparten UART_NUM_1 en PCB final. Solución pendiente (separar UARTs, compartir con mux, o usar software UART).

---

## Partitions (`partitions.csv`)

OTA dual + SPIFFS para almacenamiento:

| Nombre | Offset | Tamaño |
|---|---|---|
| nvs | 0x9000 | 20KB |
| otadata | 0xe000 | 8KB |
| app0 | 0x10000 | 1280KB |
| app1 | 0x150000 | 1280KB |
| spiffs | 0x290000 | 1472KB |
