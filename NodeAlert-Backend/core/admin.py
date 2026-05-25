"""Admin configuration for core models."""
from django.contrib import admin
from .models import Device, Reading, Event, User


@admin.register(Device)
class DeviceAdmin(admin.ModelAdmin):
    """Admin configuration for Device model."""

    list_display = ['device_id', 'name', 'location', 'is_active', 'created_at']
    list_filter = ['is_active']
    search_fields = ['device_id', 'name', 'location']


@admin.register(Reading)
class ReadingAdmin(admin.ModelAdmin):
    """Admin configuration for Reading model."""

    list_display = ['device', 'sensor_type', 'value', 'unit', 'timestamp']
    list_filter = ['sensor_type', 'timestamp']
    search_fields = ['device__device_id']


@admin.register(Event)
class EventAdmin(admin.ModelAdmin):
    """Admin configuration for Event model."""

    list_display = ['device', 'event_type', 'severity', 'message', 'resolved', 'timestamp']
    list_filter = ['severity', 'resolved']
    search_fields = ['device__device_id', 'event_type']


@admin.register(User)
class UserAdmin(admin.ModelAdmin):
    """Admin configuration for User model."""
    pass
