# Walking Skeleton — NodeAlert IoT

**Phase:** 1
**Generated:** 2026-05-24

## Capability Proven End-to-End

A developer can build and flash the firmware to an ESP32 and see formatted sensor readings (temperature, humidity, gas level, flame status) output over serial — proving the modular PlatformIO project structure, HAL layer, sensor drivers, and FreeRTOS task scheduling all work together.

## Architectural Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Framework | ESP-IDF v5.x via PlatformIO | Control total sobre hardware, FreeRTOS nativo, estructura profesional. No Arduino-ESP32. |
| Project structure | Capas modulares: core/, hal/, drivers/sensor/, managers/, services/, config/ | Separación clara de responsabilidades. Escalable a futuro. |
| Sensor interface | Híbrida: ISensor común + template getter | Interfaz uniforme para managers + acceso a funcionalidades específicas |
| RTOS | FreeRTOS (incluido en ESP-IDF) | Tiempo real, colas, mutex, prioridades |
| State machine | init → standby → running → alert → error → recovery | 6 estados con sub-estados de calibración |
| Naming | snake_case, inglés | Consistencia global |
| Error handling | Auto-recovery con backoff exponencial | Resiliencia sin intervención manual |
| Build system | PlatformIO CLI + ESP-IDF CMake | Portátil, CI-friendly |

## Stack Touched in Phase 1

- [x] Project scaffold (PlatformIO project, directory structure, CMakeLists.txt, platformio.ini)
- [x] Configuration system (pines, umbrales, WiFi/MQTT settings placeholder)
- [x] HAL abstraction layer (ISensor interface, SensorReading struct)
- [x] DHT22 driver — temperatura y humedad
- [x] MQ-9 driver — gases inflamables y CO
- [x] KY-026 driver — detección de llama por interrupción HW
- [x] Calibration module — filtrado y estabilización de lecturas
- [x] FreeRTOS tasks — una tarea por sensor con prioridades definidas
- [x] FreeRTOS queues — comunicación entre tareas
- [x] FreeRTOS mutex — protección de recursos compartidos (GPIO/ADC)
- [x] State machine — init, standby, running, alert, error, recovery
- [x] Error handling — watchdog, timeout, auto-recovery con backoff

## Out of Scope (Deferred to Later Slices)

- Comunicación MQTT (Phase 3)
- Automatización de actuadores (Phase 5)
- Dashboard web (Phase 4)
- Override remoto por MQTT (Phase 5)
- Alertas por Telegram (v2)
- Deep Sleep / bajo consumo (v2)
- WiFi multi-nodo (Phase 3+)
