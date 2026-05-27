# NodeAlert IoT Backend — Full Codebase Analysis

**Analysis Date:** 2026-05-26
**Scope:** All Python files in `core/`, `nodealert/`, `manage.py`, `entrypoint.sh`, `Dockerfile`, `requirements.txt`
**Total files analyzed:** 18 Python files + 2 shell/Dockerfile + 1 config

---

## OVERALL SCORE: **FAIL**

**Justification:** The codebase contains 3 critical security vulnerabilities (no authorization enforcement beyond authentication, `ALLOWED_HOSTS = ['*']` in production, and MQTT topic injection vector) plus significant data-loss risks in the MQTT subscriber and exposed password hashes in the admin interface. These issues directly impact the security and reliability of a production IoT monitoring system.

---

# 1. CRITICAL ISSUES

## C-01: No Access Control — Any Authenticated User Has Full CRUD on All Resources

**Files:** `core/views.py` lines 37-75, 93-101
**Severity:** CRITICAL
**Type:** Authorization bypass

Both `DeviceViewSet` and `EventViewSet` use only `permissions.IsAuthenticated`:

```python
# views.py line 37-41
class DeviceViewSet(viewsets.ModelViewSet):
    permission_classes = [permissions.IsAuthenticated]
    # ... full CRUD — anyone authenticated can create/update/delete any device

# views.py line 93-97
class EventViewSet(viewsets.ModelViewSet):
    permission_classes = [permissions.IsAuthenticated]
    # ... full CRUD — anyone authenticated can create/update/delete any event
```

The project ships with `init_groups.py` (`core/management/commands/init_groups.py`) which creates `admin` and `analyst` groups with DISTINCT model-level permissions, but the views never check them. An `analyst` (who should only `view_device`, `view_reading`, `view_event`, `change_event`) can DELETE any device or event.

**Impact:** Any authenticated user (including a low-privilege analyst account) can:
- Delete all devices, destroying FK relationships
- Delete or modify all events (including critical fire/gas alerts)
- Create arbitrary devices with malicious payloads
- Impersonate ESP32 nodes (no ownership model)

**Fix:**
- Use `DjangoModelPermissions` or `DjangoObjectPermissions` instead of bare `IsAuthenticated`
- Or implement custom permission classes that check group membership against the roles defined in `init_groups.py`
- For `DeviceViewSet.command` action, verify the user has permission to send commands to the specific device

```python
# Example fix:
from rest_framework.permissions import DjangoModelPermissions

class DeviceViewSet(viewsets.ModelViewSet):
    permission_classes = [permissions.IsAuthenticated, DjangoModelPermissions]
    # ...
```

---

## C-02: `ALLOWED_HOSTS = ['*']` in Production

**File:** `nodealert/settings.py` line 20
**Severity:** CRITICAL
**Type:** Host header injection / security misconfiguration

```python
ALLOWED_HOSTS = ['*']
```

Django's documentation explicitly warns: *"A value of '*' will match anything and you will be vulnerable to host header attacks."* The deployment uses `entrypoint.sh` which can run `gunicorn nodealert.wsgi:application` bound to `0.0.0.0:8000`, meaning an attacker can craft requests with arbitrary Host headers.

**Impact:**
- Cache poisoning through Host header manipulation
- Password reset poisoning (if password reset is added later)
- CSRF bypass via host mismatch
- Debug traceback exposure if `DEBUG` is accidentally enabled

**Fix:**
```python
# Use env-based whitelist
ALLOWED_HOSTS = os.environ.get('DJANGO_ALLOWED_HOSTS', 'localhost,127.0.0.1,.nodealert.local').split(',')
```

---

## C-03: Potential MQTT Topic Injection via Device ID

**File:** `core/mqtt_publisher.py` line 37
**Severity:** CRITICAL
**Type:** MQTT topic injection

```python
topic=f"{TOPIC_PREFIX}{device_id}/commands"
```

