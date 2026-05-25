"""URL routing for core app under /api/v1/."""
from django.urls import path, include
from rest_framework.routers import DefaultRouter
from .views import DeviceViewSet, ReadingViewSet, EventViewSet

router = DefaultRouter()
router.register(r'devices', DeviceViewSet, basename='device')
router.register(r'readings', ReadingViewSet, basename='reading')
router.register(r'events', EventViewSet, basename='event')

urlpatterns = router.urls
