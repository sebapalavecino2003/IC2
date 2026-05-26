# Phase 5: Automatización Local y Alertas - Pattern Map

**Mapped:** 2026-05-25
**Files analyzed:** 14 new/modified
**Analogs found:** 14 / 14

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `NodeAlert-Firmware/src/managers/automation_manager.h` | manager | event-driven + control | `sensor_manager.h` | exact |
| `NodeAlert-Firmware/src/managers/automation_manager.cpp` | manager | event-driven + control | `task_manager.cpp` | exact |
| `NodeAlert-Firmware/src/hal/thresholds.h` | config | static | `sensor_reading.h` + `pins_config.h` | role-match |
| `NodeAlert-Firmware/src/core/main.cpp` | main | initialization | `main.cpp` (existing) | self-modify |
| `NodeAlert-Firmware/src/managers/mqtt_manager.cpp` | manager | event-driven | `mqtt_manager.cpp` (existing) | self-modify |
| `NodeAlert-Backend/core/mqtt_publisher.py` | utility | MQTT publish | `mqtt_subscriber.py` | role-match |
| `NodeAlert-Backend/core/views.py` | controller | request-response | `views.py` (existing) | self-modify |
| `NodeAlert-Backend/core/serializers.py` | serializer | validation | `serializers.py` (existing) | self-modify |
| `NodeAlert-Frontend/src/components/ActiveAlertsPanel.tsx` | component | request-response | `AlertPanel.tsx` + `EventTable.tsx` | exact |
| `NodeAlert-Frontend/src/context/AlarmContext.tsx` | provider | state management | `AlarmContext.tsx` (existing) | self-modify |
| `NodeAlert-Frontend/src/components/SummaryBar.tsx` | component | display | `SummaryBar.tsx` (existing) | self-modify |
| `NodeAlert-Frontend/src/components/EventTable.tsx` | component | display | `EventTable.tsx` (existing) | self-modify |
| `NodeAlert-Frontend/src/components/AlertPanel.tsx` | component | display | `AlertPanel.tsx` (existing) | self-modify |
| `NodeAlert-Frontend/src/types/index.ts` | types | — | `types/index.ts` (existing) | self-modify |

---

## Pattern Assignments

### `NodeAlert-Firmware/src/managers/automation_manager.h` (manager, event-driven + control)

**Analog:** `NodeAlert-Firmware/src/managers/task_manager.h` (lines 1-62)

**Imports / header guard pattern** (task_manager.h lines 1-18):
```cpp
/**
 * @file automation_manager.h
 * @brief [description]
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "hal/thresholds.h"
#include <cstdint>

// Forward declarations
class MqttManager;
class ErrorHandler;
```

**Class structure pattern** (task_manager.h lines 21-61):
```cpp
class AutomationManager {
public:
    AutomationManager();
    void init(QueueHandle_t sensor_queue, ErrorHandler* eh);
    void startTask();

    // Called from MQTT event handler to inject override commands
    void processCommand(const char* command_json);

private:
    QueueHandle_t           m_sensor_queue;
    ErrorHandler*           m_error_handler;
    TaskHandle_t            m_task_handle;

    // Override state (not RTC-persisted per D-10)
    bool                    m_override_active;
    bool                    m_override_actuator_on;
    bool                    m_alarm_acknowledged;
    uint32_t                m_actuator_on_ms;

    static void automationTask(void* pvParams);
    void evaluateThresholds();
    void controlActuator(bool turn_on);
    void publishEvent(const char* type, const char* severity, const char* message);
};
```

---

### `NodeAlert-Firmware/src/managers/automation_manager.cpp` (manager, event-driven + control)

**Analog:** `NodeAlert-Firmware/src/managers/task_manager.cpp` (lines 1-112)

**Constructor pattern** (task_manager.cpp lines 23-29):
```cpp
AutomationManager::AutomationManager()
    : m_sensor_queue(nullptr)
    , m_error_handler(nullptr)
    , m_task_handle(nullptr)
    , m_override_active(false)
    , m_override_actuator_on(false)
    , m_alarm_acknowledged(false)
    , m_actuator_on_ms(0)
{
}
```

**Init pattern** (task_manager.cpp lines 31-36):
```cpp
void AutomationManager::init(QueueHandle_t sensor_queue, ErrorHandler* eh)
{
    m_sensor_queue  = sensor_queue;
    m_error_handler = eh;
}
```

**Task creation pattern** (task_manager.cpp lines 38-41):
```cpp
void AutomationManager::startTask()
{
    xTaskCreatePinnedToCore(automationTask, "auto_task", 4096, this, 2, &m_task_handle, 1);
}
```

