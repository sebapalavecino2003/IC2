# Phase 6: Hardening y Documentación — Context

**Gathered:** 2026-05-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Robustecer el sistema completo con tolerancia a fallos (Watchdog en ESP32), migración a servidor productivo (Gunicorn + nginx), monitoreo (health checks en Django + Docker + reportes MQTT), hardening de autenticación (rate limiting) y documentación técnica (READMEs + guía de despliegue + arquitectura + API). Es la fase final del milestone v1.0.

No incluye: notificaciones externas (v2), multi-nodo (v2), deep sleep / optimización energética (v2), SSL/TLS (v2), logging remoto centralizado (v2).

</domain>

<decisions>
## Implementation Decisions

### Watchdog ESP32 (Task WDT + Interrupt WDT)
- **D-01:** Se habilitan ambos watchdogs: Task WDT (*esp_task_wdt*) para detección de hilos colgados + Interrupt WDT (*esp_int_wdt*) como red de seguridad ante cuelgue total de CPU.
- **D-02:** Al dispararse el watchdog: auto-reboot vía `esp_restart()`. Antes de reiniciar, se publica un mensaje Last Will en MQTT (tópico `nodealert/{device_id}/status`) indicando la causa del reset para que el backend lo registre.
- **D-03:** Todas las tareas FreeRTOS quedan bajo vigilancia del Task WDT: main loop, monitor task, DHT22, MQ-9, KY-026, MqttManager, AutomationManager.
- **D-04:** `WATCHDOG_TIMEOUT_MS 10000` (ya definido en `sampling_config.h`) se usa como timeout base para todas las tareas. Se configura vía `esp_task_wdt_init()` al arrancar.

### Servidor Django Producción (Gunicorn + nginx)
- **D-05:** Migración a Gunicorn con 2-4 workers (fórmula: `2 × CPU cores + 1`). La configuración se habilita mediante variable de entorno `GUNICORN_ENABLED=true`. Si no está presente, `entrypoint.sh` mantiene `runserver` para desarrollo local.
- **D-06:** Se añade un contenedor nginx como proxy inverso en `docker-compose.yml`. nginx sirve como reverse proxy hacia Gunicorn en `django:8000`, manejando buffering, cabeceras y sirviendo archivos estáticos directamente.
- **D-07:** nginx reemplaza la exposición directa de Django al exponer el puerto 80/443. Django queda únicamente accesible a través de la red interna de Docker.
- **D-08:** La entrada `entrypoint.sh` se modifica para soportar `GUNICORN_ENABLED`: si está en true, lanza `gunicorn nodealert.wsgi:application --workers $GUNICORN_WORKERS --bind 0.0.0.0:8000`; si no, lanza `runserver`.

### Monitoreo y Health Checks
- **D-09:** Endpoint de liveness: `GET /api/v1/health/` — responde 200 OK si el proceso está vivo. Sin dependencias externas.
- **D-10:** Endpoint de readiness: `GET /api/v1/health/ready/` — verifica conexión a MySQL y estado del suscriptor MQTT. Responde 200 si todo OK, 503 si alguna dependencia falla.
- **D-11:** Se añade directiva `HEALTHCHECK` en `docker-compose.yml` para todos los contenedores:
  - **Django:** `curl --fail http://localhost:8000/api/v1/health/` (liveness)
  - **MySQL:** `mysqladmin ping -u root -p$MYSQL_ROOT_PASSWORD --silent`
  - **Mosquitto:** `mosquitto_sub -t '$SYS/#' -C 1` o similar
  - **Frontend:** `curl --fail http://localhost:80/`
- **D-12:** ESP32 publica estado periódico en `nodealert/{device_id}/status` (tópico nuevo, QoS 1). Payload JSON con: uptime (ms), free_heap (bytes), system_state, wifi_rssi (dBm), errores_activos (Array de {source, message, elapsed_ms}), last_reset_reason.
- **D-13:** El ESP32 publica el estado cada 60s en condiciones normales, e inmediatamente en cada transición de estado (INIT, RUNNING, ALERT, ERROR, RECOVERY).

### Hardening de Autenticación (Rate Limiting)
- **D-14:** Se configura throttling vía `DEFAULT_THROTTLE_CLASSES` en DRF:
  - **Login (`/api/v1/auth/login/`):** `5 solicitudes/minuto por IP` (`AnonRateThrottle`)
  - **API general:** `100 solicitudes/minuto por usuario` (`UserRateThrottle`)
