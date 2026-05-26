# NodeAlert IoT — Backend Django

## Stack

- **Framework:** Django 5 + Django REST Framework
- **Base de datos:** MySQL 8
- **Servidor producción:** Gunicorn 22+
- **Proxy inverso:** nginx (contenedor separado)
- **Cliente MQTT:** paho-mqtt 2+

## Dependencias

Ver `requirements.txt` para la lista completa.

```bash
pip install -r requirements.txt
```

## Migraciones

```bash
python manage.py migrate
```

## Tests

```bash
python manage.py test
```

## Management Commands

- `mqtt_subscriber` — Suscriptor MQTT que corre en background. Se inicia automáticamente desde `entrypoint.sh` si las credenciales MQTT están configuradas.

## Desarrollo

```bash
python manage.py runserver 0.0.0.0:8000
```

## Producción

El backend soporta toggle entre Gunicorn y runserver mediante variable de entorno:

```bash
# Gunicorn (por defecto 3 workers)
GUNICORN_ENABLED=true docker compose up -d

# Workers configurables
GUNICORN_WORKERS=4 GUNICORN_ENABLED=true docker compose up -d

# Desarrollo
GUNICORN_ENABLED=false docker compose up -d
```

## Endpoints

| Ruta | Descripción |
|------|-------------|
| `GET /api/v1/health/` | Liveness check (público) |
| `GET /api/v1/health/ready/` | Readiness check (público) |
| `POST /api/v1/auth/login/` | Autenticación (5 req/min por IP) |
| `GET /api/v1/devices/` | Listar dispositivos |
| `POST /api/v1/devices/{id}/command/` | Enviar comando MQTT |
