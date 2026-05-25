# Roadmap: NodeAlert IoT

**Created:** 2026-05-24
**Phases:** 6 | **Requirements mapped:** 42/42 ✓

---

## Phase 1: Firmware Foundation + Sensores

**Goal:** Establecer la base del firmware ESP32 con arquitectura modular profesional y drivers de los 3 sensores.

**Mode:** mvp

**Progress:** 3/4 plans complete (75%) — 2026-05-25

| Plan | Status | Date |
|------|--------|------|
| 01-A-SCAFFOLD | ✅ Done | 2026-05-24 |
| 01-B-DHT22 | ✅ Done | 2026-05-24 |
| 01-C-MQ9-KY026 | ✅ Done | 2026-05-25 |
| 01-D-RTOS-INTEGRATION | 🔲 Pending | — |

**Requirements:**

- FWK-01, FWK-02, FWK-03, FWK-04, FWK-05
- DRV-01, DRV-02, DRV-03, DRV-04
- RTOS-01, RTOS-02, RTOS-03, RTOS-04, RTOS-05

**Success Criteria:**

1. Proyecto PlatformIO compila con estructura de módulos (hal/, drivers/, managers/, services/, config/)
2. Los 3 sensores (DHT22, MQ-9, KY-026) se leen correctamente con sus drivers HAL
3. Tareas FreeRTOS ejecutan concurrentemente: lectura de sensores, monitoreo de estado, manejo de errores
4. Colas y mutex protegen el acceso a recursos compartidos
5. La máquina de estados transiciona correctamente (init → running → error)
6. Las lecturas se imprimen por serial con formato estructurado

---

## Phase 2: Core Infrastructure + Gateway MQTT

**Goal:** Desplegar la infraestructura base del servidor con Docker Compose, broker MQTT, base de datos y API Django.

**Mode:** mvp

**Progress:** 3 plans created — 2026-05-24

**Requirements:**

- GATE-01, GATE-02, GATE-03
- API-01, API-04, API-05, API-06
- INFRA-01, INFRA-02, INFRA-03

**Success Criteria:**

1. Docker Compose levanta Mosquitto, MySQL y Django correctamente en Raspberry Pi
2. Broker MQTT acepta conexiones y mensajes con autenticación básica
3. API REST Django permite CRUD de dispositivos y consulta de lecturas
4. MySQL almacena lecturas históricas y eventos correctamente
5. Script de despliegue automatiza la configuración inicial
6. Backend accesible desde la red local

| Plan | Status | Date |
|------|--------|------|
| 02-01 | 📋 Planned | 2026-05-24 |
| 02-02 | 📋 Planned | 2026-05-24 |
| 02-03 | 📋 Planned | 2026-05-24 |

---

## Phase 3: Comunicación MQTT ESP32 → Backend

**Goal:** Integrar la comunicación MQTT entre el ESP32 y el backend, estableciendo el flujo de datos completo.

**Mode:** mvp

**Progress:** 2 plans completed — 2026-05-25
**Plans:**

- [x] 03-01 — ESP32 MQTT Publisher (firmware): MessageBuffer + MQTT manager + main.cpp integration
- [x] 03-02 — Django MQTT Subscriber + Deployment: management command + entrypoint + setup.sh credentials

**Requirements:**

- MQTT-01, MQTT-02, MQTT-03, MQTT-04, MQTT-05
- API-03

**Success Criteria:**

1. ESP32 se conecta al broker MQTT y publica lecturas periódicamente
2. Backend se suscribe a tópicos y persiste las lecturas entrantes
3. Reconexión automática funciona ante caída del broker
4. Buffer local almacena mensajes durante pérdida de conectividad y los reenvía al reconectar
5. Keep Alive mantiene la conexión estable sin consumo excesivo
6. Flujo extremo a extremo: sensor → ESP32 → MQTT → Django → MySQL

---

## Phase 4: Dashboard en Tiempo Real

**Goal:** Construir el dashboard web React con visualización en tiempo real, autenticación y acceso local/remoto.

**Mode:** mvp

**Progress:** 3 plans created — 2026-05-25

| Plan | Status | Date |
|------|--------|------|
| 04-01 | 📋 Planned | 2026-05-25 |
| 04-02 | 📋 Planned | 2026-05-25 |
| 04-03 | 📋 Planned | 2026-05-25 |

**Requirements:**

- API-02
- UI-01, UI-02, UI-03, UI-04, UI-05, UI-06

**Success Criteria:**

1. Frontend React + Vite + MUI compila y se despliega como parte de Docker Compose
2. Dashboard muestra temperatura, humedad, gas y llama con indicadores visuales en tiempo real (polling cada 3s)
3. Gráficos históricos (Recharts) muestran tendencias con filtros de tiempo (1h/6h/24h/7d)
4. Panel de alertas lista eventos críticos con timestamp y severidad
5. Autenticación DRF Token protege el acceso (local y remoto)
6. Estado de conexión del nodo ESP32 visible en el dashboard (online/offline por recencia)

---

## Phase 5: Automatización Local y Alertas

**Goal:** Implementar la lógica de automatización híbrida en el ESP32 y el sistema de alertas en el dashboard.

**Mode:** mvp

**Requirements:**

- AUTO-01, AUTO-02, AUTO-03, AUTO-04

**Success Criteria:**

1. ESP32 evalúa condiciones críticas (umbrales de temperatura, gas, llama) de forma autónoma
2. Histéresis evita falsos positivos en la detección de eventos
3. Actuadores de ventilación se activan localmente al superar umbrales
4. Comandos MQTT de override desde el servidor son recibidos y ejecutados
5. Eventos y alertas se registran en el backend con timestamp y tipo
6. Dashboard muestra alertas activas e históricas en tiempo real

---

## Phase 6: Hardening y Documentación

**Goal:** Robustecer el sistema completo con tolerancia a fallos, scripts de deploy, monitoreo y documentación.

**Mode:** mvp

**Requirements:**

- API-06 (complete auth hardening)
- INFRA-02 (deploy scripts)
- All previous phases hardened

**Success Criteria:**

1. Watchdog del ESP32 recupera el sistema ante fallos
2. Gestión de errores cubre todos los módulos (timeouts, sensores muertos, pérdida WiFi)
3. Scripts de deploy automatizados y probados
4. Documentación de arquitectura, configuración y despliegue completa
5. Monitoreo básico del sistema (health checks, logs)
6. Prueba de integración extremo a extremo superada

---

## Coverage Summary

| Phase | Requirements | Success Criteria | Plans |
|-------|-------------|------------------|-------|
| 1 | 14 | 6 | 4 |
| 2 | 10 | 6 | 3 |
| 3 | 6 | 6 | 2 |
| 4 | 1/3 | In Progress|  |
| 5 | 4 | 6 | — |
| 6 | 1 | 6 | — |

**Total v1 requirements:** 42
**Mapped:** 42 ✓
**Coverage:** 100%