- **D-15:** Solo el endpoint de login llevará throttling anónimo. El resto de endpoints usan throttling por usuario autenticado.

### Documentación Técnica
- **D-16:** Estructura: README por componente (firmware/, backend/, frontend/) + carpeta `docs/` en la raíz con documentos transversales.
- **D-17:** Documentos a crear (todos en Español):
  - **README raíz (`README.md`):** Descripción general del proyecto, stack tecnológico, inicio rápido (`./setup.sh`), enlaces a docs.
  - **Guía de Despliegue (`docs/DEPLOY.md`):** Arquitectura de despliegue, requisitos, `setup.sh` paso a paso, configuración de firmware, troubleshooting común.
  - **Arquitectura del Sistema (`docs/ARCHITECTURE.md`):** Diagrama de flujo (ESP32 → MQTT → Django → Frontend), descripción de componentes, decisiones técnicas clave.
  - **Referencia de API (`docs/API.md`):** Todos los endpoints REST: autenticación, dispositivos, lecturas, eventos, comandos, health checks.
- **D-18:** READMEs de componente breves y específicos:
  - **Firmware:** requisitos de compilación (PlatformIO), configuración de pines, sensores soportados.
  - **Backend:** dependencias Python, migraciones, tests, management commands.
  - **Frontend:** stack (Vite + React + MUI), variables de entorno, desarrollo vs producción.

### Agente Discreción
- Implementación exacta de `esp_task_wdt` y `esp_int_wdt` (config de timeouts, subscription de todas las tareas)
- Ruta exacta de los endpoints de health (liveness/readiness) y su implementación
- Config exacta de nginx (worker_processes, proxy_pass, cabeceras)
- Config exacta de Gunicorn (bind, workers, timeout)
- Template exacto del payload JSON de estado MQTT
- Formato y organización exacta de la documentación

</decisions>

<canonical_refs>
## Canonical References

Downstream agents MUST read these before planning or implementing.

### Project Context
- `.planning/PROJECT.md` — Project context, core value, constraints
- `.planning/REQUIREMENTS.md` — Requirements traceability (API-06, INFRA-02)
- `.planning/ROADMAP.md` — Phase overview, goal, 6 success criteria
- `.planning/STATE.md` — Current project state

### Phase 1 Context (Firmware Foundation)
- `.planning/phases/01-firmware-foundation-sensores/01-CONTEXT.md` — Sensor sampling rates, RTOS priorities, state machine, error handler
- `NodeAlert-Firmware/src/core/main.cpp` — Main entry point with all FreeRTOS task creation
- `NodeAlert-Firmware/src/config/sampling_config.h` — `WATCHDOG_TIMEOUT_MS` constant
- `NodeAlert-Firmware/src/core/error_handler.h` — Error reporting with exponential backoff
- `NodeAlert-Firmware/src/managers/task_manager.cpp/.h` — Monitor task pattern (priority 1, 10s interval)

### Phase 2 Context (Backend Infrastructure)
- `.planning/phases/02-core-infrastructure-gateway-mqtt/02-CONTEXT.md` — Docker Compose, API endpoints, auth
- `docker-compose.yml` — Current 4-service stack (mosquitto, django, frontend, mysql)
- `setup.sh` — Full deployment script (476 lines)
- `NodeAlert-Backend/core/views.py` — LoginView, DeviceViewSet, etc.
- `NodeAlert-Backend/nodealert/settings.py` — DRF config (no throttling currently)
- `NodeAlert-Backend/requirements.txt` — Gunicorn already included (v22+)

### Phase 3 Context (MQTT Integration)
- `.planning/phases/03-mqtt-firmware-integration/03-CONTEXT.md` — MQTT topics, payload format
- `NodeAlert-Firmware/src/managers/mqtt_manager.h` — MQTT manager task interface
- `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py` — MQTT subscriber

### Phase 4 Context (Frontend Dashboard)
- `.planning/phases/04-dashboard-en-tiempo-real/04-CONTEXT.md` — Dashboard decisions
- `NodeAlert-Frontend/Dockerfile` — Multi-stage build with nginx
- `NodeAlert-Frontend/nginx.conf` — Current nginx config (proxies /api/ to django:8000)

