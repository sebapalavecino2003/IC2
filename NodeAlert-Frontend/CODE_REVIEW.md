# NodeAlert Frontend — Code Review Report

**Review Date:** 2026-05-26
**Scope:** All 26 TypeScript/TSX source files + configuration files
**Analyst:** AI Code Review

---

## Overall Score: **PASS WITH WARNINGS**

The project has a solid architectural foundation and follows React/MUI patterns reasonably well. However, there are **3 CRITICAL issues** (security, data integrity, UX failure state) and **13 WARNING issues** that should be addressed before production deployment.

---

## 1. CRITICAL ISSUES

### C1. Token Stored in localStorage (XSS Vulnerability)

**File:** `src/services/api.ts` (line 11)

```typescript
const token = localStorage.getItem('token')
```

**Problem:** The authentication token is stored in `localStorage` and read on every API request. `localStorage` is accessible to any JavaScript executing in the same origin, making it trivially stealable via any XSS vulnerability. Once stolen, an attacker has full API access as the victim user.

**Impact:** A single XSS vulnerability anywhere in the app (or any third-party script loaded on the page) compromises all user sessions.

**Fix:** Use httpOnly, Secure, SameSite cookies for the token. The backend should set the cookie on login, and the frontend should not handle the token at all. Alternatively, use a more secure storage mechanism with shorter expiry.

---

### C2. Alarm Sound Never Plays on Mobile / Autoplay-Blocked Browsers

**File:** `src/components/AlarmSound.tsx` (line 16)

```typescript
audioRef.current.play().catch(() => {})
```

**Problem:** Modern browsers (Chrome 66+, Safari 11+, all mobile browsers) block audio autoplay without prior user interaction. The `.catch(() => {})` silently swallows the `DOMException: play() failed because the user didn't interact with the document first` error. Additionally, on iOS Safari, audio with base64 data URIs has unreliable support.

**Impact:** The core safety feature of the system — audible alarm on gas/flame detection — is completely silent on most mobile devices and many desktop browsers. Users may not be alerted to critical conditions.

**Fix:**
- Use the [Web Audio API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API) instead of `<Audio>` element for more reliable playback.
- Pre-warm audio context on first user interaction (click/tap anywhere).
- Show a visual fallback indicator when audio is blocked.
- Host the audio as an actual file instead of inline base64.

---

### C3. Race Conditions in Polling Cause Stale Data Overwrite

**Files:**
- `src/hooks/usePolling.ts` (all)
- `src/context/ReadingsContext.tsx` (line 22-34)

**Problem:** The poll callback fires every 3 seconds via `setInterval`. If a poll cycle's requests take longer than 3 seconds (due to network latency or server load), the response from the *previous* cycle can overwrite the data from the *next* cycle:

```
t=0  Poll 1 starts (takes 4s)
t=3  Poll 2 starts (takes 1s) → sets data to snapshot B
t=4  Poll 1 completes → sets data to snapshot A (STALE! overwrites B)
```

Additionally, no `AbortController` is used, so in-flight requests continue even after the component unmounts, causing React warnings about setting state on unmounted components.

**Impact:** Users see outdated sensor readings. Critical alarms could be missed or incorrectly dismissed.

**Fix:**
- Use a sequence counter or timestamp comparison to discard stale responses.
- Add `AbortController` to cancel in-flight requests on new poll cycle or unmount.
- Consider using `react-query` or `SWR` which handle this properly out of the box.

---

### C4. Inline `@keyframes blink` Creates Duplicate Style Tags (Performance / Fragile)

**File:** `src/components/SensorGauge.tsx` (lines 34-37)

```typescript
sx={{
  '@keyframes blink': {
    '0%, 100%': { opacity: 1 },
    '50%': { opacity: 0.3 },
  },
}}
```

Referenced in: `src/components/SummaryBar.tsx` (line 35)

```typescript
sx={{ animation: 'blink 1s infinite' }}
```

**Problem:** Each instance of `SensorGauge` renders creates a new `<style>` element in the `<head>` with an identical `@keyframes blink` definition. With 4 gauges on screen, there are 4 duplicate `<style>` tags. The `SummaryBar` component also uses `animation: 'blink 1s infinite'` but does NOT define the keyframes itself — it relies on SensorGauge being mounted. If SensorGauge unmounts, the keyframe rule is removed and SummaryBar's animation silently stops.

