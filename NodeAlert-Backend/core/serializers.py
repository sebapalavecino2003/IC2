"""
Serializadores DRF para los modelos del core.

Cada serializador define cómo se convierten los modelos a/from JSON,
incluyendo validaciones de negocio (credenciales, formatos, campos
requeridos) y transformaciones (timestamps automáticos, resolución de
relaciones).
"""
from django.contrib.auth import authenticate
from django.utils import timezone
from rest_framework import serializers
from .models import Device, Reading, Event, User


class DeviceSerializer(serializers.ModelSerializer):
    """
    Serializador estándar para el modelo Device.

    Expone todos los campos del modelo. Los timestamps created_at y
    updated_at son de solo lectura porque los gestiona Django mediante
    auto_now_add y auto_now respectivamente.
    """

    class Meta:
        model = Device
        fields = '__all__'
        read_only_fields = ['created_at', 'updated_at']


class ReadingSerializer(serializers.ModelSerializer):
    """
    Serializador para el modelo Reading.

    Aunque el ViewSet es de solo lectura, el serializador soporta
    escritura porque el subscriber MQTT crea instancias directamente
    desde código (no vía API REST).
    """

    class Meta:
        model = Reading
        fields = '__all__'


class EventSerializer(serializers.ModelSerializer):
    """
    Serializador para eventos con asignación automática de timestamp.

    Si el timestamp no se proporciona en la creación, se asigna la
    hora actual del servidor. Esto permite que eventos creados desde
    el subscriber MQTT (que sí incluyen timestamp del dispositivo)
    preserven su tiempo original, mientras que eventos creados
    manualmente desde el admin usen la hora del servidor.
    """

    class Meta:
        model = Event
        fields = '__all__'
        read_only_fields = ['timestamp']

    def create(self, validated_data):
        """Asigna timestamp actual si no fue provisto en la creación."""
        if 'timestamp' not in validated_data:
            validated_data['timestamp'] = timezone.now()
        return super().create(validated_data)


class LoginSerializer(serializers.Serializer):
    """
    Serializador de autenticación por username/password.

    No está vinculado a un modelo. Valida las credenciales contra el
    backend de autenticación de Django y retorna el usuario autenticado
    para que la vista genere el token correspondiente.

    La contraseña se marca como write_only para que nunca sea incluida
    en las respuestas de la API.
    """

    username = serializers.CharField()
    password = serializers.CharField(write_only=True, style={'input_type': 'password'})

    def validate(self, attrs):
        """
        Autentica al usuario contra el backend de Django.

        Retorna error si las credenciales son inválidas o si la cuenta
        está desactivada (is_active=False). Esto último permite que un
        administrador deshabilite el acceso sin eliminar el usuario.
        """
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
    """
    Serializador para comandos MQTT dirigidos a dispositivos.

    Valida que el comando sea uno de los soportados por el firmware
    ESP32. El campo params es opcional y permite pasar configuración
    adicional (ej. nuevos umbrales para update_thresholds).

    Los comandos válidos corresponden a los implementados en el
    callback MQTT del firmware (mqttCallback en NodeAlert-Firmware.ino).
    """
    command = serializers.ChoiceField(choices=[
        'buzzer_on', 'buzzer_off', 'return_to_auto',
        'acknowledge_alarm', 'update_thresholds',
    ])
    params = serializers.JSONField(required=False, default=None)


class UserSerializer(serializers.ModelSerializer):
    """
    Serializador para el modelo User que incluye el rol del perfil.

    El campo 'role' se obtiene del UserProfile relacionado mediante un
    método SerializerMethodField. Si el perfil no existe (caso borde),
    retorna 'viewer' como valor por defecto para no romper el frontend.
    """

    role = serializers.SerializerMethodField()

    class Meta:
        model = User
        fields = ['id', 'username', 'role', 'is_staff']

    def get_role(self, obj):
        """
        Obtiene el rol desde el UserProfile asociado.

        Usa getattr para ser tolerante a la ausencia del perfil
        (ej. durante migraciones o creación de superusuarios antes
        de que la señal post_save haya ejecutado).
        """
        profile = getattr(obj, 'profile', None)
        return profile.role if profile else 'viewer'
