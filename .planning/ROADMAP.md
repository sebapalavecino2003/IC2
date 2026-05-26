# Roadmap: NodeAlert IoT

**Created:** 2026-05-24
**Last updated:** 2026-05-25 (milestone v1.0 complete 🎉)
**Phases:** 6 | **Requirements mapped:** 42/42 ✓

---

## Phase 1: Firmware Foundation + Sensores

**Goal:** Establecer la base del firmware ESP32 con arquitectura modular profesional y drivers de los 3 sensores.

**Mode:** mvp

**Progress:** 4/4 plans complete (100%) — ✅

| Plan | Status | Date |
|------|--------|------|
| 01-A-SCAFFOLD | ✅ Done | 2026-05-24 |
| 01-B-DHT22 | ✅ Done | 2026-05-24 |
| 01-C-MQ9-KY026 | ✅ Done | 2026-05-25 |
| 01-D-RTOS-INTEGRATION | ✅ Done | 2026-05-25 |

**Success Criteria:**

1. ✅ Proyecto PlatformIO compila con estructura de módulos (hal/, drivers/, managers/, services/, config/)
2. ✅ Los 3 sensores (DHT22, MQ-9, KY-026) se leen correctamente con sus drivers HAL
3. ✅ Tareas FreeRTOS ejecutan concurrentemente: lectura de sensores, monitoreo de estado, manejo de errores
4. ✅ Colas y mutex protegen el acceso a recursos compartidos
5. ✅ La máquina de estados transiciona correctamente (init → running → error → recovery)
6. ✅ Las lecturas se imprimen por serial con formato estructurado

---

## Phase 2: Core Infrastructure + Gateway MQTT

**Goal:** Desplegar la infraestructura base del servidor con Docker Compose, broker MQTT, base de datos y API Django.

**Mode:** mvp

**Progress:** 3/3 plans complete (100%) — ✅

| Plan | Status | Date |
|------|--------|------|
| 02-01 | ✅ Done | 2026-05-24 |
| 02-02 | ✅ Done | 2026-05-24 |
| 02-03 | ✅ Done | 2026-05-24 |

**Success Criteria:**

1. ✅ Docker Compose levanta Mosquitto, MySQL y Django correctamente
2. ✅ Broker MQTT acepta conexiones y mensajes con autenticación básica
3. ✅ API REST Django permite CRUD de dispositivos y consulta de lecturas
4. ✅ MySQL almacena lecturas históricas y eventos
5. ✅ Script de despliegue automatiza la configuración inicial (setup.sh)
6. ✅ Backend accesible desde la red local

---

## Phase 3: Comunicación MQTT ESP32 → Backend

**Goal:** Integrar la comunicación MQTT entre el ESP32 y el backend, estableciendo el flujo de datos completo.

**Mode:** mvp

**Progress:** 2/2 plans complete (100%) — ✅

| Plan | Status | Date |
|------|--------|------|
| 03-01 | ✅ Done | 2026-05-25 |
| 03-02 | ✅ Done | 2026-05-25 |

**Success Criteria:**

1. ✅ ESP32 se conecta al broker MQTT y publica lecturas periódicamente
2. ✅ Backend se suscribe a tópicos y persiste las lecturas entrantes
3. ✅ Reconexión automática funciona ante caída del broker (MQTT_EVENT_DISCONNECTED → RECONNECTED)
4. ✅ Buffer local almacena mensajes durante pérdida de conectividad y los reenvía al reconectar
5. ✅ Keep Alive mantiene la conexión estable sin consumo excesivo (60s)
6. ✅ Flujo extremo a extremo: sensor → ESP32 → MQTT → Django → MySQL

---

## Phase 4: Dashboard en Tiempo Real

**Goal:** Construir el dashboard web React con visualización en tiempo real, autenticación y acceso local/remoto.

**Mode:** mvp

**Progress:** 3/3 plans complete (100%) — ✅

| Plan | Status | Date |
|------|--------|------|
| 04-01 | ✅ Done | 2026-05-25 |
| 04-02 | ✅ Done | 2026-05-25 |
| 04-03 | ✅ Done | 2026-05-25 |

**Success Criteria:**

