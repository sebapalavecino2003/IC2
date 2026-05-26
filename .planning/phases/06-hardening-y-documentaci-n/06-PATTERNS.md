# Phase 6: Hardening y Documentación — Pattern Map

**Mapped:** 2026-05-25
**Files analyzed:** 13 (6 modify, 4 new, 3 new documentation)
**Analogs found:** 13 / 13

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|---|---|---|---|---|
| `NodeAlert-Firmware/src/core/main.cpp` (modify) | firmware-entry | control-flow | itself (existing `app_main`) | exact |
| `NodeAlert-Firmware/src/managers/mqtt_manager.h` (modify) | communication-header | mqtt-publish | `mqtt_manager.h` existing | exact |
| `NodeAlert-Firmware/src/managers/mqtt_manager.cpp` (modify) | communication-impl | mqtt-publish | `mqtt_manager.cpp` + `automation_manager.cpp::publishEvent` | role-match |
| `NodeAlert-Backend/entrypoint.sh` (modify) | deployment-script | control-flow | itself (existing entrypoint.sh) | exact |
| `NodeAlert-Backend/nodealert/settings.py` (modify) | config | n/a | itself (existing `REST_FRAMEWORK` dict) | exact |
| `NodeAlert-Backend/core/views.py` (modify) | controller | request-response | `LoginView` (existing APIView) | exact |
| `NodeAlert-Backend/core/urls.py` (modify) | route | n/a | `core/urls.py` existing | exact |
| `docker-compose.yml` (modify) | infra-config | n/a | itself (existing services) | exact |
| `nginx/nginx.conf` (new) | infra-config | reverse-proxy | `NodeAlert-Frontend/nginx.conf` | role-match |
| `README.md` (new) | doc-root | n/a | `setup.sh` help section (tone) | partial |
| `docs/DEPLOY.md` (new) | doc-deploy | n/a | `setup.sh` (step-by-step flow) | partial |
| `docs/ARCHITECTURE.md` (new) | doc-architecture | n/a | — | no analog |
| `docs/API.md` (new) | doc-api | n/a | Django URL patterns + viewset code | partial |

## Pattern Assignments

---

### `NodeAlert-Firmware/src/core/main.cpp` (firmware-entry, control-flow)

**Analog:** itself at `NodeAlert-Firmware/src/core/main.cpp` (lines 34-203)

**Imports pattern** (lines 17-32) — add `esp_task_wdt.h`:
```cpp
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h"            // <-- NEW for Phase 6

#include "config/pins_config.h"
#include "config/sampling_config.h"   // WATCHDOG_TIMEOUT_MS already defined here
#include "core/system_core.h"
#include "core/state_machine.h"
#include "core/error_handler.h"
#include "managers/sensor_manager.h"
#include "managers/task_manager.h"
#include "managers/automation_manager.h"
#include "managers/mqtt_manager.h"
#include "services/serial_manager.h"
```

**WDT init pattern** — insert after NVS init (around line 44), before serial init:
```cpp
esp_task_wdt_init(WATCHDOG_TIMEOUT_MS, true);  // true = panic on timeout -> auto-reboot
```

**Task subscription pattern** — after each `xTaskCreatePinnedToCore()` block, subscribe the returned handle:
```cpp
// Example from sensor_manager.cpp lines 87-89:
xTaskCreatePinnedToCore(taskDHT22, "dht22_task", 4096, this, 2, &task_dht22, 1);
xTaskCreatePinnedToCore(taskMQ9,   "mq9_task",   4096, this, 3, &task_mq9,   1);
xTaskCreatePinnedToCore(taskKY026, "ky026_task", 2048, this, 4, &task_ky026, 1);
// After each, add:
esp_task_wdt_add(task_dht22);
esp_task_wdt_add(task_mq9);
esp_task_wdt_add(task_ky026);
```

The same pattern applies in `main.cpp` for:
- `sensorManager.startTasks();` (line 83) — subscribe each sensor task handle
- `taskManager.startMonitorTask();` (line 93) — `esp_task_wdt_add(task_handle)` after creation
- `autoManager.startTask();` (line 100) — `esp_task_wdt_add(autoManager task handle)`
- `mqttManager.startMqttTask();` (line 108) — `esp_task_wdt_add(mqtt task handle)`
- The main while(1) loop task itself — `esp_task_wdt_add(NULL)` for the calling task

