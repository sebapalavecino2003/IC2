"""
Vistas y ViewSets de la API REST NodeAlert.

Implementa los endpoints REST para dispositivos, lecturas, eventos,
autenticación y health checks. La arquitectura utiliza ViewSets de
DRF para operaciones CRUD estándar y APIView para endpoints específicos
como login y health checks.
"""
from rest_framework import status, viewsets, filters
from rest_framework.decorators import action
from rest_framework.permissions import AllowAny
from rest_framework.response import Response
from rest_framework.views import APIView
from rest_framework.authtoken.models import Token
from rest_framework.throttling import AnonRateThrottle
from django.db import connection
import django_filters
import subprocess
from .models import Device, Reading, Event
from .mqtt_publisher import publish_command
from .permissions import RolePermission
from .serializers import (DeviceSerializer, ReadingSerializer, EventSerializer,
                          LoginSerializer, CommandSerializer, UserSerializer)


class ReadingFilter(django_filters.FilterSet):
    """
    Filtro personalizado para el modelo Reading.

    Expone filtros por tipo de sensor exacto, device_id (resuelve la
    relación hacia Device), y rango de timestamps (gte/lte). Esto
    permite al frontend consultar lecturas de un dispositivo específico
    en una ventana de tiempo determinada sin filtrar en cliente.
    """

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
    """
    ViewSet completo para dispositivos (CRUD + comando remoto).

    Proporciona operaciones CRUD estándar sobre dispositivos, más un
    endpoint adicional 'command' que permite publicar comandos MQTT
    hacia un dispositivo específico. El acceso está controlado por
    RolePermission, que restringe según el rol del usuario.

    Acción adicional:
      POST /devices/{device_id}/command/ -- Publica un comando MQTT al dispositivo.
    """
    queryset = Device.objects.all()
    serializer_class = DeviceSerializer
    permission_classes = [RolePermission]
    lookup_field = 'device_id'
    filter_backends = [django_filters.rest_framework.DjangoFilterBackend,
                       filters.SearchFilter, filters.OrderingFilter]
    filterset_fields = ['is_active', 'location']
    search_fields = ['name', 'device_id', 'location']
    ordering_fields = ['created_at', 'name', 'device_id']

    @action(detail=True, methods=['post'])
    def command(self, request, pk=None):
        """
        Publica un comando MQTT hacia un dispositivo específico.

        El cuerpo de la solicitud debe contener el campo 'command' con
        uno de los valores válidos (buzzer_on, buzzer_off, etc.) y un
        campo opcional 'params' con parámetros adicionales.

        Responde con 502 si la publicación MQTT falla (el broker no
        está disponible o hay un error de conexión), permitiendo al
        frontend mostrar un mensaje de error apropiado.
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
    """
    ViewSet de solo lectura para lecturas de sensores.

    Solamente permite listar y recuperar lecturas individuales. La
    creación de lecturas ocurre exclusivamente a través del subscriber
    MQTT (management command mqtt_subscriber), garantizando que no
    se puedan insertar registros inconsistentes vía API.

    Soporta filtrado por tipo de sensor, device_id y rango de timestamps,
    más ordenamiento por timestamp descendente (default).
    """

    queryset = Reading.objects.all()
    serializer_class = ReadingSerializer
    filterset_class = ReadingFilter
    filter_backends = [django_filters.rest_framework.DjangoFilterBackend,
                       filters.OrderingFilter]
    ordering_fields = ['timestamp']


class EventFilter(django_filters.FilterSet):
    severity = django_filters.CharFilter(lookup_expr='exact')
    device_id = django_filters.CharFilter(
        field_name='device__device_id', lookup_expr='exact'
    )

    class Meta:
        model = Event
        fields = ['severity', 'resolved', 'device_id']


class EventViewSet(viewsets.ModelViewSet):
    """
    ViewSet completo para eventos y alertas.

    Permite CRUD completo. Los eventos se crean automáticamente desde
    el subscriber MQTT cuando un dispositivo reporta una condición,
    pero los operadores pueden actualizar el campo 'resolved' vía API
    para marcar eventos como atendidos.

    Filtros disponibles: severity, resolved, device_id.
    """
    queryset = Event.objects.all()
    serializer_class = EventSerializer
    permission_classes = [RolePermission]
    filterset_class = EventFilter
    filter_backends = [django_filters.rest_framework.DjangoFilterBackend,
                       filters.OrderingFilter]
    ordering_fields = ['timestamp', 'severity']


class LoginView(APIView):
    """
    Endpoint de autenticación de usuarios.

    Acepta credenciales username/password y devuelve un token de
    autenticación DRF. Es el único endpoint público (AllowAny) con
    rate limiting estricto (5 solicitudes/minuto) para mitigar ataques
    de fuerza bruta.

    El token generado debe incluirse en el header Authorization de
    todas las solicitudes posteriores como 'Token <valor>'.
    """

    authentication_classes = []
    permission_classes = [AllowAny]
    throttle_classes = [AnonRateThrottle]

    def post(self, request):
        """Valida credenciales, autentica al usuario y retorna un token."""
        serializer = LoginSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)
        user = serializer.validated_data['user']
        token, created = Token.objects.get_or_create(user=user)
        return Response({'token': token.key}, status=status.HTTP_200_OK)


class MeView(APIView):
    """
    Endpoint que retorna la información del usuario autenticado.

    Incluye el nombre de usuario, rol asignado y si es staff.
    El frontend utiliza esta información para mostrar la interfaz
    adecuada según los permisos del usuario.
    """

    def get(self, request):
        serializer = UserSerializer(request.user)
        return Response(serializer.data)


class LivenessHealthView(APIView):
    """
    Health check de liveness: verifica que el proceso Django responda.

    No verifica dependencias externas (base de datos, MQTT). Es el
    probe que Kubernetes/Docker Compose usa para saber si el contenedor
    debe reiniciarse. Público para que los healthchecks funcionen sin
    autenticación.
    """
    permission_classes = [AllowAny]

    def get(self, request):
        return Response({"status": "alive"}, status=status.HTTP_200_OK)


class ReadinessHealthView(APIView):
    """
    Health check de readiness: verifica dependencias del backend.

    Comprueba que la base de datos MySQL responda (mediante
    connection.ensure_connection) y que el proceso MQTT subscriber
    esté activo (mediante pgrep). Es el probe que determina si el
    contenedor puede recibir tráfico.

    Responde 503 si alguna dependencia falla, lo que permite a
    orquestadores evitar enrutar tráfico a una instancia no lista.
    """
    permission_classes = [AllowAny]

    def get(self, request):
        checks = {"database": False, "mqtt": False}

        # Verificación de conexión a base de datos.
        # ensure_connection lanza excepción si no puede conectar.
        try:
            connection.ensure_connection()
            checks["database"] = True
        except Exception:
            pass

        # Verificación del proceso MQTT subscriber mediante pgrep.
        # Busca el proceso por nombre (mqtt_subscriber) sin importar
        # sus argumentos completos.
        try:
            result = subprocess.run(
                ['pgrep', '-f', 'mqtt_subscriber'],
                capture_output=True, text=True,
            )
            checks["mqtt"] = (result.returncode == 0)
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
