# Phase 4: Dashboard en Tiempo Real - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-25
**Phase:** 4-dashboard-en-tiempo-real
**Areas discussed:** Real-time transport, Chart library, Frontend serving strategy, Theme & layout

---

## Real-Time Transport

| Option | Description | Selected |
|--------|-------------|----------|
| Django Channels + Redis | Full WebSocket support, proper Django approach, adds Redis dependency | |
| Server-Sent Events | Simpler than WebSockets, one-directional only | |
| Polling optimizado (Axios cada 3s) | Simple polling with Context API for alarm state, no extra dependencies | ✓ |

**User's choice:** Polling optimizado (Axios cada 3s) con Context API global para disparar el estado de alarma visual/sonora inmediatamente. Evitamos Django Channels/Redis para no saturar la RAM de la Raspberry Pi de la UNRaf.

**Notes:** Decision driven by hardware constraint (Raspberry Pi RAM). Context API handles both data state and alarm state globally.

---

## Chart Library

| Option | Description | Selected |
|--------|-------------|----------|
| Chart.js (react-chartjs-2) | More established, more chart types, imperative API | |
| Recharts | Declarative JSX components, React-idiomatic, simpler API | ✓ |

**User's choice:** Recharts para las gráficas de telemetría (líneas de tiempo de temperatura y humedad) por ser 100% declarativo y amigable con el ecosistema React.

**Notes:** Specifically for time-series line charts of temperature and humidity readings.

---

## Frontend Serving Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Django serves React static files | Simpler deployment (one container), couples lifecycle | |
| Separate Docker container (Nginx + Vite build) | Adds one service, full decoupling, standard practice | ✓ |

**User's choice:** Contenedor Docker separado (Nginx sirviendo el build estático de Vite) para mantener el desacoplamiento total entre Frontend y Backend.

**Notes:** Nginx serves the production build. Development may use Vite dev server separately.

---

## Theme & Layout

| Option | Description | Selected |
|--------|-------------|----------|
| Light theme | Standard MUI default | |
| Industrial Dark Theme | Dark background, industrial dashboard panels | ✓ |

**User's choice:** Layout basado en Grid responsivo de Material UI con estética Industrial Dark Theme. Código de colores semáforo estricto: Verde (Normal), Amarillo (Advertencia), Rojo parpadeante (Gas > 300 PPM o Flama detectada).

**Notes:** Traffic-light color coding is strict — green/yellow/red blinking. Red blinking triggers on gas > 300 PPM OR flame detected. Visual + sound alarm via Context API.

---

## the agent's Discretion

- Component tree structure and naming
- Vite project organization
- Nginx configuration (Dockerfile, nginx.conf)
- Alarm sound implementation (simple beep)
- Cache and polling optimization strategy
- Frontend container port
- Environment variable structure
- React Router routes (if applicable)

## Deferred Ideas

None — discussion stayed within phase scope
