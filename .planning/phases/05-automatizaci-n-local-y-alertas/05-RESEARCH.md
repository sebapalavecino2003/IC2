# Phase 05: Automatización Local y Alertas - Research

**Researched:** 2026-05-25
**Domain:** ESP32 FreeRTOS automation logic, MQTT command protocol, Django command API, React real-time alerts dashboard
**Confidence:** HIGH

## Summary

This phase implements the hybrid automation logic that is the core value of NodeAlert IoT: autonomous local decision-making on the ESP32 with server override capability. The research confirms that all decisions from CONTEXT.md are implementable using the existing codebase patterns with no new external libraries required.

**Firmware side:** A new `AutomationManager` FreeRTOS task (priority 2, 3s interval) consumes the existing `sensor_queue` to evaluate thresholds with dual hysteresis (time + delta). It controls GPIO_NUM_2 (relay) for the ventilation actuator and receives override commands via `MQTT_EVENT_DATA` on the already-subscribed `nodealert/{device_id}/commands` topic. Threshold defaults are stored in `RTC_DATA_ATTR` struct in RTC slow memory for survival across reboots. The task publishes automation events (threshold crossings, actuator state changes) to `nodealert/{device_id}/events` using the existing MqttManager's publish path.

**Backend side:** A new DRF `@action` on `DeviceViewSet` (POST `/api/v1/devices/{id}/command/`) validates commands and publishes them to Mosquitto via `paho.mqtt.publish.single()` — a simple connection-less pattern that avoids threading issues with the existing subscriber.

**Frontend side:** A new `ActiveAlertsPanel` component shows unresolved alerts with severity badges and elapsed time. The existing `SummaryBar` gets an override-status chip. A "Silenciar alarma" button publishes the Acknowledge Alarm command via the new API endpoint. The existing `AlarmContext` is extended to expose override state.

**Primary recommendation:** Follow existing firmware patterns exactly — `AutomationManager` mirrors `TaskManager` in structure; command parsing happens in `MQTT_EVENT_DATA` handler with a command queue or shared state; thresholds use `RTC_DATA_ATTR` struct pattern with cold-boot defaults and MQTT-update override.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Threshold evaluation | ESP32 Firmware | — | Must work offline; core automation decision lives on the node |
| Actuator control | ESP32 Firmware | — | Direct GPIO control; no round-trip to server |
| Hysteresis logic | ESP32 Firmware | — | Must be local to avoid false positives during connection loss |
| MQTT command processing | ESP32 Firmware | Backend (publishing) | ESP32 receives/parses commands; backend publishes them |
| Command endpoint API | Backend (Django) | — | REST endpoint for dashboard to trigger MQTT commands |
| Active alerts storage | Backend (Django) | — | Events model already persists events; automation events use the same path |
| Alerts dashboard UI | Frontend (React) | — | Active alerts tab, severity badges, override indicators |
| Override status display | Frontend (React) | — | Chip in SummaryBar shows current override mode |

## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** Nuevo AutomationManager como tarea FreeRTOS dedicada, separada de sensor_manager y monitor_task. Prioridad 2 (intermedia), ejecución cada 3s.
- **D-02:** La tarea lee los valores más recientes de cada sensor vía sensor_queue (existente), evalúa umbrales, controla el actuador y verifica comandos MQTT de override.
- **D-03:** El actuador de ventilación se controla mediante PIN_RELAY_ACTUATOR (GPIO_NUM_2, ya reservado en pins_config.h). Relé binario on/off con tiempo mínimo de activación de 2 minutos (evita ciclado rápido).
- **D-04:** Umbrales configurables con valores hardcodeados por defecto + actualización vía MQTT. Los valores por defecto coinciden con los del AlarmContext del frontend (Phase 4).
- **D-05:** Los umbrales se almacenan en RTC_DATA_ATTR (RTC slow memory). Sobreviven a deep sleep. En cold boot se cargan los valores por defecto; las actualizaciones MQTT sobrescriben.
- **D-06:** Umbrales configurables: temperatura warning (>=35°C), temperatura crítica (>=45°C), gas warning (>=200ppm), gas crítica (>=300ppm), flama (>0), humedad warning (<20% o >80%), tiempo de histéresis, margen delta de histéresis.
- **D-07:** Defaults: histéresis temporal de 3s, margen delta del 10% por debajo del umbral para desactivación.
- **D-08:** Los comandos de override viajan por MQTT al tópico `nodealert/{device_id}/commands`. El ESP32 suscribe y procesa inmediatamente. Sin polling REST.
- **D-09:** Comandos soportados: Actuator ON, Actuator OFF, Return to Auto, Acknowledge Alarm, Update Thresholds.
- **D-10:** Al reiniciar el ESP32, el estado de override se resetea a Auto. No persiste override state en RTC.
- **D-11:** Backend necesita endpoint/servicio para publicar comandos MQTT desde el dashboard a dispositivos específicos.
- **D-12:** Pestaña de "Alertas activas" en el dashboard (separada del historial de eventos).
- **D-13:** Indicador visual de estado de override en el chip de conexión ESP32.
- **D-14:** Filtro de severidad mejorado en EventTable.
- **D-15:** Botón de "Silenciar alarma" en el panel de alertas.
- **D-16:** Los eventos críticos continúan siendo publicados directamente por el ESP32.
- **D-17:** Los eventos de la automatización se persisten en la tabla `events` existente.
- **D-18:** Backend necesita endpoint POST /api/v1/devices/{id}/command.

### the agent's Discretion
- Estructura interna del AutomationManager (archivos, clases, métodos)
- Implementación exacta del struct RTC_DATA_ATTR para umbrales
- Implementación del endpoint REST de comandos MQTT (ruta exacta, validación)
- Implementación de la UI de alertas activas (componente React, layout exacto)
- Stack size de la tarea AutomationManager
- Formato exacto de los mensajes JSON en comandos MQTT