The `device_id` value comes from the database (`self.get_object()`), but since any authenticated user can CREATE a device (Issue C-01), an attacker can register a device with `device_id` containing MQTT wildcards or topic separators (e.g., `../../sensitive/topic` or `#/commands`). This would allow publishing commands to unintended MQTT topics.

**Attack scenario:**
1. Authenticate as any user
2. POST to `/api/v1/devices/` with `{"device_id": "sensor/../invalid", "name": "evil"}`
3. POST to `/api/v1/devices/{id}/command/` with `{"command": "buzzer_on"}`
4. MQTT publishes to `nodealert/sensor/../invalid/commands → nodealert/sensor/invalid/commands`

**Fix:**
- Validate `device_id` against a strict regex pattern in the serializer (alphanumeric + hyphens only)
- Sanitize or reject device IDs with `/`, `+`, `#` characters
- Add validation to `DeviceSerializer`:

```python
import re
from rest_framework import serializers

class DeviceSerializer(serializers.ModelSerializer):
    class Meta:
        model = Device
        fields = '__all__'
        read_only_fields = ['created_at', 'updated_at']

    def validate_device_id(self, value):
        if not re.match(r'^[a-zA-Z0-9_-]+$', value):
            raise serializers.ValidationError(
                "Device ID must contain only letters, numbers, hyphens, and underscores"
            )
        return value
```

---

# 2. WARNING ISSUES

## W-01: Exposed Password Hashes in Admin Interface

**File:** `core/admin.py` lines 33-36
**Severity:** HIGH
**Type:** Sensitive data exposure

```python
@admin.register(User)
class UserAdmin(admin.ModelAdmin):
    """Admin configuration for User model."""
    pass
```

This inherits from `admin.ModelAdmin` (NOT `django.contrib.auth.admin.UserAdmin`), which by default displays ALL model fields including the `password` field (which contains the bcrypt password hash). The standard Django `UserAdmin` handles password fields securely (displays a "password hash" read-only field with a password change form).

**Impact:** Admin users can see password hashes directly in the user list/detail view. While hashes aren't plaintext, they enable offline brute-force attacks.

**Fix:**
```python
from django.contrib.auth.admin import UserAdmin as BaseUserAdmin

@admin.register(User)
class UserAdmin(BaseUserAdmin):
    pass
```

---

## W-02: MQTT Telemetry Subscribes at QoS 0 — Data Loss Risk

**File:** `core/management/commands/mqtt_subscriber.py` line 53
**Severity:** HIGH
**Type:** Data loss / reliability

```python
client.subscribe('nodealert/+/telemetry', qos=0)
```

QoS 0 means "at most once" delivery. If the subscriber is temporarily disconnected (during broker restart, deployment, or network interruption), ALL telemetry messages published during that window are permanently lost. Events use QoS 1 (line 54: `client.subscribe('nodealert/+/events', qos=1)`), which is correct.

**Impact:** Critical telemetry data (temperature spikes, gas readings) can be silently lost during any momentary disconnection. For an environmental monitoring system focused on FIRE and GAS detection, this is a safety-reliability concern.

**Fix:**
```python
client.subscribe('nodealert/+/telemetry', qos=1)
```

---

## W-03: `os.system()` in Readiness Health Check — Fragile and Unnecessary

**File:** `core/views.py` lines 157-163
**Severity:** MEDIUM
**Type:** Code quality / security

```python
try:
    mqtt_alive = os.system(
        'pgrep -f "mqtt_subscriber" > /dev/null 2>&1'
    )
    checks["mqtt"] = (mqtt_alive == 0)
except Exception:
    pass
```

Issues:
1. **`os.system()` invokes a shell unnecessarily** — should use `subprocess.run()`
2. **`pgrep -f` pattern is too broad** — matches any process whose full command line contains `mqtt_subscriber`, including unrelated processes
3. **Silent exception swallowing** — if `os.system()` fails (e.g., EMFILE for too many processes), the check silently passes
4. **Fork overhead** — spawning a subprocess on every readiness check poll is expensive for a health endpoint
5. **No actual MQTT broker connectivity check** — only checks if the subscriber process is alive, not whether it's connected to the broker

