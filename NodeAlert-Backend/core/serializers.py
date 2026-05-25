"""DRF serializers for core models: Device, Reading, Event."""
from django.utils import timezone
from rest_framework import serializers
from .models import Device, Reading, Event


class DeviceSerializer(serializers.ModelSerializer):
    """Serializer for Device model with full CRUD support."""

    class Meta:
        model = Device
        fields = '__all__'
        read_only_fields = ['created_at', 'updated_at']


class ReadingSerializer(serializers.ModelSerializer):
    """Serializer for Reading model. Read-only via viewset enforcement."""

    class Meta:
        model = Reading
        fields = '__all__'


class EventSerializer(serializers.ModelSerializer):
    """Serializer for Event model with full CRUD support."""

    class Meta:
        model = Event
        fields = '__all__'
        read_only_fields = ['timestamp']

    def create(self, validated_data):
        """Auto-set timestamp to current time if not provided."""
        if 'timestamp' not in validated_data:
            validated_data['timestamp'] = timezone.now()
        return super().create(validated_data)
