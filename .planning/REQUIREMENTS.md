# Requirements: NodeAlert IoT

**Defined:** 2026-05-24
**Last updated:** 2026-05-25 (milestone v1.0 complete 🎉)
**Core Value:** Detectar condiciones ambientales peligrosas y actuar preventivamente antes de que escalen a emergencias, incluso sin conexión al servidor central.

## v1 Requirements

Requirements for initial release. Each maps to a roadmap phase.

### Firmware — Arquitectura y Plataforma

- [x] **FWK-01**: Proyecto PlatformIO con estructura profesional separando `.h`/`.cpp` por módulos
- [x] **FWK-02**: Capa HAL (Hardware Abstraction Layer) para aislar drivers del hardware específico
- [x] **FWK-03**: Sistema de configuración centralizada (WiFi, MQTT, pines, umbrales)
- [x] **FWK-04**: Máquina de estados del sistema (init, running, error, alert, recovery)
- [x] **FWK-05**: Gestión de errores y recuperación con watchdog y reintentos — ErrorHandler con backoff exponencial + Task WDT + Interrupt WDT

### Firmware — Drivers de Sensores

- [x] **DRV-01**: Driver modular para DHT22 (temperatura y humedad) con HAL
- [x] **DRV-02**: Driver modular para MQ-9 (gases inflamables y CO) con HAL
- [x] **DRV-03**: Driver modular para KY-026 (detección de llama) con HAL
- [x] **DRV-04**: Módulo de calibración y filtrado de lecturas de sensores

### Firmware — FreeRTOS y Tiempo Real

- [x] **RTOS-01**: Tareas FreeRTOS desacopladas por función (7 tareas: 3 sensores, monitor, automation, MQTT, main loop)
- [x] **RTOS-02**: Sistema de colas (queues) para comunicación entre tareas
- [x] **RTOS-03**: Mutex para acceso seguro a recursos compartidos (ADC, GPIO)
- [x] **RTOS-04**: Prioridades de tareas bien definidas (KY-026: prio 4 > MQ-9: prio 3 > DHT22: prio 2 > automation: prio 2 > MQTT: prio 1 > monitor: prio 1)
- [x] **RTOS-05**: Interrupciones de hardware para eventos críticos (KY-026 ISR para detección de llama)

### Firmware — Lógica de Automatización

- [x] **AUTO-01**: Evaluación autónoma de condiciones críticas (temp alta, gas, llama) con AutomationManager cada 3s
- [x] **AUTO-02**: Histéresis dual (tiempo 3s + delta 10%) para evitar falsos positivos
- [x] **AUTO-03**: Control local de actuador (relé GPIO 2) basado en umbrales configurables, mínimo 2min activo
- [x] **AUTO-04**: Recepción y ejecución de 5 comandos MQTT de override (actuator_on/off, return_to_auto, acknowledge_alarm, update_thresholds)

### Firmware — Comunicación MQTT

- [x] **MQTT-01**: Módulo MqttManager con reconexión automática vía esp_mqtt_client (event handler: MQTT_EVENT_CONNECTED/DISCONNECTED)
- [x] **MQTT-02**: Publicación periódica de telemetría cada 10s en `nodealert/{id}/telemetry`
- [x] **MQTT-03**: Suscripción a `nodealert/{id}/commands` con callback a AutomationManager::processCommand
- [x] **MQTT-04**: MessageBuffer circular (tamaño configurable) para almacenar mensajes durante desconexión, drenado al reconectar
- [x] **MQTT-05**: Keep Alive 60s (configurable en mqtt_broker_config.h)

### Gateway — Broker MQTT

- [x] **GATE-01**: Broker Mosquitto 2 funcionando como gateway central en Docker
- [x] **GATE-02**: Persistencia de sesiones y autoconfiguración vía mosquitto.conf
- [x] **GATE-03**: Autenticación básica con archivo mosquitto_passwd, 3 usuarios por rol (broker, firmware, subscriber)

### Backend — Django

