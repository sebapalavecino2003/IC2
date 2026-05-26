# NodeAlert IoT

Sistema distribuido de monitoreo ambiental crítico para detección temprana de incendios, fugas de gases y automatización preventiva.

## Stack Tecnológico

| Componente | Tecnología |
|------------|------------|
| Firmware (ESP32) | C++17, FreeRTOS, PlatformIO, ESP-IDF 6.0.1 |
| Backend | Django 5, DRF, MySQL 8, Gunicorn, nginx |
| Frontend | React 18, TypeScript, Vite, Material UI |
| Broker MQTT | Mosquitto 2 |
| Infraestructura | Docker Compose (Raspberry Pi / Linux) |

## Inicio Rápido

```bash
# 1. Clonar el repositorio
git clone <repo-url> && cd nodealert-iot

# 2. Configurar variables de entorno (editar .env con tus credenciales)
cp .env.example .env

# 3. Ejecutar script de despliegue
./setup.sh

# 4. Compilar firmware
cd NodeAlert-Firmware && pio run && cd ..

# 5. Iniciar servidores
docker compose up -d
```

## Documentación

- [Guía de Despliegue](docs/DEPLOY.md)
- [Arquitectura del Sistema](docs/ARCHITECTURE.md)
- [Referencia de API](docs/API.md)

## Componentes

- [Firmware ESP32](NodeAlert-Firmware/README.md)
- [Backend Django](NodeAlert-Backend/README.md)
- [Frontend React](NodeAlert-Frontend/README.md)
