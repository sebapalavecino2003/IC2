# Phase 5: Automatización Local y Alertas - Context

**Gathered:** 2026-05-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Implementar la lógica de automatización híbrida en el ESP32 y el sistema de alertas en el dashboard. El ESP32 evalúa condiciones críticas (temperatura, gas, llama) de forma autónoma y activa actuadores de ventilación localmente. El servidor puede enviar comandos MQTT de override. Eventos y alertas se registran en el backend y se muestran en el dashboard en tiempo real. No incluye notificaciones externas (v2), multi-nodo (v2), ni deep sleep / optimización energética (v2).

</domain>

<decisions>
## Implementation Decisions

### Lógica de Automatización (FreeRTOS)
- **D-01:** Nuevo AutomationManager como tarea FreeRTOS dedicada, separada de sensor_manager y monitor_task. Prioridad 2 (intermedia), ejecución cada 3s.
- **D-02:** La tarea lee los valores más recientes de cada sensor vía sensor_queue (existente), evalúa umbrales, controla el actuador y verifica comandos MQTT de override.
- **D-03:** El actuador de ventilación se controla mediante PIN_RELAY_ACTUATOR (GPIO_NUM_2, ya reservado en pins_config.h). Relé binario on/off con tiempo mínimo de activación de 2 minutos (evita ciclado rápido).

### Umbrales de Sensores (Híbrido)
- **D-04:** Umbrales configurables con valores hardcodeados por defecto + actualización vía MQTT. Los valores por defecto coinciden con los del AlarmContext del frontend (Phase 4).
- **D-05:** Los umbrales se almacenan en RTC_DATA_ATTR (RTC slow memory). Sobreviven a deep sleep. En cold boot se cargan los valores por defecto; las actualizaciones MQTT sobrescriben.
- **D-06:** Umbrales configurables: temperatura warning (>=35°C), temperatura crítica (>=45°C), gas warning (>=200ppm), gas crítica (>=300ppm), flama (>0), humedad warning (<20% o >80%), tiempo de histéresis, margen delta de histéresis.
- **D-07:** Defaults: histéresis temporal de 3s, margen delta del 10% por debajo del umbral para desactivación.

### Protocolo de Override (MQTT)
- **D-08:** Los comandos de override viajan por MQTT al tópico `nodealert/{device_id}/commands` (alineado con Phase 3 D-03). El ESP32 suscribe y procesa inmediatamente. Sin polling REST.
- **D-09:** Comandos soportados: Actuator ON, Actuator OFF, Return to Auto, Acknowledge Alarm (silenciar alarma remota), Update Thresholds (push de nuevos valores).
- **D-10:** Al reiniciar el ESP32, el estado de override se resetea a Auto. No persiste override state en RTC — el servidor debe reenviar override explícitamente tras un reinicio.
- **D-11:** Backend necesita endpoint/servicio para publicar comandos MQTT desde el dashboard a dispositivos específicos.

### Dashboard — Alertas en Tiempo Real
- **D-12:** Pestaña de "Alertas activas" en el dashboard (separada del historial de eventos). Muestra alertas no resueltas con severity badge y tiempo transcurrido.
- **D-13:** Indicador visual de estado de override en el chip de conexión ESP32 (muestra "Override Manual" cuando el servidor ha tomado control).
- **D-14:** Filtro de severidad mejorado en EventTable (info/warning/critical). Ya implementado parcialmente en Phase 4 — alinear con nuevos tipos de evento de la automatización.
- **D-15:** Botón de "Silenciar alarma" en el panel de alertas que envía comando MQTT Acknowledge Alarm al ESP32.
- **D-16:** Los eventos críticos continúan siendo publicados directamente por el ESP32 a `nodealert/{device_id}/events` (confirmación Phase 3 D-16). El backend no infiere eventos desde lecturas.

### Backend — Persistencia de Eventos y Alertas
- **D-17:** Los eventos de la automatización (activación/desactivación de actuador, cambios de override, cruce de umbrales) se publican como eventos y se persisten en la tabla `events` existente.
- **D-18:** El backend necesita un endpoint adicional para publicar comandos MQTT (POST /api/v1/devices/{id}/command o similar), que valida el comando y lo publica al tópico MQTT correspondiente.

### the agent's Discretion
- Estructura interna del AutomationManager (archivos, clases, métodos)
- Implementación exacta del struct RTC_DATA_ATTR para umbrales
- Implementación del endpoint REST de comandos MQTT (ruta exacta, validación)
- Implementación de la UI de alertas activas (componente React, layout exacto)
- Stack size de la tarea AutomationManager
- Formato exacto de los mensajes JSON en comandos MQTT

</decisions>

<canonical_refs>
## Canonical References

Downstream agents MUST read these before planning or implementing.

### Project Context
- `.planning/PROJECT.md` — Project context, core value, constraints
- `.planning/REQUIREMENTS.md` — Requirements traceability (AUTO-01–04)
- `.planning/ROADMAP.md` — Phase overview, goal, success criteria
- `.planning/STATE.md` — Current project state

### Phase 1 Context (Firmware Foundation)
- `.planning/phases/01-firmware-foundation-sensores/01-CONTEXT.md` — Sensor sampling rates, RTOS priorities (Critical=3, Periodic=2, Background=1), state machine, error_handler
- `NodeAlert-Firmware/src/config/pins_config.h` — PIN_RELAY_ACTUATOR (GPIO_NUM_2) reservado para esta fase
- `NodeAlert-Firmware/src/core/state_machine.h` — 6-state FSM (INIT→STANDBY→RUNNING↔ALERT→RECOVERY→ERROR)
- `NodeAlert-Firmware/src/core/error_handler.h` — Error reporting with exponential backoff
- `NodeAlert-Firmware/src/managers/task_manager.cpp` — FreeRTOS task creation pattern
- `NodeAlert-Firmware/src/managers/sensor_manager.h` — Sensor reading queue interface