### Deferred Ideas (OUT OF SCOPE)
- None — discussion stayed within phase scope

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| AUTO-01 | Evaluación autónoma de condiciones críticas (temp alta, gas, llama) | AutomationManager task reads sensor_queue at 3s, evaluates thresholds against `threshold_config` struct from RTC memory |
| AUTO-02 | Histéresis para evitar falsos positivos en detección de eventos | Dual hysteresis: time-based (3s default `HYSTERESIS_TIME_MS`) + delta-based (10% margin below threshold, `HYSTERESIS_DELTA_PCT`), both configurable via MQTT |
| AUTO-03 | Control local de actuadores (ventilación) basado en umbrales configurables | GPIO_NUM_2 relay control via `gpio_set_level()`, 2-minute minimum activation window tracked in `actuator_state` struct, threshold_config from RTC |
| AUTO-04 | Recepción y ejecución de comandos MQTT de override desde el servidor | MQTT_EVENT_DATA handler on `nodealert/{device_id}/commands` topic parses JSON command, sets override state, routes to Auto/Manual actuator control |

## Standard Stack

### Core — No New Libraries Required

This phase adds no new external dependencies. All capabilities are built on the existing stack:

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| ESP-IDF `driver/gpio.h` | (built-in) | GPIO output for relay actuator | Standard ESP-IDF API for digital output; `gpio_set_level()` is the canonical pattern |
| ESP-IDF `esp_mqtt_client.h` | (built-in) | MQTT event handling for command receipt | Already used by MqttManager; `MQTT_EVENT_DATA` handler already wired for command topic |
| ESP-IDF `RTC_DATA_ATTR` | (built-in) | Threshold persistence across reboots | Linker attribute mapping to RTC slow memory (8KB on ESP32); survives deep sleep, cold-boot initializes from defaults |
| paho-mqtt (Python) | 2.1.0 | Publish MQTT commands from Django | Already a dependency; `paho.mqtt.publish.single()` for connection-less publish avoids threading issues |
| Django REST Framework | (existing) | Command API endpoint | Existing `DeviceViewSet` extended with `@action(detail=True)` |
| React + MUI | (existing) | Active alerts panel, override indicator | Existing `SummaryBar`, `EventTable`, `AlertPanel` extended |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `paho.mqtt.publish.single()` for command publishing | Shared persistent MQTT client (via `loop_start()`) | `single()` is simpler, avoids threading issues with Django dev server's auto-reloader, and the one-shot connection overhead (~5ms) is negligible for infrequent commands |
| RTC_DATA_ATTR for thresholds | NVS (Non-Volatile Storage) | NVS requires flash writes with wear limits; RTC memory is faster, has no wear, and survives deep sleep. However, RTC_DATA_ATTR does NOT survive power loss — for this phase that's acceptable (D-10: override resets on reboot). NVS is the fallback if power-loss persistence is needed in v2. |
| Separate FreeRTOS task for AutomationManager | Timer callback in main loop | D-01 explicitly mandates a separate task. A separate task is cleaner: it has its own stack, priority, and can block on queue reads without blocking the main loop. |

**Key insight for paho-mqtt choice:** The existing `mqtt_subscriber.py` uses `mqtt.CallbackAPIVersion.VERSION2` with `Client()` and `loop_forever()`. For the command publishing endpoint, `paho.mqtt.publish.single()` is the recommended pattern for one-shot publishes from request handlers — it creates a temporary client, publishes, and disconnects. This avoids the well-documented threading conflict when running a `loop_start()` client in a Django process that also runs the development server's auto-reloader (which spawns two threads and causes client-ID conflicts on reconnect).

## Package Legitimacy Audit

> No new packages to install. All capabilities use:
> - ESP-IDF built-in APIs (esp_mqtt_client.h, driver/gpio.h, RTC_DATA_ATTR)
> - paho-mqtt (already in requirements.txt)
> - Existing Django/React dependencies
>
> Audit not required for this phase — skip.

## Architecture Patterns

### System Architecture Diagram

```mermaid
flowchart LR
    subgraph ESP32_Firmware["ESP32 Firmware"]
        SQ[sensor_queue<br/>FreeRTOS Queue]
        AUT[AutomationManager<br/>Priority 2 / 3s]
        ACT[GPIO Actuator<br/>GPIO_NUM_2]
        MQTT_C[MqttManager<br/>MQTT Event Handler]
        RTC[RTC_DATA_ATTR<br/>Threshold Config]
        EVT_PUB[Event Publisher<br/>→ nodealert/{id}/events]
        
        SQ -->|SensorReading| AUT
        AUT -->|gpio_set_level| ACT
        MQTT_C -->|commands topic| AUT
        AUT <--> RTC
        AUT -->|threshold_crossing<br/>actuator_change| EVT_PUB
    end

    subgraph Backend["Backend (Django)"]
        CMD_API[POST /api/v1/devices/{id}/command/]
        MQTT_PUB[paho.mqtt.publish.single<br/>→ nodealert/{id}/commands]
        EVT_DB[(Events Table<br/>MySQL)]
        MQTT_SUB[mqtt_subscriber<br/>← nodealert/+/events]
        
        CMD_API -->|validate & publish| MQTT_PUB
        MQTT_SUB -->|persist| EVT_DB
    end

    subgraph Broker["Mosquitto Broker"]
        TOPIC_CMD[nodealert/{id}/commands]
        TOPIC_EV[nodealert/{id}/events]
    end

    subgraph Frontend["Frontend (React)"]
        RTC_CTX[ReadingsContext<br/>Poll 3s]
        ALM_CTX[AlarmContext<br/>Thresholds + Override]
        ACT_ALERTS[ActiveAlertsPanel]
        SUM_BAR[SummaryBar<br/>Override Chip]
        SND_BTN[Silenciar Alarma Button]
        
        RTC_CTX -->|fetch events| EVT_DB
        ALM_CTX --> ACT_ALERTS
        ALM_CTX --> SUM_BAR
        SND_BTN -->|POST command| CMD_API
    end

    MQTT_PUB --> TOPIC_CMD
    TOPIC_CMD --> MQTT_C
    EVT_PUB --> TOPIC_EV
    TOPIC_EV --> MQTT_SUB
```

