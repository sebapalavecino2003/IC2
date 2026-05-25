# Phase 3: Comunicación MQTT ESP32 → Backend - Context

**Gathered:** 2026-05-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Integrar la comunicación MQTT entre el ESP32 y el backend Django a través del broker Mosquitto. Establecer el flujo de datos completo: sensores → ESP32 → MQTT → Django → MySQL. Incluye el módulo MQTT en firmware, la ingesta en backend, el buffer local de mensajes, reconexión automática y Keep Alive optimizado. No incluye el dashboard web (fase 4) ni la automatización de actuadores (fase 5).

</domain>

<decisions>
## Implementation Decisions

### Topic Structure and Message Format
- **D-01:** Single telemetry topic por dispositivo: `nodealert/{device_id}/telemetry`. Un solo tópico por ESP32 con JSON plano conteniendo todos los valores de sensores.
- **D-02:** Payload JSON plano (no array de readings). Formato: `{device_id, timestamp, temperature, humidity, gas_ppm, flame_detected}`. El backend deserializa y crea registros individuales en la tabla `readings`.
- **D-03:** Tópicos dedicados para eventos y comandos: `nodealert/{device_id}/events` (eventos del ESP32 al backend) y `nodealert/{device_id}/commands` (comandos del backend al ESP32).
- **D-04:** Frecuencia de publicación: cada 10s con los valores más recientes de cada sensor (aggregated, no por ciclo de sampleo).

### MQTT Task en Firmware ESP32
- **D-05:** Tarea FreeRTOS dedicada para MQTT (no integrada en main loop). Se comunica con el resto del sistema vía queues.
- **D-06:** Prioridad 1 (lowest, según D-07 de Phase 1).
- **D-07:** Stack size 6144 words.
- **D-08:** Core 0 (Pro CPU / WiFi) — el stack MQTT de ESP-IDF usa WiFi internamente.
- **D-09:** Usar el cliente MQTT nativo de ESP-IDF (`mqtt_client.h`). No requiere librerías externas.
- **D-10:** Habilitar `PRIV_REQUIRES mqtt` en `src/CMakeLists.txt` para activar el componente MQTT en la build de ESP-IDF.

### Manejo de Errores MQTT
- **D-11:** Reportar errores MQTT vía `error_handler` con `source='MQTT'` — reusa el backoff exponencial existente (Phase 1). El cliente ESP-MQTT también maneja auto-reconnect internamente.
- **D-12:** La desconexión MQTT no cambia el estado del sistema. MQTT corre como servicio background independiente de la máquina de estados.

### Django Ingestion Service (API-03)
- **D-13:** Management command de Django: `python manage.py mqtt_subscriber` usando la librería `paho-mqtt`.
- **D-14:** El comando se inicia automáticamente desde `entrypoint.sh` después de migrations.
- **D-15:** Solo dispositivos conocidos — rechazar mensajes de `device_id` no registrados.
- **D-16:** Los eventos críticos los publica el ESP32 directamente a `nodealert/{device_id}/events`. El backend no infiere eventos desde lecturas.
- **D-17:** Usuario MQTT dedicado para el subscriber de Django (`mqtt_subscriber`), separado de las credenciales compartidas de los nodos ESP32.

### Credenciales MQTT en Firmware
- **D-18:** Añadir `MQTT_USER` y `MQTT_PASS` a `mqtt_config.h` con `#ifndef` guards para override vía `user_config.h`.
- **D-19:** `setup.sh` debe generar también `MQTT_USER` y `MQTT_PASS` en `user_config.h`.

### Message Buffering Local
- **D-20:** Buffer circular en RAM (cola de mensajes) para almacenar publicaciones durante desconexión. Tamaño: 20 mensajes. Política de overflow: descartar los más antiguos.
- **D-21:** Al reconectar, drenar el buffer y reenviar mensajes acumulados. Sin persistencia en flash para MVP.

### Agent's Discretion
- Valores concretos de Keep Alive (default 60s está bien de mqtt_config.h)
- Implementación exacta del management command (estructura de archivos, logging)
- Formato exacto del JSON de telemetría (nombres de campos)
- Configuración de QoS (default 0 para telemetría periódica, 1 para eventos críticos)
- Detalles de la cola FreeRTOS para comunicación MQTT ↔ sistema

</decisions>

<canonical_refs>
## Canonical References

Downstream agents MUST read these before planning or implementing.

### Project Context
- `.planning/PROJECT.md` — Project context, core value, constraints
- `.planning/REQUIREMENTS.md` — Requirements traceability (MQTT-01–05, API-03)
- `.planning/ROADMAP.md` — Phase overview, goal, success criteria
- `.planning/STATE.md` — Current project state

