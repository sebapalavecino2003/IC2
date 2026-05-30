"""
Modelos principales del dominio NodeAlert IoT.

Define las entidades fundamentales del sistema de monitoreo ambiental:
dispositivos (ESP32), lecturas de sensores, eventos/alertas, usuarios
y perfiles con roles. Cada modelo refleja una necesidad de negocio
identificada en la arquitectura del sistema.
"""
from django.db import models
from django.contrib.auth.models import AbstractUser

# Opciones de tipo de sensor que puede reportar un nodo ESP32.
# Cada valor corresponde al campo 'sensor_type' en el modelo Reading
# y es utilizado por el subscriber MQTT para mapear claves del JSON
# entrante al tipo correcto en base de datos.
SENSOR_TYPE_CHOICES = [
    ('temperature', 'Temperature'),
    ('humidity', 'Humidity'),
    ('gas', 'Gas'),
    ('flame', 'Flame'),
]

# Niveles de severidad para eventos y alertas del sistema.
# Se usan para priorizar la atención: critical requiere acción inmediata,
# warning indica condición anómala no crítica, info es informativo.
SEVERITY_CHOICES = [
    ('info', 'Info'),
    ('warning', 'Warning'),
    ('critical', 'Critical'),
]


class Device(models.Model):
    """
    Representa un nodo sensor ESP32 desplegado en campo.

    Cada dispositivo tiene un identificador único (device_id) que se
    utiliza como clave en los topics MQTT para enrutar comandos y
    telemetría. El campo is_active permite desactivar un nodo sin
    eliminar su historial, útil cuando un dispositivo se da de baja
    o se reemplaza.
    """
    device_id = models.CharField(max_length=50, unique=True)
    name = models.CharField(max_length=100)
    mac_address = models.CharField(max_length=17, blank=True, default='')
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
    """
    Lectura individual de un sensor perteneciente a un dispositivo.
    Cada instancia representa una muestra puntual en el tiempo de una
    magnitud física (temperatura, humedad, gas, llama). El índice
    compuesto (device, sensor_type) optimiza las consultas de historial
    por dispositivo y tipo de sensor, que son las más frecuentes en
    el dashboard y las APIs de monitoreo.
    """

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
    """
    Evento o alerta generada por un dispositivo.

    Los eventos representan condiciones que requieren atención: superación
    de umbrales, detección de llama, cambios de configuración, etc.
    El campo resolved permite al operador marcar eventos como atendidos
    sin eliminar el registro histórico. El índice sobre (device, severity)
    acelera las consultas de alertas activas filtradas por gravedad.
    """

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
    """
    Modelo de usuario personalizado que extiende AbstractUser.

    Se utiliza AUTH_USER_MODEL en settings.py para que Django use
    este modelo en lugar del default. Actualmente no agrega campos
    extra; los roles se delegan al modelo UserProfile para mantener
    separada la lógica de autorización de la autenticación base.
    """

    class Meta:
        verbose_name_plural = 'users'

    def __str__(self):
        return self.username


class Role(models.TextChoices):
    """
    Roles del sistema para control de acceso basado en permisos (RBAC).

    ADMIN: Acceso completo (CRUD + comandos remotos).
    OPERATOR: Lectura, actualización de eventos y envío de comandos.
    VIEWER: Solo lectura de dashboards y reportes.
    """
    ADMIN = 'admin', 'Administrador'
    OPERATOR = 'operator', 'Operador'
    VIEWER = 'viewer', 'Visualizador'


class UserProfile(models.Model):
    """
    Perfil extendido del usuario con asignación de rol.

    Se crea automáticamente mediante una señal post_save al registrar
    un nuevo usuario. La relación OneToOne garantiza que cada usuario
    tenga exactamente un perfil. El rol por defecto es VIEWER para
    evitar que usuarios nuevos tengan permisos no intencionados.
    """

    user = models.OneToOneField(
        User, on_delete=models.CASCADE, related_name='profile'
    )
    role = models.CharField(
        max_length=20, choices=Role.choices, default=Role.VIEWER
    )

    class Meta:
        verbose_name_plural = 'user profiles'

    def __str__(self):
        return f"{self.user.username} ({self.get_role_display()})"
