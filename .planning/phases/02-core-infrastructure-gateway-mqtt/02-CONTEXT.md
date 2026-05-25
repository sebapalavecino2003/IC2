# Phase 2: Core Infrastructure + Gateway MQTT - Context

**Gathered:** 2026-05-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Desplegar la infraestructura base del servidor con Docker Compose, broker MQTT (Mosquitto), base de datos MySQL, y API REST Django. Incluye la configuración de red local para que el backend y el broker sean accesibles desde los nodos ESP32. No incluye la comunicación MQTT del ESP32 (fase 3), el dashboard web (fase 4), ni la automatización de actuadores (fase 5).

</domain>

<decisions>
## Implementation Decisions

### Docker Compose Layout
- **D-01:** Múltiples archivos: `docker-compose.yml` (base, define servicios) + `docker-compose.override.yml` (overrides de desarrollo, hot reload, debug)
- **D-02:** Todos los servicios en la misma red Docker bridge. Django se conecta a Mosquitto por nombre de contenedor (`mosquitto:1883`). MySQL también por nombre (`mysql:3306`)
- **D-03:** Named volumes de Docker para persistencia: `mysql_data` para MySQL, `mosquitto_data` y `mosquitto_log` para Mosquitto
- **D-04:** Exponer Django (puerto 8000) y Mosquitto (puerto 1883) al host. MySQL permanece interno a la red Docker, sin exponer al host

### Esquema de Base de Datos (MySQL)
- **D-05:** 4 tablas: `devices`, `readings`, `events`, `users`
- **D-06:** Tabla única `readings` con `device_id`, `sensor_type` (ENUM: temperature/humidity/gas/flame), `value` (FLOAT), `unit` (VARCHAR), `timestamp` (DATETIME). DHT22 produce 2 filas por muestra (temperatura + humedad)
- **D-07:** Tabla única `events` con `device_id`, `event_type` (VARCHAR), `severity` (ENUM: info/warning/critical), `message` (TEXT), `resolved` (BOOL), `timestamp`. Sin tabla separada de alertas

### Estructura del Proyecto Django
- **D-08:** Single app Django: `core` (contiene models, vistas API, integración MQTT)
- **D-09:** Nombre del proyecto Django: `nodealert`
- **D-10:** Directorio del proyecto: `NodeAlert-Backend/` (paralelo a `NodeAlert-Firmware/`)
- **D-11:** Docker Compose en la raíz del repo

### Diseño de API REST
- **D-12:** Django REST Framework con viewsets, serializers, token authentication, paginación
- **D-13:** API versionada con prefijo `/api/v1/`
- **D-14:** Endpoints MVP: `GET /api/v1/readings` (con filtros por sensor_type, device, rango de tiempo), CRUD completo en `GET/POST/PUT/DELETE /api/v1/devices` y `GET/POST/PUT/DELETE /api/v1/events`, `POST /api/v1/auth/login`. Readings POST se añade en Fase 3 (ingesta MQTT desde Django)
- **D-15:** Sin exponer MySQL al host — solo Django y Mosquitto son accesibles desde la LAN

### Autenticación MQTT
- **D-16:** Credenciales compartidas (un único usuario/contraseña) para todos los dispositivos en MVP de 1 nodo
- **D-17:** Credenciales en archivo `.env` en tiempo de deploy (`.env` está en `.gitignore`)
- **D-18:** Archivo `mosquitto_passwd` montado dentro del contenedor Mosquitto para autenticación nativa
- **D-19:** Django lee las mismas credenciales del `.env` para conectarse como suscriptor MQTT

### Script de Despliegue
- **D-20:** Script único `setup.sh` que: crea `.env` con valores solicitados al usuario, genera `mosquitto_passwd`, ejecuta `docker compose up`, corre migraciones de Django, crea superusuario admin
- **D-21:** Soporte multi-plataforma: detecta arquitectura y usa imágenes Docker apropiadas (arm64 para Raspberry Pi, amd64 para desarrollo)
- **D-22:** `setup.sh` también genera configuración de firmware (WiFi + MQTT) para compilar y flashear el ESP32 desde un solo punto

