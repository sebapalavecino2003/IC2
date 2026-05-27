"""URL routing for core app under /api/v1/."""
from django.urls import path, include
from rest_framework.routers import DefaultRouter
from .views import (DeviceViewSet, ReadingViewSet, EventViewSet, LoginView,
                    LivenessHealthView, ReadinessHealthView, MeView)

router = DefaultRouter()
router.register(r'devices', DeviceViewSet, basename='device')
router.register(r'readings', ReadingViewSet, basename='reading')
router.register(r'events', EventViewSet, basename='event')

urlpatterns = [
    path('auth/login/', LoginView.as_view(), name='auth-login'),
    path('auth/me/', MeView.as_view(), name='auth-me'),
    path('health/', LivenessHealthView.as_view(), name='health-liveness'),
    path('health/ready/', ReadinessHealthView.as_view(), name='health-readiness'),
] + router.urls
