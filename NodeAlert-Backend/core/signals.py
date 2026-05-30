"""
Señales de Django para la creación automática de perfiles de usuario.

Garantiza que cada usuario del sistema tenga un UserProfile asociado
con un rol por defecto, eliminando la necesidad de que un administrador
asigne roles manualmente tras cada registro.
"""
from django.db.models.signals import post_save
from django.dispatch import receiver
from .models import User, UserProfile, Role


@receiver(post_save, sender=User)
def create_user_profile(sender, instance, created, **kwargs):
    """
    Crea un UserProfile automáticamente al registrar un nuevo User.

    La asignación del rol sigue esta lógica de negocio:
      - Si el usuario es superuser o staff → rol ADMIN.
      - En cualquier otro caso → rol VIEWER (el más restrictivo).

    Esto asegura que los usuarios creados por defecto tengan el mínimo
    nivel de acceso necesario. Si un VIEWER necesita más permisos, un
    ADMIN debe actualizar su rol explícitamente.
    """
    if created:
        role = Role.ADMIN if (instance.is_superuser or instance.is_staff) else Role.VIEWER
        UserProfile.objects.create(user=instance, role=role)