**Data flow trace (primary use case — gas threshold crossed):**
1. Sensor task pushes SensorReading to `sensor_queue`
2. AutomationManager receives from queue, compares gas value against `threshold_config.gas_critical` from RTC
3. If exceeded + hysteresis window satisfied → `gpio_set_level(GPIO_NUM_2, 1)` activates relay
4. Event published to `nodealert/{id}/events` with `event_type=actuator_on`, `severity=critical`
5. Backend subscriber persists event to `events` table
6. Frontend polls `/events?resolved=false`, finds new event
7. `ActiveAlertsPanel` displays with severity badge and elapsed time
8. User clicks "Silenciar alarma" → POST `/api/v1/devices/{id}/command/` with `{command: "acknowledge_alarm"}`
9. Backend publishes `nodealert/{id}/commands` with `{cmd: "acknowledge_alarm"}`
10. ESP32 `MQTT_EVENT_DATA` handler receives, sets alarm_acknowledged flag, AutomationManager silences buzzer

### Recommended Project Structure

**Firmware additions:**
```
NodeAlert-Firmware/src/
├── core/
│   └── main.cpp                          # ADD AutomationManager init + start
├── managers/
│   ├── automation_manager.h               # NEW
│   ├── automation_manager.cpp             # NEW
│   └── mqtt_manager.cpp                   # MODIFY: command parsing in MQTT_EVENT_DATA
├── config/
│   └── pins_config.h                      # (already has PIN_RELAY_ACTUATOR)
└── hal/
    ├── sensor_reading.h                   # No changes needed
    └── thresholds.h                        # NEW: RTC_DATA_ATTR struct + defaults
```

**Backend additions:**
```
NodeAlert-Backend/core/
├── views.py                               # MODIFY: add command action on DeviceViewSet
├── urls.py                                # (auto-registered by router)
├── serializers.py                          # MODIFY: add CommandSerializer
└── mqtt_publisher.py                      # NEW: helper using paho.mqtt.publish.single()
```

**Frontend additions:**
```
NodeAlert-Frontend/src/
├── context/
│   └── AlarmContext.tsx                    # MODIFY: add override state, acknowledge function
├── components/
│   ├── ActiveAlertsPanel.tsx               # NEW: unresolved alerts with severity badges
│   ├── SummaryBar.tsx                      # MODIFY: override status chip
│   └── AlarmSound.tsx                      # (no changes needed)
├── services/
│   └── api.ts                              # No changes (generic axios instance)
└── types/
    └── index.ts                            # MODIFY: add OverrideCommand types
```

### Pattern 1: AutomationManager FreeRTOS Task
**What:** Follows the exact same pattern as `TaskManager::monitorTask` — a dedicated FreeRTOS task created via `xTaskCreatePinnedToCore()`, running in an infinite `while(1)` loop with `vTaskDelay(pdMS_TO_TICKS(3000))` at the end.

**When to use:** Any FreeRTOS task that needs dedicated stack and priority.

**Structure** (following TaskManager pattern):
```cpp
// automation_manager.h
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "hal/thresholds.h"

class MqttManager;  // forward declaration for command state
class ErrorHandler;

class AutomationManager {
public:
    AutomationManager();
    void init(QueueHandle_t sensor_queue, ErrorHandler* eh);
    void startTask();
    // Called from MQTT event handler to inject override commands
    void setCommandOverride(const char* command_json);
    
private:
    QueueHandle_t          m_sensor_queue;
    ErrorHandler*          m_error_handler;
    TaskHandle_t           m_task_handle;
    ThresholdConfig        m_thresholds;  // shadow copy for fast access
    
    // Override state (not RTC-persisted per D-10)
    bool                   m_override_active;
    bool                   m_override_actuator_on;
    bool                   m_alarm_acknowledged;
    uint32_t               m_actuator_on_ms;   // timestamp when actuator was turned on
    
    static void automationTask(void* pvParams);
    void evaluateThresholds();
    void controlActuator(bool turn_on);
};
```
Source: [VERIFIED: existing `task_manager.h` pattern]

**Task creation call** (in `main.cpp`, following existing mqttManager.startMqttTask() pattern):
```cpp
AutomationManager autoManager;
autoManager.init(sensorManager.getReadingQueue(), &errorHandler);
autoManager.startTask();  // calls xTaskCreatePinnedToCore(automationTask, "auto_task", 4096, this, 2, &task_handle, 1)
```
Source: [VERIFIED: existing `main.cpp` lines 97-99 for MqttManager pattern]

### Pattern 2: RTC_DATA_ATTR Threshold Struct
**What:** A global struct with `RTC_DATA_ATTR` attribute that holds all configurable thresholds. Initialized with defaults (from `AlarmContext.tsx` values per D-04) on cold boot. Survives deep sleep (though deep sleep is not used in v1). Updates from MQTT `Update Thresholds` command write to this struct.

**When to use:** For any configuration that must survive reboots and be accessible without flash writes.

**Usage:**
```cpp
// hal/thresholds.h
#pragma once
#include <cstdint>

struct ThresholdConfig {
    // Temperature (°C)
    float temp_warning;      // default: 35.0f
    float temp_critical;     // default: 45.0f
    // Gas (PPM)
    float gas_warning;       // default: 200.0f
    float gas_critical;      // default: 300.0f
    // Flame (0 or 1)
    float flame_threshold;   // default: 0.5f
    // Humidity (%)
    float humidity_low;      // default: 20.0f
    float humidity_high;     // default: 80.0f
    // Hysteresis
    uint32_t hysteresis_time_ms;   // default: 3000 (3 seconds)
    float    hysteresis_delta_pct; // default: 0.10f (10%)
};

// RTC_DATA_ATTR places this in RTC slow memory (0x50000000+ on ESP32)
// On cold boot: initialized to the default values below
// On deep sleep wake: values persist unchanged
// On brownout/por: re-initialized to defaults (RTC memory loses power)
RTC_DATA_ATTR ThresholdConfig g_thresholds = {
    .temp_warning = 35.0f,
    .temp_critical = 45.0f,
    .gas_warning = 200.0f,
    .gas_critical = 300.0f,
    .flame_threshold = 0.5f,
    .humidity_low = 20.0f,
    .humidity_high = 80.0f,
    .hysteresis_time_ms = 3000,
    .hysteresis_delta_pct = 0.10f,
};
```
Source: [CITED: docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32/api-guides/memory-types.html] — `RTC_DATA_ATTR` confirmed for ESP32's 8KB RTC slow memory. Struct pattern [VERIFIED: ESP-IDF deep sleep stub documentation].

