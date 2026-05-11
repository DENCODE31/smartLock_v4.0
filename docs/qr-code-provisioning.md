# QR Code Provisioning — SmartLock 4.0

## Problema

La URL que el firmware imprime en el serial:

```
https://rainmaker.espressif.com/qrcode.html?data={"ver":"v1","name":"SmartLock_XXXX","pop":"smartlock4","transport":"ble"}
```

Falla con **"No data provided for the QR code"** porque el JSON no está URL-encoded y el parser del sitio lo interpreta como parámetros separados.

## Solución — Client-side (navegador)

Usar la URL con el JSON codificado:

```
https://rainmaker.espressif.com/qrcode.html?data=%7B%22ver%22%3A%22v1%22%2C%22name%22%3A%22SmartLock_a1967b%22%2C%22pop%22%3A%22smartlock4%22%2C%22transport%22%3A%22ble%22%7D
```

## Solución — Firmware (parche permanente)

Modificar `components/app_network/app_network.c` donde se imprime la URL del QR (buscar `rainmaker.espressif.com/qrcode.html`) para aplicar URL encoding al JSON antes de imprimir la URL.

En `app_network.c`, función que muestra el QR, cambiar:

```c
ESP_LOGI(TAG, "https://rainmaker.espressif.com/qrcode.html?data=%s", (char *)event_data);
```

Por una versión que haga URL-encode del payload:

```c
// URL-encode el JSON data para el QR link
char *url_encoded = url_encode((char *)event_data);
if (url_encoded) {
    ESP_LOGI(TAG, "https://rainmaker.espressif.com/qrcode.html?data=%s", url_encoded);
    free(url_encoded);
}
```

### Implementación de url_encode

```c
static char *url_encode(const char *str)
{
    if (!str) return NULL;
    size_t len = strlen(str);
    // Max expansion: every char could become %XX (3x)
    char *encoded = malloc(len * 3 + 1);
    if (!encoded) return NULL;
    const char *hex = "0123456789ABCDEF";
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = str[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded[j++] = c;
        } else {
            encoded[j++] = '%';
            encoded[j++] = hex[c >> 4];
            encoded[j++] = hex[c & 0xf];
        }
    }
    encoded[j] = '\0';
    return encoded;
}
```

Incluir `#include <ctype.h>` y `#include <stdlib.h>` al inicio del archivo.

## Alternativa — Uso directo desde la app

No es necesario escanear un QR. La app ESP RainMaker descubre el dispositivo automáticamente por BLE con el nombre `SmartLock_XXXXXX`. Solo hay que:

1. Abrir la app → **Add Device**
2. Seleccionar `SmartLock_a1967b` de la lista de dispositivos cercanos
3. Ingresar POP: `smartlock4`

## Referencia

- [ESP RainMaker Provisioning docs](https://github.com/espressif/esp-rainmaker/blob/master/docs/en/provisioning.rst)
- El formato del payload QR es `{"ver":"v1","name":"<prefix>_<md5>","pop":"<pop>","transport":"<ble|softap>"}`
