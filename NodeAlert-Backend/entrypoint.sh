#!/bin/bash
set -e

# Run database migrations
python manage.py migrate --noinput

# Start Django development server
# Note: Phase 3 will add MQTT subscriber startup here
# Phase 6 will switch to gunicorn for production
python manage.py runserver 0.0.0.0:8000
