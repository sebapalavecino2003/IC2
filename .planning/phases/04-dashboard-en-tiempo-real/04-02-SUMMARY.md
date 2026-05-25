---
phase: 04-dashboard-en-tiempo-real
plan: 02
subsystem: frontend
tags: react, typescript, axios, mui, context-api, polling, drf-token-auth

requires:
  - phase: 02-core-infrastructure-gateway-mqtt
    provides: Django REST API with Token Auth (POST /api/v1/auth/login, GET /api/v1/readings, GET /api/v1/events)
  - phase: 04-01
    provides: Vite scaffold, IndustrialDarkTheme, theme wrapper in main.tsx

provides:
  - AuthContext with login/logout and localStorage token persistence
  - ReadingsContext with 3s polling loop for real-time sensor data
  - AlarmContext with threshold-based status derivation (temperature, humidity, gas, flame)
  - LoginPage + LoginForm MUI Card component
  - ProtectedRoute guard component (redirects to /login if unauthenticated)
  - SensorGauge with traffic-light colors and blink animation for critical states
  - SummaryBar with 4 SensorGauges in responsive Grid
  - DashboardPage with monitoring panel layout
  - Axios service with Token auth interceptor and automatic 401 redirect
  - Generic usePolling hook with interval cleanup
  - TypeScript types (Device, Reading, Event, LatestReadings, SensorStatus)

affects: 04-03 (Charts + Alerts + Device Pages)

tech-stack:
  added: []
  patterns:
    - Context API for global state management (Auth, Readings, Alarm)
    - Axios interceptors for auth token injection and error handling
    - Custom usePolling hook for interval-based data refresh
    - Derived alarm state from raw readings using pure functions + useMemo

key-files:
  created:
    - NodeAlert-Frontend/src/types/index.ts
    - NodeAlert-Frontend/src/services/api.ts
    - NodeAlert-Frontend/src/services/polling.ts
    - NodeAlert-Frontend/src/hooks/usePolling.ts
    - NodeAlert-Frontend/src/context/AuthContext.tsx
    - NodeAlert-Frontend/src/context/ReadingsContext.tsx
    - NodeAlert-Frontend/src/context/AlarmContext.tsx
    - NodeAlert-Frontend/src/components/LoginForm.tsx
    - NodeAlert-Frontend/src/components/ProtectedRoute.tsx
    - NodeAlert-Frontend/src/components/SensorGauge.tsx
    - NodeAlert-Frontend/src/components/SummaryBar.tsx
    - NodeAlert-Frontend/src/pages/LoginPage.tsx
    - NodeAlert-Frontend/src/pages/DashboardPage.tsx
    - NodeAlert-Frontend/src/vite-env.d.ts
  modified:
    - NodeAlert-Frontend/src/App.tsx

key-decisions:
  - "Alarm threshold constants aligned with UI-SPEC §3.3: temp>45°C critical, >=35°C warning; gas>300 PPM critical, >=200 warning; flame>0 critical; humidity<20% or >80% warning"
  - "Device connection status determined by reading recency (<35s threshold = 3s poll × 10 + 5s grace)"
  - "AlarmContext derives statuses from ReadingsContext data using pure functions memoized with useMemo"

requirements-completed: [UI-02, UI-05, UI-06, API-02, API-06]

duration: ~5 min
completed: 2026-05-25
---

# Phase 4 Plan 2: Data Layer + Auth + Sensor Cards Summary

**AuthContext, ReadingsContext, AlarmContext with 3s polling, LoginForm, ProtectedRoute, and 4 traffic-light SensorGauge cards in SummaryBar**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-05-25
- **Completed:** 2026-05-25
- **Tasks:** 3
- **Files created/modified:** 15

## Accomplishments

- TypeScript interfaces for Device, Reading, Event, LatestReadings, SensorStatus
- Axios service with Token auth interceptor (reads from localStorage) and automatic 401 redirect to /login
- AuthContext with login()/logout() methods, token persistence in localStorage
- LoginForm MUI Card with username/password fields and error Alert display
- ProtectedRoute guard redirecting unauthenticated users to /login
- ReadingsContext with 3-second polling loop fetching readings and events in parallel
- Generic usePolling hook with cleanup on unmount and enabled toggle
- AlarmContext deriving sensor statuses from raw readings using thresholds (temp/humidity/gas/flame)
- Device connection status detection based on reading recency (<35s = online)
- SensorGauge component with traffic-light color scheme (green/amber/red) and blink animation for critical
- SummaryBar with ESP32 connection chip + 4 SensorGauges in responsive Grid (2-col xs, 4-col sm+)
- DashboardPage with "Panel de Monitoreo" title and SummaryBar
- App.tsx wiring: AuthProvider → ReadingsProvider → AlarmProvider → Routes

