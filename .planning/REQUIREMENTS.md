# Requirements: NodeAlert IoT

**Defined:** 2026-05-24
**Core Value:** Detectar condiciones ambientales peligrosas y actuar preventivamente antes de que escalen a emergencias, incluso sin conexión al servidor central.

## v1 Requirements

Requirements for initial release. Each maps to a roadmap phase.

### Firmware — Arquitectura y Plataforma

- [x] **FWK-01**: Proyecto PlatformIO con estructura profesional separando `.h`/`.cpp` por módulos
- [x] **FWK-02**: Capa HAL (Hardware Abstraction Layer) para aislar drivers del hardware específico
- [x] **FWK-03**: Sistema de configuración centralizada (WiFi, MQTT, pines, umbrales)
- [x] **FWK-04**: Máquina de estados del sistema (init, running, error, sleep, alert)
- [ ] **FWK-05**: Gestión de errores y recuperación con watchdog y reintentos

### Firmware — Drivers de Sensores

- [x] **DRV-01**: Driver modular para DHT22 (temperatura y humedad) con HAL
- [x] **DRV-02**: Driver modular para MQ-9 (gases inflamables y CO) con HAL
- [x] **DRV-03**: Driver modular para KY-026 (detección de llama) con HAL
- [x] **DRV-04**: Módulo de calibración y filtrado de lecturas de sensores

### Firmware — FreeRTOS y Tiempo Real

- [ ] **RTOS-01**: Tareas FreeRTOS desacopladas por función (sensor, comunicación, actuación, monitoreo)
- [ ] **RTOS-02**: Sistema de colas (queues) para comunicación entre tareas
- [ ] **RTOS-03**: Mutex para acceso seguro a recursos compartidos (I2C, SPI)
- [ ] **RTOS-04**: Prioridades de tareas bien definidas (críticas > periódicas > background)
- [ ] **RTOS-05**: Interrupciones de hardware para eventos críticos (detección de llama)

### Firmware — Lógica de Automatización

- [ ] **AUTO-01**: Evaluación autónoma de condiciones críticas (temp alta, gas, llama)
- [ ] **AUTO-02**: Histéresis para evitar falsos positivos en detección de eventos
- [ ] **AUTO-03**: Control local de actuadores (ventilación) basado en umbrales configurables
- [ ] **AUTO-04**: Recepción y ejecución de comandos MQTT de override desde el servidor

### Firmware — Comunicación MQTT

- [ ] **MQTT-01**: Módulo de comunicación MQTT con reconexión automática
- [ ] **MQTT-02**: Publicación periódica de lecturas de sensores en tópicos estructurados
- [ ] **MQTT-03**: Suscripción a tópicos de comandos y configuración remota
- [ ] **MQTT-04**: Buffer de mensajes locales para tolerancia a fallos de conectividad
- [ ] **MQTT-05**: Keep Alive optimizado para mantener conexión estable

### Gateway — Broker MQTT

- [ ] **GATE-01**: Broker Mosquitto funcionando como gateway central
- [ ] **GATE-02**: Persistencia de sesiones y mensajes retain
- [ ] **GATE-03**: Autenticación básica en MQTT

### Backend — Django

- [ ] **API-01**: API REST Django para gestión de dispositivos y lecturas
- [ ] **API-02**: WebSockets para streaming de datos en tiempo real al frontend
- [ ] **API-03**: Suscripción MQTT del backend para ingesta de telemetría
- [ ] **API-04**: Almacenamiento histórico de lecturas en MySQL
- [ ] **API-05**: Persistencia de eventos y alertas
- [ ] **API-06**: Autenticación de usuarios (login, sesión)

### Frontend — Dashboard React

- [ ] **UI-01**: Dashboard React + Vite + Material UI con diseño responsive
- [ ] **UI-02**: Visualización en tiempo real de temperatura, humedad, gas y llama
- [ ] **UI-03**: Historial de lecturas con gráficos (Chart.js o Recharts)
- [ ] **UI-04**: Panel de alertas y eventos críticos
- [ ] **UI-05**: Acceso local y remoto con autenticación
- [ ] **UI-06**: Visualización del estado de conexión del nodo ESP32

### Infraestructura

- [ ] **INFRA-01**: Docker Compose para backend (Django + MySQL + Mosquitto)
- [ ] **INFRA-02**: Script de despliegue en Raspberry Pi
- [ ] **INFRA-03**: Configuración de red (WiFi para nodo, LAN para servidor)

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Alertas y Notificaciones

