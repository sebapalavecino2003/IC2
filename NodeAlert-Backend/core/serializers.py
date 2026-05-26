"""DRF serializers for core models: Device, Reading, Event."""
from django.contrib.auth import authenticate
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


class LoginSerializer(serializers.Serializer):
    """Serializer for user authentication via username/password.

    Validates credentials against Django auth backend and returns
    the authenticated user for token generation.
    """

    username = serializers.CharField()
    password = serializers.CharField(write_only=True, style={'input_type': 'password'})

    def validate(self, attrs):
        """Validate credentials via Django auth backend."""
        user = authenticate(
            username=attrs['username'],
            password=attrs['password']
        )
        if user is None:
            raise serializers.ValidationError("Invalid credentials")
        if not user.is_active:
            raise serializers.ValidationError("User account is disabled")
        attrs['user'] = user
        return attrs


class CommandSerializer(serializers.Serializer):
    """Serializer for device MQTT commands (AUTO-04)."""
    command = serializers.ChoiceField(choices=[
        'buzzer_on', 'buzzer_off', 'return_to_auto',
        'acknowledge_alarm', 'update_thresholds',
    ])
    params = serializers.JSONField(required=False, default=None)
