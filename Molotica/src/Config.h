#pragma once

// ====================================================================
// CONFIGURACIÓN CENTRALIZADA DEL NODO NODEALERT
// ====================================================================
// Toda constante tunable del firmware vive aquí. Ningún valor mágico
// debería aparecer en el .ino. Esto permite reconfigurar el nodo sin
// tocar la lógica. Los valores con #ifndef pueden ser sobrescritos
// desde flags del compilador (build_flags en platformio.ini).
//
// Esta arquitectura permite que setup.sh genere un archivo
// user_config.h que se incluye antes que este, sobreescribiendo
// los valores por defecto mediante -include en platformio.ini.
// ====================================================================

// ----------------------- RED WIFI -----------------------
// Credenciales de la red WiFi a la que el ESP32 se conectará.
#define WIFI_SSID       "UNRaf_Libre"
#define WIFI_PASS       "unraf2021"

// ----------------------- BROKER MQTT -----------------------
// Dirección IP del broker Mosquitto y puerto estándar 1883.
// PREFIJO_TOPIC_MQTT define el namespace para todos los topics MQTT
// del sistema, permitiendo múltiples despliegues en el mismo broker.
#define MQTT_BROKER_IP   "10.1.1.10"
#define MQTT_BROKER_PORT  1883
#define PREFIJO_TOPIC_MQTT "nodealert"          // namespace para todos los topics MQTT

// Credenciales MQTT con #ifndef para permitir override desde build_flags
// o desde el archivo user_config.h generado por setup.sh.
#ifndef USUARIO_MQTT
#define USUARIO_MQTT      "nodealert_gateway"
#endif
#ifndef CLAVE_MQTT
#define CLAVE_MQTT        "password"
#endif

// Identificador único del dispositivo.
// Se utiliza como parte del client ID MQTT y como device_id en los
// mensajes de telemetría. Puede sobreescribirse por despliegue.
#ifndef ID_DISPOSITIVO
#define ID_DISPOSITIVO    "nodo1"
#endif

// ----------------------- PINES DE SENSORES Y ACTUADORES -----------------------
// Definición de pines GPIO a los que están conectados los sensores y
// actuadores. Estos valores dependen del cableado físico y no deben
// cambiarse sin modificar el hardware.
#define DHT22_PIN           4   // GPIO4  — sensor temp/humedad (DHT22, 1-Wire)
#define PIN_MQ9             34  // ADC1_CH6 — sensor de gas LP (salida analógica)
#define PIN_KY026           35  // ADC1_CH7 — sensor de llama (salida analógica)
#define PIN_KY026_DIGITAL   27  // GPIO27 — sensor de llama (salida digital, LOW = detecta)
#define PIN_BUZZER          26  // GPIO26 — zumbador (activo HIGH, se mantiene en sleep via gpio_hold)

// ----------------------- UMBRALES POR DEFECTO -----------------------
// Se cargan en RTC_DATA_ATTR al primer boot y persisten entre ciclos
// de deep sleep. El backend puede modificarlos dinámicamente vía MQTT.
#define UMBRAL_TEMP_DEFECTO     50     // °C  — temperatura máxima antes de alarmar
#define UMBRAL_GAS_DEFECTO     2000   // raw ADC — concentración de gas LP máxima
#define UMBRAL_LLAMA_DEFECTO   2000   // raw ADC — intensidad de llama máxima

// ----------------------- DEEP SLEEP -----------------------
// Intervalo entre ciclos de wakeup en microsegundos.
// 60 segundos entre lecturas con consumo de ~5 µA en deep sleep.
#define INTERVALO_SUENIO_US   60000000ULL  // 60 segundos entre lecturas (en microsegundos)
