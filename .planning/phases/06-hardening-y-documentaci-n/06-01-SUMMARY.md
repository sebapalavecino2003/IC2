# Plan 06-01 Summary: ESP32 Watchdog + Status MQTT

**Estado:** Completado

## Cambios Realizados

### Task 1: getTaskHandle() accessors + m_last_status_ms
- **sensor_manager.h**: Added `getTaskHandle(SensorType)` with inline switch mapping 4 sensor types to 3 task handles
- **task_manager.h**: Added `getTaskHandle() const` returning `task_handle`
- **automation_manager.h**: Added `getTaskHandle() const` returning `m_task_handle`
- **mqtt_manager.h**: Added `getTaskHandle()`, `publishStatus()`, `buildStatusJson()` declarations; added `m_last_status_ms`, `m_last_state` private members; added `#include "core/system_core.h"`

### Task 2: WDT init + task subscriptions + Last Will
- **main.cpp**: Added `#include "esp_task_wdt.h"`, `esp_task_wdt_init()` after NVS init, 7 `esp_task_wdt_add()` calls (3 sensor + 1 monitor + 1 automation + 1 MQTT + 1 main loop), `esp_task_wdt_reset()` in main loop
- **mqtt_manager.cpp**: Added Last Will config (`topic`, `msg`, `qos=1`, `retain=false`) after keepalive

### Task 3: buildStatusJson + publishStatus + periodic status
- **mqtt_manager.cpp**: Added `#include "esp_heap_caps.h"`, `"esp_system.h"`, `"esp_wifi.h"`; implemented `buildStatusJson()` (fields: device_id, status, uptime_ms, free_heap, system_state, wifi_rssi, last_reset_reason, errores_activos); implemented `publishStatus()` with QoS 1; added periodic status publishing in `mqttTask()` loop (60s interval + immediate on state transition); initialized `m_last_status_ms(0)` and `m_last_state(SystemState::INIT)` in constructor

## Verificación
- `esp_task_wdt_init`: 1
- `esp_task_wdt_add`: 7
- `esp_task_wdt_reset`: 1
- `last_will`: 4 (topic + msg + qos + retain)
- `publishStatus`: 2 (decl + impl)
- `wifi_rssi`: 1
- `errores_activos`: 1
- `state_changed`: 2
- `esp_get_free_heap_size`: 1
- `esp_reset_reason`: 1
