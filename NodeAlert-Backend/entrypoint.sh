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

# Production toggle: Gunicorn vs runserver (Phase 6, D-05/D-08)
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
