"""
Configuración WSGI para el proyecto NodeAlert.

Expone la aplicación WSGI para servidores como Gunicorn. Django
configura automáticamente el módulo de settings basándose en la
variable de entorno DJANGO_SETTINGS_MODULE.
"""
import os

from django.core.wsgi import get_wsgi_application

os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'nodealert.settings')

application = get_wsgi_application()
