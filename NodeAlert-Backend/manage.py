#!/usr/bin/env python
"""
Utilidad de línea de comandos de Django para tareas administrativas.

Punto de entrada para comandos como runserver, migrate, createsuperuser,
y comandos personalizados como mqtt_subscriber.
"""
import os
import sys


def main():
    """Ejecuta las tareas administrativas de Django."""
    os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'nodealert.settings')
    try:
        from django.core.management import execute_from_command_line
    except ImportError as exc:
        raise ImportError(
            "Couldn't import Django. Are you sure it's installed and "
            "available on your PYTHONPATH environment variable? Did you "
            "forget to activate a virtual environment?"
        ) from exc
    execute_from_command_line(sys.argv)


if __name__ == '__main__':
    main()