**Impact:** The health check can report "mqtt healthy" when the subscriber is actually disconnected from the broker, providing false confidence.

**Fix:**
```python
import subprocess

try:
    # Check only the exact management command process
    result = subprocess.run(
        ['pgrep', '-f', 'manage.py mqtt_subscriber'],
        capture_output=True, timeout=5
    )
    checks["mqtt"] = (result.returncode == 0)
except (subprocess.TimeoutExpired, FileNotFoundError):
    checks["mqtt"] = False
```

Better: track connection state via a module-level flag set by `on_connect`/`on_disconnect` callbacks in the subscriber.

---

## W-04: No MQTT Reconnection Callback — `on_disconnect` Missing

**File:** `core/management/commands/mqtt_subscriber.py` lines 41-48
**Severity:** MEDIUM
**Type:** Reliability

```python
self._client.on_connect = self.on_connect
self._client.on_message = self.on_message
# No on_disconnect handler!
```

While `loop_forever()` in paho-mqtt v2 has built-in automatic reconnection, there is no `on_disconnect` callback to detect connection loss, log it, or implement exponential backoff. If the broker returns `rc != 0` during reconnection, the `on_connect` handler just logs the error (line 56) but doesn't attempt to reconnect with different parameters or exit cleanly.

**Impact:** During prolonged broker outages, the subscriber runs in a broken state silently. The `pgrep` health check (W-03) shows the process as alive even though it's disconnected.

**Fix:**
```python
def on_connect(self, client, userdata, flags, rc, properties=None):
    if rc == 0:
        self.stdout.write(self.style.SUCCESS('Connected to MQTT broker'))
        client.subscribe('nodealert/+/telemetry', qos=1)
        client.subscribe('nodealert/+/events', qos=1)
    elif rc == 5:
        self.stderr.write('MQTT connection refused: not authorized')
        # Don't retry — credentials are wrong
        client.disconnect()
    else:
        self.stderr.write(f'Connection failed with result code {rc}')
        # Retry will be handled by loop_forever

def on_disconnect(self, client, userdata, rc, properties=None):
    if rc != 0:
        self.stderr.write(f'Unexpected MQTT disconnect (rc={rc}), will reconnect...')
    # rc=0 means clean disconnect by client.request
```

---

## W-05: Timestamp Parsing Logic Duplicated with Silent Fallback

**File:** `core/management/commands/mqtt_subscriber.py` lines 91-99 and 137-145
**Severity:** MEDIUM
**Type:** Code quality / observability

The same timestamp parsing block appears in both `_handle_telemetry` and `_handle_event`:

```python
raw_ts = payload.get('timestamp')
timestamp = timezone.now()
if raw_ts is not None:
    try:
        parsed = parse_datetime(str(raw_ts))
        if parsed is not None:
            timestamp = parsed
    except (ValueError, TypeError):
        pass  # <-- silently falls back to now() with no warning
```

**Issues:**
1. **No warning when timestamp parse fails** — if a device sends a malformed timestamp, the data is silently recorded with the server's current time instead. This breaks temporal accuracy for critical events (e.g., when was the gas leak actually detected?).
2. **Duplicated code** — violates DRY principle; changes need to be made in two places.

**Fix:**
```python
def _parse_timestamp(self, raw_ts):
    """Parse timestamp from payload, falling back to now() with logging."""
    if raw_ts is None:
        return timezone.now()
    try:
        parsed = parse_datetime(str(raw_ts))
        if parsed is not None:
            return parsed
    except (ValueError, TypeError):
        pass
    self.stderr.write(f'Invalid timestamp format: {raw_ts}, using server time')
    return timezone.now()
```

---

## W-06: `UserAdmin` Subclass Changes Default Field Display

