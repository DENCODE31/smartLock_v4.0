# SmartLock 4.0 — Progreso del Firmware

## Objetivo
Reconstruir el firmware SmartLock 4.0 desde cero siguiendo el patrón del ejemplo `switch` oficial de ESP RainMaker, lograr conectividad RainMaker funcional, corregir el crash de boot, habilitar provisioning QR por BLE, y arreglar el stack overflow del `Tmr Svc`.

---

## Progreso Completado

### 1. Estructura del Proyecto
- Eliminado `managers/cloud_manager.c/h` (obsoleto)
- Creados componentes locales desde ejemplos oficiales de Espressif:
  - `components/app_network/` — gestión WiFi
  - `components/app_insights/` — stub (sin dependencia `esp_insights`)
  - `components/app_reset/` — reseteo a fábrica
- `main/main.c` reescrito siguiendo el switch example
- `main/app_priv.h` y `main/app_driver.c` con driver de:
  - Relay lock (GPIO 2)
  - Buzzer (GPIO 3)
  - Botón interior (GPIO 4)
  - LED rojo (GPIO 5)
  - LED verde (GPIO 12)
- `main/CMakeLists.txt` actualizado con referencias a los nuevos componentes

### 2. Configuración de SDK
- `sdkconfig.defaults` con:
  - NimBLE como stack BLE
  - Transporte BLE para provisioning
  - Prefix `SmartLock`, QR habilitado, timeout 15 min
  - Flash de 8MB
  - Tabla de particiones personalizada
  - Challenge response deshabilitado
  - `CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH=4096`
- `partitions.csv` con layout:
  - `nvs` 0x5000
  - `otadata` 0x2000
  - `app0` 0x1E0000
  - `app1` 0x1E0000
  - `spiffs` 0x420000
  - `fctry` 0x10000 (NVS de fábrica, requerido por RainMaker)

### 3. Boot y Provisioning
- Flash completo borrado con `idf.py -p COM7 erase-flash`
- Boot exitoso — serial muestra `RainMaker Initialised`
- QR generado correctamente con `"transport":"ble"` y `"pop":"smartlock4"`
- Provisioning BLE funcional desde app ESP RainMaker

### 4. Control desde la Nube
- Comando remoto recibido: `Remote command: UNLOCK`
- Reporte de parámetros: `{"SmartLock":{"Power":true}}`
- Control desde app ESP RainMaker funcional

### 5. Fix Tmr Svc Stack Overflow
- **Problema**: El timer de OTA polling (`esp_rmaker_ota_using_topics`) ejecuta su callback en la tarea `Tmr Svc` de FreeRTOS, y el stack por defecto (2048 bytes) es insuficiente para el fetch de detalles OTA.
- **Síntoma**: `***ERROR*** A stack overflow in task Tmr Svc has been detected.` ~11s después de conectar MQTT.
- **Solución**: `CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH=4096` en `sdkconfig.defaults`.
- **Verificación**: Dispositivo estable por >2 minutos post-boot, sin crash, OTA polling ejecutado correctamente, SNTP sincronizado.

---

## Errores Encontrados y Resueltos

| Error | Causa | Solución |
|---|---|---|
| Boot crash: `Brownout detector` / error de partición | sdkconfig corrupto por mezcla de opciones old+new | Borrar `sdkconfig` y `build/`, regenerar desde `sdkconfig.defaults` |
| Boot crash: partición no encontrada | Faltaba `fctry` NVS (RainMaker requiere factory NVS en 0x7F0000) | Agregar `fctry, data, nvs, 0x7F0000, 0x10000` a `partitions.csv` |
| GPIO_NUM_47/48 undeclared | Target reset a `esp32` en vez de `esp32s3` al borrar build | Ejecutar `idf.py set-target esp32s3` |
| `Tmr Svc` stack overflow | OTA polling consume más stack del que asigna FreeRTOS por defecto (2048) | `CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH=4096` |

---

## Pendientes

### Prioridad Baja
- [ ] **QR URL encoding para Alexa**: El QR generado por defecto no tiene el encoding correcto para el flujo de Alexa. Ver `docs/qr-code-provisioning.md` (documentación creada en sesión anterior, puede necesitar actualización).
- [ ] **Habilitar ESP Insights**: `app_insights` es un stub. Activar `CONFIG_ESP_INSIGHTS_ENABLED` para diagnóstico en producción.
- [ ] **Validar buzzer y LEDs**: Confirmar que los tonos del buzzer y la secuencia de LEDs funcionan en hardware real.
- [ ] **Pruebas de OTA**: Probar flujo completo de OTA desde la nube de RainMaker.
- [ ] **Agregar parametros adicionales**: Sensor de batería, estado de conexión, historial de eventos.

### No Planificado
- **RainMaker vinculado con Alexa**: El dispositivo está conectado a RainMaker pero aún NO se ha enlazado la skill de Alexa con la cuenta de RainMaker. Pendiente hacer el linking desde la app de Alexa (habilitar "ESP RainMaker" skill y vincular con credenciales derainmaker.espressif.com).
- Modo offline con Bluetooth Local Control
- Carcasa 3D e integración mecánica

---

## Observaciones Técnicas

### Flujo de compilación
- **IDF**: `v5.5.2` en `C:\esp\v5.5.2\esp-idf\`
- **Target**: `esp32s3` (Heltec V3 rev v0.2)
- **Puerto**: COM7, baud 115200
- **Monitor serial**: `python -m serial.tools.miniterm --raw COM7 115200`
- **Comando de build+flash**: `idf.py -p COM7 flash`
- **Siempre borrar sdkconfig al cambiar defaults**: `Remove-Item sdkconfig` + `idf.py set-target esp32s3` + `idf.py build`

### Dependencias
- Los componentes `app_network`, `app_insights`, `app_reset` se copiaron del repo oficial `espressif/esp-rainmaker` (no disponibles en IDF Component Registry) como componentes locales en `components/`.

### Configuración de GPIO (Heltec V3)
| Señal | GPIO |
|---|---|
| Relay lock | GPIO 2 |
| Buzzer | GPIO 3 |
| Botón interior | GPIO 4 |
| LED rojo | GPIO 5 |
| LED verde | GPIO 12 |
| UART TX | GPIO 43 |
| UART RX | GPIO 44 |

### Sobre Tmr Svc
El timer task de FreeRTOS (`Tmr Svc`) ejecuta TODOS los callbacks de software timers del sistema. En RainMaker, el polling de OTA usa `esp_rmaker_ota_using_topics` que se suscribe a un topic MQTT y periódicamente verifica disponibilidad de OTA. Si ese callback usa mucha pila (por ejemplo, por HTTP requests internos), el stack por defecto de 2048 es insuficiente. El incremento a 4096 resolvió el crash.

---

## Recursos
- [Firmware Developer Guide — ESP RainMaker](https://rainmaker.espressif.com/docs/firmware-developer-guide)
- [Repositorio fork del proyecto](https://github.com/DENCODE31/esp-rainmaker)
- [ESP RainMaker — Repo oficial](https://github.com/espressif/esp-rainmaker)
- [Documentación de provisioning QR](qr-code-provisioning.md)
