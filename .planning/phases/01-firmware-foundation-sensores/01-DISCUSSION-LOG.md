# Phase 1: Firmware Foundation + Sensores - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-24
**Phase:** 01-firmware-foundation-sensores
**Areas discussed:** Estructura de directorios, Interfaz de sensores, Frecuencia de muestreo, Máquina de estados

---

## Estructura de Directorios

| Option | Description | Selected |
|--------|-------------|----------|
| Por capas (hal/, drivers/, etc) | Cada módulo con su propia carpeta. Clara separación de responsabilidades. | ✓ |
| Plana en src/ | Todo junto en src/ sin subcarpetas. Simple pero menos escalable. | |
| Componentes ESP-IDF | Cada módulo como componente con CMakeLists.txt. | |

**User's choice:** Por capas (hal/, drivers/, managers/, services/, config/, core/)

### Nombres

| Option | Description | Selected |
|--------|-------------|----------|
| snake_case | Estándar en C/C++ embebido | ✓ |
| CamelCase | Más orientado a objetos | |
| Híbrido | Mix según contexto | |

**User's choice:** snake_case

### Idioma

| Option | Description | Selected |
|--------|-------------|----------|
| Inglés | Consistencia global | ✓ |
| Español | Dominio claro en español | |
| Híbrido | Inglés para API, español para comentarios | |

**User's choice:** Inglés para todo el código

### Capas específicas

| Option | Description | Selected |
|--------|-------------|----------|
| Estructura completa | core/, hal/, drivers/sensor/, managers/, services/, config/ | ✓ |
| Mínima v1 | Sin managers/services en v1 | |

**User's choice:** Estructura completa desde v1

---

## Interfaz de Sensores

| Option | Description | Selected |
|--------|-------------|----------|
| Interfaz común ISensor | Clase base con read(), calibrate(), getStatus() | |
| API específica por sensor | Funciones específicas por sensor | |
| Híbrida | Interfaz común + template getter para acceso específico | ✓ |

**User's choice:** Híbrida

### Método read()

| Option | Description | Selected |
|--------|-------------|----------|
| Struct SensorReading | Tipo, valor, timestamp, error_code | ✓ |
| Tipo por sensor | Cada sensor con su propio struct | |
| Output parameter | Callback por referencia | |

**User's choice:** Struct SensorReading

### Acceso a funcionalidades específicas

| Option | Description | Selected |
|--------|-------------|----------|
| Downcasting | dynamic_cast desde interfaz común | |
| Template getter específico | getSensor<T>() tipo concreto en compilación | ✓ |
| Command pattern | Interfaz común + sendCommand() | |

**User's choice:** Template getter específico

---

## Frecuencia de Muestreo

| Option | Description | Selected |
|--------|-------------|----------|
| 2s / 1s / 200ms | DHT22 2s, MQ-9 1s, KY-026 200ms | ✓ |
| 5s / 2s / 500ms | Más lento, menor consumo | |
| Personalizado | Definir según caso de uso | |

**User's choice:** 2s / 1s / 200ms (DHT22 / MQ-9 / KY-026)

### Prioridades RTOS

| Option | Description | Selected |
|--------|-------------|----------|
| KY-026 > MQ-9 > DHT22 > MQTT | Jerarquía clara de prioridades | ✓ |
| Prioridad única | Todas las lecturas misma prioridad | |
| Interrupción HW + polling | KY-026 por interrupción, resto polling | |

**User's choice:** KY-026 (crítica) > MQ-9 (alta) > DHT22 (normal) > MQTT (baja)

### KY-026: Interrupción vs Polling

| Option | Description | Selected |
|--------|-------------|----------|
| Interrupción HW | Flanco ascendente, detección inmediata | ✓ |
| Polling | Simple, suficiente para detección | |
| Híbrido | Polling + interrupción para alerta | |

**User's choice:** Interrupción HW

---

## Máquina de Estados

| Option | Description | Selected |
|--------|-------------|----------|
| init, standby, running, alert, error, recovery | Completa con sub-estados | ✓ |
| init, running, error | Simple | |
| init, running, alert, error | Sin recovery automático | |

**User's choice:** init → standby → running → alert → error → recovery (con sub-estados de calibración)

### Transición a alerta

| Option | Description | Selected |
|--------|-------------|----------|
| Automática con histéresis | Alerta al cruzar umbral, vuelve al bajar | ✓ |
| Alerta manual reset | Requiere comando MQTT para reset | |
| Mixta según sensor | Gas/llama automática, temperatura semi | |

**User's choice:** Automática con histéresis

### Error recovery

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-recovery con backoff | Reintentar N veces, backoff exponencial | ✓ |
| Bloqueo con watchdog | Error permanente hasta reset | |
| Mixto | Según tipo de error | |

**User's choice:** Auto-recovery con backoff exponencial

---

## the agent's Discretion

- Implementación concreta de la HAL
- Timing de backoff exponencial
- Valores numéricos de histéresis
- Nombres exactos de clases/archivos

## Deferred Ideas

None