**File:** `core/admin.py` lines 33-36
**Severity:** LOW-MEDIUM
**Type:** UI / sensitive data exposure

Beyond just showing password hashes (W-01), the plain `ModelAdmin` subclass does not configure `list_display`, `list_filter`, or `search_fields` for the User model. The default Django admin for users provides these out of the box through `UserAdmin`.

---

## W-07: Missing `on_disconnect` Logging for Publisher

**File:** `core/mqtt_publisher.py` lines 35-47
**Severity:** LOW
**Type:** Observability

```python
try:
    publish.single(...)
    return True
except Exception as e:
    print(f"[MQTT PUBLISH ERROR] {e}")
    return False
```

The `publish.single()` function uses a short-lived connection. If the broker is unreachable, the exception is caught and logged, but there's no retry mechanism. For a command system (sending buzzer_on to a device), a single failure could mean missing a critical safety action.

**Fix:** Add configurable retry with exponential backoff.

---

## W-08: Missing Security-Related Django Settings

**File:** `nodealert/settings.py`
**Severity:** MEDIUM
**Type:** Security configuration

The following production security settings are absent:
- `SECURE_SSL_REDIRECT` — no HTTPS redirect
- `SECURE_PROXY_SSL_HEADER` — no proxy SSL header (needed behind nginx)
- `SESSION_COOKIE_SECURE` — session cookies sent over HTTP
- `CSRF_COOKIE_SECURE` — CSRF cookies sent over HTTP
- `SECURE_HSTS_SECONDS` — no HSTS header
- `SECURE_CONTENT_TYPE_NOSNIFF` — missing X-Content-Type-Options
- `SECURE_BROWSER_XSS_FILTER` — missing X-XSS-Protection
- `X_FRAME_OPTIONS` — defaults to DENY, this is OK

**Impact:** In a production deployment behind nginx, if internal communication between nginx and Django is not isolated, traffic is unencrypted. More importantly, if deployed without nginx, there's no HTTPS at all.

---

## W-09: MAC Address Validation Silently Skips If Empty

**File:** `core/management/commands/mqtt_subscriber.py` lines 70-82
**Severity:** MEDIUM
**Type:** Authentication bypass (device-level)

```python
def _validate_device(self, device_id, mac_address=None):
    try:
        device = Device.objects.get(device_id=device_id, is_active=True)
        if mac_address and device.mac_address:
            if device.mac_address.lower() != mac_address.lower():
                ...
                return None
        return device  # <-- Returns the device even if mac_address is empty
```

If a device has no `mac_address` stored (default is `''`), MAC validation is completely skipped — any payload claiming to be from that `device_id` is accepted. An attacker who knows a device ID can send spoofed telemetry.

**Fix:**
```python
if mac_address and device.mac_address:
    if device.mac_address.lower() != mac_address.lower():
        ...
        return None
elif device.mac_address:
    # Device has a MAC but payload didn't provide one — reject
    self.stderr.write(f'Payload missing MAC for {device_id}')
    return None
# If device has no MAC configured, accept (or log a warning)
```

---

# 3. INFO ISSUES

## I-01: Type Hints Mostly Missing

**Files:** All Python files
**Severity:** INFO

Only `core/mqtt_publisher.py` has type hints on `publish_command()`. The majority of the codebase lacks function parameter and return type annotations.

**Files affected:**
- `core/views.py` — no type hints on any method
- `core/serializers.py` — no type hints on any method
- `core/management/commands/mqtt_subscriber.py` — no type hints on methods
- `core/management/commands/init_groups.py` — no type hints

---

## I-02: Import Inside Function Body

**File:** `core/mqtt_publisher.py` line 25
**Severity:** INFO

```python
def publish_command(device_id: str, cmd: str, params: dict = None) -> bool:
    import time as _time
```

`import time` should be at the top of the file. This works but violates Python convention and slightly delays the import until the function's first call.

---

## I-03: Duplicate Timestamp Parsing (see W-05 for severity)

