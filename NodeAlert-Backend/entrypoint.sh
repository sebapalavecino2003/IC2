#!/bin/bash
set -e

# Punto de entrada del contenedor Django.
# Orquesta la secuencia de inicialización: migraciones, archivos
# estáticos, creación del superusuario por defecto, inicio del
# subscriber MQTT y finalmente el servidor Django.

# Aplica migraciones pendientes de base de datos.
python manage.py migrate --noinput

# Recolecta archivos estáticos para servirlos via nginx.
python manage.py collectstatic --noinput

# Crea el superusuario admin si no existe.
# El redirect a stderr y el '|| true' ocultan errores cuando el
# usuario ya fue creado en ejecuciones previas.
python manage.py createsuperuser \
  --username admin \
  --email admin@nodealert.local \
  --noinput 2>/dev/null || true

# Inicia el subscriber MQTT en background.
# Es un proceso independiente que corre mientras el contenedor vive.
echo "Starting MQTT subscriber..."
python manage.py mqtt_subscriber &
MQTT_PID=$!
echo "MQTT subscriber started (PID: ${MQTT_PID})"

# Inicia el servidor de desarrollo Django.
# En producción, esto sería reemplazado por Gunicorn.
echo "Starting Django development server..."
python manage.py runserver 0.0.0.0:8000