- **ALRT-01**: Notificaciones por Telegram Bot API
- **ALRT-02**: Alertas por email para eventos críticos
- **ALRT-03**: Historial y gestión de alertas

### Optimización Energética

- **PWR-01**: Deep Sleep y Light Sleep en nodos ESP32
- **PWR-02**: Wake-up rápido por evento (interrupción o timer)
- **PWR-03**: Reducción de tiempo TX/RX WiFi
- **PWR-04**: Optimización de Keep Alive MQTT
- **PWR-05**: Gestión eficiente de consumo en sensores
- **PWR-06**: Soporte para nodos alimentados por batería

### Escalabilidad

- **SCALE-01**: Soporte multi-nodo (2+ nodos ESP32)
- **SCALE-02**: ESP-NOW o LoRa como respaldo ante pérdida de WiFi
- **SCALE-03**: Sincronización y agregación de datos multi-nodo

### Features Avanzadas

- **ADV-01**: OTA (Over-The-Air) updates para firmware ESP32
- **ADV-02**: Dashboard con mapas de calor y tendencias
- **ADV-03**: Exportación de datos (CSV, PDF)
- **ADV-04**: Roles de usuario (admin, viewer, operator)

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Alertas Telegram | Postergado a v2. Las alertas se manejan en el dashboard en v1. |
| Deep Sleep / batería | Alimentación USB/transformador en v1. Optimización energética en v2. |
| ESP-NOW / LoRa | Solo WiFi en v1. Protocolos alternativos evaluados para v2. |
| Despliegue cloud / VPS | Todo local en Raspberry Pi con Docker Compose. |
| OTA updates | Excede el alcance de v1. Requiere infraestructura adicional. |
| Aplicación móvil nativa | Web-first. App móvil nativa no planificada. |
| Video / cámaras | Solo sensores ambientales. Sin soporte de video. |
| Machine Learning | Sin análisis predictivo en v1. Evaluar para v3+. |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| FWK-01 | Phase 1 | ✅ Complete (01-A-SCAFFOLD) |
| FWK-02 | Phase 1 | ✅ Complete (01-A-SCAFFOLD) |
| FWK-03 | Phase 1 | ✅ Complete (01-A-SCAFFOLD) |
| FWK-04 | Phase 1 | ✅ Complete (01-B-DHT22) |
| FWK-05 | Phase 1 | Pending |
| DRV-01 | Phase 1 | ✅ Complete (01-B-DHT22) |
| DRV-02 | Phase 1 | ✅ Complete (01-C-MQ9-KY026) |
| DRV-03 | Phase 1 | ✅ Complete (01-C-MQ9-KY026) |
| DRV-04 | Phase 1 | ✅ Complete (01-C-MQ9-KY026) |
| RTOS-01 | Phase 1 | Pending |
| RTOS-02 | Phase 1 | Pending |
| RTOS-03 | Phase 1 | Pending |
| RTOS-04 | Phase 1 | Pending |
| RTOS-05 | Phase 1 | Pending |
| AUTO-01 | Phase 5 | Pending |
| AUTO-02 | Phase 5 | Pending |
| AUTO-03 | Phase 5 | Pending |
| AUTO-04 | Phase 5 | Pending |
| MQTT-01 | Phase 3 | Pending |
| MQTT-02 | Phase 3 | Pending |
| MQTT-03 | Phase 3 | Pending |
| MQTT-04 | Phase 3 | Pending |
| MQTT-05 | Phase 3 | Pending |
| GATE-01 | Phase 2 | Pending |
| GATE-02 | Phase 2 | Pending |
| GATE-03 | Phase 2 | Pending |
| API-01 | Phase 2 | Pending |
| API-02 | Phase 4 | Pending |
| API-03 | Phase 3 | Pending |
| API-04 | Phase 2 | Pending |
| API-05 | Phase 2 | Pending |
| API-06 | Phase 2 | Pending |
| UI-01 | Phase 4 | Pending |
| UI-02 | Phase 4 | Pending |
| UI-03 | Phase 4 | Pending |
| UI-04 | Phase 4 | Pending |
| UI-05 | Phase 4 | Pending |
| UI-06 | Phase 4 | Pending |
| INFRA-01 | Phase 2 | Pending |
| INFRA-02 | Phase 2 | Pending |
| INFRA-03 | Phase 2 | Pending |

**Coverage:**
- v1 requirements: 42 total
- Mapped to phases: 42
- Unmapped: 0 ✓

---
*Requirements defined: 2026-05-24*
*Last updated: 2026-05-24 after initial definition*
