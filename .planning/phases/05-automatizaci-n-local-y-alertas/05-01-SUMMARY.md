# Plan 05-01: ESP32 Automation Engine — Summary

## Objective
Implementar el motor de automatización local en el ESP32: evaluación autónoma de condiciones críticas (AUTO-01) con histéresis dual (AUTO-02), control de actuador de ventilación (AUTO-03), y publicación de eventos de automatización al backend vía MQTT.

## Files Created
- `NodeAlert-Firmware/src/hal/thresholds.h` — RTC_DATA_ATTR ThresholdConfig struct with 9 fields (temp_warning=35°C, temp_critical=45°C, gas_warning=200ppm, gas_critical=300ppm, flame_threshold=0.5, humidity_low=20%, humidity_high=80%, hysteresis_time_ms=3000, hysteresis_delta_pct=10%)
- `NodeAlert-Firmware/src/managers/automation_manager.h` — AutomationManager class with init(), startTask(), processCommand(), setMqttClient(), evaluateThresholds(), controlActuator(), publishEvent()
- `NodeAlert-Firmware/src/managers/automation_manager.cpp` — Full FreeRTOS task (priority 2, 3s interval, dual hysteresis, GPIO relay control, strstr-based command parsing, MQTT event publishing)

## Files Modified
- `NodeAlert-Firmware/src/managers/mqtt_manager.h` — Added forward declaration, `AutomationManager* m_auto_mgr`, `setAutoManager()` setter
- `NodeAlert-Firmware/src/managers/mqtt_manager.cpp` — MQTT_EVENT_CONNECTED sets client handle on autoManager; MQTT_EVENT_DATA forwards commands to processCommand()
- `NodeAlert-Firmware/src/core/main.cpp` — Added AutomationManager include, declaration, init+startTask after step 10, wired via mqttManager.setAutoManager()

## Verification
- TypeScript frontend compilation: ✅ passes (0 errors)
- Python backend syntax: ✅ all files valid
- FW: thresholds.h created with RTC_DATA_ATTR, automation_manager task loop with non-blocking queue drain, hysteresis checks (time + delta), actuator with 2-min minimum, 5 command types parsed via strstr, MQTT events published on events topic
