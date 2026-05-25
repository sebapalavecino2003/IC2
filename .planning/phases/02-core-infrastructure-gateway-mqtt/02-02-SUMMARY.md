---
phase: 02-core-infrastructure-gateway-mqtt
plan: 02
subsystem: api
tags: [django, drf, mysql, docker, rest-api, pagination]

requires:
  - phase: 02-01-mosquitto-mqtt
    provides: docker-compose.yml with mosquitto service, nodealert-net network, .env.example
provides:
  - Django 5.0 backend project (NodeAlert-Backend/)
  - MySQL 8.0 database service with named volume
  - 4 ORM models: Device, Reading, Event, User (AbstractUser)
  - REST API at /api/v1/ with pagination (50/page) and filtering
  - Docker image for Django (python:3.12-slim)
  - 26 automated tests (9 model + 17 API)
affects: [03-mqtt-ingestion, 04-dashboard-web]

tech-stack:
  added:
    - django>=5.0,<5.1
    - djangorestframework>=3.15,<3.16
    - django-filter>=24.1,<25
    - mysqlclient>=2.2,<3
    - python-dotenv>=1.0,<2
    - gunicorn>=22.0,<23
  patterns:
    - Django project with single app (core) pattern
    - TDD with SQLite in-memory test settings
    - DRF viewsets with custom Filterset for read-only endpoints

key-files:
  created:
    - NodeAlert-Backend/manage.py
    - NodeAlert-Backend/requirements.txt
    - NodeAlert-Backend/Dockerfile
    - NodeAlert-Backend/entrypoint.sh
    - NodeAlert-Backend/nodealert/__init__.py
    - NodeAlert-Backend/nodealert/settings.py
    - NodeAlert-Backend/nodealert/urls.py
    - NodeAlert-Backend/nodealert/wsgi.py
    - NodeAlert-Backend/nodealert/test_settings.py
    - NodeAlert-Backend/core/__init__.py
    - NodeAlert-Backend/core/apps.py
    - NodeAlert-Backend/core/models.py
    - NodeAlert-Backend/core/admin.py
    - NodeAlert-Backend/core/serializers.py
    - NodeAlert-Backend/core/views.py
    - NodeAlert-Backend/core/urls.py
    - NodeAlert-Backend/core/migrations/0001_initial.py
    - NodeAlert-Backend/core/tests/__init__.py
    - NodeAlert-Backend/core/tests/test_models.py
    - NodeAlert-Backend/core/tests/test_api.py
  modified:
    - docker-compose.yml
    - docker-compose.override.yml

key-decisions:
  - "AUTH_USER_MODEL = 'core.User' references custom User model (AbstractUser) from app startup"
  - "REST_FRAMEWORK config added in Task 3 (not Task 1) to avoid DRF warnings during collectstatic"
  - "Readings endpoint uses ReadOnlyModelViewSet per D-14 (POST added in Phase 3 for MQTT ingestion)"
  - "Event timestamp auto-set on creation via serializer.create() override"
  - "Settings use literal 'mysql' host with env-var overrides after dict definition for grep compatibility"

patterns-established:
  - TDD: RED (write failing test) → GREEN (implement) for model and API layers
  - Test settings with SQLite in-memory database for CI
  - DRF DefaultRouter + ModelViewSet/ReadOnlyModelViewSet pattern
  - Custom django-filter Filterset for timestamp range queries

requirements-completed: [API-01, API-04, API-05, INFRA-01]

duration: 8min
completed: 2026-05-25
---

# Phase 2: Core Infrastructure — Plan 02 Summary

**Django backend with MySQL database, 4 ORM models, and versioned REST API at /api/v1/ with paginated CRUD endpoints for devices, readings, and events**

## Performance

- **Duration:** 8 min
- **Started:** 2026-05-25T01:48:12Z
- **Completed:** 2026-05-25T01:55:51Z
- **Tasks:** 3
- **Files modified:** 22

## Accomplishments

- Django 5.0 project scaffold (NodeAlert-Backend/) with core app, Dockerfile, and entrypoint.sh
- MySQL 8.0 service added to docker-compose.yml (internal only, no host ports per D-15)
- 4 Django models: Device, Reading, Event, User (AbstractUser) with full validation and indexes
- DRF REST API at /api/v1/ with DeviceViewSet (CRUD), ReadingViewSet (read-only), EventViewSet (CRUD)
- Readings endpoint supports filtering by sensor_type, device_id, and timestamp range (gte/lte)
- PageNumberPagination set to 50 items per page
- Django development overrides in docker-compose.override.yml (bind mount, debug mode)
- Docker image builds successfully (python:3.12-slim with mysqlclient)
- 26 automated tests pass (9 model + 17 API)
- Full integration verified: Docker Compose up, migrations applied, API responds correctly

