# Guía de Configuración Manual

Todas las contraseñas, IPs y credenciales están hardcodeadas directamente en el código. Aquí están las ubicaciones exactas para modificarlas:

---

## Django

| Variable | Valor actual | Dónde cambiarlo |
|---|---|---|
| `SECRET_KEY` | `django-insecure-dev-key-not-for-production-use` | `NodeAlert-Backend/nodealert/settings.py:12` |
| `DEBUG` | `True` | `NodeAlert-Backend/nodealert/settings.py:15` |
| `DJANGO_SUPERUSER_USERNAME` | `admin` | `NodeAlert-Backend/entrypoint.sh:11-15` |
| `DJANGO_SUPERUSER_PASSWORD` | `admin_password` | `NodeAlert-Backend/entrypoint.sh:11-15` |
| `DJANGO_SUPERUSER_EMAIL` | `admin@nodealert.local` | `NodeAlert-Backend/entrypoint.sh:14` |

## MySQL

| Variable | Valor actual | Dónde cambiarlo |
|---|---|---|
| `MYSQL_DATABASE` | `nodealert` | `NodeAlert-Backend/nodealert/settings.py:69` |
| `MYSQL_USER` | `nodealert` | `NodeAlert-Backend/nodealert/settings.py:70` |
| `MYSQL_PASSWORD` | `db_password` | `NodeAlert-Backend/nodealert/settings.py:71` |
| `MYSQL_ROOT_PASSWORD` | `root_password` | `NodeAlert-Backend/nodealert/settings.py:82` |
| Host MySQL | `mysql` | `NodeAlert-Backend/nodealert/settings.py:72` |
| Puerto MySQL | `3306` | `NodeAlert-Backend/nodealert/settings.py:73` |
| `MYSQL_ROOT_PASSWORD` (healthcheck) | `root_password` | `docker-compose.yml:40` |

## MQTT

| Variable | Valor actual | Dónde cambiarlo |
|---|---|---|
| Host MQTT | `mosquitto` | `NodeAlert-Backend/core/mqtt_publisher.py:30` |
| | | `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py:35` |
| Puerto MQTT | `1883` | `NodeAlert-Backend/core/mqtt_publisher.py:31` |
| | | `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py:36` |
| `MQTT_KEEPALIVE` | `60` | `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py:37` |
| `MQTT_PUBLISHER_USER` | `mqtt_publisher` | `NodeAlert-Backend/core/mqtt_publisher.py:32` |
| `MQTT_PUBLISHER_PASSWORD` | `test_password` | `NodeAlert-Backend/core/mqtt_publisher.py:33` |
| `MQTT_SUBSCRIBER_USER` | `mqtt_subscriber` | `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py:38` |
| | | `NodeAlert-Backend/entrypoint.sh:19` |
| `MQTT_SUBSCRIBER_PASSWORD` | `test_password` | `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py:39` |
| | | `NodeAlert-Backend/entrypoint.sh:19` |
| `MQTT_TOPIC_PREFIX` | `nodealert/` | `NodeAlert-Backend/core/mqtt_publisher.py:10` |
| healthcheck mosquitto | `nodealert_gateway` / `test_password` | `docker-compose.yml:24` |

## Frontend

| Variable | Valor actual | Dónde cambiarlo |
|---|---|---|
| `VITE_API_BASE_URL` | `/api/v1` | `NodeAlert-Frontend/src/services/api.js:3` |

## Gunicorn

| Variable | Valor actual | Dónde cambiarlo |
|---|---|---|
| `GUNICORN_ENABLED` | `false` | `NodeAlert-Backend/entrypoint.sh:29` |
| `GUNICORN_WORKERS` | `3` | `NodeAlert-Backend/entrypoint.sh:30` |

---

## Cómo modificar

1. Abrí el archivo indicado en "Dónde cambiarlo"
2. Buscá la línea indicada
3. Reemplazá el valor actual por el que necesites

### Ejemplo

Si querés cambiar la contraseña de MySQL:

**Archivo:** `NodeAlert-Backend/nodealert/settings.py`
```python
'PASSWORD': 'db_password',   # <-- cambiá 'db_password' por tu nueva contraseña
```
