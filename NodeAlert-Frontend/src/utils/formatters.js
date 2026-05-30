/**
 * Utilidades de formateo para la interfaz de NodeAlert.
 *
 * Centraliza las funciones de transformación de datos para presentación
 * en la UI: timestamps a formato legible, estado de dispositivos,
 * colores según severidad, y valores con unidades.
 *
 * Mantener estas funciones aquí evita duplicación de lógica de
 * presentación entre componentes y facilita cambios de formato
 * sin modificar múltiples archivos.
 */

/**
 * Formatea un timestamp ISO a fecha/hora local en español.
 * Ejemplo: "27/05/2026 14:30:15"
 */
export function formatTimestamp(isoStr) {
  if (!isoStr) return '—'
  const d = new Date(isoStr)
  return d.toLocaleString('es-ES', {
    day: '2-digit',
    month: '2-digit',
    year: 'numeric',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  })
}

/**
 * Calcula el tiempo transcurrido desde una fecha ISO hasta ahora.
 * Retorna un formato compacto: "3m 12s", "2h 30m", "5d".
 * Ideal para mostrar antigüedad de eventos en listas compactas.
 */
export function timeAgo(isoStr) {
  if (!isoStr) return '—'
  const diff = Date.now() - new Date(isoStr).getTime()
  const sec = Math.floor(diff / 1000)
  if (sec < 60) return `${sec}s`
  const min = Math.floor(sec / 60)
  if (min < 60) return `${min}m ${sec % 60}s`
  const hr = Math.floor(min / 60)
  if (hr < 24) return `${hr}h ${min % 60}m`
  return `${Math.floor(hr / 24)}d`
}

/**
 * Determina el estado de un sensor basado en su valor respecto al máximo.
 *   - normal: valor por debajo del 80%.
 *   - warning: entre 80% y 100%.
 *   - critical: igual o superior al 100%.
 *   - offline: valor nulo (sin lectura).
 */
export function sensorStatus(value, max) {
  if (value == null) return 'offline'
  const ratio = value / max
  if (ratio >= 1) return 'critical'
  if (ratio >= 0.8) return 'warning'
  return 'normal'
}

/**
 * Determina si un dispositivo está online basado en su última conexión.
 * Se considera online si la última conexión fue hace menos de 180 segundos.
 * El firmware publica cada 60s; este umbral tolera hasta 2 ciclos perdidos
 * con margen para reconexión WiFi antes de marcar offline.
 */
export function deviceStatus(isoStr) {
  if (!isoStr) return 'offline'
  const diff = Date.now() - new Date(isoStr).getTime()
  return diff < 180000 ? 'online' : 'offline'
}

/**
 * Retorna el color hexadecimal asociado a cada estado del sistema.
 * Los colores están alineados con el tema oscuro de la aplicación.
 */
export function statusColor(status) {
  const map = {
    online: '#00e676',
    offline: '#64748b',
    warning: '#ffd600',
    critical: '#ff1744',
    normal: '#00e676',
    info: '#2979ff',
  }
  return map[status] || '#64748b'
}

/**
 * Formatea un valor numérico con una unidad, redondeado a 1 decimal.
 * Ejemplo: formatValue(23.456, '°C') → "23.5 °C"
 */
export function formatValue(value, unit) {
  if (value == null) return '—'
  return `${Number(value).toFixed(1)} ${unit}`
}

const sensorKeyMap = {
  temperature: 'temperature',
  humidity: 'humidity',
  gas: 'gas_ppm',
  flame: 'flame',
}

/**
 * Retorna la última lectura de un sensor específico para un dispositivo.
 * El backend almacena cada tipo de sensor como una fila separada con
 * { sensor_type, value, unit }. Esta función busca la fila más reciente
 * del tipo solicitado y devuelve su valor.
 */
export function getSensorValue(readings, deviceId, sensorType) {
  const match = readings
    .filter((r) => r.device === deviceId && r.sensor_type === sensorType)
    .sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp))
  return match.length ? match[0].value : null
}

/**
 * Agrupa las últimas lecturas de cada dispositivo en un objeto plano.
 * Convierte el modelo del backend (filas por tipo de sensor) al formato
 * que espera el frontend: { temperature, humidity, gas_ppm, flame }.
 * Retorna un array de objetos { device_id, temperature, humidity, ... }.
 */
export function groupLatestReadings(readings) {
  const devices = new Set(readings.map((r) => r.device))
  return Array.from(devices).map((devId) => {
    const latest = {}
    for (const [dbType, frontKey] of Object.entries(sensorKeyMap)) {
      const reading = readings
        .filter((r) => r.device === devId && r.sensor_type === dbType)
        .sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp))
      if (reading.length) {
        latest[frontKey] = reading[0].value
      }
    }
    const deviceReadings = readings.filter((r) => r.device === devId)
    if (deviceReadings.length) {
      latest.timestamp = deviceReadings.sort(
        (a, b) => new Date(b.timestamp) - new Date(a.timestamp)
      )[0].timestamp
    }
    latest.device = devId
    return latest
  })
}

/**
 * Filtra lecturas por tipo de sensor y las mapea a { timestamp, value }.
 * Necesario para los gráficos Recharts que requieren una serie temporal
 * plana con dataKey="value".
 */
export function readingsForChart(readings, dataKey) {
  const reverseMap = {
    temperature: 'temperature',
    humidity: 'humidity',
    gas_ppm: 'gas',
    flame: 'flame',
  }
  const sensorType = reverseMap[dataKey] || dataKey
  return readings
    .filter((r) => r.sensor_type === sensorType)
    .map((r) => ({ timestamp: r.timestamp, [dataKey]: r.value }))
}
