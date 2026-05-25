"""DRF viewsets for core models: Device, Reading, Event."""
from rest_framework import status, viewsets, filters
from rest_framework.decorators import action
from rest_framework.permissions import AllowAny
from rest_framework.response import Response
from rest_framework.views import APIView
from rest_framework.authtoken.models import Token
import django_filters
from .models import Device, Reading, Event
from .serializers import (DeviceSerializer, ReadingSerializer, EventSerializer,
                          LoginSerializer)


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
    """

    permission_classes = [AllowAny]

    def post(self, request):
        """Validate credentials and return auth token."""
        serializer = LoginSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        user = serializer.validated_data['user']
        token, created = Token.objects.get_or_create(user=user)
        return Response({'token': token.key}, status=status.HTTP_200_OK)
