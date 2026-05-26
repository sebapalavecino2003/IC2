# Plan 06-02 Summary: Servidor Producción

**Estado:** Completado

## Cambios Realizados

### Task 1: Gunicorn toggle + nginx + docker-compose
- **entrypoint.sh**: Reemplazado `runserver` fijo con toggle `GUNICORN_ENABLED` (true → Gunicorn con workers configurables, false → runserver)
- **nginx/nginx.conf**: Nuevo archivo — reverse proxy con proxy_pass a django:8000, location /static/ con caché, headers proxy y timeouts
- **docker-compose.yml**: Puerto 8000 eliminado de django; healthchecks en los 5 servicios (mosquitto, django, frontend, mysql, nginx); depends_on con `condition: service_healthy` en django → mysql y nginx → django; nuevo servicio nginx:alpine en puerto 80

### Task 2: Health check endpoints
- **views.py**: Nuevos endpoints `LivenessHealthView` (GET /api/v1/health/ → {"status":"alive"}, 200) y `ReadinessHealthView` (GET /api/v1/health/ready/ → verifica MySQL + MQTT, 200/503)
- **urls.py**: Rutas registradas para ambos endpoints

### Task 3: Rate limiting
- **settings.py**: `DEFAULT_THROTTLE_CLASSES` con `UserRateThrottle`, `DEFAULT_THROTTLE_RATES` con `user: 100/minute`, `anon: 5/minute`
- **views.py**: `AnonRateThrottle` en LoginView (5/min por IP), importado `rest_framework.throttling.AnonRateThrottle`

## Verificación
- `GUNICORN_ENABLED`: 1 | `proxy_pass`: 1
- `healthcheck`: 5 | `condition: service_healthy`: 2
- `LivenessHealthView`: 1 | `ReadinessHealthView`: 1
- `DEFAULT_THROTTLE_CLASSES`: 1 | `100/minute`: 1 | `5/minute`: 1
- `AnonRateThrottle`: 2 | `throttle_classes`: 1
- Puerto 8000:8000 eliminado de docker-compose
