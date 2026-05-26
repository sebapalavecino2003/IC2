# Arquitectura del Sistema — NodeAlert IoT

## Visión General

NodeAlert IoT es un sistema distribuido de monitoreo ambiental con 4 capas:

1. **Sensores ESP32** — Adquisición de datos y procesamiento local
2. **Broker MQTT** — Comunicación asíncrona entre nodos y servidor
3. **Backend Django** — API REST, persistencia y suscripción MQTT
4. **Frontend React** — Dashboard en tiempo real para operadores

## Diagrama de Flujo

```
[Sensor DHT22] ──┐
[Sensor MQ-9]   ──┤──→ ESP32 FreeRTOS ──→ MQTT Broker ──→ Django Subscriber ──→ MySQL
[Sensor KY-026] ──┘      (7 tareas)       (Mosquitto)     (paho-mqtt)
                   │                                             │
                   │  ┌──────────────────────────────────────────┘
                   │  ↓
                   │  Django REST API ←── React Dashboard (MUI)
                   │  (DRF, Gunicorn)       (polling 3s)
                   │
                   └── Actuador (relé) ──→ Control local
                       (AutomationManager)
```

## Componentes

### Firmware ESP32

- **Microcontrolador:** ESP32 con FreeRTOS y ESP-IDF 6.0.1
- **Máquina de estados:** 6 estados (INIT → STANDBY → RUNNING ↔ ALERT → RECOVERY → ERROR)
- **Tareas FreeRTOS (7):** Main loop, DHT22, MQ-9, KY-026, Monitor, Automation, MQTT
- **Watchdog:** Task WDT + Interrupt WDT con auto-reboot, 10s timeout
- **MQTT Publisher:** Telemetría cada 10s, estado cada 60s, eventos en transiciones, Last Will en desconexión
- **Automatización local:** Evaluación de umbrales sin conexión al servidor, control de actuador con histéresis

### Broker Mosquitto

- **Puerto:** 1883 (MQTT)
- **Autenticación:** Archivo mosquitto_passwd con usuarios separados por rol
- **Persistencia:** Sesiones duraderas configuradas en mosquitto.conf
- **Tópicos:** `nodealert/{device_id}/telemetry`, `events`, `commands`, `status`

### Backend Django

- **Framework:** Django 5 + Django REST Framework + MySQL 8
- **Servidor producción:** Gunicorn con toggle por variable de entorno (`GUNICORN_ENABLED`)
- **Proxy inverso:** nginx en contenedor separado, punto de entrada en puerto 80
- **Suscriptor MQTT:** Management command `mqtt_subscriber` que corre en background
- **Autenticación:** DRF Token Authentication
- **Rate limiting:** 5 req/min en login (anónimo), 100 req/min en API general (autenticado)
- **Health checks:** Endpoints de liveness (`/health/`) y readiness (`/health/ready/`)
- **Contenedores Docker:** django, mysql, mosquitto, frontend, nginx (todos con HEALTHCHECK)

### Frontend React

- **Framework:** React 18 + TypeScript + Vite + Material UI
- **Dashboard:** 4 vistas (Dashboard, Dispositivos, Historial, Configuración)
- **Autenticación:** Token en localStorage, Authorization header
- **Polling:** Consulta cada 3 segundos de datos en tiempo real
- **Despliegue:** Build multi-stage Docker con nginx para archivos estáticos

## Decisiones Técnicas Clave

### Watchdog ESP32 (D-01 a D-04)
- Task WDT para detección de hilos colgados en las 7 tareas FreeRTOS
- Interrupt WDT como red de seguridad ante cuelgue total de CPU
- Auto-reboot + Last Will MQTT para notificar desconexión al backend

### Gunicorn + nginx (D-05 a D-08)
- Migración a Gunicorn con workers configurables mediante variable de entorno
- nginx como proxy inverso eliminando exposición directa de Django
- Archivos estáticos servidos directamente por nginx

### Health Checks (D-09 a D-13)
- Endpoint de liveness (200 si el proceso vive) sin dependencias externas
- Endpoint de readiness verificando MySQL y suscriptor MQTT (200/503)
- HEALTHCHECK en todos los contenedores Docker
- Heartbeat ESP32 cada 60s con payload completo (wifi_rssi, errores_activos, etc.)
- Publicación inmediata de estado en transiciones de estado

### Rate Limiting (D-14, D-15)
- Login: 5 solicitudes/minuto por IP (AnonRateThrottle)
- API general: 100 solicitudes/minuto por usuario (UserRateThrottle)

### Automatización (Phase 5)
- Evaluación local en ESP32 sin dependencia del servidor
- Override remoto vía comandos MQTT desde el servidor
- Histéresis para evitar oscilación rápida de estados
