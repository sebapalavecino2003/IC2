# Guía de Despliegue — NodeAlert IoT

## Requisitos

- **Hardware:** Raspberry Pi 3/4/5 (arm64) o servidor Linux (amd64) con Docker
- **Docker Engine** 24+ con Docker Compose V2
- **ESP32** con PlatformIO para compilar firmware
- **Python 3.10+** para ejecutar `setup.sh`
- **Git** para clonar el repositorio

## Arquitectura de Despliegue

El sistema se despliega como 5 contenedores Docker:

```
[Cliente Web] → nginx (puerto 80) → django:8000 (Gunicorn) → MySQL
                                    → /static/ (nginx directo)
[ESP32] → Mosquitto (1883) → Django MQTT Subscriber
```

- **nginx:** Proxy inverso, punto de entrada único en puerto 80
- **Django:** API REST con Gunicorn, sin exposición directa de puertos
- **Mosquitto:** Broker MQTT para comunicación con ESP32
- **MySQL:** Base de datos relacional
- **Frontend:** SPA React servida por nginx en puerto 3000

## Paso a Paso

### 1. Clonar el repositorio

```bash
git clone <repo-url> nodealert-iot
cd nodealert-iot
```

### 2. Ejecutar script de despliegue

```bash
./setup.sh
```

El script `setup.sh` guía interactivamente la configuración:

- Credenciales WiFi (para generar cabeceras de firmware)
- Credenciales del broker MQTT
- Credenciales de MySQL
- Configuración de Django (secret key, superuser)
- Generación de `.env` con todas las variables
- Generación de archivo `mosquitto_passwd`
- Inicio de `docker compose up -d`
- Migraciones de base de datos Django
- Creación de superusuario Django
- Generación de cabecera de configuración del firmware (`user_config.h`)

### 3. Compilar firmware ESP32

```bash
cd NodeAlert-Firmware
pio run
pio run --target upload   # Conectar ESP32 por USB
cd ..
```

La configuración de pines está en `src/config/pins_config.h`:

| Sensor | Pin |
|--------|-----|
| DHT22 (temperatura/humedad) | GPIO 4 |
| MQ-9 (gas) | GPIO 34 (ADC) |
| KY-026 (llama) | GPIO 5 (digital), GPIO 35 (ADC) |
| Buzzer activo | GPIO 2 (alarma sonora) |

### 4. Verificar health checks

```bash
# Liveness — debe responder 200
curl http://localhost:80/api/v1/health/

# Readiness — verifica MySQL + MQTT
curl http://localhost:80/api/v1/health/ready/

# Estado de contenedores
docker compose ps
```

### 5. Producción vs Desarrollo

El backend soporta dos modos mediante variable de entorno:

```bash
# Producción: Gunicorn (por defecto 3 workers)
GUNICORN_ENABLED=true docker compose up -d

# Desarrollo: runserver de Django
GUNICORN_ENABLED=false docker compose up -d
```

El número de workers de Gunicorn se configura con `GUNICORN_WORKERS` (default: 3).

## Troubleshooting

### El broker MQTT no conecta

```bash
# Verificar logs de Mosquitto
docker compose logs mosquitto

# Verificar que el archivo mosquitto_passwd existe y es válido
ls -la mosquitto/config/mosquitto_passwd
```

### El ESP32 no compila

```bash
# Verificar que PlatformIO está instalado
pio --version

# Forzar limpieza y recompilar
pio run --target clean && pio run
```

### Health check falla

```bash
# Verificar que todos los contenedores están running
docker compose ps

# Revisar logs del contenedor específico
docker compose logs django
docker compose logs mysql
```

### Puerto 80 ocupado

Si el puerto 80 ya está en uso, cambiar el mapeo en `docker-compose.yml`:

```yaml
nginx:
  ports:
    - "8080:80"   # Cambiar 80 por 8080
```