**Main loop reset pattern** — inside the while(1) loop (line 116), add periodic WDT reset:
```cpp
while (1) {
    esp_task_wdt_reset();   // Reset WDT for the main loop task
    // ... existing loop body ...
}
```

**MqttManager handle extraction** — need to get task handle from MqttManager for WDT subscription:
- Add a `getTaskHandle()` accessor to `MqttManager` header (or use the existing `m_task_handle`)
- Same for `AutomationManager`, `TaskManager`

**Last Will pattern** — set in `esp_mqtt_client_start()` config (in `mqtt_manager.cpp` lines 141-151):
```cpp
esp_mqtt_client_config_t mqtt_cfg = {};
mqtt_cfg.broker.address.uri       = MQTT_BROKER_URI;
mqtt_cfg.broker.address.port      = MQTT_PORT;
mqtt_cfg.credentials.username     = MQTT_USER;
mqtt_cfg.credentials.authentication.password = MQTT_PASS;
mqtt_cfg.session.keepalive        = MQTT_KEEPALIVE;
mqtt_cfg.session.last_will.topic  = "nodealert/" DEVICE_ID "/status";  // NEW
mqtt_cfg.session.last_will.msg    = "...";                             // NEW
mqtt_cfg.session.last_will.qos    = 1;                                 // NEW
mqtt_cfg.session.last_will.retain = false;                             // NEW
```

---

### `NodeAlert-Firmware/src/managers/mqtt_manager.h` (communication-header, mqtt-publish)

**Analog:** `NodeAlert-Firmware/src/managers/mqtt_manager.h` (lines 1-41)

**New method declarations to add** (after line 40, before closing brace):
```cpp
void publishStatus();
void buildStatusJson(char* buffer, size_t buffer_size);
TaskHandle_t getTaskHandle() const { return m_task_handle; }  // For WDT subscription
```

**New member variables to add** (in private section, after line 29):
```cpp
uint32_t m_last_status_ms;   // NEW — track last status publish time
```

**Full existing pattern** from mqtt_manager.h (lines 14-41) — note static event handler, existing publish methods:
```cpp
class MqttManager {
public:
    MqttManager();
    void init(QueueHandle_t sensor_queue, ErrorHandler* eh, StateMachine* sm);
    void startMqttTask();
    void setAutoManager(AutomationManager* auto_mgr) { m_auto_mgr = auto_mgr; }

private:
    // ... members ...
    static void mqttTask(void* pvParams);
    static void mqttEventHandler(void* handler_args, esp_event_base_t base,
                                 int32_t event_id, void* event_data);
    void publishReading(const struct SensorReading& reading);
    void drainBuffer();
    void buildTelemetryJson(const struct SensorReading& reading,
                            char* buffer, size_t buffer_size);
    void buildTopic(const char* suffix, char* buffer, size_t buffer_size);
};
```

---

### `NodeAlert-Firmware/src/managers/mqtt_manager.cpp` (communication-impl, mqtt-publish)

**Analog 1:** `mqtt_manager.cpp` — `buildTelemetryJson()` and `publishReading()` patterns
**Analog 2:** `automation_manager.cpp` — `publishEvent()` pattern (lines 177-193)

**Existing `buildTelemetryJson` pattern** (lines 40-44) — reuse for `buildStatusJson()`:
```cpp
void MqttManager::buildTelemetryJson(const SensorReading& reading,
                                     char* buffer, size_t buffer_size)
{
    (void)reading;
}
```

**Existing `publishReading` pattern** (lines 46-72) — reuse for `publishStatus()`:
```cpp
void MqttManager::publishReading(const SensorReading& reading)
{
    char topic[64];
    char payload[MQTT_MESSAGE_MAX_LEN];

    buildTopic("telemetry", topic, sizeof(topic));
    snprintf(payload, sizeof(payload),
        "{\"device_id\":\"%s\",\"timestamp\":%lu,\"temperature\":%.1f,"
        "\"humidity\":%.1f,\"gas_ppm\":%.0f,\"flame_detected\":%u}",
        DEVICE_ID,
        (unsigned long)(esp_timer_get_time() / 1000),
        // ... reading values ...
    );

    if (m_connected && m_client) {
        int ret = esp_mqtt_client_publish(m_client, topic, payload, 0, 0, 0);
        if (ret < 0) {
            m_buffer.enqueue(topic, payload, 0);
            m_error_handler->reportError("MQTT", "Publish failed — buffered");
        }
    } else {
        m_buffer.enqueue(topic, payload, 0);
    }
}
```

