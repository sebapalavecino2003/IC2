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

# Start Django development server
# Note: Phase 3 will add MQTT subscriber startup here
# Phase 6 will switch to gunicorn for production
python manage.py runserver 0.0.0.0:8000
