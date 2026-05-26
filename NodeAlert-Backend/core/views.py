"""DRF viewsets for core models: Device, Reading, Event."""
from rest_framework import status, viewsets, filters
from rest_framework.decorators import action
from rest_framework.permissions import AllowAny
from rest_framework.response import Response
from rest_framework.views import APIView
from rest_framework.authtoken.models import Token
from rest_framework.throttling import AnonRateThrottle
from django.db import connection
import django_filters
import os
from .models import Device, Reading, Event
from .mqtt_publisher import publish_command
from .serializers import (DeviceSerializer, ReadingSerializer, EventSerializer,
                          LoginSerializer, CommandSerializer)


class ReadingFilter(django_filters.FilterSet):
    """Custom FilterSet for Reading model with timestamp range and device_id lookup."""

    sensor_type = django_filters.CharFilter(lookup_expr='exact')
    device_id = django_filters.CharFilter(
        field_name='device__device_id', lookup_expr='exact'
    )
    timestamp__gte = django_filters.DateTimeFilter(
        field_name='timestamp', lookup_expr='gte'
    )
    timestamp__lte = django_filters.DateTimeFilter(
        field_name='timestamp', lookup_expr='lte'
    )

    class Meta:
        model = Reading
        fields = ['sensor_type', 'device_id', 'timestamp__gte', 'timestamp__lte']


class DeviceViewSet(viewsets.ModelViewSet):
    """Full CRUD viewset for Device model.

    Supports filtering by is_active, location.
    Supports search by name, device_id, location.
    Supports ordering by created_at, name, device_id.
    """

    queryset = Device.objects.all()
    serializer_class = DeviceSerializer
    filter_backends = [django_filters.rest_framework.DjangoFilterBackend,
                       filters.SearchFilter, filters.OrderingFilter]
    filterset_fields = ['is_active', 'location']
    search_fields = ['name', 'device_id', 'location']
    ordering_fields = ['created_at', 'name', 'device_id']

    @action(detail=True, methods=['post'])
    def command(self, request, pk=None):
        """Publish an MQTT command to a device (AUTO-04).

        POST body: {"command": "actuator_on", "params": {...}}
        Valid commands: actuator_on, actuator_off, return_to_auto,
                       acknowledge_alarm, update_thresholds
        """
        device = self.get_object()

        serializer = CommandSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)

        command = serializer.validated_data['command']
        params = serializer.validated_data.get('params')

        success = publish_command(device.device_id, command, params)
        if not success:
            return Response(
                {'error': 'Failed to publish command to MQTT broker'},
                status=status.HTTP_502_BAD_GATEWAY,
            )

        return Response({
            'status': 'published',
            'device': device.device_id,
            'command': command,
        })


class ReadingViewSet(viewsets.ReadOnlyModelViewSet):
    """Read-only viewset for Reading model.

    Supports filtering by sensor_type, device_id, timestamp__gte, timestamp__lte.
    Supports ordering by timestamp.
    """

    queryset = Reading.objects.all()
    serializer_class = ReadingSerializer
    filterset_class = ReadingFilter
    filter_backends = [django_filters.rest_framework.DjangoFilterBackend,
                       filters.OrderingFilter]
    ordering_fields = ['timestamp']


class EventViewSet(viewsets.ModelViewSet):
    """Full CRUD viewset for Event model.

    Supports filtering by severity, resolved.
    Supports ordering by timestamp, severity.
    """

    queryset = Event.objects.all()
    serializer_class = EventSerializer
    filter_backends = [django_filters.rest_framework.DjangoFilterBackend,
                       filters.OrderingFilter]
    filterset_fields = ['severity', 'resolved']
    ordering_fields = ['timestamp', 'severity']


class LoginView(APIView):
    """View for user authentication via username/password.

    Accepts POST requests with username and password, validates credentials,
    and returns a DRF Token. Publicly accessible (AllowAny).
    Rate limited: 5 requests/min per IP (D-14).
    """

    permission_classes = [AllowAny]
    throttle_classes = [AnonRateThrottle]

    def post(self, request):
        """Validate credentials and return auth token."""
        serializer = LoginSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        user = serializer.validated_data['user']
        token, created = Token.objects.get_or_create(user=user)
        return Response({'token': token.key}, status=status.HTTP_200_OK)


class LivenessHealthView(APIView):
    """GET /api/v1/health/ — Liveness check. No dependencies. Public."""
    permission_classes = [AllowAny]

    def get(self, request):
        return Response({"status": "alive"}, status=status.HTTP_200_OK)


class ReadinessHealthView(APIView):
    """GET /api/v1/health/ready/ — Readiness check. Verifies MySQL + MQTT.

    Returns 200 if all dependencies healthy, 503 if any fail.
    Public endpoint — no sensitive data in response.
    """
    permission_classes = [AllowAny]

    def get(self, request):
        checks = {"database": False, "mqtt": False}

        try:
            connection.ensure_connection()
            checks["database"] = True
        except Exception:
            pass

        try:
            mqtt_alive = os.system(
                'pgrep -f "mqtt_subscriber" > /dev/null 2>&1'
            )
            checks["mqtt"] = (mqtt_alive == 0)
        except Exception:
            pass

        if all(checks.values()):
            return Response(
                {"status": "ready", "checks": checks},
                status=status.HTTP_200_OK
            )
        return Response(
            {"status": "not ready", "checks": checks},
            status=status.HTTP_503_SERVICE_UNAVAILABLE
        )