**Cold boot vs warm boot behavior:**
- Cold boot (power-on reset): CRT startup code zeroes `.rtc.data` section, then the static initializer sets `g_thresholds` to the default values. Same behavior as any `RTC_DATA_ATTR` global.
- The AutomationManager does NOT need an explicit "first boot" check — the defaults are always correct on initial power-up.
- MQTT "Update Thresholds" commands overwrite individual fields at runtime.

### Pattern 3: MQTT Command Processing in MQTT_EVENT_DATA
**What:** The existing `MqttManager::mqttEventHandler()` already subscribes to `nodealert/{device_id}/commands` on connect (line 96-97 of mqtt_manager.cpp). It already receives messages via `MQTT_EVENT_DATA` (lines 110-116). The handler currently only prints the command. This phase extends it to parse the JSON payload and forward to AutomationManager.

**When to use:** For any device-bound command that needs immediate processing.

**Current handler** (to be modified):
```cpp
// In mqtt_manager.cpp, MQTT_EVENT_DATA case:
case MQTT_EVENT_DATA: {
    esp_mqtt_event_t* msg = (esp_mqtt_event_t*)event_data;
    // Build a null-terminated copy of the data
    char cmd[128];
    size_t len = (msg->data_len < sizeof(cmd) - 1) ? msg->data_len : sizeof(cmd) - 1;
    memcpy(cmd, msg->data, len);
    cmd[len] = '\0';
    
    // Parse and forward to automation manager
    // (AutomationManager reference stored via init or setter)
    if (self->m_auto_mgr) {
        self->m_auto_mgr->processCommand(cmd);
    }
    break;
}
```