### Phase 1 Context (Firmware)
- `.planning/phases/01-firmware-foundation-sensores/01-CONTEXT.md` — Sensor sampling rates, RTOS priorities, state machine, error handler
- `NodeAlert-Firmware/src/core/state_machine.h` — SystemState enum (6 states), transition validation
- `NodeAlert-Firmware/src/core/error_handler.h` — Error reporting with exponential backoff
- `NodeAlert-Firmware/src/config/mqtt_config.h` — MQTT defines (MQTT_BROKER_URI, PORT, KEEPALIVE, TOPIC_PREFIX)
- `NodeAlert-Firmware/src/config/wifi_config.h` — WiFi SSID/PASS defines
- `NodeAlert-Firmware/src/managers/task_manager.cpp` — FreeRTOS task creation pattern

### Phase 2 Context (Backend Infrastructure)
- `.planning/phases/02-core-infrastructure-gateway-mqtt/02-CONTEXT.md` — Docker Compose, Mosquitto config, API endpoints, auth
- `docker-compose.yml` — Mosquitto service on port 1883, nodealert-net
- `mosquitto/config/mosquitto.conf` — Broker config (auth, persistence, retain, limits)
- `NodeAlert-Backend/core/models.py` — Device, Reading, Event models
- `NodeAlert-Backend/core/views.py` — Existing DRF viewsets
- `NodeAlert-Backend/nodealert/settings.py` — Django settings, INSTALLED_APPS
- `NodeAlert-Backend/entrypoint.sh` — Entrypoint with auto-superuser (MQTT subscriber will be added here)
- `setup.sh` — Deployment script that generates user_config.h (needs update for credentials)

### No external specs
No external specs — requirements fully captured in decisions above

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets (Firmware)
- **mqtt_config.h**: Defines 4 MQTT constants with `#ifndef` guards — ready for `user_config.h` override. Needs `MQTT_USER` and `MQTT_PASS` added.
- **wifi_config.h**: WiFi SSID/PASS with `#ifndef` guards — ready for override.
- **error_handler**: Exponential backoff (1000ms base, 3 retries, capped at 60s) — reusable for MQTT connection retries via `source='MQTT'`.
- **task_manager.cpp**: Pattern for `xTaskCreatePinnedToCore()` — MQTT task will follow same pattern.
- **sensor_queue**: Existing queue (20 items of `SensorReading`) — MQTT task can consume this same queue, or a dedicated command queue can be added.
- **State machine**: 6 states (INIT → STANDBY → RUNNING → ALERT → ERROR → RECOVERY) — MQTT runs independently, no new state needed.

### Reusable Assets (Backend)
- **Device, Reading, Event models**: Ready for MQTT ingestion — management command creates Reading/Event instances from MQTT payloads.
- **Mosquitto service**: Already deployed and authenticated — new MQTT subscriber user needs to be added to mosquitto_passwd.
- **entrypoint.sh**: Auto-superuser creation already implemented — MQTT subscriber startup will be added here.
- **.env**: Already has `MQTT_BROKER_USER`, `MQTT_BROKER_PASSWORD` — a second pair for the subscriber can be added.

### Established Patterns
- **Firmware**: snake_case naming, ISensor interface, FreeRTOS tasks with queues, error_handler for error tracking
- **Backend**: Django REST Framework, management commands, env var configuration

### Integration Points
- **Firmware MQTT topic**: `nodealert/{device_id}/telemetry` — backend subscribes to `nodealert/+/telemetry`
- **Firmware event topic**: `nodealert/{device_id}/events` — backend subscribes to `nodealert/+/events`
- **Firmware command topic**: `nodealert/{device_id}/commands` — backend publishes to specific device topics
- **Django subscriber**: Reads `MQTT_BROKER_*` env vars from `.env`, connects with paho-mqtt
- **setup.sh**: Must generate `MQTT_USER`/`MQTT_PASS` in `user_config.h` alongside existing WiFi/MQTT URI

</code_context>

<specifics>
## Specific Ideas

- Flujo end-to-end: sensor → ESP32 → MQTT → Django → MySQL
- Reconexión automática tanto en firmware (ESP-IDF mqtt_client + error_handler) como en backend (paho-mqtt loop)
- Keep Alive de 60s mantiene conexión estable sin consumo excesivo
- Buffer local de 20 mensajes protege contra caídas breves del broker
- Management command `mqtt_subscriber` se inicia automáticamente con el contenedor Django

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 3-mqtt-firmware-integration*
*Context gathered: 2026-05-24*