- [x] **API-01**: API REST Django (DRF) con CRUD de dispositivos, lecturas (RO), eventos, comandos
- [~] **API-02**: Streaming de datos — implementado con polling (3s) en vez de WebSockets. Aceptado para v1.
- [x] **API-03**: Suscripción MQTT del backend vía management command `mqtt_subscriber` con paho-mqtt
- [x] **API-04**: Almacenamiento histórico de lecturas en MySQL con filtros por sensor_type, device_id, rango de tiempo
- [x] **API-05**: Persistencia de eventos y alertas con severidad y estado resolved
- [x] **API-06**: Autenticación DRF Token + rate limiting (5/min login, 100/min API)

### Frontend — Dashboard React

- [x] **UI-01**: Dashboard React 18 + Vite + Material UI (tema Industrial Dark), responsive, Docker multi-stage
- [x] **UI-02**: Visualización en tiempo real con SensorGauge (temp, humedad, gas, llama) y polling 3s
- [x] **UI-03**: Historial con gráficos Recharts (líneas de temperatura/humedad/gas), filtros 1h/6h/24h/7d
- [x] **UI-04**: Panel de alertas con tabs (Alertas Activas / Historial), severidad, silenciar, resolver
- [x] **UI-05**: Acceso con autenticación DRF Token + axios interceptor + ruta protegida
- [x] **UI-06**: Chip ESP32 Conectado/Desconectado + modo Auto/Override Manual en SummaryBar

### Infraestructura

- [x] **INFRA-01**: Docker Compose con 5 servicios (mosquitto, django, frontend, mysql, nginx) + HEALTHCHECKs
- [x] **INFRA-02**: Script `setup.sh` con configuración interactiva completa (WiFi, MQTT, MySQL, Django) + generación de firmware headers
- [x] **INFRA-03**: Red Docker bridge interna + WiFi para ESP32, LAN para servidor

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Alertas y Notificaciones

- **ALRT-01**: Notificaciones por Telegram Bot API
- **ALRT-02**: Alertas por email para eventos críticos
- **ALRT-03**: Historial y gestión de alertas

### Optimización Energética

- **PWR-01**: Deep Sleep y Light Sleep en nodos ESP32
- **PWR-02**: Wake-up rápido por evento (interrupción o timer)
- **PWR-03**: Reducción de tiempo TX/RX WiFi
- **PWR-04**: Optimización de Keep Alive MQTT
- **PWR-05**: Gestión eficiente de consumo en sensores
- **PWR-06**: Soporte para nodos alimentados por batería

### Escalabilidad

- **SCALE-01**: Soporte multi-nodo (2+ nodos ESP32)
- **SCALE-02**: ESP-NOW o LoRa como respaldo ante pérdida de WiFi
- **SCALE-03**: Sincronización y agregación de datos multi-nodo

### Features Avanzadas

- **ADV-01**: OTA (Over-The-Air) updates para firmware ESP32
- **ADV-02**: Dashboard con mapas de calor y tendencias
- **ADV-03**: Exportación de datos (CSV, PDF)
- **ADV-04**: Roles de usuario (admin, viewer, operator)

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Alertas Telegram | Postergado a v2. Las alertas se manejan en el dashboard en v1. |
| Deep Sleep / batería | Alimentación USB/transformador en v1. Optimización energética en v2. |
| ESP-NOW / LoRa | Solo WiFi en v1. Protocolos alternativos evaluados para v2. |
| Despliegue cloud / VPS | Todo local en Raspberry Pi con Docker Compose. |
| OTA updates | Excede el alcance de v1. Requiere infraestructura adicional. |
| Aplicación móvil nativa | Web-first. App móvil nativa no planificada. |
| Video / cámaras | Solo sensores ambientales. Sin soporte de video. |
| Machine Learning | Sin análisis predictivo en v1. Evaluar para v3+. |

## Traceability