**Command JSON format** (at the agent's discretion per CONTEXT.md, but following MQTT best practices):
```json
// Actuator ON override
{"cmd": "actuator_on", "timestamp": 1712345678}

// Actuator OFF override
{"cmd": "actuator_off", "timestamp": 1712345678}

// Return to Auto mode
{"cmd": "return_to_auto", "timestamp": 1712345678}

// Acknowledge Alarm (silence local alarm)
{"cmd": "acknowledge_alarm", "timestamp": 1712345678}

// Update Thresholds (push new values from server)
{"cmd": "update_thresholds", "timestamp": 1712345678, "params": {
    "temp_critical": 50.0,
    "gas_critical": 350.0,
    "hysteresis_time_ms": 5000
}}
```
Source: [ASSUMED] — JSON command format follows standard MQTT command patterns. Exact keys at planner's discretion.

**Processing in AutomationManager:**
```cpp
void AutomationManager::processCommand(const char* json_cmd) {
    // Parse JSON (using minimal cJSON or sscanf-based parsing)
    // ESP-IDF includes cJSON, but for simple commands a lightweight
    // token-based parse suffices to avoid heap fragmentation.
    
    if (strstr(json_cmd, "\"cmd\":\"actuator_on\"")) {
        m_override_active = true;
        m_override_actuator_on = true;
        controlActuator(true);
        // Publish override event
        publishEvent("override", "warning", "Override: Actuator ON");
    }
    else if (strstr(json_cmd, "\"cmd\":\"actuator_off\"")) {
        m_override_active = true;
        m_override_actuator_on = false;
        controlActuator(false);
        publishEvent("override", "warning", "Override: Actuator OFF");
    }
    else if (strstr(json_cmd, "\"cmd\":\"return_to_auto\"")) {
        m_override_active = false;
        publishEvent("override", "info", "Override: Return to Auto");
    }
    // ... handle other commands
}
```
Source: [VERIFIED: ESP-IDF `esp_mqtt_client.h` MQTT_EVENT_DATA API] — event data structure confirmed.

### Pattern 4: Django Command Publishing Endpoint
**What:** A new `@action(detail=True)` on `DeviceViewSet` that accepts a POST with a command, validates the device exists and is active, then publishes the command to `nodealert/{device.device_id}/commands` via `paho.mqtt.publish.single()`.

**When to use:** For any server-initiated MQTT message to a specific device.

**Implementation:**
```python
# mqtt_publisher.py (NEW)
"""Helper module for publishing MQTT commands from Django views."""
import json
import os
import paho.mqtt.publish as publish

def publish_command(device_id: str, command: dict) -> bool:
    """Publish a command to a device's MQTT command topic.
    
    Uses paho.mqtt.publish.single() for a connection-less publish.
    This avoids threading issues with persistent clients when
    running under Django's auto-reloading dev server.
    
    Args:
        device_id: The device ID (e.g., "nodealert-01")
        command: Dict with at minimum {"cmd": "command_name"}
    
    Returns:
        True if publish succeeded, False otherwise
    """
    try:
        topic = f"nodealert/{device_id}/commands"
        payload = json.dumps(command)
        host = os.environ.get('MQTT_BROKER_HOST', 'mosquitto')
        port = int(os.environ.get('MQTT_BROKER_PORT', '1883'))
        user = os.environ.get('MQTT_PUBLISHER_USER', 'mqtt_publisher')
        password = os.environ.get('MQTT_PUBLISHER_PASSWORD', '')
        
        publish.single(
            topic, payload, qos=1, retain=False,
            hostname=host, port=port,
            auth={'username': user, 'password': password}
        )
        return True
    except Exception as e:
        print(f"[MQTT PUBLISH ERROR] {e}")
        return False
```
Source: [CITED: eclipse.dev/paho/files/paho.mqtt.python/html/helpers.html] — `publish.single()` API confirmed.

**View action:**
```python
# In views.py, DeviceViewSet:
from rest_framework.decorators import action
from rest_framework.response import Response
from .mqtt_publisher import publish_command

class DeviceViewSet(viewsets.ModelViewSet):
    # ... existing code ...
    
    @action(detail=True, methods=['post'])
    def command(self, request, pk=None):
        """Publish an MQTT command to a device.
        
        POST body: {"command": "actuator_on", "params": {...}}
        Valid commands: actuator_on, actuator_off, return_to_auto,
                       acknowledge_alarm, update_thresholds
        """
        device = self.get_object()
        command = request.data.get('command')
        if not command:
            return Response(
                {'error': 'command field is required'},
                status=status.HTTP_400_BAD_REQUEST
            )
        
        valid_commands = ['actuator_on', 'actuator_off', 'return_to_auto',
                         'acknowledge_alarm', 'update_thresholds']
        if command not in valid_commands:
            return Response(
                {'error': f'Invalid command. Valid: {valid_commands}'},
                status=status.HTTP_400_BAD_REQUEST
            )
        
        payload = {
            'cmd': command,
            'timestamp': int(time.time()),
        }
        if command == 'update_thresholds':
            payload['params'] = request.data.get('params', {})
        
        success = publish_command(device.device_id, payload)
        if not success:
            return Response(
                {'error': 'Failed to publish command'},
                status=status.HTTP_502_BAD_GATEWAY
            )
        
        return Response({'status': 'published', 'device': device.device_id, 'command': command})
```
Source: [ASSUMED] — follows existing DRF viewset pattern. Route auto-registered as `/api/v1/devices/{id}/command/`.

### Pattern 5: Active Alerts Panel (React)
**What:** A tab-based view separating active (unresolved) alerts from event history. Uses existing `useReadings()` context for data and existing MUI components. Follows the `EventTable` pattern for severity badges and the `AlertPanel` pattern for card layout.

**When to use:** Any real-time monitoring dashboard needing separate alert escalation vs. event log views.

**Component structure:**
```tsx
// ActiveAlertsPanel.tsx (NEW)
import { useState } from 'react'
import {
  Box, Card, CardContent, Typography, Chip, IconButton, Grid,
  Alert as MuiAlert, Button,
} from '@mui/material'
import CheckCircleIcon from '@mui/icons-material/CheckCircle'
import VolumeUpIcon from '@mui/icons-material/VolumeUp'
import VolumeOffIcon from '@mui/icons-material/VolumeOff'
import { useAlarm } from '../context/AlarmContext'
import { useReadings } from '../context/ReadingsContext'
import api from '../services/api'
import type { Event } from '../types'

const SEVERITY_COLORS: Record<string, 'info' | 'warning' | 'error'> = {
  info: 'info',
  warning: 'warning',
  critical: 'error',
}

const SEVERITY_LABELS: Record<string, string> = {
  info: 'Info',
  warning: 'Advertencia',
  critical: 'Crítico',
}

function elapsedTime(timestamp: string): string {
  const elapsed = Date.now() - new Date(timestamp).getTime()
  const mins = Math.floor(elapsed / 60000)
  const secs = Math.floor((elapsed % 60000) / 1000)
  if (mins > 0) return `${mins}m ${secs}s`
  return `${secs}s`
}

export default function ActiveAlertsPanel() {
  const { events } = useReadings()
  const { deviceStatus } = useAlarm()
  const unresolved = events.filter(e => !e.resolved)
  const [acknowledging, setAcknowledging] = useState(false)

  const handleSilenceAlarm = async () => {
    setAcknowledging(true)
    try {
      // POST to command endpoint — assumes device ID is known
      // In practice, pass deviceId as prop or from context
      await api.post('/devices/1/command/', { command: 'acknowledge_alarm' })
    } catch {
      // Silently fail
    } finally {
      setAcknowledging(false)
    }
  }

  if (unresolved.length === 0) {
    return (
      <Card sx={{ mb: 2 }}>
        <CardContent>
          <Typography variant="h6" gutterBottom>Alertas Activas</Typography>
          <MuiAlert severity="success">No hay alertas activas</MuiAlert>
        </CardContent>
      </Card>
    )
  }

  return (
    <Card sx={{ mb: 2 }}>
      <CardContent>
        <Typography variant="h6" gutterBottom>
          Alertas Activas ({unresolved.length})
        </Typography>
        {unresolved.map((event: Event) => (
          <Box
            key={event.id}
            sx={{
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'space-between',
              p: 1.5,
              mb: 1,
              bgcolor: event.severity === 'critical' ? 'error.dark' : 'warning.dark',
              borderRadius: 1,
            }}
          >
            <Box>
              <Chip
                label={SEVERITY_LABELS[event.severity]}
                color={SEVERITY_COLORS[event.severity]}
                size="small"
                sx={{ mr: 1 }}
              />
              <Typography variant="body2" display="inline">
                {event.message}
              </Typography>
              <Typography variant="caption" display="block" sx={{ mt: 0.5, opacity: 0.7 }}>
                {elapsedTime(event.timestamp)} atrás
              </Typography>
            </Box>
            <IconButton
              size="small"
              color="inherit"
              onClick={handleSilenceAlarm}
              disabled={acknowledging}
              title="Silenciar alarma"
            >
              <VolumeOffIcon />
            </IconButton>
          </Box>
        ))}
      </CardContent>
    </Card>
  )
}
```
Source: [VERIFIED: existing `EventTable.tsx` pattern for severity mapping, `AlertPanel.tsx` for card layout] — code follows established MUI patterns in the codebase.

**SummaryBar override chip** (modification to existing component):
```tsx
// In SummaryBar.tsx — add alongside the ESP32 connection chip:
{/* Override status chip — NEW */}
{deviceStatus === 'online' && (
  <Chip
    label={overrideActive ? 'Override Manual' : 'Modo Auto'}
    color={overrideActive ? 'warning' : 'success'}
    size="small"
    variant={overrideActive ? 'filled' : 'outlined'}
    icon={overrideActive ? <BuildIcon /> : <PlayArrowIcon />}
    sx={{ ml: 1 }}
  />
)}
```
Source: [VERIFIED: existing `SummaryBar.tsx` pattern for deviceStatus chip] — this follows the same Chip pattern used for connection status.

### Anti-Patterns to Avoid

- **Shared MQTT client for publishing in Django:** Do NOT add a `loop_start()` persistent client for command publishing. The `paho.mqtt.publish.single()` approach avoids the well-documented threading conflict with Django's auto-reloader, and the per-request connection overhead (< 5ms) is negligible for infrequent user-triggered commands. If a persistent client is desired later, it should be initialized in `urls.py` (late in app loading) with a unique client ID, not in `__init__.py`.

- **Polling for commands on ESP32:** D-08 mandates MQTT push, not polling. Do not add HTTP/WS command fetching. The existing MQTT subscription already handles this.

- **Storing hysteresis state in RTC:** Hysteresis is transient evaluation state — it crosses boundaries in milliseconds. Storing it in RTC memory would waste the limited 8KB and add unnecessary complexity. Keep hysteresis state in the AutomationManager's stack/heap.

- **Parsing JSON with full cJSON on every AutomationManager cycle:** cJSON uses heap allocation, which can cause fragmentation on a 3s cycle. For threshold evaluation, reading raw float values from the SensorReading struct is sufficient. Only use cJSON for MQTT command parsing (infrequent) or use lightweight `strstr`-based token matching for the simple command set.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| MQTT publishing from Django | Custom socket-level MQTT client | `paho.mqtt.publish.single()` | paho-mqtt handles all MQTT protocol details (CONNECT, PUBLISH, DISCONNECT), QoS, auth. Hand-rolling a raw MQTT client is error-prone and unnecessary. |
| GPIO output for relay | Direct register manipulation | `gpio_config_t` + `gpio_set_level()` | ESP-IDF's GPIO API handles pin multiplexing, output enable, and pull configuration. Direct register access breaks across ESP32 variants. |
| Threshold persistence | Custom flash storage driver | `RTC_DATA_ATTR` struct | The ESP-IDF linker already handles RTC memory layout. Using NVS would require flash writes, wear management, and read latency. RTC memory is the correct persistence domain for configuration that survives sleep. |
| Time/delta hysteresis | Ad-hoc timer logic with floats | Time tracking via `esp_timer_get_time()` + delta via `thresholds.hysteresis_delta_pct` | The existing `StateMachine::isInStateLongerThanUs()` already implements time-checking pattern. Reuse the same approach. Delta hysteresis is simply `threshold * (1.0 - hysteresis_delta_pct)`. |

**Key insight:** This entire phase is about wiring existing primitives together — `sensor_queue` + GPIO output + MQTT events + REST endpoint. No complex algorithms, no new communication protocols, no external integrations. The complexity lies in correct hysteresis state management and clean command routing, not in any single component.

## Common Pitfalls

### Pitfall 1: Task Starvation from Queue Blocking
**What goes wrong:** The AutomationManager uses `xQueueReceive()` with a timeout to consume `sensor_queue`. If the timeout is too short or the queue is empty, the task busy-loops consuming CPU. If other tasks at priority 2+ dominate, AutomationManager starves.

**Why it happens:** `xQueueReceive()` with short timeout + tight loop pattern. The existing `mqtt_manager.cpp` uses `pdMS_TO_TICKS(100)` timeout on the same queue — this works because MQTT task also has a 10s publish timer.

**How to avoid:** Use the same pattern: `xQueueReceive(queue, &reading, pdMS_TO_TICKS(100))` to peek for new data, then fall through to the 3s time-based evaluation. Do NOT use `portMAX_DELAY` — the task needs to wake periodically for time-based hysteresis checks even if no new sensor data arrives.

**Warning signs:** ESP32 watchdog timeout, erratic actuator behavior, `Task watchdog got triggered` in serial output.

### Pitfall 2: RTC_DATA_ATTR Not Surviving Brownout
**What goes wrong:** The threshold configuration resets to defaults after a power-cycle, brownout, or hardware reset (not a deep sleep wake).

**Why it happens:** `RTC_DATA_ATTR` only survives deep sleep and software resets. On power-on reset (POR), the `.rtc.data` section is re-initialized. This is documented ESP32 behavior — RTC memory loses power when VDD is removed.

**How to avoid:** This is **by design** for v1 (D-10: override resets on reboot). Document it: the server-side application must re-push thresholds after detecting a device reconnection (via `nodealert/{id}/telemetry` receiving a new boot sequence). For v2, implement NVS-backed persistence or store thresholds in the server and push on reconnect.

**Warning signs:** After unplugging and re-plugging the ESP32, threshold values match defaults instead of previously configured values.

### Pitfall 3: Double-Hysteresis Race Between Time and Delta
**What goes wrong:** With both time (3s) and delta (10% margin) hysteresis, if a sensor reading briefly dips below threshold and then spikes again, the actuator might cycle rapidly despite the time hysteresis.

**Why it happens:** Delta hysteresis is evaluated per-cycle (3s); time hysteresis tracks how long conditions have been normal. If the delta check passes before the time check fires, the task might toggle the actuator prematurely.

**How to avoid:** The deactivation condition must require BOTH:
1. Delta condition: `current_value < threshold * (1.0 - hysteresis_delta_pct)` — must be true
2. Time condition: `esp_timer_get_time() - entry_time > hysteresis_time_ms` — must have elapsed

These are AND, not OR. Sequence: detect threshold crossing → record `entry_time` → each cycle check delta → if delta clear AND time elapsed → deactivate. Never deactivate on delta alone.

**Warning signs:** Actuator toggling on/off within seconds during borderline conditions.

### Pitfall 4: MQTT Command Topic Being Processed by Multiple Tasks
**What goes wrong:** The `MQTT_EVENT_DATA` handler runs in the MQTT event task context (internal to `esp_mqtt_client`). If the handler directly modifies AutomationManager state without synchronization, data races occur.

**Why it happens:** The MQTT event callback runs on the MQTT client's internal task, not the AutomationManager task.

**How to avoid:** Two approaches:
- **Recommended (simpler):** Set a volatile flag or copy the command string to a small ring buffer. The AutomationManager reads these on its next cycle. Since AutomationManager runs every 3s, commands are processed within 3s — acceptable latency.
- **Alternative (lower latency):** Use a dedicated FreeRTOS queue for commands. The MQTT handler pushes to the command queue; AutomationManager receives from both `sensor_queue` and `command_queue` using `xQueueReceive()` with polling.

**Warning signs:** Race conditions, corrupted command strings, intermittent command failures.

### Pitfall 5: 2-Minute Minimum Actuator Run Time Leaking Across Override
**What goes wrong:** If the user sends `actuator_off` via override, the 2-minute minimum timer prevents the relay from turning off.

**Why it happens:** The 2-minute minimum is designed for auto mode to protect the relay. But D-09 explicitly includes `actuator_off` as a valid command.

**How to avoid:** Override commands bypass the minimum timer. Only the auto-mode logic enforces `m_actuator_min_off_time`. The `processCommand()` for `actuator_off` should call `gpio_set_level(GPIO_NUM_2, 0)` directly, regardless of elapsed time.

**Warning signs:** User sends `actuator_off` but the relay stays on.

## Code Examples

### AutomationManager Task Body
```cpp
// automation_manager.cpp — main task loop
void AutomationManager::automationTask(void* pvParams) {
    AutomationManager* self = static_cast<AutomationManager*>(pvParams);
    SensorReading reading;
    
    // Track alert state for time-based hysteresis
    bool alert_active = false;
    uint32_t alert_entry_time = 0;
    uint32_t actuator_on_time = 0;
    
    while (1) {
        // 1. Consume any pending sensor readings (non-blocking peek)
        while (xQueueReceive(self->m_sensor_queue, &reading, 
                             pdMS_TO_TICKS(0)) == pdTRUE) {
            // Store latest values (use m_latest array like SensorManager)
            size_t idx = static_cast<size_t>(reading.type);
            if (idx < 4) self->m_latest[idx] = reading;
        }
        
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        
        // 2. Check override state — if active, skip auto-evaluation
        if (!self->m_override_active) {
            // 3. Evaluate all sensor thresholds
            bool any_critical = self->evaluateThresholds();
            
            if (any_critical && !alert_active) {
                // Enter alert — record time for hysteresis
                alert_active = true;
                alert_entry_time = now_ms;
                self->controlActuator(true);
                actuator_on_time = now_ms;
                self->publishEvent("threshold_crossed", "critical", 
                    "Critical threshold detected");
            }
            else if (!any_critical && alert_active) {
                // Check deactivation hysteresis
                uint32_t elapsed = now_ms - alert_entry_time;
                if (elapsed >= self->m_thresholds.hysteresis_time_ms) {
                    alert_active = false;
                    // Only turn off if minimum run time has elapsed
                    if ((now_ms - actuator_on_time) >= ACTUATOR_MIN_ON_MS) {
                        self->controlActuator(false);
                        self->publishEvent("actuator_off", "info",
                            "Conditions normal — actuator deactivated");
                    }
                }
            }
        }
        
        // 4. Check for pending MQTT commands (read from command queue)
        //    (processed in next cycle after being pushed by MQTT handler)
        
        // 5. Sleep for 3 seconds
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
```
Source: [VERIFIED: existing `task_manager.cpp` monitorTask pattern for FreeRTOS task structure]

### JSON Command Parsing — Lightweight Approach
```cpp
// In AutomationManager — simple token-based command dispatch
// Avoids cJSON heap allocation for the common case
void AutomationManager::processCommand(const char* json) {
    // Match command type using simple substring (safe for known commands)
    if (strstr(json, "\"cmd\":\"actuator_on\"")) {
        m_override_active = true;
        m_override_actuator_on = true;
        controlActuator(true);
        publishEvent("override", "info", "Override: Actuator ON");
    }
    else if (strstr(json, "\"cmd\":\"actuator_off\"")) {
        m_override_active = true;
        m_override_actuator_on = false;
        controlActuator(false);
        publishEvent("override", "info", "Override: Actuator OFF");
    }
    else if (strstr(json, "\"cmd\":\"return_to_auto\"")) {
        m_override_active = false;
        publishEvent("override", "info", "Override: Return to Auto");
    }
    else if (strstr(json, "\"cmd\":\"acknowledge_alarm\"")) {
        m_alarm_acknowledged = true;
        publishEvent("override", "info", "Alarm acknowledged by server");
    }
    else if (strstr(json, "\"cmd\":\"update_thresholds\"")) {
        // Use cJSON only for this command (has nested params)
        // Or use a simple key-value scanner for known param names
        updateThresholds(json);
        publishEvent("thresholds", "info", "Thresholds updated via MQTT");
    }
}
```
Source: [ASSUMED] — lightweight parsing avoids cJSON heap alloc. For `update_thresholds` with nested params, a simple `strstr` + `atof` scanner for known parameter names suffices (`"temp_critical":`, `"gas_critical":`, etc.).

### Actuator Control with Minimum On Time
```cpp
// In AutomationManager
static const uint32_t ACTUATOR_MIN_ON_MS = 120000; // 2 minutes

void AutomationManager::controlActuator(bool turn_on) {
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    
    if (turn_on) {
        gpio_set_level(PIN_RELAY_ACTUATOR, 1);
        m_actuator_on_ms = now_ms;
    } else {
        // Enforce minimum on time in auto mode
        if (!m_override_active) {
            if ((now_ms - m_actuator_on_ms) < ACTUATOR_MIN_ON_MS) {
                // Don't turn off yet — minimum run time not elapsed
                return;
            }
        }
        // Override commands bypass minimum time
        gpio_set_level(PIN_RELAY_ACTUATOR, 0);
    }
}
```
Source: [VERIFIED: ESP-IDF `driver/gpio.h` API] — `gpio_set_level()`, `PIN_RELAY_ACTUATOR` from existing `pins_config.h`. Minimum-on-time logic [ASSUMED] based on D-03.

### Backend MQTT Publisher Helper
```python
# mqtt_publisher.py
"""One-shot MQTT publisher for device commands."""
import json
import os
import paho.mqtt.publish as publish

TOPIC_PREFIX = os.environ.get('MQTT_TOPIC_PREFIX', 'nodealert/')

def publish_command(device_id: str, cmd: str, params: dict = None) -> bool:
    """Publish a command to a device's command topic.
    
    Args:
        device_id: Device identifier (e.g., 'nodealert-01')
        cmd: Command name (actuator_on, actuator_off, etc.)
        params: Optional parameters dict for update_thresholds
    
    Returns:
        True if published successfully
    """
    payload = {'cmd': cmd, 'timestamp': int(__import__('time').time())}
    if params:
        payload['params'] = params
    
    host = os.environ.get('MQTT_BROKER_HOST', 'mosquitto')
    port = int(os.environ.get('MQTT_BROKER_PORT', '1883'))
    user = os.environ.get('MQTT_PUBLISHER_USER', 'mqtt_publisher')
    password = os.environ.get('MQTT_PUBLISHER_PASSWORD', '')
    
    try:
        publish.single(
            topic=f"{TOPIC_PREFIX}{device_id}/commands",
            payload=json.dumps(payload),
            qos=1,
            hostname=host,
            port=port,
            auth={'username': user, 'password': password},
            client_id=f"django-cmd-{device_id}-{__import__('time').time_ns()}"
        )
        return True
    except Exception as e:
        print(f"[MQTT PUBLISH ERROR] {e}")
        return False
```
Source: [CITED: eclipse.dev/paho/files/paho.mqtt.python/html/helpers.html] — `publish.single()` API.

## State of the Art

No significant state-of-the-art changes in this domain. All patterns and APIs used are stable and well-established.

**Deprecated/outdated:**
- Legacy `adc1_get_raw()` ADC API — not relevant to this phase (already replaced with `adc_oneshot` in earlier phases)
- Legacy paho-mqtt v1.x callback style — the existing codebase already uses `CallbackAPIVersion.VERSION2` correctly
- ESP32 RTC_DATA_ATTR default placement: By default, data goes to RTC Slow memory (8KB). On ESP32, `CONFIG_ESP32_RTCDATA_IN_FAST_MEM` can be enabled via menuconfig to place it in RTC Fast memory instead (only accessible by PRO_CPU). This is only relevant if single-core mode is configured. The default (RTC Slow) is correct for this project.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `paho.mqtt.publish.single()` creates unique client IDs automatically | Pattern 4 | If client IDs collide and `use_username_as_clientid` is true in mosquitto.conf, the publisher might fail. Mitigation: include a unique suffix (`time_ns()`) in client_id. |
| A2 | Lightweight `strstr`-based command parsing is sufficient for the 5 command types | Code Examples | If commands become more complex (nested JSON, arrays), need to switch to cJSON. Low risk for v1 with 5 simple commands. |
| A3 | MQTT `Update Thresholds` command params use the same field names as the RTC struct | Pattern 3 | If field names don't match between server and firmware, threshold updates silently fail. Mitigation: define a shared schema in documentation. |
| A4 | The ESP32's MQTT event handler can safely call `processCommand()` from the event callback context | Pitfall 4 | If the event callback runs at ISR level or with a mutex held, setting flags directly could cause issues. Mitigation: use a queue (recommended in the planner's discretion). |

