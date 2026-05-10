# SmartLock 4.0 — Puertardino Next Gen

> Proyecto: Cerradura electrónica inteligente con reconocimiento facial, huella, NFC, PIN y Alexa
> Estado: En planificación
> Inicio: Mayo 2026
> Fabricante PCB: **JLCPCB** — todas las reglas de diseño ajustadas a sus capacidades

---

## 📂 ESTRUCTURA DEL PROYECTO

```
smartlock 4.0/
├── SMARTLOCK_4.0.md          ← Este archivo (documentación maestra)
├── FIRMWARE.md               ← Documentación específica de firmware
├── CARCASA.md                ← Documentación de diseño 3D e impresión
├── datasheets/               ← Datasheets de todos los componentes
│   ├── esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf
│   ├── LM2596_DS.pdf
│   ├── TP4056_DS.pdf
│   ├── PN532_DS.pdf
│   ├── R503_DS.pdf
│   ├── ESP32-S3-EYE_GS.pdf
│   ├── SRD-05VDC_DS.pdf
│   └── ...
├── nextion_ui/               ← Diseños HMI de la pantalla Nextion (.HMI, .tft)
├── carcasa_3d/               ← Diseños 3D para impresión (Fusion 360 + STL)
├── altium/                   ← Proyecto Altium Designer
└── firmware/                 ← Código fuente ESP-IDF
    └── smartlock_v4/
```

### Documentos de referencia rápida

| Documento | Contenido |
|---|---|
| [`FIRMWARE.md`](FIRMWARE.md) | Pines, build, arquitectura, protocolos, feature flags |
| [`CARCASA.md`](CARCASA.md) | STLs, Fusion 360, impresión Kobra 2 Neo, tolerancias |

> **⚠️ Regla de consulta de componentes:** Si pregunto por un componente, primero busco su datasheet en `datasheets/` antes de responder. Si no está, sugiero descargarlo de los enlaces en la BOM.
> **⚠️ Regla de consulta de carcasas:** Si pregunto por la carcasa o diseño 3D, primero reviso `CARCASA.md` y los archivos en `carcasa_3d/`.
> **⚠️ Regla de consulta de firmware:** Si pregunto por pines, build, o arquitectura de firmware, primero reviso `FIRMWARE.md`.

---

## 📋 CONTEXTO — Historia del proyecto

| Versión | Plataforma | Funcionalidad | Estado |
|---|---|---|---|
| PUERTA_2.0 | Arduino + Keypad 4x4 | Solo teclado matricial | Prototipo |
| PUERTARDUINO (V1) | Arduino Uno | Huella AS608 + Ultrasonido HC-SR04 + LCD I2C + pulsador | ✅ 3 años funcionando |
| PUERTARDUINO 4.0 | Arduino Mega? + Nextion | Huella + ATtiny + modos manual/auto + Nextion | ❌ Sin terminar |
| **SmartLock 4.0** | **ESP32 + ESP32-S3-EYE** | **Facial + Huella + NFC + PIN + Alexa** | **🚀 En desarrollo** |

### Lecciones de V3.0 (EasyEDA, 1er semestre)
- ✅ MOSFET IRF620 para driver de cerradura (buena elección)
- ✅ Buck LM2596 12V→5V en vez de lineal
- ✅ Conectores JST-XH para periféricos
- ❌ Sensor ultrasónico HC-SR04 no funcionó (eco rebota, mala detección)
- ❌ ESP-01S limitado + Arduino separado = complejidad innecesaria
- ❌ Sin protecciones en entrada 12V
- ❌ Sin respaldo de batería

---

> 📖 Documentación detallada de firmware en [`FIRMWARE.md`](FIRMWARE.md) — pines, build, arquitectura, protocolos, feature flags.

## 🏗️ ARQUITECTURA DEL SISTEMA

