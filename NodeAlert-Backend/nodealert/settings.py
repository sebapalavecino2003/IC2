"""
Configuración principal del proyecto Django NodeAlert.

Define todos los aspectos de configuración del backend: aplicaciones
instaladas, middleware, base de datos, autenticación, REST framework
y CORS. La configuración está orientada a ejecución en contenedores
Docker con valores hardcodeados para el entorno de desarrollo/
demostración.
"""
import sys
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent

SECRET_KEY = 'contraseña'

DEBUG = True

ALLOWED_HOSTS = ['*']

# Aplicaciones instaladas.
# El orden es importante: django.contrib.admin debe ir después de
# django.contrib.auth y django.contrib.contenttypes.
INSTALLED_APPS = [
    'corsheaders',
    'django.contrib.admin',
    'django.contrib.auth',
    'django.contrib.contenttypes',
    'django.contrib.sessions',
    'django.contrib.messages',
    'django.contrib.staticfiles',
    'rest_framework',
    'rest_framework.authtoken',
    'django_filters',
    'core',
]

# Middleware.
# CorsMiddleware debe estar lo más arriba posible para que las
# respuestas a OPTIONS preflight incluyan los headers CORS.
MIDDLEWARE = [
    'corsheaders.middleware.CorsMiddleware',
    'django.middleware.security.SecurityMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django.middleware.common.CommonMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware',
    'django.contrib.auth.middleware.AuthenticationMiddleware',
    'django.contrib.messages.middleware.MessageMiddleware',
    'django.middleware.clickjacking.XFrameOptionsMiddleware',
]

ROOT_URLCONF = 'nodealert.urls'

TEMPLATES = [
    {
        'BACKEND': 'django.template.backends.django.DjangoTemplates',
        'DIRS': [],
        'APP_DIRS': True,
        'OPTIONS': {
            'context_processors': [
                'django.template.context_processors.debug',
                'django.template.context_processors.request',
                'django.contrib.auth.context_processors.auth',
                'django.contrib.messages.context_processors.messages',
            ],
        },
    },
]

WSGI_APPLICATION = 'nodealert.wsgi.application'

# Base de datos MySQL.
# Configurada para conectarse al contenedor 'mysql' en la red Docker
# interna. El modo SQL STRICT_TRANS_TABLES evita truncamientos de datos
# silenciosos que MySQL permite por defecto.
DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.mysql',
        'NAME': 'nodealert',
        'USER': 'nodealert',
        'PASSWORD': 'nodealert',
        'HOST': 'mysql',
        'PORT': '3306',
        'OPTIONS': {
            'init_command': "SET sql_mode='STRICT_TRANS_TABLES'",
        },
    }
}

# En tests, usar root para evitar problemas de permisos con las
# migraciones de test que Django ejecuta.
if 'test' in sys.argv:
    DATABASES['default']['USER'] = 'root'
    DATABASES['default']['PASSWORD'] = 'root_password'


# Validadores de contraseña de Django.
# Se aplican al crear o modificar usuarios a través del admin.
AUTH_PASSWORD_VALIDATORS = [
    {'NAME': 'django.contrib.auth.password_validation.UserAttributeSimilarityValidator'},
    {'NAME': 'django.contrib.auth.password_validation.MinimumLengthValidator'},
    {'NAME': 'django.contrib.auth.password_validation.CommonPasswordValidator'},
    {'NAME': 'django.contrib.auth.password_validation.NumericPasswordValidator'},
]

# Internacionalización.
LANGUAGE_CODE = 'en-us'
TIME_ZONE = 'UTC'
USE_I18N = True
USE_TZ = True

# Archivos estáticos (CSS, JavaScript, imágenes).
STATIC_URL = 'static/'
STATIC_ROOT = BASE_DIR / 'static'

# Tipo de clave primaria por defecto para todos los modelos.
# BigAutoField = entero de 64 bits, soporta >2^31 registros.
DEFAULT_AUTO_FIELD = 'django.db.models.BigAutoField'

# Modelo de usuario personalizado.
# Reemplaza el AbstractUser de Django con nuestra versión en core.models.
AUTH_USER_MODEL = 'core.User'

# Configuración de Django REST Framework.
# Autenticación por token (API) y sesión (navegador DRF).
# Paginación por defecto de 50 elementos.
# Rate limiting: 100 req/min para usuarios autenticados, 5 req/min para anónimos.
REST_FRAMEWORK = {
    'DEFAULT_AUTHENTICATION_CLASSES': [
        'rest_framework.authentication.TokenAuthentication',
        'rest_framework.authentication.SessionAuthentication',
    ],
    'DEFAULT_PERMISSION_CLASSES': [
        'rest_framework.permissions.IsAuthenticated',
    ],
    'DEFAULT_PAGINATION_CLASS': 'rest_framework.pagination.PageNumberPagination',
    'PAGE_SIZE': 50,
    'DEFAULT_FILTER_BACKENDS': ['django_filters.rest_framework.DjangoFilterBackend'],
    'DEFAULT_THROTTLE_CLASSES': [
        'rest_framework.throttling.UserRateThrottle',
    ],
    'DEFAULT_THROTTLE_RATES': {
        'user': '100/minute',
        'anon': '5/minute',
    },
}

# Configuración CORS para permitir solicitudes desde el frontend.
# Incluye orígenes de desarrollo (Vite en puerto 5173) y producción
# (nginx en puerto 80/3000).
CORS_ALLOWED_ORIGINS = [
    "http://localhost:3000",
    "http://127.0.0.1:3000",
    "http://localhost:5173",
    "http://127.0.0.1:5173",
    "http://localhost:80",
    "http://127.0.0.1:80",
    "http://localhost",
]

CORS_ALLOW_CREDENTIALS = True
