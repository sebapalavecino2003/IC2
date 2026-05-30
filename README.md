# NodeAlert IoT

Sistema distribuido de monitoreo ambiental crítico para detección temprana de incendios, fugas de gases y automatización preventiva.

## Stack Tecnológico

| Componente | Tecnología |
|------------|------------|
| Firmware (ESP32) | C++, PlatformIO, Arduino framework |
| Backend | Django 5, DRF, MySQL 8, Gunicorn |
| Frontend | React 18, JavaScript, Vite, Material UI |
| Broker MQTT | Mosquitto 2 |
| Infraestructura | Docker Compose |

## Inicio Rápido

```bash
# 1. Configurar variables de entorno (editar .env con tus credenciales)
cp .env.example .env
# O generar automáticamente:
./setup.sh

# 2. Iniciar servidores
docker compose up -d --build

# 3. Abrir frontend
http://localhost:3000
```

## Componentes

- [Backend Django](NodeAlert-Backend/README.md)
- [Frontend React](NodeAlert-Frontend/README.md)
- Firmware ESP32 — `Molotica/`
