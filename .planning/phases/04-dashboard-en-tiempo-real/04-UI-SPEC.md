# UI Design Contract — Phase 4: Dashboard en Tiempo Real

**Version:** 1.0
**Date:** 2026-05-25
**Phase:** 4-dashboard-en-tiempo-real
**Requires:** CONTEXT.md D-01 through D-09, ROADMAP.md, REQUIREMENTS.md UI-01–06, API specs from Phase 2

---

## 1. Layout Architecture

### 1.1 App Shell

```
+--------------------------------------------------+
|  AppBar (logo + "NodeAlert IoT" + user menu)     |
+----------+---------------------------------------+
|          |  Summary Bar (4 gauges)                |
|  Sidebar +----------+----------------------------+
| (mobile  |  Main Content Area                     |
|  drawer) |  [Grid: dynamic 12-col layout]         |
|          |  Panel 1 | Panel 2 | Panel 3 | Panel 4 |
|          |  Temp    | Humidity| Gas     | Flame   |
|          |  +-------+  +------+  +-----+  +-----+ |
|          |  | gauge |  | gauge|  |gauge|  |gauge| |
|          |  +-------+  +------+  +-----+  +-----+ |
|          |                                          |
|          |  Charts Panel (full width)              |
|          |  [LineChart: Temperature / Humidity]    |
|          |                                          |
|          |  Alert Panel (full width)               |
|          |  [Event log table: timestamp + type +   |
|          |   severity + message]                   |
|          |                                          |
+----------+----------------------------------------+
```

**Breakpoints (MUI Grid):**
- **xs (0–599px):** Single column — Summary Bar stacks vertically, panels stack full-width, sidebar becomes bottom nav or hidden drawer
- **sm (600–899px):** 2 columns — 2 sensor panels per row, charts full-width below
- **md (900–1199px):** 4 columns — all 4 sensor panels in one row, charts full-width below
- **lg+ (1200px+):** 4 columns — same as md but with sidebar always visible

### 1.2 Route Map

| Route | Component | Auth Required | Description |
|-------|-----------|---------------|-------------|
| `/login` | `LoginPage` | No | Login form |
| `/` | `DashboardLayout` | Yes | Main dashboard |
| `/devices` | `DeviceListPage` | Yes | Device management (CRUD) |
| `/device/:id` | `DeviceDetailPage` | Yes | Single device telemetry detail |

---

## 2. Component Tree

```
App
├── ThemeProvider (IndustrialDarkTheme)
├── AuthProvider (Context)
│   └── AlarmProvider (Context — reads from AuthProvider data)
├── Router
│   ├── LoginPage
│   │   └── LoginForm (MUI TextField + Button)
│   └── ProtectedRoute
│       └── DashboardLayout
│           ├── AppBar
│           │   ├── Logo + Title
│           │   └── UserMenu (logout button)
│           ├── Sidebar (optional: mobile Drawer)
│           │   └── NavItems (Dashboard, Devices)
│           ├── SummaryBar
│           │   ├── SensorGauge (temperature)
│           │   ├── SensorGauge (humidity)
│           │   ├── SensorGauge (gas)
│           │   └── SensorGauge (flame)
│           ├── ChartsPanel
│           │   ├── TimeFilter (Last 1h / 6h / 24h / 7d)
│           │   ├── TempChart (LineChart — Recharts)
│           │   └── HumidityChart (LineChart — Recharts)
│           └── AlertPanel
│               ├── AlertToolbar (filter by severity, mark resolved)
│               └── EventTable (MUI Table + severity badges)
└── AlarmSound (audio element, triggered by context)
```

### Component Specifications

#### SensorGauge
| Prop | Type | Description |
|------|------|-------------|
| `label` | string | "Temperature", "Humidity", "Gas", "Flame" |
| `value` | number \| null | Current reading value |
| `unit` | string | "°C", "%", "PPM", "—" |
| `status` | "normal" \| "warning" \| "critical" | Based on D-07 thresholds |