### Phase 2 Context (Backend Infrastructure)
- `.planning/phases/02-core-infrastructure-gateway-mqtt/02-CONTEXT.md` — Docker Compose, Mosquitto config, API endpoints, auth
- `docker-compose.yml` — Mosquitto service, Django, MySQL, nodealert-net
- `mosquitto/config/mosquitto.conf` — Broker config
- `NodeAlert-Backend/core/views.py` — Existing DRF viewsets
- `NodeAlert-Backend/core/models.py` — Device, Reading, Event models
- `NodeAlert-Backend/core/serializers.py` — API serializers

### Phase 3 Context (MQTT Integration)
- `.planning/phases/03-mqtt-firmware-integration/03-CONTEXT.md` — MQTT topics, payload format, subscriber pattern
- `NodeAlert-Firmware/src/config/mqtt_broker_config.h` — MQTT broker URI, port, topic prefix
- `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py` — MQTT subscriber (event ingestion + telemetry)
- `NodeAlert-Firmware/src/services/mqtt_manager.cpp` / `.h` — MQTT manager task (topic subscription for commands)

### Phase 3 Decisions (MQTT topics — MUST follow)
- `nodealert/{device_id}/telemetry` — periodic sensor data (QoS 0)
- `nodealert/{device_id}/events` — critical events published by ESP32 (QoS 1)
- `nodealert/{device_id}/commands` — server-to-device commands (QoS 1)
- MQTT topic prefix: `nodealert/`

### Phase 4 Context (Frontend Dashboard)
- `.planning/phases/04-dashboard-en-tiempo-real/04-CONTEXT.md` — Dashboard decisions (polling 3s, Context API, Industrial Dark theme)
- `NodeAlert-Frontend/src/context/AlarmContext.tsx` — Current alarm thresholds and logic
- `NodeAlert-Frontend/src/context/ReadingsContext.tsx` — Polling context with readings
- `NodeAlert-Frontend/src/components/AlarmSound.tsx` — Alarm sound component
- `NodeAlert-Frontend/src/components/EventTable.tsx` — Event table with severity filtering
- `NodeAlert-Frontend/src/components/AlertPanel.tsx` — Alert panel
- `NodeAlert-Frontend/src/components/SummaryBar.tsx` — Summary bar with ESP32 connection status

### No external specs
No external specs — requirements and decisions fully captured above.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets (Firmware)
- **PIN_RELAY_ACTUATOR** (GPIO_NUM_2): Already defined in pins_config.h — ready for automation control
- **sensor_queue**: Existing FreeRTOS queue (20 items of SensorReading) — AutomationManager can consume this same queue for threshold evaluation
- **MQTT topic subscription**: ESP32 already subscribes to `nodealert/{device_id}/commands` via MqttManager — ready for command processing
- **error_handler**: Exponential backoff with `source='MQTT'` — reusable for automation command failures
- **state_machine**: Existing ALERT state transitions into RUNNING or RECOVERY — automation can leverage these states
- **task_manager pattern**: `xTaskCreatePinnedToCore()` call pattern — new AutomationManager follows the same structure

### Reusable Assets (Backend)
- **events model/API**: Event model and `GET/PUT /api/v1/events` already exist — ready for automation event ingestion
- **Mosquitto broker**: Already deployed, authenticated, with topic persistence — commands published here reach ESP32
- **mqtt_subscriber.py**: Pattern for MQTT client in Django — new command-publishing service follows similar pattern
- **Django REST Framework**: Token auth, viewsets, serializers already configured

### Reusable Assets (Frontend)
- **AlarmContext.tsx**: Threshold logic and active alarm state — needs update for server-side automation events
- **AlarmSound.tsx**: Audio alarm component — reusable as-is
- **SummaryBar.tsx**: ESP32 connection status chip — candidate for override status indicator
- **EventTable.tsx**: Severity filterable event table — foundation for active alerts tab
- **AlertPanel.tsx**: Event/alert container — base for new active alerts panel

### Integration Points
- **AutomationManager → sensor_queue**: Reads latest sensor values at 3s interval
- **AutomationManager → MqttManager**: Receives override commands from `nodealert/{device_id}/commands`
- **AutomationManager → actuator (GPIO_NUM_2)**: Controls relay via gpio_set_level()
- **AutomationManager → events topic**: Publishes threshold crossings and actuator state changes
- **Backend command endpoint → Mosquitto**: Django publishes to `nodealert/{device_id}/commands`
- **Frontend AlarmContext → API**: Polls events and readings to update dashboard alert displays

</code_context>

<specifics>
## Specific Ideas

- La tarea de automatización corre cada 3s, igual que el polling del dashboard — sincronización natural
- El actuador mínimo 2 minutos evita ciclado rápido del relé (protege el relé y evita ruido)
- Los umbrales se sincronizan entre frontend y firmware: los valores por defecto de AlarmContext son la fuente de verdad
- Override-reset en reboot: el ESP32 siempre arranca en modo autónomo, el servidor debe re-aplicar override si es necesario
- El botón "Silenciar alarma" en el dashboard publica un comando MQTT que el ESP32 recibe y procesa

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 5-automatizaci-n-local-y-alertas*
*Context gathered: 2026-05-25*