### Agent's Discretion
- Estructura interna de archivos dentro de la app `core` de Django
- Configuraciones exactas de Docker Compose (versión, health checks, restart policies)
- Implementación concreta de viewsets y serializers DRF (sigue convenciones estándar de DRF)
- Valores numéricos: tamaños de paginación, timeouts, tamaños de buffer
- Configuración específica de Mosquitto (`mosquitto.conf`, listeners, log settings)

</decisions>

<canonical_refs>
## Canonical References

Downstream agents MUST read these before planning or implementing.

### Project Context
- `.planning/PROJECT.md` — Project context, core value, constraints, key decisions
- `.planning/REQUIREMENTS.md` — Full requirements with traceability (GATE-01–03, API-01/04/05/06, INFRA-01–03)
- `.planning/ROAMAP.md` — Phase overview, goal, success criteria, requirements list
- `.planning/STATE.md` — Current project state

### Phase 1 Context (Firmware Integration Points)
- `.planning/phases/01-firmware-foundation-sensores/01-CONTEXT.md` — Locked firmware decisions. MQTT topic prefix `nodealert/`, port 1883, keepalive 60s
- `NodeAlert-Firmware/src/config/mqtt_config.h` — Firmware MQTT configuration placeholder (broker URI, port, topic prefix, keepalive)
- `NodeAlert-Firmware/src/config/wifi_config.h` — Firmware WiFi configuration placeholder (SSID, password)
- `NodeAlert-Firmware/src/config/pins_config.h` — GPIO pin definitions (PIN_RELAY_ACTUATOR, etc.)

### No external specs
No external specs — requirements fully captured in decisions above

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **Firmware MQTT config**: `mqtt_config.h` defines `MQTT_BROKER_URI`, `MQTT_PORT`, `MQTT_KEEPALIVE`, `MQTT_TOPIC_PREFIX` (`nodealert/`) with `#ifndef` guards — ready for deploy-time override
- **Firmware WiFi config**: `wifi_config.h` defines `WIFI_SSID`, `WIFI_PASS` as placeholders
- **Firmware actuator pin**: `pins_config.h` defines `PIN_RELAY_ACTUATOR` (GPIO_NUM_2) — reserved for Phase 5 automation

### Established Patterns
- **snake_case naming**: All firmware files and identifiers follow snake_case. Backend should follow Django convention (snake_case for Python, PascalCase for classes)
- **Centralized config**: Firmware uses centralized config headers. Backend follows Django pattern: settings.py + .env
- **Modular directory layers**: Firmware organized by layers (hal/, drivers/, managers/, services/). Backend follows Django app structure

### Integration Points
- **Firmware ↔ Backend via MQTT**: Firmware publishes to `nodealert/{device_id}/{sensor_type}` topics. Backend subscribes to `nodealert/+/+` for telemetry ingestion (Phase 3)
- **MQTT topic hierarchy**: `nodealert/` prefix, device_id level, sensor_type/command leaf. Must be consistent across firmware and backend
- **Django ↔ ESP32 API**: Django REST API serves historical readings and device management. ESP32 may query device config via API (future)

</code_context>

<specifics>
## Specific Ideas

- Despliegue Docker Compose en Raspberry Pi como servidor local unificado (un solo dispositivo)
- Backend Django como suscriptor MQTT para ingesta automática de telemetría
- API REST como fuente de datos para el dashboard web (Fase 4)
- Script setup.sh como punto único de configuración para evitar pasos manuales
- Mosquitto como gateway central — todos los mensajes pasan por él
- La configuración de red debe permitir que los ESP32 se conecten al broker desde la LAN

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-core-infrastructure-gateway-mqtt*
*Context gathered: 2026-05-24*