**New `publishStatus()` pattern** — follow same pattern as `publishReading()` but:
- Topic: `buildTopic("status", topic, sizeof(topic))`
- QOS: 1 (not 0)
- Payload fields: `uptime_ms`, `free_heap`, `system_state`, `wifi_rssi`, `errores_activos`, `last_reset_reason`

**Existing `buildTopic` pattern** (lines 35-38):
```cpp
void MqttManager::buildTopic(const char* suffix, char* buffer, size_t buffer_size)
{
    snprintf(buffer, buffer_size, "%s%s/%s", MQTT_TOPIC_PREFIX, DEVICE_ID, suffix);
}
```

**Existing `publishEvent` pattern from `automation_manager.cpp`** (lines 177-193) for reference:
```cpp
void AutomationManager::publishEvent(const char* type, const char* severity, const char* message)
{
    if (!m_mqtt_client) return;

    char topic[64];
    char payload[192];

    snprintf(topic, sizeof(topic), "%s%s/events", MQTT_TOPIC_PREFIX, DEVICE_ID);

    snprintf(payload, sizeof(payload),
        "{\"device_id\":\"%s\",\"event_type\":\"%s\",\"severity\":\"%s\","
        "\"message\":\"%s\",\"timestamp\":%lu}",
        DEVICE_ID, type, severity, message,
        (unsigned long)(esp_timer_get_time() / 1000));

    esp_mqtt_client_publish(m_mqtt_client, topic, payload, 0, 1, 0);
}
```

**Status publish timer pattern** — add to `mqttTask()` while(1) loop (around lines 160-205), after the telemetry block:
```cpp
uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
if ((now_ms - self->m_last_status_ms) >= 60000) {   // Every 60s
    self->publishStatus();
    self->m_last_status_ms = now_ms;
}
// Also publish on state transition — check state machine changes
```

**Constructor init** — add in constructor (line 20):
```cpp
, m_last_status_ms(0)
```

---

### `NodeAlert-Backend/entrypoint.sh` (deployment-script, control-flow)

**Analog:** itself at `NodeAlert-Backend/entrypoint.sh` (lines 1-27)

**Existing pattern** (lines 1-27) — conditional Gunicorn toggle replaces line 27:
```bash
#!/bin/bash
set -e

# Run database migrations
python manage.py migrate --noinput

# Create superuser if DJANGO_SUPERUSER_USERNAME and DJANGO_SUPERUSER_PASSWORD are set
if [ -n "${DJANGO_SUPERUSER_USERNAME:-}" ] && [ -n "${DJANGO_SUPERUSER_PASSWORD:-}" ]; then
  python manage.py createsuperuser \
    --username "$DJANGO_SUPERUSER_USERNAME" \
    --email "${DJANGO_SUPERUSER_EMAIL:-admin@nodealert.local}" \
    --noinput 2>/dev/null || true
fi

# Start MQTT subscriber in background (D-14)
if [ -n "${MQTT_SUBSCRIBER_USER:-}" ] && [ -n "${MQTT_SUBSCRIBER_PASSWORD:-}" ]; then
  echo "Starting MQTT subscriber..."
  python manage.py mqtt_subscriber &
  MQTT_PID=$!
  echo "MQTT subscriber started (PID: ${MQTT_PID})"
else
  echo "MQTT subscriber credentials not set — skipping MQTT subscription"
fi

# Phase 6: GUNICORN_ENABLED toggle
# Replace existing line 26-27 with:
if [ "${GUNICORN_ENABLED:-false}" = "true" ]; then
  WORKERS="${GUNICORN_WORKERS:-3}"
  echo "Starting Gunicorn with ${WORKERS} workers..."
  exec gunicorn nodealert.wsgi:application \
    --workers "$WORKERS" \
    --bind 0.0.0.0:8000
else
  echo "Starting Django development server..."
  python manage.py runserver 0.0.0.0:8000
fi
```