### Phase 5 Context (Automation)
- `.planning/phases/05-automatizaci-n-local-y-alertas/05-CONTEXT.md` — Automation decisions, override protocol
- `NodeAlert-Firmware/src/managers/automation_manager.h/.cpp` — Automation task

### Key Files to Modify
- `NodeAlert-Firmware/src/core/main.cpp` — Add esp_task_wdt_init, subscribe all tasks, status publisher
- `NodeAlert-Firmware/src/managers/mqtt_manager.h/.cpp` — Add status topic publishing
- `NodeAlert-Backend/entrypoint.sh` — Add GUNICORN_ENABLED toggle
- `NodeAlert-Backend/nodealert/settings.py` — Add DEFAULT_THROTTLE_CLASSES
- `NodeAlert-Backend/core/views.py` — Add HealthCheck views
- `NodeAlert-Backend/core/urls.py` — Add health endpoints
- `docker-compose.yml` — Add nginx service, HEALTHCHECK directives
- New: `nginx/nginx.conf` — nginx reverse proxy config

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets (Firmware)
- **WATCHDOG_TIMEOUT_MS (10000):** Already defined in `sampling_config.h` — ready for `esp_task_wdt_init()`
- **main.cpp pattern:** All FreeRTOS tasks created in `app_main()` — one central place to subscribe each to Task WDT
- **mqtt_manager:** Existing MQTT publisher with `buildTelemetryJson()` — pattern reusable for status topic
- **mqtt_event_handler:** Existing event handler can be extended to publish Last Will on watchdog reset

### Reusable Assets (Backend)
- **Gunicorn in requirements.txt:** Already present (v22+) — no new pip dependency needed
- **DRF settings.py:** REST_FRAMEWORK dict ready for `DEFAULT_THROTTLE_CLASSES` addition
- **LoginView:** Existing authentication view — target for rate limiting
- **entrypoint.sh:** Already has TODO comment "Phase 6 will switch to gunicorn for production"
- **Dockerfile:** Python 3.12-slim image — Gunicorn works out of the box

### Reusable Assets (Frontend/Docker)
- **nginx.conf (frontend):** Existing nginx config pattern — reuse for new nginx reverse proxy service
- **docker-compose.yml:** Service pattern with `restart: unless-stopped` and `networks: nodealert-net`

### Integration Points
- **ESP32 Task WDT ↔ FreeRTOS scheduler:** `esp_task_wdt_add()` per task handle after `xTaskCreatePinnedToCore()`
- **ESP32 status MQTT ↔ Backend subscriber:** New `nodealert/{device_id}/status` topic consumed by Django subscriber
- **Gunicorn ↔ nginx reverse proxy:** nginx proxy_pass to `http://django:8000` via Docker internal network
- **Docker HEALTHCHECK ↔ Django liveness:** `curl --fail http://localhost:8000/api/v1/health/`
- **Rate limiting ↔ LoginView:** DRF throttle classes applied globally, login gets AnonRateThrottle

</code_context>

<specifics>
## Specific Ideas

- El Last Will MQTT se configura en `esp_mqtt_client_start()` para que el broker publique automáticamente si el ESP32 se desconecta inesperadamente. Esto cubre el caso de watchdog reboot sin necesidad de lógica extra en el reset handler.
- El estado MQTT cada 60s también sirve como heartbeat: si el backend no recibe status en > 120s, puede marcar el dispositivo como offline.
- La migración a Gunicorn debe probarse primero en local con `GUNICORN_ENABLED=true` antes de integrarlo en el flujo de deploy.
- El nginx reverse proxy puede reutilizar el patrón del nginx.frontend.conf existente.
- Los endpoints de health (/health/ y /health/ready/) son públicos (AllowAny) pero solo responden 200/503 sin datos sensibles.

</specifics>

<deferred>
## Deferred Ideas

- SSL/TLS para nginx (v2) — certificados Let's Encrypt, HTTPS
- Logging remoto centralizado (v2) — rsyslog, Loki, o similar
- Notificaciones externas (email/SMS/telegram) (v2)
- Multi-nodo (v2)
- Deep sleep / optimización energética (v2)
- Pruebas de integración automatizadas (E2E) — se pospone para v2 por complejidad de infraestructura
- Tests unitarios de documentación — fuera del alcance

</deferred>

---

*Phase: 06-hardening-y-documentaci-n*
*Context gathered: 2026-05-25*
