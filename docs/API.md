# Referencia de API — NodeAlert IoT

## Base URL

Todas las rutas bajo `/api/v1/`. Proxy inverso nginx en puerto 80.

## Autenticación

| Método | Ruta | Auth | Descripción |
|--------|------|------|-------------|
| POST | `/api/v1/auth/login/` | No | Inicio de sesión (username + password → token) |

**Rate limit:** 5 solicitudes/minuto por IP.

**Ejemplo request:**
```json
{"username": "admin", "password": "secreto123"}
```

**Ejemplo response (200):**
```json
{"token": "a1b2c3d4e5f6..."}
```

## Dispositivos

| Método | Ruta | Auth | Descripción |
|--------|------|------|-------------|
| GET | `/api/v1/devices/` | Sí | Listar dispositivos |
| POST | `/api/v1/devices/` | Sí | Crear dispositivo |
| GET | `/api/v1/devices/{id}/` | Sí | Detalle de dispositivo |
| PUT | `/api/v1/devices/{id}/` | Sí | Actualizar dispositivo completo |
| PATCH | `/api/v1/devices/{id}/` | Sí | Actualización parcial |
| DELETE | `/api/v1/devices/{id}/` | Sí | Eliminar dispositivo |
| POST | `/api/v1/devices/{id}/command/` | Sí | Enviar comando MQTT |

**Parámetros de consulta (GET /devices/):**
- `is_active` (boolean) — Filtrar por estado activo/inactivo
- `location` (string) — Filtrar por ubicación
- `search` (string) — Buscar por name, device_id, location
- `ordering` (string) — `created_at`, `name`, `device_id`

**Ejemplo response (GET /devices/):**
```json
{
  "count": 2,
  "next": null,
  "previous": null,
  "results": [
    {
      "id": 1,
      "device_id": "nodealert-01",
      "name": "Sensor Sala Principal",
      "location": "Planta Baja",
      "is_active": true,
      "created_at": "2026-05-20T10:00:00Z"
    }
  ]
}
```

**Comandos (POST /devices/{id}/command/):**
- `actuator_on` — Activar actuador
- `actuator_off` — Desactivar actuador
- `return_to_auto` — Volver a modo automático
- `acknowledge_alarm` — Confirmar alarma
- `update_thresholds` — Actualizar umbrales

## Lecturas

| Método | Ruta | Auth | Descripción |
|--------|------|------|-------------|
| GET | `/api/v1/readings/` | Sí | Listar lecturas (solo lectura) |

**Parámetros de consulta:**
- `sensor_type` (string) — `DHT22_TEMPERATURE`, `DHT22_HUMIDITY`, `MQ9_GAS`, `KY026_FLAME`
- `device_id` (string) — Filtrar por ID de dispositivo
- `timestamp__gte` (datetime) — Desde (ISO 8601)
- `timestamp__lte` (datetime) — Hasta (ISO 8601)
- `ordering` (string) — `timestamp`

**Ejemplo response:**
```json
{
  "count": 1,
  "results": [
    {
      "id": 42,
      "device": 1,
      "sensor_type": "DHT22_TEMPERATURE",
      "value": 23.5,
      "unit": "°C",
      "status": "OK",
      "timestamp": "2026-05-25T12:00:00Z"
    }
  ]
}
```

## Eventos

| Método | Ruta | Auth | Descripción |
|--------|------|------|-------------|
| GET | `/api/v1/events/` | Sí | Listar eventos |
| POST | `/api/v1/events/` | Sí | Crear evento |
| GET | `/api/v1/events/{id}/` | Sí | Detalle de evento |
| PUT | `/api/v1/events/{id}/` | Sí | Actualizar evento |

**Parámetros de consulta (GET /events/):**
- `severity` (string) — `INFO`, `WARNING`, `CRITICAL`
- `resolved` (boolean) — Filtrar por resuelto/no resuelto
- `ordering` (string) — `timestamp`, `severity`

**Rate limit en endpoints autenticados:** 100 solicitudes/minuto por usuario.

## Health Checks

| Método | Ruta | Auth | Descripción |
|--------|------|------|-------------|
| GET | `/api/v1/health/` | No | Liveness — 200 si el proceso está vivo |
| GET | `/api/v1/health/ready/` | No | Readiness — 200 si DB + MQTT ok, 503 si falla |

**Ejemplo response liveness (200):**
```json
{"status": "alive"}
```

**Ejemplo response readiness (200):**
```json
{"status": "ready", "checks": {"database": true, "mqtt": true}}
```

**Ejemplo response readiness (503):**
```json
{"status": "not ready", "checks": {"database": true, "mqtt": false}}
```

## Tópicos MQTT

Todos los tópicos usan el prefijo `nodealert/{device_id}/`.

| Tópico | Dirección | QoS | Descripción |
|--------|-----------|-----|-------------|
| `nodealert/{device_id}/telemetry` | ESP32 → Broker | 0 | Lecturas de sensores cada 10s |
| `nodealert/{device_id}/events` | ESP32 → Broker | 1 | Eventos del sistema en transiciones |
| `nodealert/{device_id}/commands` | Broker → ESP32 | 1 | Comandos del servidor (override) |
| `nodealert/{device_id}/status` | ESP32 → Broker | 1 | Heartbeat de estado cada 60s + Last Will |

### Formato de payloads

**Telemetría (publicado cada 10s):**
```json
{
  "device_id": "nodealert-01",
  "timestamp": 1716624000000,
  "temperature": 23.5,
  "humidity": 60.2,
  "gas_ppm": 150,
  "flame_detected": 0
}
```

**Estado (publicado cada 60s, QoS 1, con Last Will):**
```json
{
  "device_id": "nodealert-01",
  "status": "online",
  "uptime_ms": 3600000,
  "free_heap": 120000,
  "system_state": "RUNNING",
  "wifi_rssi": -65,
  "last_reset_reason": 1,
  "errores_activos": []
}
```

**Last Will (publicado por el broker al desconectarse el ESP32):**
```json
{
  "device_id": "nodealert-01",
  "status": "offline",
  "reason": "watchdog_reboot"
}
```

**Evento (publicado en transiciones de estado):**
```json
{
  "device_id": "nodealert-01",
  "event_type": "ALERT",
  "severity": "CRITICAL",
  "message": "Temperatura excedida: 52.3°C",
  "timestamp": 1716624000000
}
```

**Comando (recibido por el ESP32 desde el servidor):**
```json
{
  "cmd": "actuator_on",
  "params": {
    "duration_ms": 120000
  }
}
```
