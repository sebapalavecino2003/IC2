# Plan 05-02: Backend Command API + Frontend Override — Summary

## Objective
Implementar el endpoint REST de comandos MQTT en el backend (AUTO-04) y la interfaz de override en el frontend: botón "Silenciar alarma", indicador de override Manual/Auto en SummaryBar, estado en AlarmContext.

## Files Created
- `NodeAlert-Backend/core/mqtt_publisher.py` — One-shot MQTT publisher via paho.mqtt.publish.single(), accepts device_id, cmd, params, returns bool

## Files Modified
- `NodeAlert-Backend/core/serializers.py` — Added CommandSerializer with ChoiceField whitelist (5 command types from D-09)
- `NodeAlert-Backend/core/views.py` — Added DeviceViewSet.command @action (POST /api/v1/devices/{id}/command/), validates via CommandSerializer, publishes via publish_command()
- `NodeAlert-Frontend/src/types/index.ts` — Added OverrideCommand type, CommandPayload, OverrideState interfaces
- `NodeAlert-Frontend/src/context/AlarmContext.tsx` — Added overrideActive state, currentDeviceId, acknowledgeAlarm() async callback that POSTs to backend command endpoint
- `NodeAlert-Frontend/src/components/SummaryBar.tsx` — Added override status Chip ("Override Manual" warning/filled or "Modo Auto" success/outlined) with BuildIcon/PlayArrowIcon, shown alongside ESP32 connection chip

## Verification
- TypeScript compilation: ✅ passes
- Python syntax: ✅ all files valid
- CommandSerializer validates: actuator_on/off, return_to_auto, acknowledge_alarm, update_thresholds (5 commands)
- CommandSerializer rejects: invalid command strings
