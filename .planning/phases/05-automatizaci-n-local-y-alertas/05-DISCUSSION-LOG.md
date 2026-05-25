# Phase 5: Automatización Local y Alertas - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-25
**Phase:** 05-automatizaci-n-local-y-alertas
**Areas discussed:** Threshold Location, Hysteresis Strategy, Actuator Behavior, Override Protocol, Dashboard Alerts, Automation Task Design

---

## Threshold Location

| Option | Description | Selected |
|--------|-------------|----------|
| Hardcoded in firmware | Thresholds in config header. Deterministic, no server dependency. | |
| Configurable via MQTT | Server pushes values. Enables remote tuning. | |
| Hybrid — hardcoded defaults + MQTT overrides | Sensible defaults + runtime updates. | ✓ |

**User's choice:** Hybrid — hardcoded defaults + MQTT overrides
**Notes:** All thresholds configurable: temp warning/critical, gas warning/critical, flame, humidity, actuator hysteresis duration and delta margin. Defaults match frontend AlarmContext.

---

## Hysteresis Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Time-based | Condition must persist N seconds. | |
| Delta-based | Value must return below threshold by margin. | |
| Both — time + delta | Condition must persist AND clear with margin. | ✓ |

**User's choice:** Both — time + delta
**Notes:** Default duration: 3 seconds. Default delta: 10% below threshold before clearing.

---

## Actuator Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Binary on/off relay | Simple on/off. | |
| On/off with minimum run time | Runs at least N min after activation. | ✓ |
| On/off + delayed shutoff | Stays on N seconds after condition clears. | |

**User's choice:** On/off with minimum run time
**Notes:** Minimum run time: 2 minutes. Prevents relay chatter.

---

## Override Protocol

| Option | Description | Selected |
|--------|-------------|----------|
| MQTT command topic | Server publishes to nodealert/{device_id}/commands. | ✓ |
| REST API + polling | ESP32 polls commands periodically. | |
| Both — MQTT + API fallback | MQTT primary, API as backup. | |

**User's choice:** MQTT command topic (Recommended)
**Notes:** Commands: Actuator ON/OFF, Return to Auto, Acknowledge Alarm, Update Thresholds. Override resets to Auto on boot.

---

## Dashboard Alerts

| Option | Description | Selected |
|--------|-------------|----------|
| Active alerts tab | Dedicated section for unresolved alerts. | ✓ |
| Override status indicator | Show when ESP32 is in manual override mode. | ✓ |
| Event severity filtering | Enhanced filtering in EventTable. | ✓ |
| Alarm acknowledgment button | Send MQTT acknowledge from dashboard. | ✓ |
| None — existing UI is sufficient | No dashboard changes needed. | |

**User's choice:** All enhancements selected

---

## Automation Task Design

| Option | Description | Selected |
|--------|-------------|----------|
| New dedicated AutomationManager task | Separate FreeRTOS task. | ✓ |
| Integrate into monitor task | Add logic to existing monitor. | |
| Integrate into sensor task | Check thresholds in sensor loop. | |

**User's choice:** New dedicated AutomationManager task (Recommended)
**Notes:** Priority 2, every 3s interval. Reads from sensor_queue.

---

## the agent's Discretion

- Internal structure of AutomationManager (files, classes, methods)
- Exact RTC_DATA_ATTR struct implementation
- Exact REST command endpoint (route, validation)
- Exact active alerts UI component
- Stack size of Automation task
- Exact JSON format for MQTT command messages

## Deferred Ideas

None
