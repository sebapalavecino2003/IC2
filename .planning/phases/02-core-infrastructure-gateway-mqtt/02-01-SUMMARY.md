---
phase: 02-core-infrastructure-gateway-mqtt
plan: 01
subsystem: infra
tags: mqtt, mosquitto, docker-compose, gateway, broker
requires: []
provides:
  - Broker MQTT Mosquitto funcional con persistencia
  - Configuración Docker Compose base + override para desarrollo
  - Template de variables de entorno compartidas
  - Red Docker bridge para comunicación entre servicios
affects:
  - 02-core-infrastructure-gateway-mqtt (Plan 02)
  - 03-comunicacion-mqtt-esp32-backend
tech-stack:
  added:
    - eclipse-mosquitto:2 (Docker image)
    - Docker Compose v3.8
  patterns:
    - Base + override docker-compose.yml pattern (D-01)
    - Named volumes para datos y logs
    - Red bridge compartida nodealert-net
    - Archivo de configuración montado read-only
key-files:
  created:
    - docker-compose.yml
    - docker-compose.override.yml
    - mosquitto/config/mosquitto.conf
    - mosquitto/config/mosquitto_passwd
    - .env.example
    - .gitignore
  modified: []
key-decisions:
  - "Named volumes de Docker (mosquitto_data, mosquitto_log) para persistencia y logs"
  - "Mosquitto config montado read-only (:ro) dentro del contenedor para seguridad"
  - "Puerto 9001 (WebSockets) no expuesto en v1 — solo 1883 para MQTT TCP"
  - "override pattern separa config base de overrides de desarrollo"
patterns-established:
  - "Docker Compose: base file define servicios, override file define diferencias para desarrollo"
  - "Mosquitto: autenticación por archivo password_file, persistencia con named volumes"
  - "Segregación de config: mosquitto.conf es el único punto de entrada para ajustes del broker"
  - "Variables compartidas: .env.example como template único para todo el stack (MQTT + MySQL + Django)"
requirements-completed: [GATE-01, GATE-02, GATE-03, INFRA-01, INFRA-03]
duration: ~3 min
completed: 2026-05-24
---

# Phase 2 Plan 01: Gateway MQTT — Broker Mosquitto Summary

**Broker MQTT Mosquitto funcional con persistencia, autenticación básica, red Docker bridge y configuración de desarrollo Docker Compose**

## Performance

- **Duration:** ~3 min
- **Started:** 2026-05-24T22:43:37-03:00
- **Completed:** 2026-05-24T22:46:03-03:00
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments

- **docker-compose.yml** con servicio Mosquitto (eclipse-mosquitto:2), red `nodealert-net` bridge, volúmenes nombrados `mosquitto_data` y `mosquitto_log`, puerto 1883 mapeado al host
- **mosquitto.conf** con autenticación (`allow_anonymous false`, `password_file`), persistencia de sesiones y mensajes retain, logging completo
- **docker-compose.override.yml** para desarrollo con restart policy `"no"` (evita reinicios automáticos durante desarrollo)
- **.env.example** con 11 variables de entorno para MQTT Broker, MySQL y Django (template unificado para todo el stack)
- **.gitignore** raíz con `.env`, `mosquitto_passwd`, `__pycache__` y `.DS_Store`
- Verificación completa del broker: contenedor inicia, puerto 1883 escucha, conexiones anónimas rechazadas, resolución por nombre de contenedor funciona

## Task Commits

Each task was committed atomically:

1. **Task 1: Create docker-compose.yml** — `d6ccf72` (feat)
2. **Task 2: Create Mosquitto config, .env template, and .gitignore** — `9af3fe2` (feat)
3. **Task 3: Create docker-compose.override.yml** — `ee90ce1` (feat)

**Plan metadata:** pending (committed with SUMMARY)

## Files Created/Modified

- `docker-compose.yml` — Base Docker Compose con servicio Mosquitto, red nodealert-net, volúmenes mosquitto_data y mosquitto_log
- `mosquitto/config/mosquitto.conf` — Configuración del broker (auth, persistencia, retain, logging)
- `mosquitto/config/mosquitto_passwd` — Archivo de contraseñas vacío (placeholder, setup.sh lo completa)
- `.env.example` — Template de variables de entorno compartidas (MQTT, MySQL, Django)
- `.gitignore` — Ignorar .env, mosquitto_passwd, __pycache__, .DS_Store
- `docker-compose.override.yml` — Override de desarrollo (restart: "no")

## Decisions Made

- **Named volumes de Docker** en lugar de bind mounts para datos y logs de Mosquitto — mejor aislamiento y portabilidad
- **Mosquitto config montado read-only** (`./mosquitto/config:/mosquitto/config:ro`) — Mitigación T-02-03: solo el host puede modificar la configuración
- **Puerto 9001 no expuesto** — WebSockets no necesarios en v1 (mitigación de superficie de ataque)
- **Base + override pattern (D-01)** — docker-compose.yml define la base, docker-compose.override.yml solo las diferencias para desarrollo
- **13 directivas en mosquitto.conf** cubriendo listener, auth, persistencia, retención, límites de mensajes y logging

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

- `mosquitto_passwd` está en `.gitignore` (porque contiene credenciales) pero es necesario trackear el archivo vacío como placeholder. Se usó `git add -f` para incluirlo en el commit inicial.
- Docker Compose emite warnings sobre el atributo `version` obsoleto (nuevas versiones lo ignoran). No afecta funcionalidad.

## User Setup Required

None — no external service configuration required. Credenciales se generarán en Plan 03 (setup.sh).

## Verification Results

All plan-level verification checks passed:

| Check | Result |
|-------|--------|
| `docker compose config --quiet` | ✅ Exit 0 |
| `docker compose up -d mosquitto` → container status | ✅ Up/running |
| `mosquitto_sub -u nonexistent -P wrong` → auth error | ✅ "not authorised" |
| `mosquitto_sub -h localhost` (anonymous) → auth error | ✅ "not authorised" |
| Container name resolution (`mosquitto` hostname) | ✅ DNS resuelve (auth error vs connection refused) |
| Persistence volume writable | ✅ Mosquitto escribe logs en /mosquitto/log/ |

## Threat Surface Scan

No new threat surface beyond what's defined in the plan's threat model:
- T-02-01 (Spoofing): Mitigated via `allow_anonymous false` + `password_file`
- T-02-02 (Info disclosure): Accepted — TLS deferred to hardening phase
- T-02-03 (Tampering): Mitigated via read-only config mount
- T-02-04 (Tampering volumes): Accepted — single-user deployment
- T-02-SC (Image tampering): Deferred to SHA-pinned digest in hardening

## Next Phase Readiness

- Infraestructura MQTT lista: broker Mosquitto funcional, persistente y autenticado
- Red Docker bridge `nodealert-net` creada para servicios futuros
- Template `.env.example` listo para MySQL, Django y superuser
- Próximo plan (02-02): Agregar servicios MySQL y Django al docker-compose

---

*Phase: 02-core-infrastructure-gateway-mqtt*
*Completed: 2026-05-24*