**Impact:** 4+ duplicate style tags waste rendering performance. The blinking alarm chip in SummaryBar depends on a side effect from SensorGauge, making it fragile.

**Fix:**
- Define `@keyframes blink` once globally (e.g., in `index.css`, `CssBaseline` override, or a `GlobalStyles` component).
- Remove the inline keyframes from SensorGauge's `sx` prop.

---

## 2. WARNING ISSUES

### W1. 9 Silent Empty `catch` Blocks Across the Codebase

**Files:**
- `src/context/AuthContext.tsx` (line 31) — `catch {}` in `fetchUserInfo`
- `src/context/ReadingsContext.tsx` (lines 31-33) — `catch {}` in poll
- `src/context/AlarmContext.tsx` (line 65) — `catch {}` in `acknowledgeAlarm`
- `src/components/LoginForm.tsx` (line 14) — `catch {}` in login submit
- `src/components/AlertPanel.tsx` (line 19) — `catch {}` in `handleResolve`
- `src/components/AlarmSound.tsx` (line 16) — `.catch(() => {})` on play
- `src/pages/DeviceListPage.tsx` (line 15) — `.catch(() => {})`
- `src/pages/DeviceDetailPage.tsx` (line 24) — `.catch(() => {})`

**Problem:** Every API call error is silently swallowed. The comment "api.ts interceptor handles 401" only covers 401 errors. Network failures, 500 errors, malformed responses, and rate limiting are all invisible to the user. The app appears to be working when it's actually in a broken state.

**Impact:** Catastrophic UX — users see stale data, empty lists, or loading spinners indefinitely with no indication that something is wrong.

**Fix:** At minimum log errors, show user-facing error notifications (snackbar/toast), and differentiate between "empty data" and "failed to fetch" in the UI.

---

### W2. No Request Cleanup on Unmount (Memory Leaks / Warnings)

**Files:**
- `src/pages/DeviceListPage.tsx` (lines 12-17)
- `src/pages/DeviceDetailPage.tsx` (lines 15-26)

**Problem:** When these components unmount before API calls complete, `setState` is called on unmounted components. React 18 warns about this in Strict Mode. In-flight requests also continue consuming bandwidth.

**Impact:** React warnings in development, potential memory pressure from accumulated in-flight requests, state updates on unmounted components in production.

**Fix:** Use `AbortController` + `signal` with axios, and clean up in the `useEffect` return. Example:

```typescript
useEffect(() => {
  const controller = new AbortController()
  api.get('/devices', { signal: controller.signal })
    .then(...)
    .catch(...)
  return () => controller.abort()
}, [])
```

---

### W3. Hard 401 Redirect Causes Full Page Reload, Loss of State

**File:** `src/services/api.ts` (lines 21-23)

```typescript
if (error.response?.status === 401) {
  localStorage.removeItem('token')
  window.location.href = '/login'
}
```

**Problem:** Using `window.location.href` triggers a full browser navigation, destroying all React state (context, Redux, etc.). This is unnecessarily aggressive — a single request getting a 401 might not mean the entire session is invalid (could be a race condition with token refresh).

**Impact:** Full page reload on any 401, losing any unsaved work or transient UI state. Poor UX.

**Fix:** Use React Router's `navigate('/login')` instead. Consider handling 401 more gracefully (e.g., retry once, check if token exists before redirecting).

---

### W4. Stale Closure Bug in AlertPanel

**File:** `src/components/AlertPanel.tsx` (lines 11-13)

```typescript
const [localEvents, setLocalEvents] = useState(events)
const currentEvents = events.length > 0 ? events : localEvents
```

**Problem:** `localEvents` is initialized ONCE from the initial `events` value and never updated when `events` changes from polling. The ternary `currentEvents` uses `events` when it has data, falling back to `localEvents` when `events` is empty. This means:
- When new events arrive via polling, they're shown (from `events`).
- If `events` becomes empty (e.g., all resolved), but `localEvents` has data, stale data is shown.
- The `handleResolve` function filters from `localEvents`, but the events passed to `EventTable` may be from `events` (the fresh poll data). So clicking "resolve" in EventTable calls `onResolve` which filters from a different array than what's displayed.

**Impact:** Inconsistent state — resolving an event visually removes it from `localEvents`, but it may still appear in the next poll cycle from `events`. The component state doesn't correctly track resolved events across polling cycles.