| Requirement | Phase | Status | Notes |
|-------------|-------|--------|-------|
| FWK-01 | Phase 1 | ✅ | 01-A-SCAFFOLD |
| FWK-02 | Phase 1 | ✅ | 01-A-SCAFFOLD |
| FWK-03 | Phase 1 | ✅ | 01-A-SCAFFOLD |
| FWK-04 | Phase 1 | ✅ | 01-B-DHT22 |
| FWK-05 | Phase 1+6 | ✅ | ErrorHandler Phase 1 + WDT Phase 6 |
| DRV-01 | Phase 1 | ✅ | 01-B-DHT22 |
| DRV-02 | Phase 1 | ✅ | 01-C-MQ9-KY026 |
| DRV-03 | Phase 1 | ✅ | 01-C-MQ9-KY026 |
| DRV-04 | Phase 1 | ✅ | 01-C-MQ9-KY026 |
| RTOS-01 | Phase 1 | ✅ | 01-D-RTOS-INTEGRATION |
| RTOS-02 | Phase 1 | ✅ | 01-D-RTOS-INTEGRATION |
| RTOS-03 | Phase 1 | ✅ | 01-D-RTOS-INTEGRATION |
| RTOS-04 | Phase 1 | ✅ | 01-D-RTOS-INTEGRATION |
| RTOS-05 | Phase 1 | ✅ | 01-C-MQ9-KY026 (KY-026 ISR) |
| AUTO-01 | Phase 5 | ✅ | 05-01 AutomationManager |
| AUTO-02 | Phase 5 | ✅ | 05-01 Hysteresis time+delta |
| AUTO-03 | Phase 5 | ✅ | 05-01 Relay GPIO 2 |
| AUTO-04 | Phase 5 | ✅ | 05-02 5 MQTT commands |
| MQTT-01 | Phase 3 | ✅ | 03-01 MqttManager + event handler |
| MQTT-02 | Phase 3 | ✅ | 03-01 10s telemetry publish |
| MQTT-03 | Phase 3 | ✅ | 03-01 commands subscription |
| MQTT-04 | Phase 3 | ✅ | 03-01 MessageBuffer |
| MQTT-05 | Phase 3 | ✅ | 03-01 Keep alive 60s |
| GATE-01 | Phase 2 | ✅ | 02-01 Mosquitto Docker |
| GATE-02 | Phase 2 | ✅ | 02-01 mosquitto.conf persistence |
| GATE-03 | Phase 2 | ✅ | 02-01 mosquitto_passwd auth |
| API-01 | Phase 2 | ✅ | 02-02 DRF CRUD |
| API-02 | Phase 4 | ~ | Polling 3s (no WebSockets) — aceptado v1 |
| API-03 | Phase 3 | ✅ | 03-02 mqtt_subscriber |
| API-04 | Phase 2 | ✅ | 02-02 MySQL + Reading model |
| API-05 | Phase 2 | ✅ | 02-02 Event model |
| API-06 | Phase 2+6 | ✅ | Token auth (P2) + rate limiting (P6) |
| UI-01 | Phase 4 | ✅ | 04-01 Vite+MUI scaffold |
| UI-02 | Phase 4 | ✅ | 04-02 SensorGauge |
| UI-03 | Phase 4 | ✅ | 04-03 Recharts history |
| UI-04 | Phase 4 | ✅ | 04-03 + 05-03 AlertPanel |
| UI-05 | Phase 4 | ✅ | 04-02 Auth context |
| UI-06 | Phase 4 | ✅ | 04-02 SummaryBar online/offline |
| INFRA-01 | Phase 2 | ✅ | 02-01 Docker Compose base |
| INFRA-02 | Phase 2 | ✅ | 02-03 + 06-02 setup.sh + nginx |
| INFRA-03 | Phase 2 | ✅ | 02-03 WiFi+LAN config gen |

**Coverage:**
- v1 requirements: 42 total
- Completed: 41 ✅
- Partial: 1 ~ (API-02: polling en vez de WebSockets)
- Uncompleted: 0
- Coverage: 97.6% (100% funcional)

---

*Requirements defined: 2026-05-24*
*Last updated: 2026-05-25 (milestone v1.0 complete)*
