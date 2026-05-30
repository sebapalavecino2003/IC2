"""
Sistema de permisos basado en roles (RBAC) para NodeAlert.

Implementa un control de acceso granular donde cada usuario tiene un
rol (Admin, Operator, Viewer) que determina qué acciones puede realizar
sobre los recursos del sistema. Los permisos se verifican a nivel de
vista mediante una clase Permission personalizada de DRF.

Mapeo de acciones:
  - Admin:    CRUD completo + comandos remotos
  - Operator: Lectura + comandos + resolución de eventos (PATCH)
  - Viewer:   Solo lectura (list, retrieve)
"""
from rest_framework import permissions
from .models import Role

# Mapeo de métodos HTTP a acciones del sistema.
# Permite que las vistas que no usan ViewSet (APIView) también sean
# controladas por el mismo sistema de permisos.
ACTION_MAP = {
    'GET': 'read',
    'HEAD': 'read',
    'OPTIONS': 'read',
    'POST': 'create',
    'PUT': 'update',
    'PATCH': 'update',
    'DELETE': 'delete',
}

# Mapeo de acciones de ViewSet DRF a acciones del sistema.
# Cubre tanto las acciones estándar (create, list, retrieve, etc.)
# como las acciones personalizadas (command).
VIEWSET_ACTIONS = {
    'create': 'create',
    'list': 'read',
    'retrieve': 'read',
    'update': 'update',
    'partial_update': 'update',
    'destroy': 'delete',
    'command': 'command',
}

# Matriz de permisos por rol.
# Cada entrada define el conjunto de acciones permitidas para ese rol.
# Si se agrega un nuevo rol en el futuro, debe registrarse aquí.
ROLE_PERMISSIONS = {
    Role.ADMIN:    {'create', 'read', 'update', 'delete', 'command'},
    Role.OPERATOR: {'read', 'update', 'command'},
    Role.VIEWER:   {'read'},
}


class RolePermission(permissions.BasePermission):
    """
    Permiso personalizado basado en el rol del UserProfile.

    La verificación ocurre en dos pasos:
      1. Determinar la acción solicitada según el ViewSet.action o
         el método HTTP.
      2. Verificar que el rol del usuario tenga esa acción en su
         conjunto de permisos.

    Si el usuario no tiene perfil (profile es None), se deniega el
    acceso automáticamente como medida de seguridad.
    """

    def has_permission(self, request, view):
        # Rechazar inmediatamente si el usuario no está autenticado.
        if not request.user or not request.user.is_authenticated:
            return False

        # Obtener el rol desde el UserProfile asociado al usuario.
        # La doble guarda con getattr maneja el caso donde el perfil
        # aún no fue creado (ej. mid-migration).
        role = getattr(getattr(request.user, 'profile', None), 'role', None)
        if role is None:
            return False

        # Determinar la acción solicitada priorizando el ViewSet.action
        # (para ViewSets) y cayendo al método HTTP (para APIView).
        action = VIEWSET_ACTIONS.get(view.action)
        if action is None:
            action = ACTION_MAP.get(request.method, 'read')

        # Verificar si el rol del usuario tiene permiso para la acción.
        allowed = ROLE_PERMISSIONS.get(role, {Role.VIEWER})
        return action in allowed
