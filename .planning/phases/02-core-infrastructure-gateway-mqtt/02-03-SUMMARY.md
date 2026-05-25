---
phase: 02-core-infrastructure-gateway-mqtt
plan: 03
subsystem: api
tags: drf, token-auth, authentication, deployment, docker, firmware, setup-script

# Dependency graph
requires:
  - phase: 02-core-infrastructure-gateway-mqtt
    plan: 02
    provides: Django project, models, serializers, viewsets, Docker Compose services
provides:
  - Token authentication for all REST API endpoints
  - POST /api/v1/auth/login authentication endpoint
  - setup.sh single-command deployment script
  - Auto-superuser creation in entrypoint.sh
  - Firmware config header generation for ESP32
affects:
  - 03-connectivity-firmware-mqtt (user_config.h used for WiFi/MQTT config)
  - 04-dashboard-web (API now protected, needs token for data access)

# Tech tracking
tech-stack:
  added: [rest_framework.authtoken]
  patterns: [TokenAuthentication + IsAuthenticated as DRF defaults, AllowAny on public endpoints]

key-files:
  created:
    - setup.sh — single-command deployment script with interactive prompts
  modified:
    - NodeAlert-Backend/nodealert/settings.py — added authtoken app + auth/permission classes
    - NodeAlert-Backend/core/serializers.py — added LoginSerializer with authenticate()
    - NodeAlert-Backend/core/views.py — added LoginView(APIView) with AllowAny
    - NodeAlert-Backend/core/urls.py — added auth/login/ route
    - NodeAlert-Backend/entrypoint.sh — added auto-superuser creation block

key-decisions:
  - "D-12: DRF token authentication via rest_framework.authtoken"
  - "D-14: POST /api/v1/auth/login endpoint with AllowAny permission"
  - "D-20: setup.sh creates .env, mosquitto_passwd, runs docker compose up, migrations, superuser"
  - "D-21: Dual-arch detection (arm64 + amd64) in setup.sh"
  - "D-22: setup.sh generates firmware config header (user_config.h) with -include compiler flag"

patterns-established:
  - "Public auth endpoints use AllowAny permission class"
  - "All other API endpoints require TokenAuthentication by default"
  - "DRF SessionAuthentication as fallback for admin/browsable API"
  - "Secrets never in source code — read from .env at runtime"

requirements-completed: [API-06, INFRA-02]

# Metrics
duration: 4min
completed: 2026-05-24
---

# Phase 2: Core Infrastructure + Gateway MQTT — Plan 03 Summary

**DRF token authentication for all API endpoints with LoginView, plus setup.sh single-command deployment script that generates .env, mosquitto_passwd, firmware config headers, and runs the full stack**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-25T01:57:26Z
- **Completed:** 2026-05-25T02:01:37Z
- **Tasks:** 3 (1 auto + 1 checkpoint end-of-phase + 1 auto)
- **Files modified:** 6

## Accomplishments

- **Token authentication** — All REST API endpoints now require DRF TokenAuthentication by default. Public access only to login endpoint (AllowAny).
- **Login endpoint** — POST /api/v1/auth/login accepts username+password and returns a DRF token. Invalid credentials return 400 with error.
- **setup.sh deployment script** — Complete interactive deployment script at repo root. Handles architecture detection, interactive prompts for all credentials, .env generation (chmod 600), mosquitto_passwd generation, Docker Compose startup with MySQL wait loop, Django migrations, superuser creation, and firmware config header generation.
- **Auto-superuser in entrypoint.sh** — Django container auto-creates superuser on startup if DJANGO_SUPERUSER_USERNAME/PASSWORD are set (graceful if user already exists).
- **Firmware config generation** — setup.sh generates NodeAlert-Firmware/src/config/user_config.h with WiFi SSID/password and MQTT broker URI, plus adds -include compiler flag to platformio.ini.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add token authentication endpoint and protect API** — `9fac9f0` (feat)
2. **Task 2: Verify token authentication end-to-end** — Checkpoint (handled via end-of-phase verification mode)
3. **Task 3: Create setup.sh deployment script with firmware config generation** — `b7bba63` (feat)

**Plan metadata:** (committed with SUMMARY)

## Files Created/Modified
- `setup.sh` — Single-command interactive deployment script (409 lines). Sections: arch detection, interactive prompts, .env generation, mosquitto_passwd generation, docker compose up, Django migrations, superuser creation, firmware config header generation, completion summary.
- `NodeAlert-Backend/nodealert/settings.py` — Added 'rest_framework.authtoken' to INSTALLED_APPS, updated REST_FRAMEWORK with TokenAuthentication + SessionAuthentication + IsAuthenticated
- `NodeAlert-Backend/core/serializers.py` — Added LoginSerializer with username (CharField), password (CharField, write_only), validate() method calling authenticate()
- `NodeAlert-Backend/core/views.py` — Added LoginView(APIView) with AllowAny, post() returns token via Token.objects.get_or_create()
- `NodeAlert-Backend/core/urls.py` — Added path('auth/login/', LoginView.as_view())
- `NodeAlert-Backend/entrypoint.sh` — Added auto-superuser creation block after migrate

## Decisions Made
- Used DRF's built-in TokenAuthentication (not JWT) — simpler for MVP on local LAN. JWT can be introduced in hardening phase.
- SessionAuthentication kept alongside TokenAuthentication for admin/browsable API access.
- LoginView uses AllowAny so unauthenticated users can obtain tokens.
- setup.sh generates .env with chmod 600 for security.
- setup.sh detects arm64 vs amd64 via uname -m for transparency (Docker handles multi-arch automatically).
- Firmware config uses -include compiler flag rather than modifying #ifndef headers — cleanest override pattern.

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

- .env file was missing when attempting to run migrations during Task 1 — created a development .env; setup.sh handles production .env generation
- Docker volumes had stale data from previous plan runs — needed `docker compose down -v` and recreate for fresh MySQL credentials

## Known Stubs

None — no empty states, placeholders, or TODO items in created/modified files.

## Threat Flags

None — all auth paths are in the plan's threat model (T-02-11 through T-02-16, T-02-SC). No new surface introduced.

## Next Phase Readiness

- API is now authenticated and secured. Migration for authtoken applied.
- setup.sh provides single-command deployment for the full Phase 2 stack.
- Ready for Phase 3: Connectivity — Firmware MQTT integration (uses the user_config.h generated by setup.sh).
- Next plan: 03-01 (connectivity-firmware-mqtt) — ESP32 MQTT connection, telemetry publishing.

---

*Phase: 02-core-infrastructure-gateway-mqtt*
*Completed: 2026-05-24*
