"""Core models for NodeAlert IoT: Device, Reading, Event, User."""
from django.db import models
from django.contrib.auth.models import AbstractUser

# Sensor type choices for the Reading model
SENSOR_TYPE_CHOICES = [
    ('temperature', 'Temperature'),
    ('humidity', 'Humidity'),
    ('gas', 'Gas'),
    ('flame', 'Flame'),
]

# Severity choices for the Event model
SEVERITY_CHOICES = [
    ('info', 'Info'),
    ('warning', 'Warning'),
    ('critical', 'Critical'),
]


class Device(models.Model):
    """An ESP32 sensor node device."""

    device_id = models.CharField(max_length=50, unique=True)
    name = models.CharField(max_length=100)
    location = models.CharField(max_length=200, blank=True, default='')
    is_active = models.BooleanField(default=True)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    class Meta:
        ordering = ['-created_at']
        verbose_name_plural = 'devices'

    def __str__(self):
        return self.name or self.device_id


class Reading(models.Model):
    """A sensor reading from a device."""

    device = models.ForeignKey(
        Device, on_delete=models.CASCADE, related_name='readings'
    )
    sensor_type = models.CharField(max_length=20, choices=SENSOR_TYPE_CHOICES)
    value = models.FloatField()
    unit = models.CharField(max_length=20)
    timestamp = models.DateTimeField()

    class Meta:
        ordering = ['-timestamp']
        verbose_name_plural = 'readings'
        indexes = [
            models.Index(fields=['device', 'sensor_type']),
            models.Index(fields=['timestamp']),
        ]

    def __str__(self):
        return f"{self.device.device_id}/{self.sensor_type} @ {self.timestamp}"


class Event(models.Model):
    """An event or alert from a device."""

    device = models.ForeignKey(
        Device, on_delete=models.CASCADE, related_name='events'
    )
    event_type = models.CharField(max_length=50)
    severity = models.CharField(max_length=20, choices=SEVERITY_CHOICES)
    message = models.TextField()
    resolved = models.BooleanField(default=False)
    timestamp = models.DateTimeField()

    class Meta:
        ordering = ['-timestamp']
        verbose_name_plural = 'events'
        indexes = [
            models.Index(fields=['device', 'severity']),
            models.Index(fields=['resolved']),
        ]

    def __str__(self):
        return f"[{self.severity}] {self.event_type} @ {self.device.device_id}"


class User(AbstractUser):
    """Custom user model extending AbstractUser with no extra fields."""

    class Meta:
        verbose_name_plural = 'users'

    def __str__(self):
        return self.username