Already detailed in W-05 — listed here for completeness.

---

## I-04: No Device Auto-Registration

**File:** `core/management/commands/mqtt_subscriber.py` line 81
**Severity:** INFO (feature gap)

When an unknown device sends telemetry, the subscriber logs an error and drops the data. There is no mechanism to:
- Auto-register new devices
- Notify an admin of unknown devices
- Accept first-contact telemetry for registration

**Fix suggestion:** Add a `settings.AUTO_REGISTER_DEVICES` toggle that creates `Device` objects for unknown device IDs.

---

## I-05: Token Authentication Without Expiration

**File:** `core/views.py` line 120
**Severity:** INFO

```python
token, created = Token.objects.get_or_create(user=user)
return Response({'token': token.key}, status=status.HTTP_200_OK)
```

DRF Token authentication provides no token expiration, rotation, or revocation mechanism (beyond manual deletion from the DB). Once a token is issued, it's valid forever unless explicitly deleted.

**Fix suggestion:** Use `rest_framework.authtoken.models.Token` with custom middleware that checks `token.created` age, or switch to JWT (djangorestframework-simplejwt) with configurable expiration.

---

## I-06: `requires_system_checks = []` in Management Command

**File:** `core/management/commands/mqtt_subscriber.py` line 20
**Severity:** INFO

```python
class Command(BaseCommand):
    help = 'Subscribe to MQTT topics and ingest sensor telemetry into the database'
    requires_system_checks = []
```

This suppresses all Django system checks (migration consistency, model validation errors). While common for long-running processes, consider setting it to `['site']` or `['all']` to catch misconfigurations at startup.

---

## I-07: Reading Model String Representation Causes N+1 in Admin

**File:** `core/models.py` line 60
**Severity:** INFO

```python
def __str__(self):
    return f"{self.device.device_id}/{self.sensor_type} @ {self.timestamp}"
```

Accessing `self.device.device_id` triggers a database query for each `Reading` displayed in the admin list view if `device_id` is not already selected via `select_related()`. The `ReadingAdmin` doesn't set `list_select_related`. This only affects the admin interface, not the API.

---

## I-08: Tests Lack MQTT Subscriber Coverage

**Files:** `core/tests/test_api.py`, `core/tests/test_models.py`
**Severity:** INFO

The test suite covers:
- Model field definitions ✓
- API CRUD operations ✓
- Basic filtering ✓

But is missing:
- Integration tests for `mqtt_subscriber` command
- Tests for invalid/edge-case MQTT payloads
- Tests for `mqtt_publisher` failure handling
- Tests for authorization (every test uses a single user, so permission bypasses never surface)
- Tests for the health check endpoints

---

## I-09: Default Database Password Hardcoded in Settings

**File:** `nodealert/settings.py` lines 73-74
**Severity:** INFO (documented as defaults)

```python
'USER': os.environ.get('MYSQL_USER', 'nodealert'),
'PASSWORD': os.environ.get('MYSQL_PASSWORD', 'nodealert'),
```

While this clearly uses env vars with defaults, the default password `nodealert` for `nodealert` user is trivially guessable. In Docker Compose, these defaults are likely overridden, but a standalone deployment might forget to set them.

---

## I-10: CSRF_TRUSTED_ORIGINS Not Set

**File:** `nodealert/settings.py`
**Severity:** INFO

While the project uses `rest_framework.authentication.SessionAuthentication` alongside TokenAuthentication, `CSRF_TRUSTED_ORIGINS` is not configured. This means API requests using session auth from the React frontend might fail CSRF validation, or be inconsistently enforced if the CORS check passes but the CSRF check doesn't.

---

# SUMMARY TABLE

