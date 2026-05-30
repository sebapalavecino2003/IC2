"""
Configuración de URLs raíz del proyecto NodeAlert.

Enruta el panel de administración y la API REST bajo /admin/ y
/api/v1/ respectivamente.
"""
from django.contrib import admin
from django.urls import path, include

urlpatterns = [
    # Panel de administración Django.
    path('admin/', admin.site.urls),
    # API REST de NodeAlert (incluye /api/v1/auth/, /api/v1/devices/, etc.).
    path('api/v1/', include('core.urls')),
]