```
                       12V DC ENTRADA
                            │
                  ┌─────────▼─────────┐
                  │   Buck LM2596      │
                  │   12V → 5V 3A     │
                  └─────────┬─────────┘
                            │
               ┌────────────┼────────────────┐
               ▼            ▼                  ▼
         ┌──────────┐ ┌──────────┐ ┌──────────────────┐
         │ TP4056   │ │ ESP32    │ │ Nextion 5"       │
         │ Cargador │ │ WROOM-32 │ │ (ya la tienes)   │
         │ Li-ion   │ │          │ │                  │
         └────┬─────┘ │ Principal│ │ UART: TX2/RX2    │
              ▼       └────┬─────┘ └──────────────────┘
        ┌────────┐         │
        │ Batería│         │
        │ 18650  │         │
        └────────┘         │
                           │
     ┌─────────────────────┼──────────────────────────┐
     │                     │                           │
     ▼                     ▼                           ▼
 ┌────────┐     ┌──────────────────┐     ┌──────────────────┐
 │ R503   │     │ ESP32-S3-EYE     │     │ PN532 NFC        │
 │ Huella │◄───►│ Visión Facial    │◄───►│ SPI: CS5,MOSI23  │
 │ UART   │     │ UART: RX1/TX1    │     │ MISO19,SCK18     │
 │ 57600  │     │ Proto: "OK:3\n"  │     │                  │
 └────────┘     └──────────────────┘     └──────────────────┘

      ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
      │ MOSFET   │   │ Buzzer   │   │ Pulsador │   │ Reed     │
      │ Cerradura│   │ PWM(33)  │   │ INT(34)  │   │ Door(35) │
      │ GPIO 32  │   │          │   │          │   │          │
      └──────────┘   └──────────┘   └──────────┘   └──────────┘

     ☁️ ESP-RainMaker → Alexa + App iOS/Android + OTA + Notificaciones
```

---

## 📍 MAPEO DE PINES (ESP32-WROOM-32)

| Periférico | Protocolo | GPIO ESP32 | Pines |
|---|---|---|---|
| Nextion 5" | UART | TX2=GPIO17, RX2=GPIO16 | 115200 baud |
| R503 Huella | UART | TX3=GPIO15, RX3=GPIO14 | 57600 baud |
| ESP32-S3-EYE | UART | TX1=GPIO10, RX1=GPIO9 | 115200 baud |
| PN532 NFC | SPI | CS=GPIO5, MOSI=GPIO23, MISO=GPIO19, SCK=GPIO18 | SPI |
| Relé cerradura | GPIO | GPIO32 | OUT |
| Buzzer (PWM) | GPIO | GPIO33 | OUT |
| Pulsador interior | GPIO | GPIO34 | INPUT_PULLUP |
| Reed switch puerta | GPIO | GPIO35 | INPUT_PULLUP |
| LED estado RGB | GPIO | GPIO25, GPIO26, GPIO27 | R,G,B |

### Protocolo UART ESP32 ↔ ESP32-S3-EYE
```
→ ESP32 solicita:      "SCAN\n"
← ESP32-S3-EYE:        "OK:3\n"     (user_id reconocido)
                       "UNKNOWN\n"  (rostro no registrado)
                       "NOFACE\n"   (no hay rostro)
```

---

## 🔩 BOM — BILL OF MATERIALS

| # | Componente | Modelo | JLCPCB Part | DigiKey PN | Precio USD | Precio COP |
|---|---|---|---|---|---|---|
| 1 | **ESP32 principal** | ESP32-WROOM-32E-N8 | **C701342** | 1965-ESP32-WROOM-32E-N8CT-ND | $4.42 | ~$16,562 |
| 2 | **Visión facial** | ESP32-S3-EYE | No aplica (devkit) | - | $28.00 | ~$104,919 |
| 3 | **Sensor huella** | GROW R503 | No aplica (módulo) | - | $14.00 | ~$52,459 |
| 4 | **Módulo NFC** | PN5321A3HN (IC) | **C880904** | PN5321A3HN/C106,51 | $4.00 | ~$14,988 |
| 5 | **Pantalla** | Nextion 5" | Ya la tienes | $0 | $0 | $0 |
| 6 | **Buck converter** | LM2596S-ADJ | **C10000** | LM2596S-ADJ/NOPBCT-ND | $0.43 | ~$1,611 |
| 7 | **Cargador Li-ion** | TP4056 ESOP8 | **C382139** | TP4056 | $0.13 | ~$487 |
| 8 | **Batería** | 18650 2600mAh | No aplica | - | $5.00 | ~$18,735 |
| 9 | **MOSFET cerradura** | IRLZ44N (THT) / AO4404 (SMD) | **C2965699** (AO4404) | IRLZ44NPBF-ND | $0.60 | ~$2,248 |
| 10 | **Flyback diode** | 1N4007 | **C59288** | 1N4007DICT-ND | $0.05 | ~$187 |
| 11 | **Gate R + pulldown** | 100Ω + 10kΩ | Pasivos | - | $0.02 | ~$75 |
| 12 | **Cerradura 12V** | Electrinal | No aplica | - | $15.00 | ~$56,206 |
| 13 | **PCB** | JLCPCB 2 capas 100x80mm | Fabricación → | - | $5.00 | ~$18,735 |
| 14 | **Pasivos varios** | R, C, diodos, LED | - | - | $5.00 | ~$18,735 |
| | **TOTAL** | | | | **~$81.65** | **~$305,960 COP** |