**Fix:** Remove `localEvents` entirely. Handle event resolution optimistically by calling the API and then waiting for the next poll to reflect the change. Or use a single source of truth.

---

### W5. `overrideActive` Never Synced from Server

**File:** `src/context/AlarmContext.tsx` (line 58)

```typescript
const [overrideActive, setOverrideActive] = useState(false)
```

**Problem:** The `overrideActive` state is initialized to `false` and never updated. There's no API call to fetch the current override state from the backend. The `OverrideState` type is defined in `src/types/index.ts` but never used.

**Impact:** The UI always shows "Modo Auto" even when the server has activated an override. The override chip in `SummaryBar.tsx` is permanently misleading.

**Fix:** Fetch override state from an API endpoint and poll it alongside readings. Add a `setOverrideActive` call when fetching device status.

---

### W6. `useEffect` Missing Dependency in AuthContext

**File:** `src/context/AuthContext.tsx` (lines 60-64)

```typescript
useEffect(() => {
  if (token) {
    fetchUserInfo()
  }
}, [])  // Missing: token
```

**Problem:** The effect depends on `token` but the dependency array is empty. While this works because `login()` calls `fetchUserInfo()` inline, it means:
- If `token` changes for any reason OTHER than `login()` (e.g., restored from session storage, set by another tab), `fetchUserInfo` won't run.
- ESLint's `react-hooks/exhaustive-deps` rule would flag this.

**Impact:** Potential missed user info fetch if token is set outside the login flow. Relies on implicit call order rather than declarative dependencies.

**Fix:** Add `token` and `fetchUserInfo` to the dependency array.

---

### W7. No Input Validation on Login Form

**File:** `src/components/LoginForm.tsx` (lines 31-48)

**Problem:** The form has `required` HTML attributes on TextFields but no programmatic validation. Empty strings or whitespace-only input is submitted to the server. If a user bypasses HTML5 validation (e.g., via browser dev tools), empty credentials are sent.

**Impact:** Unnecessary API calls with invalid data. Poor user feedback — the server's 400 error message is shown as a generic "Error de autenticación" rather than "Username is required".

**Fix:** Add client-side validation before submission:
```typescript
if (!username.trim() || !password.trim()) {
  setError('Usuario y contraseña son requeridos')
  return
}
```

---

### W8. Promise.all Causes Partial Failure in DeviceDetailPage

**File:** `src/pages/DeviceDetailPage.tsx` (line 16-19)

```typescript
Promise.all([
  api.get<Device>(`/devices/${id}`),
  api.get<Reading[]>('/readings', { params: { device_id: id, ... } }),
])
```

**Problem:** If the readings API fails but the device API succeeds, `Promise.all` rejects entirely and the user sees a generic error. The device info is lost even though it was fetched successfully.

**Impact:** When readings are unavailable (e.g., device temporarily offline), the entire device detail page fails instead of showing partial data.

**Fix:** Use `Promise.allSettled` or separate `useEffect` calls to handle partial success gracefully.

---

### W9. `useParams` `id` Can Be `undefined` at TypeScript Level

**File:** `src/pages/DeviceDetailPage.tsx` (line 10)

```typescript
const { id } = useParams()
// Used as: api.get(`/devices/${id}`)
```

**Problem:** `useParams` returns `{ id?: string }`, so `id` can technically be `undefined`. When used in template literals, `undefined` becomes the string `"undefined"`, resulting in a request to `/devices/undefined`. While React Router normally won't render this route without an `id` segment, TypeScript offers no protection here.

**Impact:** If the route configuration changes or a bug in React Router occurs, silent 404 errors happen instead of a clear error state.

**Fix:** Guard early:
```typescript
if (!id) return <Typography>ID de dispositivo no válido</Typography>
```

---

### W10. No Error/Retry State in Polling

**File:** `src/context/ReadingsContext.tsx`

**Problem:** The `ReadingsState` has no `error` or `isFetching` properties. When polling fails (network down, server restart), the old data is simply retained with no indication that updates have stopped. The `lastUpdate` is only set on success, so it becomes stale silently.

**Impact:** Users see old sensor data with no warning that connectivity is lost. In a safety-critical monitoring system, this is dangerous.

**Fix:** Add `lastError: string | null` and `isFetching: boolean` to the context. Show a warning chip/banner when data is stale.

---

