"""
Configuración de la aplicación Django 'core'.

Define los metadatos de la aplicación y el punto de entrada para
registrar señales. El método ready() se ejecuta cuando Django termina
de cargar todas las aplicaciones, momento seguro para importar señales
sin riesgos de importación circular.
"""
from django.apps import AppConfig


class CoreConfig(AppConfig):
    """
    Configuración principal de la aplicación core.

    default_auto_field: Define el tipo de clave primaria por defecto
      para los modelos de esta aplicación (BigAutoField = 64 bits).
    name: Ruta Python de la aplicación (debe coincidir con INSTALLED_APPS).
    verbose_name: Nombre legible en el panel de administración.
    """
    default_auto_field = 'django.db.models.BigAutoField'
    name = 'core'
    verbose_name = 'NodeAlert Core'

    def ready(self):
        """
        Registra las señales de la aplicación.

        La importación se realiza dentro de ready() en lugar de al
        inicio del archivo para evitar importar modelos antes de que
        Django haya completado su inicialización, lo que causaría
        errores de importación circular.
        """
        import core.signals  # noqa
