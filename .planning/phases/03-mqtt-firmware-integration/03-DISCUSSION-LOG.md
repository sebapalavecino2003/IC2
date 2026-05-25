# Phase 3: Comunicación MQTT ESP32 → Backend - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-24
**Phase:** 3-mqtt-firmware-integration
**Areas discussed:** Topic structure & message format, MQTT task on ESP32, Django ingestion service, State machine integration

---

## Topic Structure & Message Format

| Option | Description | Selected |
|--------|-------------|----------|
| Single telemetry topic | `nodealert/{device_id}/telemetry` with flat JSON | ✓ |
| Per-sensor topics | Separate topic per sensor type | |
| Flat object | `{device_id, timestamp, temperature, humidity, gas_ppm, flame_detected}` | ✓ |
| Array of readings | `{device_id, timestamp, readings: [{sensor_type, value, unit}]}` | |
| Dedicated sub-topics | `nodealert/{device_id}/commands` and `nodealert/{device_id}/events` | ✓ |
| Single shared topic | `nodealert/{device_id}/messages` with type field | |
| Match sensor rate | Publish every ~2s | |
| Aggregated (10s) | Publish every 10s with latest values | ✓ |

**User's choice:** Single telemetry topic with flat JSON object, dedicated command and event sub-topics, publish every 10s (aggregated).
**Notes:** User emphasized keeping it simple for MVP. The flat object format means backend needs to split into per-sensor readings on ingestion.

---

## MQTT Task on ESP32

| Option | Description | Selected |
|--------|-------------|----------|
| Dedicated FreeRTOS task | Separate task with queue communication | ✓ |
| Integrated in main loop | MQTT publish in main loop after sensor_queue | |
| Use existing error_handler | Report MQTT errors via error_handler('MQTT', ...) | ✓ |
| Dedicated MQTT retry logic | Separate retry logic outside error_handler | |
| 4096 words stack | Same as sensor tasks | |
| 6144 words stack | More margin for JSON + payload building | ✓ |
| Core 0 (Pro CPU / WiFi) | Minimizes WiFi communication latency | ✓ |
| Core 1 (App CPU) | Same core as sensor tasks | |

**User's choice:** Dedicated FreeRTOS task on Core 0, 6144 words stack, using error_handler for error tracking.
**Notes:** ESP-IDF native mqtt_client (no external libs). Task pinned to Core 0 since MQTT uses WiFi internally.

---

## Django Ingestion Service

| Option | Description | Selected |
|--------|-------------|----------|
| Django management command | `python manage.py mqtt_subscriber` with paho-mqtt | ✓ |
| Separate Python process | Independent script writing directly to MySQL | |
| Auto-create devices | Create Device for unknown device_id | |
| Only known devices | Reject messages from unregistered devices | ✓ |
| ESP32 publishes events | ESP32 evaluates conditions and publishes events | ✓ |
| Backend calculates events | Backend infers events from reading thresholds | |
| Same shared credentials | Reuse MQTT_BROKER_USER/PASSWORD from .env | |
| Dedicated subscriber user | Separate mqtt_subscriber user for Django | ✓ |

**User's choice:** Django management command using paho-mqtt, only known devices, ESP32 publishes events directly, dedicated MQTT subscriber user.
**Notes:** Entrypoint.sh will start the management command after migrations. A second MQTT user (mqtt_subscriber) needs to be added to setup.sh's mosquitto_passwd generation.

---

## State Machine Integration

| Option | Description | Selected |
|--------|-------------|----------|
| MQTT as condition for RUNNING | STANDBY→RUNNING requires MQTT connected | |
| MQTT runs independently | No state change on disconnect, sensors keep sampling | ✓ |
| After WiFi, before RUNNING | Initiate MQTT connection during STANDBY→RUNNING transition | ✓ |
| At first publish attempt | Lazy connect when data is ready | |
| Publish offline via API | ESP32 sets is_connected flag readable by Django | |
| Report error to error_handler | MQTT disconnection handled by error_handler | ✓ |

**User's choice:** MQTT runs independently of state machine. Connect attempt starts after WiFi connects, before RUNNING (but doesn't block). On disconnection, report to error_handler.
**Notes:** The system keeps sampling sensors during MQTT disconnection. Messages are buffered locally (ring buffer, 20 messages). Auto-reconnect handled by ESP-IDF mqtt_client + error_handler backoff.

---

## Deferred Ideas

None — discussion stayed within phase scope