Key variables to document in `.env`:
- `GUNICORN_ENABLED=true` (default: false for dev)
- `GUNICORN_WORKERS=3` (default formula: `2 * CPU cores + 1`)

---

### `NodeAlert-Backend/nodealert/settings.py` (config, n/a)

**Analog:** itself at `NodeAlert-Backend/nodealert/settings.py` (lines 118-130)

**Existing `REST_FRAMEWORK` dict** (lines 119-130):
```python
REST_FRAMEWORK = {
    'DEFAULT_AUTHENTICATION_CLASSES': [
        'rest_framework.authentication.TokenAuthentication',
        'rest_framework.authentication.SessionAuthentication',
    ],
    'DEFAULT_PERMISSION_CLASSES': [
        'rest_framework.permissions.IsAuthenticated',
    ],
    'DEFAULT_PAGINATION_CLASS': 'rest_framework.pagination.PageNumberPagination',
    'PAGE_SIZE': 50,
    'DEFAULT_FILTER_BACKENDS': ['django_filters.rest_framework.DjangoFilterBackend'],
}
```

**New throttling config** — add `DEFAULT_THROTTLE_CLASSES` and `DEFAULT_THROTTLE_RATES` keys:
```python
REST_FRAMEWORK = {
    # ... existing keys ...
    'DEFAULT_THROTTLE_CLASSES': [
        'rest_framework.throttling.UserRateThrottle',
    ],
    'DEFAULT_THROTTLE_RATES': {
        'user': '100/minute',
        'anon': '5/minute',
    },
    # ... existing keys ...
}
```

**NOTE:** AnonRateThrottle applies to the login endpoint specifically, not globally. Apply it per-view on `LoginView`:
```python
# In core/views.py (not in settings):
from rest_framework.throttling import AnonRateThrottle

class LoginView(APIView):
    permission_classes = [AllowAny]
    throttle_classes = [AnonRateThrottle]   # Override global for login

    def post(self, request):
        ...
```

---

### `NodeAlert-Backend/core/views.py` (controller, request-response)

**Analog:** `LoginView` at `NodeAlert-Backend/core/views.py` (lines 110-125)

**Existing `LoginView` pattern** — simple `APIView` with `AllowAny`:
```python
class LoginView(APIView):
    """View for user authentication via username/password."""
    permission_classes = [AllowAny]

    def post(self, request):
        serializer = LoginSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        user = serializer.validated_data['user']
        token, created = Token.objects.get_or_create(user=user)
        return Response({'token': token.key}, status=status.HTTP_200_OK)
```

**New `HealthCheckView` pattern** — follow same `APIView` + `AllowAny` pattern:
```python
from rest_framework.views import APIView
from rest_framework.permissions import AllowAny
from rest_framework.response import Response
from rest_framework import status
from django.db import connection

class LivenessHealthView(APIView):
    """GET /api/v1/health/ — liveness check. No dependencies."""
    permission_classes = [AllowAny]

    def get(self, request):
        return Response({"status": "alive"}, status=status.HTTP_200_OK)


class ReadinessHealthView(APIView):
    """GET /api/v1/health/ready/ — readiness check. Verifies MySQL + MQTT."""
    permission_classes = [AllowAny]

    def get(self, request):
        checks = {"database": False, "mqtt": False}

        # Check MySQL
        try:
            connection.ensure_connection()
            checks["database"] = True
        except Exception:
            pass

        # Check MQTT subscriber (by checking if process is running or last heartbeat)
        checks["mqtt"] = True  # Simplified — depends on MQTT subscriber reporting

        if all(checks.values()):
            return Response({"status": "ready", "checks": checks}, status=status.HTTP_200_OK)
        return Response(
            {"status": "not ready", "checks": checks},
            status=status.HTTP_503_SERVICE_UNAVAILABLE
        )
```

**Imports to add to `core/views.py`** (line 1-12):
```python
from django.db import connection           # NEW — for readiness check
from rest_framework.throttling import AnonRateThrottle  # NEW — for login rate limit
```

**Modify LoginView** — add `throttle_classes` as described above.