**States:**
- **Loading:** MUI Skeleton pulse animation
- **Normal:** Green background/border, value displayed
- **Warning:** Yellow/amber background, value displayed
- **Critical:** Red background, value displayed with CSS blink animation (`@keyframes blink`)
- **Disconnected (no data > 35s):** Gray/dimmed, "- -" placeholder
- **Error:** Red border, "!" icon, tooltip with error detail

#### ChartsPanel
| Prop | Type | Description |
|------|------|-------------|
| `deviceId` | string | Current device to chart |
| `readings` | Array | Time-series data from Context |

**TimeFilter:** SegmentedButton group (1h / 6h / 24h / 7d) — controls `timestamp__gte` in API query.

**Charts:**
- Temperature: Y-axis 0–50°C, X-axis time, Line stroke #FF9800
- Humidity: Y-axis 0–100%, X-axis time, Line stroke #03A9F4
- Recharts: `<LineChart>` with `<Tooltip />` + `<CartesianGrid />` + `<Legend />`

#### AlertPanel
**Columns:** Timestamp | Severity (badge) | Event Type | Message | Device | Actions

**Severity Badges:**
- Info: MUI Chip, color `info` (blue)
- Warning: MUI Chip, color `warning` (amber)
- Critical: MUI Chip, color `error` (red) with blink animation

**Empty state:** "No events recorded" with MUI `<Alert severity="info">`

---

## 3. Theme Specification (Industrial Dark)

### 3.1 MUI Theme Tokens

```json
{
  "palette": {
    "mode": "dark",
    "primary": { "main": "#00BFA5", "teal accent" },
    "secondary": { "main": "#FF6D00", "deep orange" },
    "background": {
      "default": "#0D1117",
      "paper": "#161B22",
      "card": "#1C2333"
    },
    "text": {
      "primary": "#E6EDF3",
      "secondary": "#8B949E"
    },
    "divider": "#30363D"
  },
  "typography": {
    "fontFamily": "'Roboto Mono', 'Roboto', monospace",
    "fontFamilyCode": "'Roboto Mono', monospace"
  },
  "shape": { "borderRadius": 8 }
}
```

### 3.2 Sensor Status Colors (D-07)

| Status | Background | Text | Border | Animation |
|--------|-----------|------|--------|-----------|
| `normal` | `#1B3A1B` | `#4CAF50` | `#4CAF50` | None |
| `warning` | `#3A2E1B` | `#FFC107` | `#FFC107` | None |
| `critical` | `#3A1B1B` | `#F44336` | `#F44336` | `blink 1s infinite` |

```css
@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}
```

### 3.3 Threshold Constants (D-07)

| Sensor | Normal | Warning | Critical |
|--------|--------|---------|----------|
| Temperature | < 35°C | 35–45°C | > 45°C |
| Humidity | 20–80% | < 20% or > 80% | — |
| Gas | < 200 PPM | 200–300 PPM | > 300 PPM |
| Flame | 0 (no flame) | — | 1 (flame detected) |

---

## 4. State Management (Context API)

### 4.1 Context Structure

```typescript
// AuthContext
interface AuthState {
  token: string | null;
  user: { username: string } | null;
  isAuthenticated: boolean;
  isLoading: boolean;
  error: string | null;
}

// AlarmContext (reads from AuthContext's readings data)
interface AlarmState {
  alarmActive: boolean;        // true if any sensor is critical
  alarmMessage: string | null; // human-readable description
  statuses: {
    temperature: 'normal' | 'warning' | 'critical';
    humidity: 'normal' | 'warning' | 'critical';
    gas: 'normal' | 'warning' | 'critical';
    flame: 'normal' | 'warning' | 'critical';
  };
}

// ReadingsContext (consumed via polling)
interface ReadingsState {
  latest: {
    temperature: { value: number; unit: string; timestamp: string } | null;
    humidity: { value: number; unit: string; timestamp: string } | null;
    gas: { value: number; unit: string; timestamp: string } | null;
    flame: { value: number; unit: string; timestamp: string } | null;
  };
  history: Reading[];         // last N readings for charts
  deviceStatus: 'online' | 'offline';  // based on recency of last reading
  lastReadingTime: string | null;
  isPolling: boolean;
  devices: Device[];          // list of registered devices
  events: Event[];            // recent events for alert panel
}
```