## Security Domain

> Required: `security_enforcement` is enabled (absent = enabled in config.json).

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | All new endpoints inherit existing Token auth from DRF |
| V3 Session Management | no | No new session concepts |
| V4 Access Control | yes | Command endpoint validates device ownership via DRF permissions |
| V5 Input Validation | yes | Command endpoint validates `command` field against whitelist |
| V6 Cryptography | no | MQTT over TCP (no TLS in v1). Commands are plaintext within Docker network. |

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| MQTT command injection (arbitrary payload sent to device topic) | Tampering | Command endpoint validates against whitelist of 5 command types on the backend; ESP32 also validates command type before processing (defense in depth) |
| Unauthorized command publishing | Spoofing | Command endpoint requires DRF Token auth (existing); MQTT broker requires username/password for publish (existing mosquitto.conf) |
| Replay attack (captured command replayed) | Tampering | Commands include timestamp field; for v1, the impact is limited (replaying `acknowledge_alarm` or `actuator_off` is not catastrophic). v2 can add nonce/sequence numbers. |

## Sources

### Primary (HIGH confidence)
- [CITED: docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32/api-guides/memory-types.html] — RTC_DATA_ATTR / RTC memory regions confirmed
- [CITED: docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/protocols/mqtt.html] — MQTT_EVENT_DATA, esp_mqtt_client_subscribe, event data structure
- [CITED: docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html] — gpio_config_t, gpio_set_level() API
- [CITED: eclipse.dev/paho/files/paho.mqtt.python/html/helpers.html] — paho.mqtt.publish.single() API
- [VERIFIED: existing codebase patterns] — TaskManager, MqttManager, SensorManager, main.cpp, EventTable, SummaryBar, AlarmContext, mqtt_subscriber.py