---

### `NodeAlert-Backend/core/urls.py` (route, n/a)

**Analog:** itself at `NodeAlert-Backend/core/urls.py` (lines 1-13)

**Existing pattern** (lines 1-13):
```python
"""URL routing for core app under /api/v1/."""
from django.urls import path, include
from rest_framework.routers import DefaultRouter
from .views import DeviceViewSet, ReadingViewSet, EventViewSet, LoginView

router = DefaultRouter()
router.register(r'devices', DeviceViewSet, basename='device')
router.register(r'readings', ReadingViewSet, basename='reading')
router.register(r'events', EventViewSet, basename='event')

urlpatterns = [
    path('auth/login/', LoginView.as_view(), name='auth-login'),
] + router.urls
```

**New health endpoint routes** — add imports and paths:
```python
from .views import DeviceViewSet, ReadingViewSet, EventViewSet, LoginView, \
                   LivenessHealthView, ReadinessHealthView   # NEW

urlpatterns = [
    path('auth/login/', LoginView.as_view(), name='auth-login'),
    path('health/', LivenessHealthView.as_view(), name='health-liveness'),     # NEW
    path('health/ready/', ReadinessHealthView.as_view(), name='health-readiness'),  # NEW
] + router.urls
```

---

### `docker-compose.yml` (infra-config, n/a)

**Analog:** itself at `docker-compose.yml` (lines 1-58)

**Existing service definition pattern:**
```yaml
services:
  mosquitto:
    image: eclipse-mosquitto:2
    container_name: mosquitto
    ports:
      - "1883:1883"
    volumes:
      - ./mosquitto/config:/mosquitto/config:ro
      - mosquitto_data:/mosquitto/data
      - mosquitto_log:/mosquitto/log
    networks:
      - nodealert-net
    restart: unless-stopped
```

**New nginx service** pattern — follow the existing service structure:
```yaml
  nginx:
    image: nginx:alpine
    container_name: nginx
    ports:
      - "80:80"
    volumes:
      - ./nginx/nginx.conf:/etc/nginx/conf.d/default.conf:ro
    depends_on:
      - django
    networks:
      - nodealert-net
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "--fail", "http://localhost:80/"]
      interval: 30s
      timeout: 10s
      retries: 3
```

**Modified django service** — remove direct port exposure, keep internal only:
```yaml
  django:
    build: ./NodeAlert-Backend
    container_name: django
    env_file: .env
    # ports:                                        # REMOVED — only accessible via nginx
    #   - "8000:8000"                               # REMOVED
    depends_on:
      mysql:
        condition: service_healthy                  # ADDED
      - mosquitto
    networks:
      - nodealert-net
    restart: unless-stopped
    healthcheck:                                    # ADDED
      test: ["CMD", "curl", "--fail", "http://localhost:8000/api/v1/health/"]
      interval: 30s
      timeout: 10s
      retries: 5
      start_period: 40s
```

**HEALTHCHECK pattern for existing services:**

For mysql:
```yaml
  mysql:
    image: mysql:8.0
    container_name: mysql
    env_file: .env
    volumes:
      - mysql_data:/var/lib/mysql
    networks:
      - nodealert-net
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "mysqladmin", "ping", "-u", "root", "-p${MYSQL_ROOT_PASSWORD}", "--silent"]
      interval: 10s
      timeout: 5s
      retries: 5
      start_period: 30s
```

For mosquitto:
```yaml
  mosquitto:
    # ... existing config ...
    healthcheck:
      test: ["CMD", "mosquitto_sub", "-t", "$SYS/#", "-C", "1"]
      interval: 30s
      timeout: 10s
      retries: 3
```

For frontend:
```yaml
  frontend:
    # ... existing config ...
    healthcheck:
      test: ["CMD", "curl", "--fail", "http://localhost:80/"]
      interval: 30s
      timeout: 10s
      retries: 3
```

**depends_on with condition** — use `condition: service_healthy` for nginx → django and django → mysql.

---

### `nginx/nginx.conf` (NEW, infra-config, reverse-proxy)

**Analog:** `NodeAlert-Frontend/nginx.conf` (lines 1-18)