| ID | Issue | File | Severity |
|----|-------|------|----------|
| C-01 | No authorization — full CRUD for all users | `views.py` lines 37-101 | **CRITICAL** |
| C-02 | ALLOWED_HOSTS = ['*'] in production | `settings.py` line 20 | **CRITICAL** |
| C-03 | MQTT topic injection via device_id | `mqtt_publisher.py` line 37 | **CRITICAL** |
| W-01 | Exposed password hash in admin | `admin.py` lines 33-36 | HIGH |
| W-02 | MQTT telemetry QoS 0 (data loss) | `mqtt_subscriber.py` line 53 | HIGH |
| W-03 | `os.system()` in health check | `views.py` lines 157-163 | MEDIUM |
| W-04 | Missing MQTT `on_disconnect` callback | `mqtt_subscriber.py` line 41-48 | MEDIUM |
| W-05 | Duplicated timestamp parsing with silent fallback | `mqtt_subscriber.py` lines 91-99, 137-145 | MEDIUM |
| W-06 | UserAdmin not using BaseUserAdmin | `admin.py` lines 33-36 | LOW-MEDIUM |
| W-07 | Missing retry in MQTT publisher | `mqtt_publisher.py` lines 35-47 | LOW |
| W-08 | Missing security settings (HTTPS, cookies) | `settings.py` | MEDIUM |
| W-09 | MAC validation skipped if not stored | `mqtt_subscriber.py` lines 70-82 | MEDIUM |
| I-01 | Type hints mostly missing | All files | INFO |
| I-02 | Import inside function body | `mqtt_publisher.py` line 25 | INFO |
| I-03 | Timestamp parsing duplication | `mqtt_subscriber.py` | INFO |
| I-04 | No device auto-registration | `mqtt_subscriber.py` line 81 | INFO |
| I-05 | Token auth without expiration | `views.py` line 120 | INFO |
| I-06 | requires_system_checks = [] | `mqtt_subscriber.py` line 20 | INFO |
| I-07 | Reading.__str__ N+1 in admin | `models.py` line 60 | INFO |
| I-08 | Missing test coverage for MQTT + auth | `tests/` | INFO |
| I-09 | Default DB password in settings | `settings.py` line 73-74 | INFO |
| I-10 | CSRF_TRUSTED_ORIGINS not set | `settings.py` | INFO |

---

# LINE-BY-LINE FILE AUDIT

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/settings.py` (nodealert/settings.py)

| Line | Issue | Severity |
|------|-------|----------|
| 20 | `ALLOWED_HOSTS = ['*']` | CRITICAL |
| 73-74 | Default DB credentials `nodealert:nodealert` | INFO |
| 120-137 | DRF config uses `DEFAULT_PERMISSION_CLASSES: [IsAuthenticated]` only — no model-level permissions | CRITICAL |
| 141-149 | CORS_ALLOWED_ORIGINS well-configured for dev | OK |
| 152-153 | CORS_EXTRA_ORIGINS env parsing via `split(',')` — no path injection here | OK |

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/models.py`

| Line | Issue | Severity |
|------|-------|----------|
| 60 | `self.device.device_id` in `__str__` — N+1 in admin | INFO |
| 53-57 | Good: composite indexes on Reading (device+sensor_type, timestamp) | OK |
| 78-81 | Good: composite indexes on Event (device+severity, resolved) | OK |

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/views.py`

| Line | Issue | Severity |
|------|-------|----------|
| 37-41 | DeviceViewSet: IsAuthenticated only (no group/permission check) | CRITICAL |
| 49-75 | `command` action: publishes MQTT — no permission check on WHICH user commands | CRITICAL |
| 85 | ReadingViewSet: no `permission_classes` — inherits IsAuthenticated (correct but implicit) | INFO |
| 93-97 | EventViewSet: IsAuthenticated only (no group/permission check) | CRITICAL |
| 115-121 | LoginView: Token never expires | INFO |
| 157-163 | `os.system()` readiness check | WARNING |
| 154-155 | `connection.ensure_connection()` — correct approach for DB check | OK |

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/mqtt_publisher.py`