> TRM referencia: $3,747.10 COP/USD (9 Mayo 2026)
> 📥 Datasheets descargados en: `datasheets/`
> 💡 Mejora sobre V3.0: IRF620 reemplazado por IRLZ44N (logic-level, RDS(on) ~0.022Ω vs 1.5Ω). Gate manejado directo desde GPIO 3.3V — sin driver NPN adicional. Flyback 1N4007 y gate pulldown 10kΩ incluidos.
> 💡 Si necesitas un componente que no está en esta tabla o en la carpeta `datasheets/`, dímelo y lo busco + descargo.

---

## 🧠 ARQUITECTURA DE FIRMWARE

### Estructura (PlatformIO + ESP-IDF)

```
smartlock_v4/
├── main/
│   ├── main.c
│   ├── app_config.h
│   ├── managers/
│   │   ├── lock_manager.c/h          # Máquina de estados de cerradura
│   │   ├── auth_manager.c/h          # Orquesta todos los métodos de auth
│   │   ├── fingerprint_manager.c/h   # R503 UART driver
│   │   ├── nfc_manager.c/h           # PN532 SPI driver
│   │   ├── face_manager.c/h          # Comunicación UART con S3-EYE
│   │   ├── display_manager.c/h       # Nextion protocolo UART
│   │   ├── cloud_manager.c/h         # ESP-RainMaker
│   │   └── log_manager.c/h           # Registro eventos en flash/SD
│   ├── hal/
│   │   ├── relay.c/h
│   │   ├── buzzer.c/h
│   │   └── button.c/h
│   └── Kconfig.projbuild
├── components/
│   └── esp-rainmaker/
├── partitions.csv
└── platformio.ini
```

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

### Bring-up order (periférico por periférico)
1. LED + Buzzer → hardware OK
2. UART debug → logging funcional
3. Nextion → display listo
4. Relé → cerradura controlable
5. R503 → huella funcional
6. PN532 → NFC funcional
7. ESP32-S3-EYE → facial funcional
8. ESP-RainMaker → cloud + Alexa + OTA

---

## 📅 PLAN DE TRABAJO — FASES E HITOS

| Fase | Duración | Descripción | Costo |
|---|---|---|---|
| **F1: Validación** | 1 semana | Protoboard: cada periférico por separado | ~$30 USD |
| **F2: Integración** | 1 semana | Todo conectado: huella+NFC+facial+Nextion+RainMaker | ~$20 USD |
| **F3: PCB Altium** | 2 semanas | Diseño esquemático + layout + JLCPCB (5uds) | ~$15 USD |
| **F4: Bring-up PCB** | 1 semana | Soldar + validar periférico por periférico | ~$5 USD |
| **F5: Carcasa 3D** | 1 semana | Fusion 360 + imprimir en Kobra 2 Neo | ~$5 USD |
| **F6: Release** | 3 días | Documentación, Git, fotos, video demo | $0 |
| **TOTAL** | **~6 semanas** | | **~$75 USD (~$281,000 COP)** |

---

## 🔗 ENLACES Y HERRAMIENTAS CLAVE