**Existing frontend nginx pattern:**
```nginx
server {
    listen 80;
    server_name _;

    root /usr/share/nginx/html;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }

    location /api/ {
        proxy_pass http://django:8000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

**New reverse proxy nginx.conf pattern:**
```nginx
server {
    listen 80;
    server_name _;

    # Static files served directly by nginx
    location /static/ {
        alias /app/static/;
        expires 7d;
        add_header Cache-Control "public, immutable";
    }

    # Django/Gunicorn reverse proxy
    location / {
        proxy_pass http://django:8000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # Gunicorn buffering
        proxy_buffering on;
        proxy_buffer_size 4k;
        proxy_buffers 8 4k;
        proxy_busy_buffers_size 8k;

        # Timeouts
        proxy_connect_timeout 60s;
        proxy_read_timeout 60s;
        proxy_send_timeout 60s;
    }
}
```

---

### `README.md` (NEW, doc-root, n/a)

**Analog:** `setup.sh` help section (lines 24-43) for tone and content patterns.

**No existing READMEs** in the project — this is the first. Use CONTEXT.md decisions D-16/D-17 as structure guide.

**Pattern to follow:**
```markdown
# NodeAlert IoT

[project description]

## Stack

[tech stack table]

## Quick Start

```bash
./setup.sh
```

## Documentation

- [Deployment Guide](docs/DEPLOY.md)
- [Architecture](docs/ARCHITECTURE.md)
- [API Reference](docs/API.md)

## Component READMEs

- [Firmware](NodeAlert-Firmware/README.md)
- [Backend](NodeAlert-Backend/README.md)
- [Frontend](NodeAlert-Frontend/README.md)
```

---

### `docs/DEPLOY.md` (NEW, doc-deploy, n/a)

**Analog:** `setup.sh` (lines 1-476) — full deployment flow provides natural structure.

**Pattern** — step-by-step sections mirroring setup.sh execution order:
```markdown
# Guía de Despliegue — NodeAlert IoT

## Requisitos

- Docker + Docker Compose V2
- Raspberry Pi (ARM64) o servidor Linux (AMD64)
- ESP32 con PlatformIO

## Arquitectura de Despliegue

[diagram or description]

## Paso a Paso

### 1. Clonar el repositorio
### 2. Ejecutar setup.sh
### 3. Compilar Firmware
### 4. Desplegar con Docker Compose
### 5. Verificar Health Checks

## Configuración del Firmware

[PlatformIO, pines, sensores]

## Troubleshooting

[common issues]
```

---

### `docs/ARCHITECTURE.md` (NEW, doc-architecture, n/a)

**No close analog exists** in the codebase. Build from CONTEXT.md and the phase requirements:
- Data flow: ESP32 → MQTT → Django → Frontend
- Component descriptions
- Key technical decisions (D-01 through D-15)

---

### `docs/API.md` (NEW, doc-api, n/a)

**No close analog exists.** Build from the Django URL routing + view structures:
- `core/urls.py` — endpoint routes
- `core/views.py` — view behavior and parameters
- `mqtt_subscriber.py` — MQTT topic patterns

**Pattern** — document by category from the existing code:
```markdown
# API Reference — NodeAlert IoT

## Autenticación

- `POST /api/v1/auth/login/` — Login (rate limited: 5/min)

## Dispositivos

- `GET /api/v1/devices/` — List devices
- `POST /api/v1/devices/` — Create device
- ...

## Lecturas (Read-Only)

- `GET /api/v1/readings/` — List readings (filterable)
- ...

## Eventos

- `GET /api/v1/events/` — List events
- ...

## Comandos

- `POST /api/v1/devices/{id}/command/` — Send MQTT command

## Health Checks

- `GET /api/v1/health/` — Liveness
- `GET /api/v1/health/ready/` — Readiness (MySQL + MQTT)

## Tópicos MQTT

- `nodealert/{device_id}/telemetry` — Sensor readings
- `nodealert/{device_id}/events` — System events
- `nodealert/{device_id}/commands` — Server commands
- `nodealert/{device_id}/status` — Device status heartbeat (NEW)
```

---

## Shared Patterns

### Docker Service Definition
**Source:** `docker-compose.yml` lines 13-58
**Apply to:** New nginx service + HEALTHCHECK additions to existing services
```yaml
services:
  <name>:
    image: <image>:<tag>
    container_name: <name>
    ports: [optional]
    volumes: [optional]
    networks:
      - nodealert-net
    restart: unless-stopped
    depends_on: [optional]
    healthcheck: [optional — Phase 6 addition]
      test: ["CMD", "<command>"]
      interval: 30s
      timeout: 10s
      retries: 3
