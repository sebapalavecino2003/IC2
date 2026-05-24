# NodeAlert IoT

## What This Is

NodeAlert IoT es un sistema distribuido de monitoreo ambiental crítico para detección temprana de incendios, fugas de gases y automatización preventiva. Combina nodos ESP32 con sensores ambientales (DHT22, MQ-9, KY-026) que procesan datos localmente mediante FreeRTOS, un gateway central con comunicación MQTT, un backend Django en Raspberry Pi con Docker Compose, y un dashboard web en tiempo real con React + Vite + Material UI. Orientado a entornos industriales, residenciales e infraestructura crítica.

## Core Value

Detectar condiciones ambientales peligrosas (incendio, gas, temperatura extrema) y actuar preventivamente antes de que escalen a emergencias, incluso sin conexión al servidor central.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] **NODO-01**: Firmware ESP32 con arquitectura modular profesional (HAL, drivers, managers, services) en PlatformIO
- [ ] **NODO-02**: Lectura de sensores DHT22 (temp/humedad), MQ-9 (gases), KY-026 (llama) con FreeRTOS
- [ ] **NODO-03**: Procesamiento concurrente con tareas RTOS priorizadas, queues y mutex
- [ ] **NODO-04**: Lógica autónoma local con histéresis para evitar falsos positivos
- [ ] **NODO-05**: Automatización híbrida de ventilación (local por defecto, override central por MQTT)
- [ ] **NODO-06**: Comunicación MQTT publish/subscribe con el gateway
- [ ] **NODO-07**: Tolerancia a fallos y operación autónoma ante pérdida de conectividad
- [ ] **GATEWAY-01**: Broker MQTT (Mosquitto) funcionando como gateway central
- [ ] **BACKEND-01**: API REST Django con WebSockets para datos en tiempo real
- [ ] **BACKEND-02**: Almacenamiento histórico y persistencia de eventos en MySQL
- [ ] **BACKEND-03**: Docker Compose para despliegue en Raspberry Pi
- [ ] **FRONTEND-01**: Dashboard web React + Vite + Material UI con datos en tiempo real
- [ ] **FRONTEND-02**: Acceso local y remoto con autenticación
- [ ] **ALERTAS-01**: Sistema de eventos y alertas críticas en el dashboard

### Out of Scope

- **Alertas Telegram** — Postergado a v2. Las alertas se manejan en el dashboard en v1.
- **Deep Sleep / batería** — Alimentación USB/transformador en v1. La optimización energética avanzada se añade en v2.
- **ESP-NOW / LoRa** — Solo WiFi en v1. Protocolos alternativos de respaldo se evalúan para v2.
- **Despliegue cloud / VPS** — Todo local en Raspberry Pi con Docker Compose en v1.

## Context

- Proyecto greenfield, sin código existente.
- Entorno de desarrollo: Visual Studio Code + PlatformIO para firmware ESP32.
- El firmware sigue arquitectura monolítica modular: separación clara `.h`/`.cpp`, capas HAL, drivers reutilizables, managers y servicios, configuración centralizada, máquina de estados.
- Los nodos ESP32 procesan localmente con FreeRTOS y pueden operar sin conexión al servidor.
- Raspberry Pi ejecuta Django + Mosquitto + MySQL + frontend React via Docker Compose.
- Dashboard accesible tanto en red local como remotamente con autenticación.

## Constraints

- **Tech Stack**: ESP32 + FreeRTOS + PlatformIO para firmware; Django + MySQL para backend; React + Vite + MUI para frontend; Docker Compose en Raspberry Pi.
- **Comunicación**: MQTT como protocolo único entre nodos y servidor en v1.
- **Automatización**: Híbrida — los nodos actúan localmente por defecto; el servidor puede enviar comandos de override.
- **Alimentación**: USB/transformador. No hay restricciones de consumo energético en v1.

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Arquitectura monolítica modular en firmware | Escalabilidad y mantenibilidad sin overengineering de microservicios en embebido | — Pending |
| 1 nodo + gateway en v1 | Enfoque MVP para validar stack completo antes de escalar | — Pending |
| Raspberry Pi local con Docker Compose | Simplicidad operativa, todo en un solo dispositivo | — Pending |
| Automatización híbrida | Máxima resiliencia: los nodos no dependen del servidor para actuar | — Pending |
| Dashboard primero, Telegram después | Enfoque incremental en alertas | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-05-24 after initialization*
