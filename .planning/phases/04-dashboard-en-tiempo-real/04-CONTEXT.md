# Phase 4: Dashboard en Tiempo Real - Context

**Gathered:** 2026-05-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Construir el dashboard web React + Vite + Material UI con visualización en tiempo real de lecturas de sensores, gráficos históricos, panel de alertas/eventos, autenticación de usuarios y estado de conexión del nodo ESP32. Incluye el servicio frontend como contenedor Docker independiente servido por Nginx. No incluye automatización de actuadores (fase 5), comandos remotos desde el dashboard (fase 5), ni notificaciones externas (v2).

</domain>

<decisions>
## Implementation Decisions

### Transporte de Datos en Tiempo Real
- **D-01:** Polling optimizado con Axios cada 3 segundos como mecanismo de actualización en tiempo real. No usar Django Channels, WebSockets ni Redis — evita saturar la RAM de la Raspberry Pi.
- **D-02:** Context API global para mantener el estado de las lecturas y disparar alarmas visuales/sonoras inmediatamente al detectar condiciones críticas.

### Librería de Gráficos
- **D-03:** Recharts para las gráficas de telemetría — 100% declarativo, amigable con el ecosistema React. Usar gráficos de líneas (LineChart) para temperatura y humedad a lo largo del tiempo.

### Estrategia de Despliegue Frontend
- **D-04:** Contenedor Docker separado con Nginx sirviendo el build estático de Vite (producción). El frontend no se sirve desde Django. Mantiene desacoplamiento total entre Frontend y Backend.
- **D-05:** El contenedor frontend se conecta a la red `nodealert-net` para comunicarse con el backend Django vía API REST + polling HTTP.

### Diseño Visual (Industrial Dark Theme)
- **D-06:** Layout basado en Grid responsivo de Material UI con estética Industrial Dark Theme (fondo oscuro, paneles tipo dashboard industrial).
- **D-07:** Código de colores semáforo estricto para umbrales de sensores:
  - Verde = Normal (todo dentro de rangos seguros)
  - Amarillo = Advertencia (valor elevado pero no crítico)
  - Rojo parpadeante = Alerta crítica (Gas > 300 PPM o Flama detectada)
- **D-08:** Alarma visual + sonora inmediata ante condiciones críticas, disparada desde Context API global.

### Autenticación
- **D-09:** Reutilizar el sistema de autenticación existente (DRF Token Auth). El frontend envía token en header `Authorization: Token <key>` para todas las peticiones API autenticadas.

### the agent's Discretion
- Estructura exacta del árbol de componentes React
- Nombre y organización del proyecto Vite
- Configuración de Nginx (Dockerfile, nginx.conf)
- Implementación del sonido de alarma
- Estrategia de caché y optimización de polling
- Puerto del contenedor frontend
- Variables de entorno del frontend
- Rutas de React Router (si aplica)

</decisions>

<canonical_refs>
## Canonical References

Downstream agents MUST read these before planning or implementing.

### Project Context
- `.planning/PROJECT.md` — Project context, core value, constraints
- `.planning/REQUIREMENTS.md` — Requirements traceability (API-02, UI-01–06)
- `.planning/ROADMAP.md` — Phase overview, goal, success criteria
- `.planning/STATE.md` — Current project state

### Phase 2 Context (Backend Infrastructure)
- `.planning/phases/02-core-infrastructure-gateway-mqtt/02-CONTEXT.md` — Docker Compose, Mosquitto config, API endpoints, auth, REST viewsets
- `NodeAlert-Backend/core/views.py` — Existing DRF viewsets (Device, Reading, Event, Login)
- `NodeAlert-Backend/core/serializers.py` — API serializers
- `NodeAlert-Backend/core/models.py` — Device, Reading, Event models
- `NodeAlert-Backend/nodealert/settings.py` — Django settings, REST_FRAMEWORK config, authtoken
- `docker-compose.yml` — Existing services (mosquitto, django, mysql), nodealert-net

### Phase 3 Context (MQTT Integration — sensor data flow)
- `.planning/phases/03-mqtt-firmware-integration/03-CONTEXT.md` — MQTT topics, payload format, telemetry ingestion
- `NodeAlert-Backend/core/management/commands/mqtt_subscriber.py` — MQTT subscriber (data source)

### No external specs
No external specs — requirements fully captured in decisions above

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **DRF Token Auth**: Login endpoint (`POST /api/v1/auth/login`) exists and returns token — frontend can authenticate immediately.
- **Readings API**: `GET /api/v1/readings` con filtros por `sensor_type`, `device_id`, `timestamp__gte`, `timestamp__lte` — ready for chart data.
- **Devices API**: `GET /api/v1/devices` con `is_active` — ready for connection status panel.
- **Events API**: `GET /api/v1/events` con severidad y resolución — ready for alerts panel.
- **Docker Compose**: `nodealert-net` ya definida para conectar el nuevo servicio frontend.

### Missing (to build)
- No existe proyecto frontend React + Vite
- No hay middleware CORS configurado en Django (necesario si frontend se sirve desde otro puerto)
- No hay servicio Docker para el frontend
- No hay configuración de Nginx
- No hay proxy reverso para API requests en desarrollo

### Integration Points
- Frontend → Backend REST API en `http://django:8000/api/v1/` (Docker internal) o `http://raspberry-pi-ip:8000/` (remote)
- Frontend se conecta a `nodealert-net` para comunicación con Django
- Docker Compose: nuevo servicio `frontend` con `depends_on: django`

</code_context>

<specifics>
## Specific Ideas

- Polling cada 3s es suficiente para un dashboard de monitoreo ambiental (las lecturas no cambian a milisegundos)
- Context API debe manejar: lecturas actuales, estado de alerta, historial para gráficos, token de autenticación
- El sonido de alarma debe ser un beep simple, no una sirena prolongada
- El estado de conexión del ESP32 se determina por la presencia de lecturas recientes (últimos 30s con datos = conectado)
- Gráficos Recharts con zoom y tooltip para exploración de datos históricos

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 4-dashboard-en-tiempo-real*
*Context gathered: 2026-05-25*