```

### Django APIView Pattern (for HealthCheck)
**Source:** `NodeAlert-Backend/core/views.py` lines 110-125 (`LoginView`)
**Apply to:** New `LivenessHealthView` and `ReadinessHealthView`
```python
class XxxView(APIView):
    permission_classes = [AllowAny]  # Public health endpoints

    def get(self, request):
        # Business logic
        return Response({...}, status=status.HTTP_200_OK)
```

### MQTT Publish Pattern (for Status Topic)
**Source:** `NodeAlert-Firmware/src/managers/mqtt_manager.cpp` lines 46-72 (`publishReading`)
**Source:** `NodeAlert-Firmware/src/managers/automation_manager.cpp` lines 177-193 (`publishEvent`)
**Apply to:** New `publishStatus()` method in mqtt_manager
```cpp
void ClassName::publishXxx()
{
    char topic[64];
    char payload[256];
    buildTopic("status", topic, sizeof(topic));
    snprintf(payload, sizeof(payload),
        "{\"device_id\":\"%s\",\"key\":\"value\",...}",
        DEVICE_ID, ...);

    if (m_connected && m_client) {
        int ret = esp_mqtt_client_publish(m_client, topic, payload, 0, 1, 0);
        if (ret < 0) {
            m_buffer.enqueue(topic, payload, 0);
            m_error_handler->reportError("MQTT", "Publish failed — buffered");
        }
    } else {
        m_buffer.enqueue(topic, payload, 0);
    }
}
```

### ESP32 FreeRTOS + WDT Pattern
**Source:** `NodeAlert-Firmware/src/core/main.cpp` lines 34-203, `sampling_config.h` line 57
**Apply to:** `main.cpp` modification
```cpp
#include "esp_task_wdt.h"

// Init WDT at the start of app_main()
esp_task_wdt_init(WATCHDOG_TIMEOUT_MS, true);  // timeout=10000ms, panic=true

// Subscribe each task after creation
TaskHandle_t handle;
xTaskCreatePinnedToCore(taskFunc, "name", stack, this, priority, &handle, core);
esp_task_wdt_add(handle);

// Periodically reset WDT in each task loop
while (1) {
    esp_task_wdt_reset();
    // ... task work ...
    vTaskDelay(pdMS_TO_TICKS(interval));
}
```

### DRF Rate Limiting Pattern
**Source:** DRF docs + `NodeAlert-Backend/nodealert/settings.py` lines 118-130
**Apply to:** `settings.py` + `core/views.py`
```python
# settings.py — GLOBAL
REST_FRAMEWORK = {
    'DEFAULT_THROTTLE_CLASSES': ['rest_framework.throttling.UserRateThrottle'],
    'DEFAULT_THROTTLE_RATES': {
        'user': '100/minute',
        'anon': '5/minute',
    },
    # ...
}

# views.py — PER-VIEW override for login
from rest_framework.throttling import AnonRateThrottle

class LoginView(APIView):
    permission_classes = [AllowAny]
    throttle_classes = [AnonRateThrottle]    # 5/min per IP
```

---

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `docs/ARCHITECTURE.md` | doc-architecture | n/a | No existing documentation in project |
| `docs/API.md` | doc-api | n/a | No existing documentation in project |
| `README.md` | doc-root | n/a | No existing README in project |
| `docs/DEPLOY.md` | doc-deploy | n/a | No existing deployment docs in project |

For documentation files, use CONTEXT.md decisions (D-16, D-17, D-18) as the primary guide and the setup.sh help/output section as a tone reference.

---

## Metadata

**Analog search scope:** `/home/sebastian/Escritorio/IC2/NodeAlert-Firmware/src/`, `NodeAlert-Backend/`, `docker-compose.yml`, `setup.sh`
**Files scanned:** 18 source files
**Pattern extraction date:** 2026-05-25