| Line | Issue | Severity |
|------|-------|----------|
| 25 | `import time` inside function | INFO |
| 37 | Topic injection via `device_id` | CRITICAL |
| 45-47 | Catch-all `Exception` — swallows connection errors silently | WARNING |
| 44 | Returns True even if publish.single might have queued without confirming (QoS 1) | INFO |

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/management/commands/mqtt_subscriber.py`

| Line | Issue | Severity |
|------|-------|----------|
| 20 | `requires_system_checks = []` | INFO |
| 41 | No `on_disconnect` handler | WARNING |
| 53 | QoS 0 for telemetry subscription | HIGH |
| 70-82 | MAC validation skips if device MAC is empty | WARNING |
| 91-99 | Timestamp parsing silently falls back | WARNING |
| 108-124 | Bulk create with transaction — good | OK |
| 130-153 | Timestamp parsing duplicated from _handle_telemetry | INFO/WARNING |

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/management/commands/init_groups.py`

Good — clean implementation. No issues.

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/admin.py`

| Line | Issue | Severity |
|------|-------|----------|
| 33-36 | UserAdmin inherits ModelAdmin instead of auth.UserAdmin | HIGH |

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/serializers.py`

| Line | Issue | Severity |
|------|-------|----------|
| 13 | DeviceSerializer: `fields = '__all__'` — exposes all fields including internal ones | INFO |
| 40-61 | LoginSerializer: good validation logic | OK |
| 52-53 | Authenticate with username/password — correct usage | OK |
| 64-69 | CommandSerializer: ChoiceField validation prevents arbitrary commands | OK |
| 72-80 | UserSerializer: role determined by group membership — good pattern | OK |

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/core/tests/test_api.py`

| Line | Issue | Severity |
|------|-------|----------|
| 14-22 | `_auth_client()` creates user with `testpass123` — OK for tests | INFO |
| 110-116 | Tests pagination default size of 50 — good coverage | OK |
| All | No tests for: authorization gaps, MQTT endpoints, health checks, invalid payloads | INFO |

## `/home/sebastian/Escritorio/IC2/NodeAlert-Backend/entrypoint.sh`

| Line | Issue | Severity |
|------|-------|----------|
| 37 | `runserver 0.0.0.0:8000` — dev server in container, acceptable behind nginx | INFO |
| 11-16 | Superuser creation with env vars — secure pattern | OK |

---

# KEY STRENGTHS IDENTIFIED

Despite the critical issues, the codebase has several well-implemented patterns:

1. **MVC separation is clean** — models in `models.py`, views in `views.py`, serializers in `serializers.py`, URL routing in `urls.py`
2. **Pagination is configured globally** (PAGE_SIZE=50)
3. **Filter backends are well-configured** — `django-filter`, `SearchFilter`, `OrderingFilter`
4. **Throttling is configured** for both authenticated (100/min) and anonymous (5/min) users
5. **Health check endpoints** differentiate liveness from readiness
6. **Timestamp parsing** tries device timestamp first, falls back to server time
7. **Group-based roles exist** (admin/analyst) — the infrastructure for authorization is partially in place
8. **MQTT credentials** are properly sourced from environment variables
9. **Docker entrypoint** properly handles shutdown signals for MQTT subscriber
10. **Test suite** covers basic CRUD and filtering for all three models

---

# PRIORITY FIX ORDER

1. **C-02**: Fix `ALLOWED_HOSTS` — 1 line change, immediate security improvement
2. **C-01**: Implement permission classes — requires restructuring views but closes the biggest authorization gap
3. **C-03**: Add device_id validation in serializer — prevents MQTT topic injection
4. **W-01**: Change UserAdmin to inherit from BaseUserAdmin — 1 line change
5. **W-02**: Change telemetry subscription to QoS 1 — 1 character change
6. **W-03**: Replace `os.system()` with `subprocess.run()` — local change in health view
7. **W-09**: Tighten MAC validation logic
8. **W-08**: Add security middleware settings
9. **W-04**: Add `on_disconnect` callback
10. **W-05**: Extract and improve timestamp parsing helper
