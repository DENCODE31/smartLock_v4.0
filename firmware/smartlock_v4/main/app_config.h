#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// ============================================================
// SmartLock 4.0 -- Pin Definitions & Build Configuration
// MCU: ESP32-WROOM-32E-N8 (final) / ESP32-S3 Heltec V3 (proto)
// Target: idf.py set-target esp32 / esp32s3
// Revision: 1.1
// ============================================================

#ifdef HELTEC_V3
// --- Heltec WiFi LoRa 32 V3 (ESP32-S3) ---
// UART0 fixed: GPIO43(TX), GPIO44(RX) -- USB-Serial

#define UART_DEBUG_NUM          UART_NUM_0
#define UART_DEBUG_BAUD         115200
#define UART_DEBUG_TX           GPIO_NUM_43
#define UART_DEBUG_RX           GPIO_NUM_44

#define UART_NEXTION_NUM        UART_NUM_1
#define UART_NEXTION_BAUD       115200
#define UART_NEXTION_TX         GPIO_NUM_4
#define UART_NEXTION_RX         GPIO_NUM_5

#define UART_R503_NUM           UART_NUM_2
#define UART_R503_BAUD          57600
#define UART_R503_TX            GPIO_NUM_6
#define UART_R503_RX            GPIO_NUM_7

#define UART_S3EYE_NUM          UART_NUM_1
#define UART_S3EYE_BAUD         115200
#define UART_S3EYE_TX           GPIO_NUM_19
#define UART_S3EYE_RX           GPIO_NUM_20

// PN532 I2C (SPI occupied by LoRa on Heltec V3)
#define PN532_I2C_PORT          I2C_NUM_0
#define PN532_SDA               GPIO_NUM_41
#define PN532_SCL               GPIO_NUM_42

#define RELAY_LOCK              GPIO_NUM_47
#define BUZZER_PIN              GPIO_NUM_48
#define LED_BUILTIN             GPIO_NUM_35

#define LED_R                   GPIO_NUM_NC
#define LED_G                   GPIO_NUM_NC
#define LED_B                   GPIO_NUM_NC

#define BUTTON_INTERIOR         GPIO_NUM_1
#define REED_DOOR               GPIO_NUM_2

#define VEXT_ENABLE             GPIO_NUM_36

#else
// --- Final PCB: ESP32-WROOM-32E-N8 (ESP32-D0WD-V3) ---

#define UART_DEBUG_NUM          UART_NUM_0
#define UART_DEBUG_BAUD         115200
#define UART_DEBUG_TX           GPIO_NUM_1
#define UART_DEBUG_RX           GPIO_NUM_3

#define UART_NEXTION_NUM        UART_NUM_2
#define UART_NEXTION_BAUD       115200
#define UART_NEXTION_TX         GPIO_NUM_17
#define UART_NEXTION_RX         GPIO_NUM_16

#define UART_R503_NUM           UART_NUM_1
#define UART_R503_BAUD          57600
#define UART_R503_TX            GPIO_NUM_14
#define UART_R503_RX            GPIO_NUM_15

#define UART_S3EYE_NUM          UART_NUM_1
#define UART_S3EYE_BAUD         115200
#define UART_S3EYE_TX           GPIO_NUM_10
#define UART_S3EYE_RX           GPIO_NUM_9

#define PN532_SPI_HOST          SPI2_HOST
#define PN532_CS                GPIO_NUM_5
#define PN532_MOSI              GPIO_NUM_23
#define PN532_MISO              GPIO_NUM_19
#define PN532_SCK               GPIO_NUM_18

#define RELAY_LOCK              GPIO_NUM_32
#define BUZZER_PIN              GPIO_NUM_33
#define LED_R                   GPIO_NUM_25
#define LED_G                   GPIO_NUM_26
#define LED_B                   GPIO_NUM_27

#define BUTTON_INTERIOR         GPIO_NUM_34
#define REED_DOOR               GPIO_NUM_35

#endif

// --- Timing ---
#define LOCK_UNLOCK_DELAY_MS    3000
#define BUZZER_SHORT_MS         100
#define BUZZER_LONG_MS          500
#define DEBOUNCE_MS             50

// --- Feature Flags ---
#define ENABLE_DEBUG_LOG        1
#define ENABLE_NEXTION          0
#define ENABLE_FINGERPRINT      0
#define ENABLE_NFC              0
#define ENABLE_FACE             0
#define ENABLE_RAINMAKER        0

#endif