**Task body pattern — FreeRTOS infinite loop with time tracking** (task_manager.cpp lines 47-111):
```cpp
void AutomationManager::automationTask(void* pvParams) {
    AutomationManager* self = static_cast<AutomationManager*>(pvParams);
    SensorReading reading;
    bool alert_active = false;
    uint32_t alert_entry_time = 0;
    uint32_t actuator_on_time = 0;

    while (1) {
        // 1. Consume any pending sensor readings (non-blocking peek)
        while (xQueueReceive(self->m_sensor_queue, &reading,
                             pdMS_TO_TICKS(0)) == pdTRUE) {
            // Store latest values
            // ...
        }

        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

        // 2. Check override state — if active, skip auto-evaluation
        if (!self->m_override_active) {
            // 3. Evaluate thresholds
            // ...
        }

        // 4. Sleep for 3 seconds
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
```

**GPIO control pattern** (from `main.cpp` line 139 → use `gpio_set_level`, pins from `pins_config.h`):
```cpp
// Use ESP-IDF gpio_set_level() for actuator control
#include "driver/gpio.h"
#include "config/pins_config.h"

void AutomationManager::controlActuator(bool turn_on) {
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    if (turn_on) {
        gpio_set_level(PIN_RELAY_ACTUATOR, 1);
        m_actuator_on_ms = now_ms;
    } else {
        // Enforce minimum on time in auto mode (2 minutes)
        if (!m_override_active) {
            if ((now_ms - m_actuator_on_ms) < 120000) {
                return;
            }
        }
        gpio_set_level(PIN_RELAY_ACTUATOR, 0);
    }
}
```

---

### `NodeAlert-Firmware/src/hal/thresholds.h` (NEW) (config, static)

**Analog pattern from:** `NodeAlert-Firmware/src/hal/sensor_reading.h` (struct pattern) + `NodeAlert-Firmware/src/config/pins_config.h` (config header pattern)

**Struct definition pattern** (sensor_reading.h lines 39-54):
```cpp
#pragma once
#include <cstdint>

struct ThresholdConfig {
    float temp_warning;          // default: 35.0f
    float temp_critical;         // default: 45.0f
    float gas_warning;           // default: 200.0f
    float gas_critical;          // default: 300.0f
    float flame_threshold;       // default: 0.5f
    float humidity_low;          // default: 20.0f
    float humidity_high;         // default: 80.0f
    uint32_t hysteresis_time_ms;   // default: 3000
    float    hysteresis_delta_pct; // default: 0.10f
};

// RTC_DATA_ATTR places this in RTC slow memory
// Survives deep sleep but NOT power loss
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

---

### `NodeAlert-Firmware/src/core/main.cpp` (MODIFY) — add AutomationManager

**Analog:** `main.cpp` lines 65-99 (existing SensorManager + MqttManager initialization pattern)

**Import pattern** (add to existing imports, lines 28-31):
```cpp
#include "managers/sensor_manager.h"
#include "managers/task_manager.h"
#include "managers/mqtt_manager.h"
#include "managers/automation_manager.h"   // NEW
#include "services/serial_manager.h"
```

**Init + task creation pattern** (follow existing block structure, lines 94-99):
```cpp
    /* ================================================================== */
    /* 10. Create AutomationManager — threshold evaluation & actuator     */
    /* ================================================================== */
    AutomationManager autoManager;
    autoManager.init(sensorManager.getReadingQueue(), &errorHandler);
    autoManager.startTask();
```

Place after step 10 (existing `taskManager.startMonitorTask()` at line 92) and before step 11 (MqttManager at lines 97-99). The ordering: Monitor (bg health) → Automation (threshold eval) → MQTT (telemetry/commands).

---

### `NodeAlert-Firmware/src/managers/mqtt_manager.cpp` (MODIFY) — command routing

**Analog:** Self-modify lines 85-121 (`mqttEventHandler` — MQTT_EVENT_DATA handler)

**Current MQTT_EVENT_DATA handler** (lines 110-116):
```cpp
        case MQTT_EVENT_DATA: {
            esp_mqtt_event_t* msg = (esp_mqtt_event_t*)event_data;
            printf("[MQTT] Command: %.*s -> %.*s\n",
                   msg->topic_len, msg->topic,
                   msg->data_len, msg->data);
            break;
        }