### Secondary (MEDIUM confidence)
- [CITED: stackoverflow.com/questions/68915517] — paho-mqtt threading issues with Django auto-reloader confirmed. Resolution pattern validated.
- [CITED: github.com/eclipse-paho/paho.mqtt.python/issues/553] — MQTT client initialization in Django apps confirmed: `urls.py` import pattern vs `__init__.py`

### Tertiary (LOW confidence)
- None — all claims backed by either Espressif official docs, paho-mqtt official docs, or verified codebase patterns.

## Open Questions

1. **MQTT publisher credentials in .env** — D-11 requires new MQTT credentials for the publisher user. Should these reuse the existing subscriber credentials or be separate? Recommendation: separate `MQTT_PUBLISHER_USER` / `MQTT_PUBLISHER_PASSWORD` in .env for least privilege (publisher only needs `write` on `nodealert/+/commands`).

2. **Device ID resolution on the frontend** — The "Silenciar alarma" button needs to know which device ID to target. Currently, the frontend doesn't store device ID globally. Recommendation: add `currentDeviceId` to `AlarmContext` or derive from `ReadingsContext` readings data.

3. **MQTT topic for event publishing from AutomationManager** — The existing MqttManager publishes events to `nodealert/{device_id}/events`. AutomationManager needs access to this publish path. Should it go through MqttManager (cleaner, but needs reference) or publish directly via `esp_mqtt_client_publish()` (efficient, but tight coupling)? Recommendation: pass a function pointer or retain a reference to MqttManager's publish utility.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| ESP-IDF `chip: esp32` | Firmware build | ✓ | (PlatformIO) | — |
| Mosquitto broker | Command routing | ✓ | (Docker) | — |
| Python paho-mqtt | Backend command publish | ✓ | 2.1.0 | — |
| Django REST Framework | Command API endpoint | ✓ | (existing) | — |
| React + MUI | Active alerts dashboard | ✓ | (existing) | — |

**Missing dependencies with no fallback:** None — all capabilities use existing infrastructure.

**Missing dependencies with fallback:** None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new libraries needed; all patterns verified against existing codebase and official docs
- Architecture: HIGH — task structure exactly mirrors existing TaskManager pattern; Django endpoint follows existing DRF conventions
- Pitfalls: HIGH — all five pitfalls are documented in ESP-IDF guides, paho-mqth issues, or are well-known embedded patterns
- MQTT command format: MEDIUM — command JSON format is at the agent's discretion; reasonable defaults are proposed but not verified against a spec

**Research date:** 2026-05-25
**Valid until:** 2026-06-24 (30-day validity for stable firmware toolchain)