### W11. No Security Headers in nginx.conf

**File:** `nginx.conf`

**Problem:** The nginx configuration serves the SPA without any security headers:

| Missing Header | Risk |
|---|---|
| `X-Content-Type-Options: nosniff` | MIME-type sniffing attacks |
| `X-Frame-Options: DENY` | Clickjacking |
| `Content-Security-Policy` | XSS mitigation |
| `Strict-Transport-Security` | Downgrade attacks |

**Impact:** The app is vulnerable to clickjacking, MIME confusion, and has no CSP protection against XSS.

**Fix:** Add security headers to the nginx `location /` block:
```
add_header X-Content-Type-Options nosniff;
add_header X-Frame-Options DENY;
add_header Content-Security-Policy "default-src 'self'; script-src 'self'; style-src 'self' fonts.googleapis.com; font-src fonts.gstatic.com; connect-src 'self'";
add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;
```

---

### W12. Missing `X-Forwarded-Proto` in nginx Proxy

**File:** `nginx.conf` (line 14)

```nginx
proxy_set_header Host $host;
proxy_set_header X-Real-IP $remote_addr;
proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
```

**Problem:** The proxy headers include `X-Forwarded-For` but NOT `X-Forwarded-Proto`. Django's `SECURE_PROXY_SSL_HEADER` and CSRF framework require this to generate correct `https://` redirect URLs and validate secure cookie policies.

**Impact:** If the Django backend runs behind HTTPS (via a reverse proxy), it generates `http://` URLs in redirects and may reject CSRF checks due to protocol mismatch.

**Fix:** Add `proxy_set_header X-Forwarded-Proto $scheme;`

---

### W13. No Logging / Observability

**Problem:** The entire frontend has no logging mechanism. All errors are silently swallowed (see W1). There's no:
- Structured error logging to console
- Error reporting service (Sentry, etc.)
- Performance monitoring
- User action analytics

**Impact:** Debugging production issues requires reproducing the exact user scenario. No crash reporting, no error aggregation.

**Fix:** Add a logging service (console.error at minimum, Sentry for production). Log all API failures with context (endpoint, params, user).

---

## 3. INFO ISSUES

### I1. No Test Framework or Tests

**Files:** `package.json`

**Problem:** No test dependencies, no test configuration, no test files. The project has zero test coverage.

**Impact:** No regression safety net. Every change requires manual testing.

**Fix:** Add Vitest + React Testing Library as dev dependencies. Add unit tests for contexts and hooks, integration tests for pages.

---

### I2. No ESLint / Prettier Configuration

**File:** `package.json` — no devDependencies for linting or formatting

**Problem:** No code quality tooling. This leads to inconsistent code styles across contributors and misses potential bugs that linters catch (e.g., `react-hooks/exhaustive-deps`).

**Fix:** Add eslint + prettier + eslint-plugin-react-hooks.

---

### I3. `catch (err: any)` Uses Explicit `any` Type

**File:** `src/context/AuthContext.tsx` (line 45)

```typescript
catch (err: any) {
  const msg = err.response?.data?.detail || ...
```

**Problem:** While TypeScript's `strict: true` doesn't forbid explicit `any`, using `any` bypasses all type checking on error handling. The `err.response?.data?.detail` chain is not type-checked and could silently produce `undefined`.

**Fix:** Use `unknown` and narrow the type, or define an `AxiosError` interface:
```typescript
catch (err: unknown) {
  const axiosError = err as { response?: { data?: { detail?: string } } }
```

---

### I4. Hardcoded Chart Y-Axis Domains

**Files:**
- `src/components/TempChart.tsx` (line 27): `domain={[0, 50]}`
- `src/components/HumidityChart.tsx` (line 27): `domain={[0, 100]}`

**Problem:** Fixed Y-axis ranges don't adapt to actual data ranges. If temperature readings stay between 22°C–24°C, the chart shows very small variations on a 0–50 scale, making trends invisible.

**Fix:** Use `'auto'` or calculate dynamic domains with padding based on actual data min/max.

---

### I5. Inline Base64 Audio Data

**File:** `src/components/AlarmSound.tsx` (line 10)

```typescript
audioRef.current = new Audio('data:audio/wav;base64,UklGRnoGAABXQVZFZm10IBAAAAABAAEAQB8AAEAfAAABAAgAZGF0YQoGAACAf39/f4B/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f3+AgH9/f38=')
```

