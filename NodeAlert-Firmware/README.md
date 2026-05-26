# NodeAlert IoT — Firmware ESP32

## Requisitos de Compilación

- **PlatformIO** (VSCode extension o CLI)
- **Python 3.10+**
- **ESP-IDF 6.0.1** (gestionado por PlatformIO automáticamente)
- **ESP32** (cualquier variante con al menos 4MB flash)

## Configuración de Pines

Archivo: `src/config/pins_config.h`

| Sensor | Función | Pin ESP32 |
|--------|---------|-----------|
| DHT22 | Datos | GPIO 4 |
| MQ-9 | Analógico (gas) | GPIO 34 (ADC1_CH6) |
| KY-026 | Digital (llama) | GPIO 5 |
| KY-026 | Analógico (llama) | GPIO 35 (ADC1_CH7) |
| Relé | Actuador | GPIO 2 |

## Sensores Soportados

- **DHT22** — Temperatura (-40°C a 80°C) y humedad (0-100% RH)
- **MQ-9** — Gas monóxido de carbono y metano (lectura ADC 0-4095)
- **KY-026** — Detector de llama (digital + analógico)

## Configuración WiFi y MQTT

Las credenciales se configuran mediante `setup.sh` que genera `src/config/user_config.h`:

- WiFi SSID y contraseña
- URI del broker MQTT
- Credenciales MQTT del dispositivo
- ID de dispositivo

## Comandos

```bash
# Compilar firmware
pio run

# Subir firmware al ESP32
pio run --target upload

# Ejecutar tests
pio test

# Limpiar y recompilar
pio run --target clean && pio run

# Monitor serie
pio device monitor
```