### Hardware — Docs y compra
- **ESP32-WROOM-32**: [Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf)
- **ESP32-S3-EYE**: [Getting Started](https://github.com/espressif/esp-who/blob/master/docs/en/get-started/ESP32-S3-EYE_Getting_Started_Guide.md) — [Buy AliExpress](https://www.aliexpress.com/item/1005004537440754.html)
- **R503 Fingerprint**: [Datasheet Grow](http://www.grow.com.cn) — [Buy AliExpress](https://www.aliexpress.com/item/33053655412.html)
- **PN532 NFC**: [Datasheet NXP](https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf) — [Buy AliExpress](https://www.aliexpress.com/wholesale?SearchText=pn532+v3)
- **Nextion**: [Nextion IDE](https://nextion.tech/) — [Editor HMI](https://nextion.tech/nextion-editor/)
- **LM2596 Buck**: [Datasheet TI](https://www.ti.com/lit/ds/symlink/lm2596.pdf)
- **TP4056**: [Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/TP4056.pdf)
- **JLCPCB**: [PCB Fabrication](https://jlcpcb.com/) — [Parts Library](https://jlcpcb.com/parts) — [Capabilities](https://jlcpcb.com/capabilities/pcb-capabilities) — [Design Rules](https://jlcpcb.com/blog/pcb-design-rules-best-practices) — [Impedance Calculator](https://jlcpcb.com/pcb-impedance-calculator)
- **Cerradura 12V**: [MercadoLibre Colombia](https://www.mercadolibre.com.co/)

### Firmware — Frameworks y librerías
- **ESP-IDF**: [Get Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) — v5.3+
- **PlatformIO**: [Install](https://platformio.org/install) — VS Code extension
- **ESP-RainMaker**: [Documentación](https://rainmaker.espressif.com/) — [GitHub](https://github.com/espressif/esp-rainmaker)
- **ESP-WHO (Face Recognition)**: [GitHub](https://github.com/espressif/esp-who) — [Documentación](https://docs.espressif.com/projects/esp-who/)
- **ESP-DL (Deep Learning)**: [Component Registry](https://components.espressif.com/components/espressif/esp-dl) — [GitHub](https://github.com/espressif/esp-dl)
- **PN532 Library**: [Adafruit PN532](https://github.com/adafruit/Adafruit-PN532)
- **R503 Library**: [Adafruit Fingerprint](https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library)

### Proyectos de referencia
- **HomeKey-ESP32** (Apple HomeKey en ESP32): [GitHub](https://github.com/rednblkx/HomeKey-ESP32) — Implementación NFC para iPhone
- **IoTbasedLockV2.0** (ESP32 + Nextion + 4 factores): [GitHub](https://github.com/make2explore/IoTbasedLockV2.0)
- **SecureLock-ESP32** (2FA + Telegram + Dashboard): [GitHub](https://github.com/4ldrian01/SecureLock-ESP32)
- **Insane-haustuer-gate** (Split design + R503 + PN532): [GitHub](https://github.com/babeinlovexd/Insane-haustuer-gate)

> 📖 Diseño 3D detallado en [`CARCASA.md`](CARCASA.md) — STLs, Fusion 360, impresión, tolerancias.

### Software CAD
- **Altium Designer**: Diseño PCB
- **Fusion 360**: Diseño mecánico y adaptación de carcasas
- **Nextion Editor**: UI de la pantalla táctil (proyectos en `nextion_ui/`)

---

## 📟 MCU PRINCIPAL — ESP32-WROOM-32E-N8

| Especificación | Valor |
|---|---|
| Módulo | **ESP32-WROOM-32E-N8** |
| Chip | ESP32-D0WD-V3 (ECO V3) |
| Flash | 8MB Quad SPI |
| PSRAM | No |
| CPU | Xtensa LX6 dual-core @ 240MHz |
| WiFi | 802.11 b/g/n |
| Bluetooth | v4.2 BR/EDR + BLE |
| Antena | PCB Trace (integrada) |
| GPIO disponibles | 25 (de 38 pines totales) |
| ADC | 2 x 12-bit SAR, hasta 18 canales |
| DAC | 2 x 8-bit |
| Interfaces | 3x UART, 3x SPI, 2x I2C, I2S, PWM, SDIO |
| Temp | -40°C ~ 85°C |
| JLCPCB Part | C701342 |
| DigiKey | 1965-ESP32-WROOM-32E-N8CT-ND |
| ESP-IDF target | `idf.py set-target esp32` |
| Precio | ~$4.42 USD (~$16,562 COP) |

**Razón de la elección:**
- ECO V3 = bugs corregidos (PSRAM, crystal startup, security fault injection)
- Flash 8MB suficiente para ESP-IDF + ESP-RainMaker + OTA dual
- No necesita PSRAM (la visión facial está en el ESP32-S3-EYE aparte)
- Es el estándar de la industria — documentación, ejemplos, comunidad
- Disponible en JLCPCB como **Basic Part** para ensamble PCBA

## 🏭 REGLAS DE DISEÑO — JLCPCB (estrictas)

> **Todas las reglas de diseño están ajustadas a las capacidades de fabricación de JLCPCB** para PCB de 2 capas con 1oz de cobre. No se usan reglas genéricas ni de otros fabricantes.

### Capacidades JLCPCB (2 capas, 1oz, estándar)

| Parámetro | Mínimo JLCPCB | Usaremos (seguro) | Notas |
|---|---|---|---|
| Track width | 0.1mm (4mil) | **0.2mm (8mil)** | Señales |
| Track spacing | 0.1mm (4mil) | **0.2mm (8mil)** | Señales |
| Power track width | - | **0.5mm (20mil)** | Para 5V/3.3V |
| GND track/plane | - | **1.0mm (40mil)** | Plano o pista gruesa |
| Via drill diameter | 0.3mm | **0.4mm** | Drill final |
| Via pad diameter | - | **0.7mm** | Anillo ≥0.15mm |
| Annular ring | 0.13mm | **0.15mm** | Seguro JLCPCB |
| Copper-to-board edge | 0.2mm | **0.3mm (12mil)** | Evita desprendimiento |
| Solder mask clearance | 0.05mm | **0.1mm (4mil)** | - |
| Solder mask sliver | 0.1mm | **0.15mm** | - |
| Silkscreen line width | 0.15mm | **0.2mm** | Legible |
| Silkscreen text height | 0.8mm | **1.2mm** | Legible |
| Board thickness | - | **1.6mm** | Estándar |
| Copper weight | - | **1oz** | Estándar |

### Net classes en Altium (DRC)

| Net Class | Track Width Min | Via Style | Clearance |
|---|---|---|---|
| `POWER_12V` | 0.5mm | 0.4/0.7mm | 0.3mm |
| `POWER_5V` | 0.5mm | 0.4/0.7mm | 0.2mm |
| `POWER_3V3` | 0.3mm | 0.4/0.7mm | 0.2mm |
| `GND` | 1.0mm (o plano) | 0.4/0.7mm | 0.2mm |
| `SIGNAL` | 0.2mm | 0.4/0.7mm | 0.2mm |
| `HIGH_SPEED` (SPI) | 0.25mm | 0.4/0.7mm | 0.25mm |

> ⚠️ Configurar estas net classes en Altium **antes de empezar el routing**
> ⚠️ Correr DRC con estas reglas **al final** — cero errores antes de mandar Gerbers

### Orden de pre-fabricación JLCPCB
1. [ ] DRC en Altium con reglas JLCPCB → 0 errores
2. [ ] ERC en Altium → 0 errores
3. [ ] Generar **Gerber X2** (formato preferido JLCPCB)
4. [ ] Generar **NC Drill** (Excelon)
5. [ ] Subir a JLCPCB → seleccionar: 2 capas, 1.6mm, 1oz, HASL, color verde
6. [ ] Verificar visualización de capas en el visor de JLCPCB

### Recursos JLCPCB — Enlaces clave

| Recurso | Link | Para qué sirve |
|---|---|---|
| **Altium Rules (GitHub)** | [github.com/ayberkozgur/jlcpcb-design-rules-stackups](https://github.com/ayberkozgur/jlcpcb-design-rules-stackups) | Reglas DRC/DFM pre-hechas para importar directo en Altium + stackups JLCPCB |
| **JLCPCB Capabilities** | [jlcpcb.com/capabilities/pcb-capabilities](https://jlcpcb.com/capabilities/pcb-capabilities) | Límites reales de fabricación (vias, slots, tolerancias, colores) |
| **Design Guidelines Blog** | [jlcpcb.com/blog/pcb-design-rules-best-practices](https://jlcpcb.com/blog/pcb-design-rules-best-practices) | Buenas prácticas de diseño para manufactura |
| **DRC Check How-To** | [jlcpcb.com/blog/how-to-run-design-rule-check](https://jlcpcb.com/blog/how-to-run-design-rule-check) | Cómo correr DRC y qué errores evitar antes de enviar Gerbers |
| **Impedance Calculator** | [jlcpcb.com/pcb-impedance-calculator](https://jlcpcb.com/pcb-impedance-calculator) | Cálculo de impedancia controlada para stackups JLCPCB |

> 💡 **Recomendación Altium:** El repo de GitHub permite importar las reglas directo en Altium Designer (DRC + DFM + stackups pre-configurados para JLCPCB). Úsalo como base y ajusta los valores de la tabla de net classes arriba según nuestro diseño.

---

## 🛠️ PLAN DE PCB EN ALTIUM — CHECKLIST

### Esquemático
- [ ] Conector 12V + fusible PTC 1A + TVS + diodo Schottky
- [ ] LM2596S-ADJ (C10000) — 12V→5V con caps de entrada/salida (220uF elect + 100nF cerámico)
- [ ] TP4056 (C382139) + conector batería 18650 + conmutación automática
- [ ] ESP32-WROOM-32E-N8 (C701342) con caps desacoplo (100nF + 10uF por pin de power)
- [ ] PN532 SPI con pull-ups 10k en líneas MISO/MOSI/SCK/CS
- [ ] R503 UART con divisor TX 5V→3.3V (1k + 2k) si el módulo lo requiere
- [ ] MOSFET driver cerradura 12V (IRLZ44N/AO4404): gate R 100Ω + pulldown 10kΩ + flyback 1N4007
- [ ] Buzzer con transistor driver (BC547 + resistor base 1k)
- [ ] Header UART (4 pines) para ESP32-S3-EYE
- [ ] Resistencias pulldown en EN y GPIO0 del ESP32
- [ ] Puntos de prueba: 12V, 5V, 3.3V, GND, UART debug (TX, RX)
- [ ] ERC sin errores

### Layout (PCB 2 capas — JLCPCB)
- [ ] Stackup: Top layer = señales + componentes, Bottom layer = plano GND continuo
- [ ] LM2596 en esquina con área de cobre para disipación + inductor lejos de ESP32
- [ ] Antena ESP32 en borde de PCB (sin cobre ni GND debajo de la antena)
- [ ] MOSFET driver en esquina opuesta al ESP32 (aislamiento térmico, pista gruesa 12V + GND)
- [ ] Plano GND en bottom layer con stitching vias cada 10-15mm alrededor del board
- [ ] Track widths según net classes JLCPCB (ver tabla arriba)
- [ ] Clearances según net classes (mínimo 0.2mm)
- [ ] Board outline = forma para la carcasa existente (importar DXF de Fusion 360)
- [ ] 4 holes de mounting para tornillos M3 (3.2mm drill)
- [ ] DRC con reglas JLCPCB → 0 errores

### DFM (JLCPCB)
- [ ] Soldermask clearance ≥0.1mm en todos los pads
- [ ] Silkscreen text height ≥1.2mm, line width ≥0.2mm
- [ ] Annular ring ≥0.15mm en todas las vias
- [ ] No ángulos agudos en cobre (< 90°)
- [ ] No acid traps (transitions suaves entre tracks)
- [ ] Panelización opcional si >10 unidades

---

## ⚠️ CORRECTION LOG — Errores y correcciones

> Aquí se registran errores, bugs y malas decisiones para no repetirlas.

| Fecha | Error | Corrección | Archivo/Lín |
|---|---|---|---|---|
| 10 Mayo 2026 | R503 y S3-EYE comparten UART_NUM_1 en PCB final — conflicto | Pendiente resolver: separar UARTs o compartir con mux/sw UART. Heltec V3 usa UART1 para S3-EYE y UART2 para R503. | `app_config.h:22-31` |
| 10 Mayo 2026 | Relé 5V para cerradura 12V = carga inductiva extra + clic + vida útil limitada | Reemplazado por MOSFET IRLZ44N directo: lógica 3.3V, RDS(on) 0.022Ω, flyback 1N4007. Más simple, más silencioso, más eficiente. | BOM + PCB checklist |


---
---

## 📈 LOG DE AVANCE

| Fecha | Hito | Detalle |
|---|---|---|
| 10 Mayo 2026 | 🟢 **F1 iniciada** | Estructura de firmware creada: `platformio.ini`, `app_config.h`, `main.c` (blink + serial). Toolchain listo para compilar. |
| 10 Mayo 2026 | 📄 **Datasheet ESP32-WROOM-32E-N8 agregado** | `datasheets/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf` — referenciado en SMARKT_4.0.md como chip principal. |
| 10 Mayo 2026 | 🟢 **F1: Build exitoso** | Toolchain validado: ESP-IDF v5.3.2 + target `esp32s3` (Heltec V3). `smartlock_v4.bin` generado (0x339b0 bytes). Pin mapping Heltec V3 en `#ifdef HELTEC_V3`. |
| 10 Mayo 2026 | 🟢 **F1: Heartbeat LED + serial OK** | Heltec V3 flash exitoso. LED onboard parpadea ~1.2s. Log serial visible. GPIO Vext, LED, FreeRTOS, UART debug validados. |
| 10 Mayo 2026 | 🔧 **Driver cerradura: relé → MOSFET** | IRF620 V3.0 → IRLZ44N logic-level. Sin driver NPN, sin relé. BOM actualizada, PCB checklist actualizado. |
| 10 Mayo 2026 | 📁 **Docs divididas: FIRMWARE.md + CARCASA.md + nextion_ui/** | Documentación modular. Pines, build, estados → `FIRMWARE.md`. Diseño 3D → `CARCASA.md`. UI pantalla → `nextion_ui/`. |
| - | - | - |

---

## 📝 NOTAS TÉCNICAS

### NFC con iPhone
- iOS 13+ puede leer tags NDEF en NTAG213/215/216
- PN532 lee UID de cualquier tag NFC tipo A
- HomeKey-ESP32 permite tap-to-unlock con iPhone (requiere PN532)
- Alternativa simple: tags NFC pre-programadas con UID único

### Reconocimiento facial
- ESP32-S3-EYE + ESP-WHO + ESP-DL
- Modelo: human_face_detect + human_face_recognition
- Cada feature facial = 2050 bytes
- Almacenamiento en flash (partición storage) o SD card
- Soporte oficial para ESP32-S3 y ESP32-P4

### ESP-RainMaker vs SinricPro
Decisión: **ESP-RainMaker** (gratis, ilimitado, de Espressif)

| Feature | ESP-RainMaker | SinricPro |
|---|---|---|
| Precio | Gratis cloud público | 3 disp free, $3 disp/año |
| Alexa | ✅ Integrado | ✅ Integrado |
| App iOS/Android | ✅ RainMaker App | ✅ SinricPro App |
| OTA | ✅ Incluido | ❌ Solo plan Business |
| Límite dispositivos | Ilimitado | 3 en free |
| SDK | ESP-IDF nativo | Arduino/ESP-IDF |
| Fabricante | Espressif | Tercero |

---

## 🎯 OBJETIVOS SMART

- [ ] **F1 (semana 1):** Huella R503 funcional en protoboard con ESP32
- [ ] **F1 (semana 1):** PN532 leyendo tags NFC en protoboard
- [ ] **F2 (semana 2):** ESP32-S3-EYE enviando "OK:ID" por UART
- [ ] **F2 (semana 2):** Nextion mostrando estado y PIN funcional
- [ ] **F2 (semana 2):** ESP-RainMaker conectado con Alexa
- [ ] **F3 (semanas 3-4):** PCB diseñado en Altium + pedido a JLCPCB
- [ ] **F4 (semana 5):** PCB armado y todos los periféricos validados
- [ ] **F5 (semana 5-6):** Carcasa impresa con todos los componentes montados
- [ ] **F6 (semana 6):** Release a GitHub con documentación completa
