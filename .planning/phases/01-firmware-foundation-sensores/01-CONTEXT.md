# Phase 1: Firmware Foundation + Sensores - Context

**Gathered:** 2026-05-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Establecer la base del firmware ESP32 con arquitectura modular profesional: estructura de proyecto PlatformIO por capas, HAL de abstracción de hardware, drivers para los 3 sensores (DHT22, MQ-9, KY-026), tareas FreeRTOS concurrentes, máquina de estados y manejo de errores. No incluye comunicación MQTT (fase 3) ni automatización (fase 5).

</domain>

<decisions>
## Implementation Decisions

### Estructura de Directorios
- **D-01:** Organización por capas: `core/` (state machine, system), `hal/` (abstracción HW), `drivers/sensor/` (drivers específicos), `managers/` (coordinación), `services/` (utilidades), `config/` (configuración centralizada)
- **D-02:** Convención snake_case para archivos y directorios
- **D-03:** Código en inglés (nombres de archivos, funciones, variables, comentarios)

### Interfaz de Sensores
- **D-04:** Arquitectura híbrida: interfaz común con métodos `read()`, `calibrate()`, `getStatus()` + template getter `getSensor<T>()` para acceso a funcionalidades específicas de cada sensor
- **D-05:** `read()` devuelve un struct `SensorReading` con: tipo de sensor, valor, timestamp, código de error

### Frecuencia de Muestreo y Prioridades
- **D-06:** DHT22 cada 2s, MQ-9 cada 1s, KY-026 cada 200ms
- **D-07:** Prioridades FreeRTOS: KY-026 (crítica) > MQ-9 (alta) > DHT22 (normal) > MQTT (baja)
- **D-08:** KY-026 manejado por interrupción de hardware (flanco ascendente) para detección inmediata

### Máquina de Estados
- **D-09:** Estados del sistema: `init` → `standby` → `running` → `alert` → `error` → `recovery` (con sub-estados para calibración)
- **D-10:** Transición a alerta automática al cruzar umbrales, con histéresis para evitar falsos positivos
- **D-11:** Auto-recovery con backoff exponencial: reintentar N veces, si persiste bloqueo, si se recupera volver a running

### the agent's Discretion
- Implementación concreta de la HAL, timing de backoff, valores de histéresis, nombres exactos de clases/archivos

</decisions>

<canonical_refs>
## Canonical References

Downstream agents MUST read these before planning or implementing.

### Project Context
- `.planning/PROJECT.md` — Project context, core value, constraints, key decisions
- `.planning/REQUIREMENTS.md` — Full requirements with traceability (FWK-01–05, DRV-01–04, RTOS-01–05)
- `.planning/ROADMAP.md` — Phase overview, goal, success criteria
- `.planning/STATE.md` — Current project state

### No external specs
No external specs — requirements fully captured in decisions above

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- No existing code — greenfield project

### Established Patterns
- No established patterns — first phase of development

### Integration Points
- N/A — no existing system to integrate with

</code_context>

<specifics>
## Specific Ideas

- Arquitectura monolítica modular con separación clara .h/.cpp (no Arduino monolítico en un único .ino)
- Reutilización de drivers vía HAL
- Organización profesional de proyecto PlatformIO
- El KY-026 debe poder despertar al sistema ante detección de llama vía interrupción HW

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-firmware-foundation-sensores*
*Context gathered: 2026-05-24*