### 4.2 Polling Flow

```
[Component Mount]
       ↓
[Fetch token from localStorage]
       ↓
[AuthContext.setToken()]
       ↓
[ProtectedRoute renders DashboardLayout]
       ↓
[useEffect → startPolling()]
       ↓
[setInterval 3000ms]
       ↓
[Axios.get /api/v1/readings?ordering=-timestamp&limit=50]
       ↓
[Update ReadingsContext.latest + .history]
       ↓
[Derive alarm status (threshold check)]
       ↓
[AlarmContext.alarmActive updated]
       ↓
[If alarmActive → play AlarmSound]
```

### 4.3 Alarm Logic

```typescript
function deriveAlarmStatus(readings: LatestReadings): AlarmState {
  const gasValue = readings.gas?.value ?? 0;
  const flameValue = readings.flame?.value ?? 0;
  const tempValue = readings.temperature?.value ?? 0;
  const humValue = readings.humidity?.value ?? 0;

  const statuses = {
    temperature: tempValue > 45 ? 'critical' : tempValue >= 35 ? 'warning' : 'normal',
    humidity: (humValue < 20 || humValue > 80) ? 'warning' : 'normal',
    gas: gasValue > 300 ? 'critical' : gasValue >= 200 ? 'warning' : 'normal',
    flame: flameValue > 0 ? 'critical' : 'normal',
  };

  const isCritical = statuses.gas === 'critical' || statuses.flame === 'critical';

  return {
    alarmActive: isCritical,
    alarmMessage: isCritical
      ? statuses.gas === 'critical'
        ? '⚠️ Gas crítico detectado (>300 PPM)'
        : '🔥 Flama detectada'
      : null,
    statuses,
  };
}
```

---

## 5. Data Flow

### 5.1 On Mount (Initial Fetch)
```
App mounts → AuthContext checks localStorage for token
  ├── No token → redirect /login
  └── Token exists → start polling
       ├── GET /api/v1/devices → populate devices
       ├── GET /api/v1/readings?ordering=-timestamp&limit=50 → populate history + latest
       └── GET /api/v1/events?ordering=-timestamp&limit=20 → populate events
```

### 5.2 Every 3 Seconds (Polling)
```
GET /api/v1/readings?ordering=-timestamp&limit=1
  → Update ReadingsContext.latest
  → Re-derive alarm state (AlarmContext)
  → Re-render SensorGauges + SummaryBar

GET /api/v1/events?ordering=-timestamp&resolved=false&limit=5 (every 9s — every 3rd cycle)
  → Update events list
  → Re-render AlertPanel
```

### 5.3 Authentication Flow
```
LoginPage → POST /api/v1/auth/login { username, password }
  → 200 { token } → store in localStorage + AuthContext
  → Redirect to /
Logout → clear localStorage + AuthContext + stop polling → redirect /login
401 response during polling → clear token, redirect /login
```

### 5.4 Device Connection Status
```
Every poll cycle:
  const deviceOnline = readings.length > 0
    && (Date.now() - latest.timestamp) < 35000;
  // 35s threshold = 3s poll interval * 10 + 5s grace
```

---

## 6. Acceptance Criteria (from ROADMAP Success Criteria)

