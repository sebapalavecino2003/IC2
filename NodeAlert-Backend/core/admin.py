"""
Configuración del panel de administración Django para modelos del core.

Define cómo se presentan y gestionan los modelos en el admin de Django,
incluyendo columnas visibles, filtros, búsqueda y la relación inline
entre User y UserProfile.
"""
from django.contrib import admin
from .models import Device, Reading, Event, User, UserProfile


class UserProfileInline(admin.StackedInline):
    """
    Inline para editar el UserProfile directamente desde la página
    de usuario del admin. can_delete=False evita que se elimine el
    perfil accidentalmente al editar un usuario.
    """
    model = UserProfile
    can_delete = False


@admin.register(Device)
class DeviceAdmin(admin.ModelAdmin):
    """
    Administración de dispositivos con filtro por estado activo/inactivo
    y búsqueda por identificador, nombre o ubicación.
    """
    list_display = ['device_id', 'name', 'location', 'is_active', 'created_at']
    list_filter = ['is_active']
    search_fields = ['device_id', 'name', 'location']


@admin.register(Reading)
class ReadingAdmin(admin.ModelAdmin):
    """
    Administración de lecturas con filtro por tipo de sensor y fecha,
    más búsqueda por identificador del dispositivo padre.
    """
    list_display = ['device', 'sensor_type', 'value', 'unit', 'timestamp']
    list_filter = ['sensor_type', 'timestamp']
    search_fields = ['device__device_id']


@admin.register(Event)
class EventAdmin(admin.ModelAdmin):
    """
    Administración de eventos con filtro por severidad y estado de
    resolución, más búsqueda por dispositivo y tipo de evento.
    """
    list_display = ['device', 'event_type', 'severity', 'message', 'resolved', 'timestamp']
    list_filter = ['severity', 'resolved']
    search_fields = ['device__device_id', 'event_type']


@admin.register(User)
class UserAdmin(admin.ModelAdmin):
    """
    Administración de usuarios que incluye el perfil como inline.

    La columna personalizada get_role muestra el nombre legible del
    rol (según la choice display del modelo Role), no el valor interno.
    """
    inlines = [UserProfileInline]
    list_display = ['username', 'email', 'get_role', 'is_staff', 'is_active']

    @admin.display(description='Rol')
    def get_role(self, obj):
        """
        Retorna el nombre descriptivo del rol del usuario.

        get_role_display() es el método que Django genera automáticamente
        para campos con choices. Si el perfil no existe, muestra un
        guión como indicador visual.
        """
        profile = getattr(obj, 'profile', None)
        return profile.get_role_display() if profile else '—'