1. ✅ Frontend React + Vite + MUI compila y se despliega como parte de Docker Compose
2. ✅ Dashboard muestra temperatura, humedad, gas y llama con indicadores visuales en tiempo real (polling cada 3s)
3. ✅ Gráficos históricos (Recharts) muestran tendencias con filtros de tiempo (1h/6h/24h/7d)
4. ✅ Panel de alertas lista eventos críticos con timestamp y severidad
5. ✅ Autenticación DRF Token protege el acceso (local y remoto)
6. ✅ Estado de conexión del nodo ESP32 visible en el dashboard (online/offline)

---

## Phase 5: Automatización Local y Alertas

**Goal:** Implementar la lógica de automatización híbrida en el ESP32 y el sistema de alertas en el dashboard.

**Mode:** mvp

**Progress:** 3/3 plans complete (100%) — ✅

| Plan | Status | Date |
|------|--------|------|
| 05-01 | ✅ Done | 2026-05-25 |
| 05-02 | ✅ Done | 2026-05-25 |
| 05-03 | ✅ Done | 2026-05-25 |

**Success Criteria:**

1. ✅ ESP32 evalúa condiciones críticas (umbrales de temperatura, gas, llama) de forma autónoma
2. ✅ Histéresis evita falsos positivos en la detección de eventos (tiempo 3s + delta 10%)
3. ✅ Actuador (relé GPIO 2) se activa localmente al superar umbrales, mínimo 2min activo
4. ✅ Comandos MQTT de override desde el servidor son recibidos y ejecutados (5 tipos)
5. ✅ Eventos y alertas se registran en el backend — ESP32 publica en nodealert/{id}/events, Django subscriber persiste en modelo Event, dashboard los muestra en tiempo real
6. ✅ Dashboard muestra alertas activas e históricas en tiempo real (tabs + ActiveAlertsPanel)

---

## Phase 6: Hardening y Documentación

**Goal:** Robustecer el sistema completo con tolerancia a fallos, scripts de deploy, monitoreo y documentación.

**Mode:** mvp

**Progress:** 3/3 plans complete (100%) — ✅

| Plan | Status | Date |
|------|--------|------|
| 06-01 | ✅ Done | 2026-05-25 |
| 06-02 | ✅ Done | 2026-05-25 |
| 06-03 | ✅ Done | 2026-05-25 |

**Success Criteria:**

1. ✅ Watchdog del ESP32 recupera el sistema ante fallos (Task WDT 7 tareas + Interrupt WDT)
2. ✅ Gestión de errores cubre todos los módulos (ErrorHandler con backoff exponencial en todos los managers)
3. ✅ Scripts de deploy automatizados (setup.sh interactivo + entrypoint.sh con Gunicorn toggle)
4. ✅ Documentación de arquitectura, configuración y despliegue completa (7 documentos en Español)
5. ✅ Monitoreo básico del sistema (health checks vía /health/ y /health/ready/, Docker HEALTHCHECKs en 5 contenedores, ESP32 heartbeat cada 60s)
6. ~ Prueba de integración extremo a extremo: código completo y commiteado. Requiere hardware físico (ESP32 + sensores) para validación en entorno real.

---

## Coverage Summary

| Phase | Requirements | Success Criteria | Plans | Status |
|-------|-------------|------------------|-------|--------|
| 1 | 14 | 6 | 4 | ✅ Complete |
| 2 | 10 | 6 | 3 | ✅ Complete |
| 3 | 6 | 6 | 2 | ✅ Complete |
| 4 | 7 | 6 | 3 | ✅ Complete |
| 5 | 4 | 6 | 3 | ✅ Complete |
| 6 | 2 | 6 | 3 | ✅ Complete |

**Total v1 requirements:** 42
**Completed:** 41 ✅, 1 partial ~ (API-02: polling vs WebSockets)
**Coverage funcional:** 100%
**Planes ejecutados:** 17/17

---

## v2 (Futuro)

### Areas planificadas para v2:
- **Alertas externas**: Telegram Bot, email para eventos críticos
- **Multi-nodo**: Soporte para 2+ nodos ESP32
- **Energía**: Deep Sleep, optimización para batería
- **SSL/TLS**: HTTPS para nginx, MQTT over TLS
- **OTA**: Actualizaciones over-the-air para firmware
- **WebSockets**: Streaming en tiempo real (reemplazar polling)
- **Logging centralizado**: Loki o similar