| # | Criterion | UI-SPEC Verification |
|---|-----------|---------------------|
| SC-1 | Compila y se despliega como servicio Docker Compose | Nginx Dockerfile + Vite build → 04-01 |
| SC-2 | Datos en tiempo real via polling | Context + Axios 3s → Plan 04-02 |
| SC-3 | Dashboard muestra temp/hum/gas/flame | 4 SensorGauge components → Plan 04-02 |
| SC-4 | Gráficos históricos con filtros | ChartsPanel + TimeFilter → Plan 04-03 |
| SC-5 | Panel de alertas con timestamp | AlertPanel with EventTable → Plan 04-03 |
| SC-6 | Autenticación protege acceso | LoginPage + ProtectedRoute → Plan 04-03 |
| SC-7 | Estado de conexión del ESP32 visible | SensorGauge "offline" state → Plan 04-02 |

---

## 7. Design Contracts by Plan

### Plan 04-01: Vite Scaffold + Docker + Theme
- Create `NodeAlert-Frontend/` with Vite + React + TypeScript + MUI
- Implement `IndustrialDarkTheme` (MUI `createTheme` with tokens from §3.1)
- Dockerfile (multi-stage: `node:alpine` build → `nginx:alpine` serve)
- `nginx.conf`: serve `/usr/share/nginx/html`, proxy `/api` to `http://django:8000/`
- `docker-compose.yml` add `frontend` service (port 3000, nodealert-net, depends_on: django)
- Empty DashboardLayout shell with AppBar + Sidebar skeleton

### Plan 04-02: Data Layer + Sensor Cards
- AuthContext + ReadingsContext + AlarmContext
- Axios polling service (3s interval with cleanup on unmount)
- LoginPage (MUI Card with TextField + Button, dark themed)
- ProtectedRoute wrapper
- 4 SensorGauge components with traffic-light colors + blink animation
- SummaryBar row in DashboardLayout
- Device status detection (online/offline by recency)

### Plan 04-03: Charts + Alert Panel
- ChartsPanel with TimeFilter (SegmentedButton)
- Recharts LineChart for temperature + humidity
- AlertPanel with EventTable (MUI Table sortable by timestamp, filterable by severity)
- AlarmSound (HTML5 `<audio>` single short beep loop while critical)
- DeviceListPage + DeviceDetailPage (basic views)
- CORS headers in Django (`django-cors-headers` middleware)

---

## 8. File Tree (Target)

```
NodeAlert-Frontend/
├── public/
├── src/
│   ├── main.tsx
│   ├── App.tsx
│   ├── context/
│   │   ├── AuthContext.tsx
│   │   ├── ReadingsContext.tsx
│   │   └── AlarmContext.tsx
│   ├── services/
│   │   ├── api.ts          (Axios instance + token interceptor)
│   │   └── polling.ts      (polling service)
│   ├── theme/
│   │   └── industrialDarkTheme.ts
│   ├── components/
│   │   ├── AppBar.tsx
│   │   ├── Sidebar.tsx
│   │   ├── SensorGauge.tsx
│   │   ├── SummaryBar.tsx
│   │   ├── ChartsPanel.tsx
│   │   ├── TimeFilter.tsx
│   │   ├── TempChart.tsx
│   │   ├── HumidityChart.tsx
│   │   ├── AlertPanel.tsx
│   │   ├── EventTable.tsx
│   │   ├── LoginForm.tsx
│   │   ├── ProtectedRoute.tsx
│   │   └── AlarmSound.tsx
│   ├── pages/
│   │   ├── LoginPage.tsx
│   │   ├── DashboardPage.tsx
│   │   ├── DeviceListPage.tsx
│   │   └── DeviceDetailPage.tsx
│   ├── hooks/
│   │   └── usePolling.ts
│   └── types/
│       └── index.ts
├── Dockerfile
├── nginx.conf
├── .env.example
├── index.html
├── vite.config.ts
├── tsconfig.json
└── package.json
```

---

*UI Design Contract v1.0 — Locked for Phase 4 planning and execution.*