## Task Commits

Each task was committed atomically:

1. **Task 1: Types + Axios service + AuthContext + LoginPage + ProtectedRoute** - `3b53d14` (feat)
2. **Task 2: ReadingsContext + AlarmContext + polling service + usePolling hook** - `da97fd5` (feat)
3. **Task 3: SensorGauge + SummaryBar + DashboardPage** - `1b80d1e` (feat)

**Fixes:** `c1405cc` (fix: add vite-env.d.ts for import.meta.env type declarations)

**Plan metadata:** To be committed separately.

## Files Created/Modified

- **Created:**
  - `NodeAlert-Frontend/src/types/index.ts` — Device, Reading, Event, LatestReadings, SensorStatus types
  - `NodeAlert-Frontend/src/services/api.ts` — Axios instance with Token auth + 401 redirect interceptor
  - `NodeAlert-Frontend/src/services/polling.ts` — fetchLatestReadings, fetchUnresolvedEvents API wrappers
  - `NodeAlert-Frontend/src/hooks/usePolling.ts` — Generic polling hook with interval + cleanup
  - `NodeAlert-Frontend/src/context/AuthContext.tsx` — AuthProvider + useAuth with localStorage token
  - `NodeAlert-Frontend/src/context/ReadingsContext.tsx` — ReadingsProvider with 3s polling
  - `NodeAlert-Frontend/src/context/AlarmContext.tsx` — AlarmProvider with threshold derivation
  - `NodeAlert-Frontend/src/components/LoginForm.tsx` — MUI Card login form
  - `NodeAlert-Frontend/src/components/ProtectedRoute.tsx` — Auth guard component
  - `NodeAlert-Frontend/src/components/SensorGauge.tsx` — Traffic-light colored sensor card
  - `NodeAlert-Frontend/src/components/SummaryBar.tsx` — 4-sensor summary row with ESP32 status
  - `NodeAlert-Frontend/src/pages/LoginPage.tsx` — Login page wrapper
  - `NodeAlert-Frontend/src/pages/DashboardPage.tsx` — Dashboard with title + SummaryBar
  - `NodeAlert-Frontend/src/vite-env.d.ts` — Vite client type declarations
- **Modified:**
  - `NodeAlert-Frontend/src/App.tsx` — Wired AuthProvider → ReadingsProvider → AlarmProvider → Routes

## Decisions Made

- All alarm thresholds (temp, humidity, gas, flame) follow UI-SPEC §3.3 exactly
- Device connection uses 35s threshold (3s poll × 10 + 5s grace) as specified in UI-SPEC §5.4
- usePolling hook is a standalone custom hook (not embedded in context) for reuse in future components
- Alarm status derived from raw readings using pure functions + useMemo for performance
- Context nesting: AuthProvider outermost, ReadingsProvider reads isAuthenticated, AlarmProvider reads readings

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Missing vite-env.d.ts for TypeScript compilation**
- **Found during:** Task 3 verification (full build)
- **Issue:** TypeScript build failed with `TS2339: Property 'env' does not exist on type 'ImportMeta'` — Vite's `import.meta.env` requires `vite/client` types, but the type declaration file was missing from the scaffold
- **Fix:** Created `NodeAlert-Frontend/src/vite-env.d.ts` with `/// <reference types="vite/client" />`
- **Files modified:** NodeAlert-Frontend/src/vite-env.d.ts (new file)
- **Verification:** `npm run build --prefix NodeAlert-Frontend` passes (tsc + vite build)
- **Committed in:** c1405cc (separate fix commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Auto-fix essential for TypeScript compilation. No scope creep.

## Issues Encountered

None — plan executed as specified with one auto-fixed TypeScript configuration gap.

## Threat Surface Scan

No new threat surface introduced beyond what `<threat_model>` in the plan already documents. The Axios 401 interceptor, ProtectedRoute guard, and localStorage token storage align with the registered accept/mitigate dispositions.

## Known Stubs

None — all components wired to real data sources through Context providers.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- Auth + data layer + sensor card UI complete
- Ready for Plan 04-03: Charts (Recharts), AlertPanel, Device pages, alarm sound, CORS headers
- Build compiles and passes TypeScript checks

## Self-Check: PASSED

All 14 created files verified on disk. All 4 commits found in git log.

---

*Phase: 04-dashboard-en-tiempo-real*
*Completed: 2026-05-25*
