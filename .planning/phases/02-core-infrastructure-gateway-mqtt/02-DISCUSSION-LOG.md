# Phase 2: Core Infrastructure + Gateway MQTT - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-24
**Phase:** 02-core-infrastructure-gateway-mqtt
**Areas discussed:** Docker Compose layout, MySQL schema, Django project structure, API design style, MQTT auth model, Deployment approach

---

## Docker Compose Layout

| Option | Description | Selected |
|--------|-------------|----------|
| Single file | One docker-compose.yml for MVP — simpler to understand, deploy, and debug | |
| Multiple files | Base + override pattern for dev vs prod differences | ✓ |

| Option | Description | Selected |
|--------|-------------|----------|
| Same network | All services on same Docker bridge network. Backend connects via container name | ✓ |
| Host network | Mosquitto binds to host network. Backend connects via localhost | |

| Option | Description | Selected |
|--------|-------------|----------|
| Named volumes | Docker named volumes for MySQL and Mosquitto data | ✓ |
| Bind mounts | Bind to host directories for easier inspection | |

| Option | Description | Selected |
|--------|-------------|----------|
| Expose Django + Mosquitto | Map Django (8000) and Mosquitto (1883) to host. MySQL stays internal | ✓ |
| Expose all three | Also expose MySQL (3306) for direct database access | |

**User's choice:** Multiple files, same network, named volumes, expose Django + Mosquitto only

---

## MySQL Schema

| Option | Description | Selected |
|--------|-------------|----------|
| devices + readings + events + users | 4 tables minimum set | ✓ |
| Include alerts as separate table | 5 tables: split events into events and alerts | |
| Include actuator_log | 6 tables: add actuator command log | |

| Option | Description | Selected |
|--------|-------------|----------|
| Single readings table | One table with sensor_type ENUM, value FLOAT. DHT22 = 2 rows per sample | ✓ |
| Per-sensor tables | Separate tables for each sensor type | |

| Option | Description | Selected |
|--------|-------------|----------|
| Unified events with severity | Single events table with severity ENUM + resolved BOOL | ✓ |
| Two tables: events + alerts | Separate alerts with acknowledged_at and resolved_at | |

**User's choice:** 4 tables, single readings table, unified events with severity

---

## Django Project Structure

| Option | Description | Selected |
|--------|-------------|----------|
| Single app: core | One Django app handling models, API views, MQTT integration | ✓ |
| Multiple apps | Split into api, devices, readings, events apps | |

| Option | Description | Selected |
|--------|-------------|----------|
| nodealert as project name | Django project: nodealert. App: api or core | ✓ |
| backend as project name | Django project: backend. App: api | |

| Option | Description | Selected |
|--------|-------------|----------|
| NodeAlert-Backend/ directory | Parallel to firmware, Docker Compose at repo root | ✓ |
| Single directory with subdirs | All services in one structure | |

**User's choice:** Single app "core", project name "nodealert", directory NodeAlert-Backend/

---

## API Design Style

| Option | Description | Selected |
|--------|-------------|----------|
| Django REST Framework | Viewsets, serializers, token auth, pagination | ✓ |
| Plain Django views | Lighter but more manual work | |

| Option | Description | Selected |
|--------|-------------|----------|
| No versioning | /api/devices, /api/readings | |
| Versioned: /api/v1/ | /api/v1/devices, /api/v1/readings | ✓ |

| Option | Description | Selected |
|--------|-------------|----------|
| Read-only readings + full CRUD devices/events/users | MVP endpoint set | ✓ |
| Full CRUD on everything | All mutations including readings POST | |

**User's choice:** DRF with /api/v1/ prefix, read-only readings + CRUD devices/events

---

## MQTT Auth Model

| Option | Description | Selected |
|--------|-------------|----------|
| Shared credentials | Single username/password for all devices | ✓ |
| Per-device credentials | Each device gets unique credentials | |

| Option | Description | Selected |
|--------|-------------|----------|
| Environment variables in .env | Shared credentials in gitignored .env file | ✓ |
| Django admin API | Django manages and pushes credentials | |

| Option | Description | Selected |
|--------|-------------|----------|
| Mosquitto password file | mosquitto_passwd mounted into container | ✓ |
| Django as auth backend | Django serves as Mosquitto auth_plugin via HTTP | |

**User's choice:** Shared credentials, .env file, mosquitto_passwd mounted into container

---

## Deployment Approach

| Option | Description | Selected |
|--------|-------------|----------|
| setup.sh script | Single bash script for full deployment | ✓ |
| Manual Docker Compose | User manually runs docker-compose + migrations | |
| Makefile | Makefile with targets for setup/deploy/migrate | |

| Option | Description | Selected |
|--------|-------------|----------|
| arm64 + amd64 | Script detects architecture and uses appropriate images | ✓ |
| Raspberry Pi only | Script hardcoded for arm64 | |

| Option | Description | Selected |
|--------|-------------|----------|
| Interactive setup generates firmware config | setup.sh prompts for WiFi/MQTT and writes config | ✓ |
| Firmware config is manual | User manually edits config headers | |

**User's choice:** setup.sh with dual architecture support, generates firmware config

---

## Agent's Discretion

- Estructura interna de archivos dentro de la app `core` de Django
- Configuraciones exactas de Docker Compose (versión, health checks, restart policies)
- Implementación concreta de viewsets y serializers DRF
- Valores numéricos: tamaños de paginación, timeouts, tamaños de buffer
- Configuración específica de Mosquitto (mosquitto.conf, listeners, log settings)

## Deferred Ideas

None — discussion stayed within phase scope
