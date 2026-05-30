"""
Enrutamiento URL de la aplicación core bajo /api/v1/.

Define los endpoints REST utilizando DefaultRouter de DRF para los
ViewSets (dispositivos, lecturas, eventos) y rutas explícitas para
vistas especializadas (login, health checks, información del usuario).
"""
from django.urls import path, include
from rest_framework.routers import DefaultRouter
from .views import (DeviceViewSet, ReadingViewSet, EventViewSet, LoginView,
                    LivenessHealthView, ReadinessHealthView, MeView)

# Router automático que genera URLs estándar para cada ViewSet:
#   /devices/        → list, create
#   /devices/{id}/   → retrieve, update, partial_update, destroy
#   /readings/       → list, retrieve (read-only)
#   /events/         → list, create
#   /events/{id}/    → retrieve, update, partial_update, destroy
router = DefaultRouter()
router.register(r'devices', DeviceViewSet, basename='device')
router.register(r'readings', ReadingViewSet, basename='reading')
router.register(r'events', EventViewSet, basename='event')

urlpatterns = [
    # Autenticación: login y usuario actual
    path('auth/login/', LoginView.as_view(), name='auth-login'),
    path('auth/me/', MeView.as_view(), name='auth-me'),

    # Health checks para orquestación
    path('health/', LivenessHealthView.as_view(), name='health-liveness'),
    path('health/ready/', ReadinessHealthView.as_view(), name='health-readiness'),
] + router.urls