**Problem:** Audio data is hardcoded as a base64 string in the component. This bloats the bundle, can't be cached separately, and can't be replaced without rebuilding.

**Fix:** Move audio to the `public/` directory and reference by URL. Or export the base64 string from a separate `assets/audio.ts` file.

---

### I6. `noUnusedLocals` and `noUnusedParameters` Disabled in tsconfig

**File:** `tsconfig.json` (lines 15-16)

```json
"noUnusedLocals": false,
"noUnusedParameters": false,
```

**Problem:** These flags are explicitly set to `false`, allowing unused variables and parameters to compile without error. With them enabled, the build would catch dead code like potentially unused `OverridesState` type and uncovered parameters.

**Fix:** Enable both and clean up dead code:
```json
"noUnusedLocals": true,
"noUnusedParameters": true,
```

---

### I7. `OverrideState` Type Defined but Never Used

**File:** `src/types/index.ts` (lines 60-63)

```typescript
export interface OverrideState {
  active: boolean
  mode: 'auto' | 'manual'
}
```

**Problem:** This type is exported but never imported anywhere in the frontend. It's dead code.

**Fix:** Remove or use it in AlarmContext where `overrideActive` state is managed.

---

### I8. No React Router `createBrowserRouter` — Using Legacy Route API

**File:** `src/App.tsx`

**Problem:** The app uses the older `<Routes>`/`<Route>` component-based API instead of the newer `createBrowserRouter` with loaders/actions. This misses out on:
- Route-level data loading with built-in race condition handling
- Type-safe route params
- Deferred data loading

**Fix:** Migrate to `createBrowserRouter` when upgrading React Router to v6.4+.

---

### I9. No Loading State Granularity in Polling

**File:** `src/context/ReadingsContext.tsx`

**Problem:** The context exposes `isPolling` as a boolean tied to `isAuthenticated`, but there's no `isFetching` state that tracks whether a fetch is currently in progress. The UI can't show a loading indicator during a request.

**Fix:** Add `isFetching` state that's `true` when a poll request is in-flight, `false` otherwise.

---

### I10. No Responsive Feedback for Empty States

**Files:**
- `src/components/ActiveAlertsPanel.tsx` (line 54-55)
- `src/components/EventTable.tsx` (line 61-64)
- `src/pages/DeviceListPage.tsx` (line 24)

**Problem:** Empty states show a simple text message with no illustration, no call to action, and no differentiation between "no data yet" and "data failed to load."

**Fix:** Use richer empty state components (illustration + message + retry button) and differentiate based on API error state.

---

## 4. SUMMARY TABLE

| Category | Count | Highlights |
|---|---|---|
| **CRITICAL** | 4 | Token XSS (C1), Silent alarm (C2), Race conditions (C3), Duplicate keyframes (C4) |
| **WARNING** | 13 | 9 empty catch blocks (W1), No cleanup on unmount (W2), Hard 401 redirect (W3), Stale closure (W4), Override never synced (W5), Missing deps (W6), No input validation (W7), Promise.all partial failure (W8), undefined id (W9), No error state (W10), No security headers (W11), Missing proxy header (W12), No logging (W13) |
| **INFO** | 10 | No tests (I1), No linter (I2), `any` type (I3), Hardcoded domains (I4), Inline base64 (I5), tsconfig flags (I6), Dead type (I7), Legacy router (I8), No fetch state (I9), Empty states (I10) |

---

## 5. FIX PRIORITY RECOMMENDATIONS

### Immediate (Pre-Production)
1. **C1** — Switch token storage from localStorage to httpOnly cookies
2. **C2** — Fix alarm audio to work in autoplay-blocked browsers
3. **W11** — Add nginx security headers
4. **W1** — Replace empty catch blocks with error logging + user notifications

### Short Term
1. **C3** — Add AbortController and sequence counters to polling
2. **W4** — Fix AlertPanel stale closure bug
3. **W2** — Add cleanup effects to DeviceListPage and DeviceDetailPage
4. **W5** — Sync override state from server
5. **W10** — Add error state to ReadingsContext

### Medium Term
1. **W6** — Fix useEffect dependency array
2. **W7** — Add client-side form validation
3. **C4** — Move `@keyframes blink` to global scope
4. **W9** — Add null guard for `useParams` id
5. **I1** — Set up test framework and write critical path tests
