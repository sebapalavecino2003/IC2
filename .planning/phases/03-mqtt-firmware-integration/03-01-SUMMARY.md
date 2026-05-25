# Plan 03-01: ESP32 MQTT Publisher — Summary

**Phase:** 03-mqtt-firmware-integration  
**Plan:** 01  
**Status:** ✅ Complete

## Objective
Implement the MQTT module in the ESP32 firmware: dedicated FreeRTOS task with ESP-IDF native `mqtt_client.h`, periodic telemetry publication every 10s, command subscription, 20-message circular buffer for disconnect tolerance, and automatic reconnection with buffer drain.

## Files Modified

| File | Change |
|------|--------|
| `NodeAlert-Firmware/src/config/mqtt_broker_config.h` | Renamed from `mqtt_config.h` to avoid include collision with managed component. Added `MQTT_USER`, `MQTT_PASS`, `DEVICE_ID` with `#ifndef` guards. |
| `NodeAlert-Firmware/src/CMakeLists.txt` | Added `PRIV_REQUIRES mqtt` for ESP-IDF MQTT component. |
| `NodeAlert-Firmware/src/core/message_buffer.h` | **Created.** Circular buffer declaration (20 messages, 256-byte max payload, `MqttMessage` struct). |
| `NodeAlert-Firmware/src/core/message_buffer.cpp` | **Created.** Circular buffer implementation with bounds-checked `memcpy`, FIFO order, overflow discards oldest. |
| `NodeAlert-Firmware/src/managers/mqtt_manager.h` | **Created.** `MqttManager` class with `init()`/`startMqttTask()`, `MessageBuffer` integration, static task/event handlers. |
| `NodeAlert-Firmware/src/managers/mqtt_manager.cpp` | **Created.** FreeRTOS task: ESP-MQTT client init, subscribed to `nodealert/{DEVICE_ID}/commands`, accumulates latest sensor readings, publishes every 10s, buffers on disconnect, drains on reconnect. |
| `NodeAlert-Firmware/src/core/main.cpp` | Added `#include "managers/mqtt_manager.h"`, instantiated `MqttManager` with `init()` + `startMqttTask()`, renumbered section 11→12. |
| `NodeAlert-Firmware/src/idf_component.yml` | **Created.** Adds `espressif/mqtt: "^1.0.0"` dependency for ESP-IDF 6.x MQTT component. |
| `NodeAlert-Firmware/platformio.ini` | Unchanged (build flags already include all needed dirs). |

## Verification

| Check | Result |
|-------|--------|
| `MQTT_USER`/`MQTT_PASS`/`DEVICE_ID` in broker config | ✅ 2 matches each |
| `PRIV_REQUIRES mqtt` in CMakeLists.txt | ✅ 1 match |
| `message_buffer.h` exists with `MQTT_BUFFER_CAPACITY=20` | ✅ |
| `message_buffer.cpp` exists | ✅ |
| `mqtt_manager.h` declares `class MqttManager` + `startMqttTask` | ✅ |
| `mqtt_manager.cpp` uses `xTaskCreatePinnedToCore` | ✅ |
| `mqtt_manager.cpp` has `mqttEventHandler` | ✅ |
| `mqtt_manager.cpp` reports errors via `reportError("MQTT", ...)` | ✅ |
| `main.cpp` instantiates `MqttManager` + `init()` + `startMqttTask()` | ✅ |
| **PlatformIO build (`esp32dev` + `test`)** | ✅ **2/2 SUCCESS** |

## Deviations
- **File renamed:** `mqtt_config.h` → `mqtt_broker_config.h` to avoid include path collision with `espressif/mqtt` managed component's own `mqtt_config.h`. The managed component's `lib/include/mqtt_config.h` defines `MQTT_OUTBOX_MEMORY` which `mqtt_outbox.c` requires — our file was shadowing it.
- **MQTT events prefixed** `MQTT_EVENT_*` (not `ESP_MQTT_EVENT_*`): the external `espressif/mqtt` v1.0.0 uses unprefixed event names, unlike the older builtin component.
- **`%lu` format** for `uint32_t` timestamp: the toolchain treats `uint32_t` as `long unsigned int`, requiring `%lu` instead of `%u`.
- **`idf_component.yml` created** in `src/` to pull `espressif/mqtt` from IDF Component Registry (ESP-IDF 6.x removed the builtin mqtt component).

## Deliverables
- ESP32 boots with MQTT task on Core 0, priority 1, stack 6144 words
- Connects to broker via `MQTT_BROKER_URI`/`MQTT_PORT`/`MQTT_USER`/`MQTT_PASS`
- Subscribes to `nodealert/{DEVICE_ID}/commands` (QoS 1)
- Accumulates latest readings from all 3 sensors via queue
- Publishes JSON telemetry every 10s on `nodealert/{DEVICE_ID}/telemetry`
- Buffers up to 20 messages in circular RAM buffer during disconnection
- Drains buffer on `MQTT_EVENT_CONNECTED`
- Reports MQTT errors via `error_handler` with source `"MQTT"`, disconnect does NOT change system state (per D-12)