```

**Modified pattern — parse command and forward to AutomationManager** (add `#include "managers/automation_manager.h"` and a member reference):
```cpp
        case MQTT_EVENT_DATA: {
            esp_mqtt_event_t* msg = (esp_mqtt_event_t*)event_data;
            char cmd[128];
            size_t len = (msg->data_len < sizeof(cmd) - 1) ? msg->data_len : sizeof(cmd) - 1;
            memcpy(cmd, msg->data, len);
            cmd[len] = '\0';
            // Forward to automation manager for processing
            // (requires AutomationManager* reference stored in MqttManager)
            if (self->m_auto_mgr) {
                self->m_auto_mgr->processCommand(cmd);
            }
            break;
        }
```

---

### `NodeAlert-Backend/core/mqtt_publisher.py` (NEW) (utility, MQTT publish)

**Analog:** `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py` (lines 1-48 for import/env pattern, lines 58-68 for topic/payload pattern)

**Import + env pattern** (mqtt_subscriber.py lines 1-16):
```python
"""Helper module for publishing MQTT commands from Django views."""
import json
import os
import paho.mqtt.publish as publish
```

**Publish function using `paho.mqtt.publish.single()` — connection-less pattern** (contrast with subscriber's `mqtt.Client()` + `loop_forever()`):
```python
TOPIC_PREFIX = os.environ.get('MQTT_TOPIC_PREFIX', 'nodealert/')

def publish_command(device_id: str, cmd: str, params: dict = None) -> bool:
    """Publish a command to a device's command topic.
    
    Uses paho.mqtt.publish.single() for a connection-less publish.
    Avoids threading issues with Django's auto-reloading dev server.
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
        )
        return True
    except Exception as e:
        print(f"[MQTT PUBLISH ERROR] {e}")
        return False
```

---

### `NodeAlert-Backend/core/views.py` (MODIFY) — command endpoint

**Analog:** Self-modify — existing `DeviceViewSet` (lines 33-47) + existing `@action` or viewset pattern

**Existing DeviceViewSet** (lines 33-47):
```python
class DeviceViewSet(viewsets.ModelViewSet):
    """Full CRUD viewset for Device model."""
    queryset = Device.objects.all()
    serializer_class = DeviceSerializer
    filter_backends = [django_filters.rest_framework.DjangoFilterBackend,
                       filters.SearchFilter, filters.OrderingFilter]
    filterset_fields = ['is_active', 'location']
    search_fields = ['name', 'device_id', 'location']
    ordering_fields = ['created_at', 'name', 'device_id']
```

**New `@action` to add** (follows existing decorator import from line 3):
```python
from rest_framework.decorators import action    # already imported
from rest_framework.response import Response     # already imported
from .mqtt_publisher import publish_command      # NEW

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
        
        params = request.data.get('params', {}) if command == 'update_thresholds' else None
        success = publish_command(device.device_id, command, params)
        if not success:
            return Response(
                {'error': 'Failed to publish command'},
                status=status.HTTP_502_BAD_GATEWAY
            )
        
        return Response({
            'status': 'published',
            'device': device.device_id,
            'command': command
        })
```

---

### `NodeAlert-Backend/core/serializers.py` (MODIFY) — add CommandSerializer

**Analog pattern from:** Existing serializers (lines 7-37)

**New serializer for command validation:**
```python
class CommandSerializer(serializers.Serializer):
    """Serializer for device MQTT commands."""
    command = serializers.ChoiceField(choices=[
        'actuator_on', 'actuator_off', 'return_to_auto',
        'acknowledge_alarm', 'update_thresholds',
    ])
    params = serializers.JSONField(required=False)
```

---

### `NodeAlert-Frontend/src/components/ActiveAlertsPanel.tsx` (NEW) (component, request-response)

**Analog:** `AlertPanel.tsx` (lines 1-33) + `EventTable.tsx` (lines 1-78)

**Component + context pattern** (AlertPanel.tsx lines 1-10):
```tsx
import { useState } from 'react'
import { Card, CardContent, Typography, Chip, Box, IconButton } from '@mui/material'
import CheckCircleIcon from '@mui/icons-material/CheckCircle'
import VolumeOffIcon from '@mui/icons-material/VolumeOff'
import { useReadings } from '../context/ReadingsContext'
import { useAlarm } from '../context/AlarmContext'
import api from '../services/api'
import type { Event } from '../types'
```

**Severity colors pattern** (EventTable.tsx lines 14-18):
```tsx
const SEVERITY_COLORS: Record<string, 'info' | 'warning' | 'error'> = {
  info: 'info',
  warning: 'warning',
  critical: 'error',
}
```

**Filter unresolved + card layout pattern** (AlertPanel.tsx lines 7-9, 22-29):
```tsx
export default function ActiveAlertsPanel() {
  const { events } = useReadings()
  const { deviceStatus, currentDeviceId } = useAlarm()
  const unresolved = events.filter(e => !e.resolved)

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
          <Box key={event.id} sx={{ display: 'flex', alignItems: 'center', ... }}>
            <Chip label={event.severity} color={SEVERITY_COLORS[event.severity]} size="small" />
            <Typography variant="body2">{event.message}</Typography>
          </Box>
        ))}
      </CardContent>
    </Card>
  )
}
```

**"Silenciar alarma" API call pattern** (follows AlertPanel.tsx lines 13-19):
```tsx
const handleSilenceAlarm = async () => {
  try {
    await api.post(`/devices/${deviceId}/command/`, { command: 'acknowledge_alarm' })
  } catch {
    // Silently fail
  }
}
```

---

### `NodeAlert-Frontend/src/context/AlarmContext.tsx` (MODIFY) — add override state + acknowledge function

**Analog:** Self-modify — extend existing `AlarmState` interface (lines 5-11) and `AlarmProvider` (lines 48-65)

**Add override state to interface** (lines 5-11):
```tsx
interface AlarmState {
  alarmActive: boolean
  alarmMessage: string | null
  statuses: SensorStatuses
  latest: LatestReadings
  deviceStatus: 'online' | 'offline'
  // NEW:
  overrideActive: boolean
  currentDeviceId: number | null
  acknowledgeAlarm: () => Promise<void>
}
```

**Add to provider value** (line 62):
```tsx
  const [overrideActive, setOverrideActive] = useState(false)
  const currentDeviceId = 1 // or derive from readings

  const acknowledgeAlarm = useCallback(async () => {
    try {
      await api.post(`/devices/${currentDeviceId}/command/`, { command: 'acknowledge_alarm' })
    } catch {
      // Silently fail
    }
  }, [currentDeviceId])

  return (
    <AlarmContext.Provider value={{
      alarmActive, alarmMessage, statuses, latest, deviceStatus,
      overrideActive, currentDeviceId, acknowledgeAlarm,
    }}>
      {children}
    </AlarmContext.Provider>
  )
```

**Add imports** (line 1):
```tsx
import { createContext, useContext, useMemo, useState, useCallback, type ReactNode } from 'react'
import api from '../services/api'
```

---

### `NodeAlert-Frontend/src/components/SummaryBar.tsx` (MODIFY) — override indicator chip

**Analog:** Self-modify — add chip alongside existing ESP32 status chip (lines 11-16)

**Override chip pattern — add after line 16**:
```tsx
      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 1 }}>
        <Box sx={{ display: 'flex', gap: 1, alignItems: 'center' }}>
          <Chip
            label={deviceStatus === 'online' ? 'ESP32 Conectado' : 'ESP32 Desconectado'}
            color={deviceStatus === 'online' ? 'success' : 'error'}
            size="small"
            variant="outlined"
          />
          {/* NEW: Override status chip */}
          {deviceStatus === 'online' && (
            <Chip
              label={overrideActive ? 'Override Manual' : 'Modo Auto'}
              color={overrideActive ? 'warning' : 'success'}
              size="small"
              variant={overrideActive ? 'filled' : 'outlined'}
            />
          )}
        </Box>
```

Import `overrideActive` from `useAlarm` (line 6):
```tsx
const { latest, statuses, deviceStatus, alarmActive, alarmMessage, overrideActive } = useAlarm()
```

---

### `NodeAlert-Frontend/src/components/EventTable.tsx` (MODIFY) — severity filtering

**Analog:** Self-modify — severity filter already exists (lines 22-38). The existing filter is for events; the enhancement needed is ensuring automation-specific event types (`actuator_on`, `override`, `threshold_crossed`) display correctly. Minimal changes needed — the existing severity filter is already correct.

**Potential addition — event_type column enhancement** (line 62). Automation events will now include:
- `threshold_crossed` (critical)
- `actuator_on` / `actuator_off` (info/warning)
- `override` (warning)
- `thresholds` (info)

The existing `event_type` display at line 62 already renders raw `event_type` strings correctly.

---

### `NodeAlert-Frontend/src/components/AlertPanel.tsx` (MODIFY) — evolve into active alerts view

**Analog:** Self-modify — add tab navigation between "historial" (existing EventTable) and "alertas activas" (new ActiveAlertsPanel)

**Add Tabs to existing AlertPanel**:
```tsx
import { useState } from 'react'
import { Card, CardContent, Typography, Tabs, Tab } from '@mui/material'
import EventTable from './EventTable'
import ActiveAlertsPanel from './ActiveAlertsPanel'
import { useReadings } from '../context/ReadingsContext'
import api from '../services/api'

export default function AlertPanel() {
  const { events } = useReadings()
  const [tab, setTab] = useState(0)

  return (
    <Card>
      <CardContent>
        <Typography variant="h6" gutterBottom>Alertas y Eventos</Typography>
        <Tabs value={tab} onChange={(_, v) => setTab(v)} sx={{ mb: 2 }}>
          <Tab label="Alertas Activas" />
          <Tab label="Historial" />
        </Tabs>
        {tab === 0 && <ActiveAlertsPanel />}
        {tab === 1 && <EventTable events={events} onResolve={handleResolve} />}
      </CardContent>
    </Card>
  )
}
```

---

### `NodeAlert-Frontend/src/types/index.ts` (MODIFY) — add command types

**Analog:** Self-modify — add types alongside existing interfaces (lines 1-46)

**New types:**
```ts
export interface CommandPayload {
  command: 'actuator_on' | 'actuator_off' | 'return_to_auto' | 'acknowledge_alarm' | 'update_thresholds'
  params?: Record<string, number>
}

export interface OverrideState {
  active: boolean
  mode: 'auto' | 'manual'
  command?: string
}
```

---

## Shared Patterns

### FreeRTOS Task Creation (Firmware — all managers)
**Source:** `NodeAlert-Firmware/src/managers/task_manager.cpp` lines 38-41
**Apply to:** `automation_manager.cpp`
```cpp
void AutomationManager::startTask()
{
    xTaskCreatePinnedToCore(automationTask, "auto_task", 4096, this, 2, &m_task_handle, 1);
}
```
- Priority 2 (intermediate, between DHT22 at 2 and MQ9 at 3)
- Stack size 4096 (same as DHT22 task in sensor_manager.cpp line 87)
- Core 1 (same as all sensor tasks)

### Serialization Validation (Backend — DRF serializers)
**Source:** `NodeAlert-Backend/core/serializers.py` lines 7-37
**Apply to:** New `CommandSerializer`
```python
class CommandSerializer(serializers.Serializer):
    command = serializers.ChoiceField(choices=[
        'actuator_on', 'actuator_off', 'return_to_auto',
        'acknowledge_alarm', 'update_thresholds',
    ])
    params = serializers.JSONField(required=False)
```

### API Service Pattern (Frontend — axios instance)
**Source:** `NodeAlert-Frontend/src/services/api.ts` lines 1-29
**Apply to:** All API calls in new/modified components
```typescript
const api = axios.create({
  baseURL: API_BASE,
  headers: { 'Content-Type': 'application/json' },
})
// Token auth via interceptor
api.interceptors.request.use((config) => {
  const token = localStorage.getItem('token')
  if (token) config.headers.Authorization = `Token ${token}`
  return config
})
```

### MUI Chip Color Mapping (Frontend — severity badges)
**Source:** `NodeAlert-Frontend/src/components/EventTable.tsx` lines 14-18
**Apply to:** `ActiveAlertsPanel.tsx`, `SummaryBar.tsx`
```tsx
const SEVERITY_COLORS: Record<string, 'info' | 'warning' | 'error'> = {
  info: 'info',
  warning: 'warning',
  critical: 'error',
}
```

### MQTT Broker Auth (Backend + Firmware — connection config)
**Source:** `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py` lines 38-39, 41-43
**Apply to:** `mqtt_publisher.py` — separate env vars for publisher user
```python
sub_user = os.environ.get('MQTT_PUBLISHER_USER', 'mqtt_publisher')
sub_pass = os.environ.get('MQTT_PUBLISHER_PASSWORD', '')
```

---

## No Analog Found

All files have close analogs in the existing codebase. The closest matches are:

| File | Role | Data Flow | Closest Match | Notes |
|------|------|-----------|---------------|-------|
| `automation_manager.h/.cpp` | manager | event-driven | `task_manager.h/.cpp` | Same FreeRTOS task pattern, same init/startTask lifecycle |
| `thresholds.h` | config | static | `sensor_reading.h` | Same struct-definition-in-header pattern |
| `mqtt_publisher.py` | utility | MQTT publish | `mqtt_subscriber.py` | Different paho-mqtt API (single vs Client+loop) but same env var pattern |
| `ActiveAlertsPanel.tsx` | component | display | `AlertPanel.tsx` | Same card+context+severity-chip pattern |

---

## Metadata

**Analog search scope:** `NodeAlert-Firmware/src/`, `NodeAlert-Backend/core/`, `NodeAlert-Frontend/src/`
**Files scanned:** 18 (7 firmware, 3 backend, 8 frontend)
**Pattern extraction date:** 2026-05-25