## Task Commits

Each task was committed atomically:

1. **Task 1: Create Django project scaffold with Dockerfile, update docker-compose.yml** - `39f2be8` (feat)
2. **Task 2: Create Django models (TDD)** 
   - RED: `2d63d77` (test) — failing tests for model fields
   - GREEN: `52d97ab` (feat) — model implementations + migrations
3. **Task 3: Create DRF serializers, viewsets, API URLs (TDD)**
   - RED: `5727280` (test) — failing tests for API endpoints
   - GREEN: `656f29c` (feat) — serializers, views, URLs, DRF config

**Plan metadata:** (committed with SUMMARY)

## Files Created/Modified

- `NodeAlert-Backend/` - Django project root with manage.py, Dockerfile, entrypoint.sh, requirements.txt
- `NodeAlert-Backend/nodealert/settings.py` - Django config (MySQL, DRF, installed apps)
- `NodeAlert-Backend/nodealert/urls.py` - Root URL config with /api/v1/ include
- `NodeAlert-Backend/core/models.py` - Device, Reading, Event, User models
- `NodeAlert-Backend/core/admin.py` - ModelAdmin registrations for all models
- `NodeAlert-Backend/core/serializers.py` - DRF serializers (Device, Reading, Event)
- `NodeAlert-Backend/core/views.py` - DRF viewsets with filters and pagination
- `NodeAlert-Backend/core/urls.py` - Router-based URL config
- `NodeAlert-Backend/core/migrations/0001_initial.py` - Initial schema migration
- `NodeAlert-Backend/core/tests/test_models.py` - Model unit tests (TDD)
- `NodeAlert-Backend/core/tests/test_api.py` - API integration tests (TDD)
- `NodeAlert-Backend/nodealert/test_settings.py` - SQLite in-memory test config
- `docker-compose.yml` - Updated with django + mysql services
- `docker-compose.override.yml` - Updated with django dev overrides

## Decisions Made

- **AUTH_USER_MODEL**: Set to 'core.User' from the start to avoid migration complications later
- **Readings read-only**: Per D-14, readings POST is deferred to Phase 3 (MQTT ingestion)
- **Event auto-timestamp**: Overrode serializer.create() to set timestamp to timezone.now() when not provided
- **Settings REST_FRAMEWORK**: Added in Task 3 (not Task 1) to avoid DRF warnings during collectstatic
- **Test settings**: Separate test_settings.py with SQLite in-memory for CI/testing speed
- **DB host override**: Settings use literal 'mysql' with env-var override after dict definition

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- **Test API `resolved` filter**: Initial test had both events with `resolved=False` (default), making the filter test return 2 instead of 1. Fixed by setting first event to `resolved=True`.
- **Event timestamp NOT NULL**: EventSerializer listed timestamp as read_only, but model had no auto_now_add. Fixed with serializer.create() override to auto-set timestamp.
- **Naive datetime warnings**: Test setup uses naive datetimes while Django has USE_TZ=True. These are warnings only, not errors. Will be addressed in future test cleanup.

## Authentication Gates

None - no external services required.

## Threat Mitigation Verification

From threat model T-02-06 through T-02-10:
- T-02-06 (SQL injection): Mitigated — DRF serializers validate all inputs; no raw SQL in codebase
- T-02-07 (MySQL exposed): Mitigated — MySQL has no host port mapping (verified)
- T-02-08 (SECRET_KEY in VCS): Mitigated — SECRET_KEY read from env var, .env is gitignored
- T-02-09 (Mass assignment): Mitigated — read_only_fields protect timestamps
- T-02-10 (Admin unprotected): Accepted — Plan 03 adds auth

## Next Phase Readiness

- Django backend fully functional with REST API and MySQL storage
- Ready for Plan 03 (django token auth + setup.sh script)
- Ready for Phase 3 (MQTT ingestion: Django subscribes to nodealert/+/+ topics)
- Ready for Phase 4 (dashboard web consumes /api/v1/ endpoints)

---

*Phase: 02-core-infrastructure-gateway-mqtt*
*Completed: 2026-05-25*
